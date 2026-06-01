#include "messages.hpp"

#include "format.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace lf {
	namespace {
		struct log_item {
			log::Record record;
			bool flush = false;
		};

		struct logger_state {
			std::mutex mutex;
			std::condition_variable wake;
			std::condition_variable drained;
			std::deque<log_item> queue;
			std::vector<unique<log::Sink>> sinks;
			std::thread worker;
			std::atomic<u64> next_sequence = 1;
			log::Level min_level = log::Level::Info;
			bool environment_configured = false;
			bool worker_started = false;
			bool stopping = false;
			u64 pending_records = 0;
		};

		logger_state& state() {
			static logger_state value;
			return value;
		}

		string lowercase(string_view value) {
			string result(value);
			std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return result;
		}

		bool parse_log_level(string_view value, log::Level& out) {
			string normalized = lowercase(value);
			if (normalized == "trace") { out = log::Level::Trace; return true; }
			if (normalized == "debug") { out = log::Level::Debug; return true; }
			if (normalized == "info") { out = log::Level::Info; return true; }
			if (normalized == "warning" || normalized == "warn") { out = log::Level::Warning; return true; }
			if (normalized == "error") { out = log::Level::Error; return true; }
			if (normalized == "critical" || normalized == "fatal") { out = log::Level::Critical; return true; }
			if (normalized == "assert") { out = log::Level::Assert; return true; }
			if (normalized == "none" || normalized == "off") { out = log::Level::None; return true; }
			return false;
		}

		bool parse_bool(string_view value, bool fallback) {
			string normalized = lowercase(value);
			if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") { return true; }
			if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") { return false; }
			return fallback;
		}

		bool should_log(log::Level level, log::Level min_level) {
			if (level == log::Level::None || min_level == log::Level::None) {
				return false;
			}
			return static_cast<int>(level) >= static_cast<int>(min_level);
		}

		string format_timestamp() {
			const auto now = std::chrono::system_clock::now();
			const auto seconds = std::chrono::floor<std::chrono::seconds>(now);
			const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();
			const std::time_t time = std::chrono::system_clock::to_time_t(now);
			std::tm local_time{};
#if defined(_WIN32)
			localtime_s(&local_time, &time);
#else
			localtime_r(&time, &local_time);
#endif
			std::ostringstream stream;
			stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << millis;
			return stream.str();
		}

		string render_line(const log::Record& record) {
			return format(
				"[{}] #{:06} [{}] [{}] {}",
				record.timestamp,
				record.sequence,
				log::LevelName(record.level),
				record.category,
				record.message);
		}

		string_view console_color(log::Level level) {
			switch (level) {
			case log::Level::Trace: return "\x1b[90m";
			case log::Level::Debug: return "\x1b[37m";
			case log::Level::Info: return "\x1b[97m";
			case log::Level::Warning: return "\x1b[33m";
			case log::Level::Error: return "\x1b[31m";
			case log::Level::Critical: return "\x1b[35m";
			case log::Level::Assert: return "\x1b[36m";
			case log::Level::None: return "";
			}
			return "";
		}

		void flush_sinks(logger_state& logger) {
			for (unique<log::Sink>& sink : logger.sinks) {
				sink->Flush();
			}
		}

		void worker_loop() {
			logger_state& logger = state();
			for (;;) {
				log_item item;
				{
					std::unique_lock lock(logger.mutex);
					logger.wake.wait(lock, [&] { return logger.stopping || !logger.queue.empty(); });
					if (logger.queue.empty() && logger.stopping) {
						flush_sinks(logger);
						logger.drained.notify_all();
						return;
					}

					item = std::move(logger.queue.front());
					logger.queue.pop_front();
				}

				if (item.flush) {
					std::scoped_lock lock(logger.mutex);
					flush_sinks(logger);
					logger.drained.notify_all();
					continue;
				}

				const string line = render_line(item.record);
				{
					std::scoped_lock lock(logger.mutex);
					for (unique<log::Sink>& sink : logger.sinks) {
						sink->Write(item.record, line);
					}
					if (logger.pending_records > 0) {
						--logger.pending_records;
					}
					if (logger.pending_records == 0 && logger.queue.empty()) {
						logger.drained.notify_all();
					}
				}
			}
		}

		void ensure_worker_started(logger_state& logger) {
			if (logger.worker_started) {
				return;
			}
			logger.worker_started = true;
			logger.stopping = false;
			logger.worker = std::thread(worker_loop);
		}
	}

	string make_missing_field_error_message(string_view field_name) {
		return format("missing required field '{}'", field_name);
	}

	string make_type_mismatch_error_message(string_view expected, string_view actual) {
		return format("expected type '{}', but got type '{}'", expected, actual);
	}

	namespace log {
		struct FileSink::Impl {
			std::ofstream file;
		};

		FileSink::FileSink(string_view path) : impl(make_unique<Impl>()) {
			const std::filesystem::path file_path(string(path));
			const std::filesystem::path parent = file_path.parent_path();
			if (!parent.empty()) {
				std::filesystem::create_directories(parent);
			}
			impl->file.open(file_path, std::ios::out | std::ios::app);
		}

		FileSink::~FileSink() {
			Flush();
		}

		void FileSink::Write(const Record&, string_view rendered_line) {
			if (impl->file) {
				impl->file << rendered_line << '\n';
			}
		}

		void FileSink::Flush() {
			if (impl && impl->file) {
				impl->file.flush();
			}
		}

		void ConsoleSink::Write(const Record& record, string_view rendered_line) {
			std::ostream& stream = static_cast<int>(record.level) >= static_cast<int>(Level::Warning) ? std::cerr : std::cout;
			stream << console_color(record.level) << PlainText(rendered_line) << "\x1b[0m\n";
		}

		string_view LevelName(Level level) {
			switch (level) {
			case Level::Trace: return "TRACE";
			case Level::Debug: return "DEBUG";
			case Level::Info: return "INFO";
			case Level::Warning: return "WARNING";
			case Level::Error: return "ERROR";
			case Level::Critical: return "CRITICAL";
			case Level::Assert: return "ASSERT";
			case Level::None: return "NONE";
			}
			return "INFO";
		}

		bool IsStructuredFieldKey(string_view field) {
			auto is_ident_start = [](char c) {
				return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
			};
			auto is_ident_continue = [&](char c) {
				return is_ident_start(c) || (c >= '0' && c <= '9');
			};

			if (field.empty() || !is_ident_start(field.front())) {
				return false;
			}
			bool expect_ident_start = false;
			for (size_t i = 1; i < field.size(); ++i) {
				const char c = field[i];
				if (expect_ident_start) {
					if (!is_ident_start(c)) {
						return false;
					}
					expect_ident_start = false;
					continue;
				}
				if (c == '.') {
					if (i + 1 == field.size()) {
						return false;
					}
					expect_ident_start = true;
					continue;
				}
				if (!is_ident_continue(c)) {
					return false;
				}
			}
			return !expect_ident_start;
		}

		string EscapeAnnotationText(string_view text) {
			string escaped;
			escaped.reserve(text.size());
			for (char c : text) {
				switch (c) {
				case '\\':
				case '{':
				case '}':
				case '|':
					escaped += '\\';
					escaped += c;
					break;
				default:
					escaped += c;
					break;
				}
			}
			return escaped;
		}

		string Field(string_view field, string_view display) {
			if (field.empty()) {
				return EscapeAnnotationText(display);
			}
			return format("{{{}|{}}}", EscapeAnnotationText(display), EscapeAnnotationText(field));
		}

		string Field(string_view field, const char* display) {
			return Field(field, string_view(display ? display : ""));
		}

		string Field(string_view field, const string& display) {
			return Field(field, string_view(display));
		}

		namespace {
			bool is_escaped(string_view text, size_t index) {
				size_t slashes = 0;
				while (index > slashes && text[index - slashes - 1] == '\\') {
					++slashes;
				}
				return slashes % 2 == 1;
			}

			void append_unescaped(string& out, string_view text) {
				for (size_t i = 0; i < text.size(); ++i) {
					if (text[i] == '\\' && i + 1 < text.size()) {
						const char next = text[i + 1];
						if (next == '{' || next == '}' || next == '|' || next == '\\') {
							out += next;
							++i;
							continue;
						}
					}
					out += text[i];
				}
			}

			void push_text_segment(vector<TextSegment>& segments, string_view text) {
				if (text.empty()) {
					return;
				}
				if (!segments.empty() && !segments.back().annotated) {
					append_unescaped(segments.back().text, text);
					return;
				}
				TextSegment segment;
				append_unescaped(segment.text, text);
				segments.push_back(std::move(segment));
			}
		}

		vector<TextSegment> ParseAnnotatedText(string_view text) {
			vector<TextSegment> segments;
			size_t cursor = 0;
			while (cursor < text.size()) {
				const size_t open = text.find('{', cursor);
				if (open == string_view::npos) {
					push_text_segment(segments, text.substr(cursor));
					break;
				}
				if (is_escaped(text, open)) {
					push_text_segment(segments, text.substr(cursor, open - cursor + 1));
					cursor = open + 1;
					continue;
				}

				push_text_segment(segments, text.substr(cursor, open - cursor));
				size_t close = open + 1;
				bool nested_open = false;
				for (; close < text.size(); ++close) {
					if (text[close] == '{' && !is_escaped(text, close)) {
						nested_open = true;
						break;
					}
					if (text[close] == '}' && !is_escaped(text, close)) {
						break;
					}
				}
				if (close >= text.size() || nested_open) {
					push_text_segment(segments, text.substr(open, nested_open ? close - open + 1 : text.size() - open));
					cursor = nested_open ? close + 1 : text.size();
					continue;
				}

				const string_view body = text.substr(open + 1, close - open - 1);
				size_t separator = string_view::npos;
				for (size_t i = 0; i < body.size(); ++i) {
					if (body[i] == '|' && !is_escaped(body, i)) {
						separator = i;
						break;
					}
				}
				if (separator == string_view::npos || separator == 0 || separator + 1 >= body.size()) {
					push_text_segment(segments, text.substr(open, close - open + 1));
					cursor = close + 1;
					continue;
				}

				TextSegment segment;
				append_unescaped(segment.text, body.substr(0, separator));
				append_unescaped(segment.field, body.substr(separator + 1));
				segment.annotated = true;
				segment.structured_field = IsStructuredFieldKey(segment.field);
				segments.push_back(std::move(segment));
				cursor = close + 1;
			}
			return segments;
		}

		string PlainText(string_view text) {
			string plain;
			for (const TextSegment& segment : ParseAnnotatedText(text)) {
				plain += segment.text;
			}
			return plain;
		}

		void SetMinimumLevel(Level level) {
			std::scoped_lock lock(state().mutex);
			state().min_level = level;
		}

		Level GetMinimumLevel() {
			std::scoped_lock lock(state().mutex);
			return state().min_level;
		}

		void AddSink(unique<Sink> sink) {
			if (!sink) {
				return;
			}
			std::scoped_lock lock(state().mutex);
			logger_state& logger = state();
			ensure_worker_started(logger);
			logger.sinks.emplace_back(std::move(sink));
		}

		void ClearSinks() {
			Flush();
			std::scoped_lock lock(state().mutex);
			state().sinks.clear();
		}

		void Flush() {
			logger_state& logger = state();
			std::unique_lock lock(logger.mutex);
			if (!logger.worker_started) {
				flush_sinks(logger);
				return;
			}
			logger.queue.push_back({{}, true});
			logger.wake.notify_one();
			logger.drained.wait(lock, [&] { return logger.pending_records == 0 && logger.queue.empty(); });
		}

		void Shutdown() {
			std::thread worker;
			{
				std::unique_lock lock(state().mutex);
				logger_state& logger = state();
				if (!logger.worker_started) {
					flush_sinks(logger);
					logger.sinks.clear();
					return;
				}
				logger.stopping = true;
				logger.wake.notify_one();
				worker = std::move(logger.worker);
				logger.worker_started = false;
			}
			if (worker.joinable()) {
				worker.join();
			}
			std::scoped_lock lock(state().mutex);
			state().sinks.clear();
			state().queue.clear();
			state().pending_records = 0;
		}

		void ConfigureFromEnvironment() {
			std::scoped_lock lock(state().mutex);
			logger_state& logger = state();
			if (logger.environment_configured) {
				return;
			}
			logger.environment_configured = true;

			if (const char* level = std::getenv("LEAF_LOG_LEVEL")) {
				Level parsed;
				if (parse_log_level(level, parsed)) {
					logger.min_level = parsed;
				}
			}

			if (logger.sinks.empty()) {
				if (const char* console = std::getenv("LEAF_LOG_CONSOLE")) {
					if (parse_bool(console, true)) {
						logger.sinks.emplace_back(make_unique<ConsoleSink>());
					}
				} else {
					logger.sinks.emplace_back(make_unique<ConsoleSink>());
				}
				if (const char* file = std::getenv("LEAF_LOG_FILE")) {
					if (string_view(file).size() > 0) {
						logger.sinks.emplace_back(make_unique<FileSink>(file));
					}
				}
			}
		}

		void Write(Record record) {
			ConfigureFromEnvironment();
			logger_state& logger = state();
			std::scoped_lock lock(logger.mutex);
			if (!should_log(record.level, logger.min_level)) {
				return;
			}
			ensure_worker_started(logger);
			record.sequence = logger.next_sequence.fetch_add(1, std::memory_order_relaxed);
			record.timestamp = format_timestamp();
			logger.queue.push_back({std::move(record), false});
			++logger.pending_records;
			logger.wake.notify_one();
		}
	} // namespace log

	void log_info(string_view message) {
		log::Info("leaf", "{}", message);
	}

	void log_warning(string_view message) {
		log::Warning("leaf", "{}", message);
	}

	void log_error(string_view message) {
		log::Error("leaf", "{}", message);
	}
} // namespace lf
