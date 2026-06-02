#include "logging.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace lf::log::detail {
	struct log_item {
		Record record;
		bool flush = false;
	};

	struct logger_state {
		std::mutex mutex;
		std::condition_variable wake;
		std::condition_variable drained;
		std::deque<log_item> queue;
		std::vector<unique_ptr<Sink>> sinks;
		std::thread worker;
		std::atomic<u64> next_sequence = 1;
		Level min_level = Level::Info;
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

	bool parse_log_level(string_view value, Level& out) {
		const string normalized = lowercase(value);
		if (normalized == "trace") { out = Level::Trace; return true; }
		if (normalized == "debug") { out = Level::Debug; return true; }
		if (normalized == "info") { out = Level::Info; return true; }
		if (normalized == "warning" || normalized == "warn") { out = Level::Warning; return true; }
		if (normalized == "error") { out = Level::Error; return true; }
		if (normalized == "critical" || normalized == "fatal") { out = Level::Critical; return true; }
		if (normalized == "assert") { out = Level::Assert; return true; }
		if (normalized == "none" || normalized == "off") { out = Level::None; return true; }
		return false;
	}

	bool parse_bool(string_view value, bool fallback) {
		const string normalized = lowercase(value);
		if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") { return true; }
		if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") { return false; }
		return fallback;
	}

	bool should_log(Level level, Level min_level) {
		if (level == Level::None || min_level == Level::None) {
			return false;
		}
		return static_cast<int>(level) >= static_cast<int>(min_level);
	}

	std::chrono::system_clock::time_point current_timestamp() {
		return std::chrono::system_clock::now();
	}

	string_view level_name(Level level) {
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

	string render_timestamp(std::chrono::system_clock::time_point timestamp) {
		const std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
		std::tm local_time{};
#if defined(_WIN32)
		localtime_s(&local_time, &time);
#else
		localtime_r(&time, &local_time);
#endif

		const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
		std::ostringstream stream;
		stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
		stream << '.' << std::setw(3) << std::setfill('0') << millis.count();
		return stream.str();
	}

	string render_line(const Record& record) {
		return format("[{}] [{:<5}] {}", render_timestamp(record.timestamp), level_name(record.level), record.message);
	}

	string_view console_color(Level level) {
		switch (level) {
		case Level::Trace: return "\x1b[90m";
		case Level::Debug: return "\x1b[37m";
		case Level::Info: return "\x1b[97m";
		case Level::Warning: return "\x1b[33m";
		case Level::Error: return "\x1b[31m";
		case Level::Critical: return "\x1b[35m";
		case Level::Assert: return "\x1b[36m";
		case Level::None: return "";
		}
		return "";
	}

	void flush_sinks(logger_state& logger) {
		for (unique_ptr<Sink>& sink : logger.sinks) {
			sink->flush();
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
				for (unique_ptr<Sink>& sink : logger.sinks) {
					sink->write(item.record, line);
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

} // namespace lf::log::detail

namespace lf::log {
	FileSink::FileSink(string_view path) {
		const std::filesystem::path file_path{string(path)};
		const std::filesystem::path parent = file_path.parent_path();
		if (!parent.empty()) {
			std::filesystem::create_directories(parent);
		}
		file.open(file_path, std::ios::out | std::ios::app);
	}

	FileSink::~FileSink() {
		flush();
	}

	void FileSink::write(const Record&, string_view line) {
		if (file) {
			file << line << '\n';
		}
	}

	void FileSink::flush() {
		if (file) {
			file.flush();
		}
	}

	void ConsoleSink::write(const Record& record, string_view line) {
		std::ostream& stream = static_cast<int>(record.level) >= static_cast<int>(Level::Warning) ? std::cerr : std::cout;
		stream << detail::console_color(record.level) << line << "\x1b[0m\n";
	}

	void set_minimum_level(Level level) {
		std::scoped_lock lock(detail::state().mutex);
		detail::state().min_level = level;
	}

	Level get_minimum_level() {
		std::scoped_lock lock(detail::state().mutex);
		return detail::state().min_level;
	}

	void add_sink(unique_ptr<Sink> sink) {
		if (!sink) {
			return;
		}
		std::scoped_lock lock(detail::state().mutex);
		detail::logger_state& logger = detail::state();
		logger.sinks.emplace_back(std::move(sink));
	}

	void clear_sinks() {
		flush();
		std::scoped_lock lock(detail::state().mutex);
		detail::state().sinks.clear();
	}

	void flush() {
		detail::logger_state& logger = detail::state();
		std::unique_lock lock(logger.mutex);
		logger.queue.push_back({{}, true});
		logger.wake.notify_one();
		logger.drained.wait(lock, [&] { return logger.pending_records == 0 && logger.queue.empty(); });
	}

	void write(Record record) {
		detail::logger_state& logger = detail::state();
		std::scoped_lock lock(logger.mutex);
		if (!detail::should_log(record.level, logger.min_level)) {
			return;
		}
		record.sequence = logger.next_sequence.fetch_add(1, std::memory_order_relaxed);
		record.timestamp = detail::current_timestamp();
		logger.queue.push_back({std::move(record), false});
		++logger.pending_records;
		logger.wake.notify_one();
	}
} // namespace lf::log

namespace lf {
	error init_logging(span<string_view>) {
		std::scoped_lock lock(log::detail::state().mutex);
		log::detail::logger_state& logger = log::detail::state();

		if (const char* level = std::getenv("LEAF_LOG_LEVEL")) {
			log::Level parsed;
			if (log::detail::parse_log_level(level, parsed)) {
				logger.min_level = parsed;
			}
		}

		if (logger.sinks.empty()) {
			if (const char* console = std::getenv("LEAF_LOG_CONSOLE")) {
				if (log::detail::parse_bool(console, true)) {
					logger.sinks.emplace_back(make_unique<log::ConsoleSink>());
				}
			} else {
				logger.sinks.emplace_back(make_unique<log::ConsoleSink>());
			}
			if (const char* file = std::getenv("LEAF_LOG_FILE")) {
				if (string_view(file).size() > 0) {
					logger.sinks.emplace_back(make_unique<log::FileSink>(file));
				}
			}
		}

		logger.stopping = false;
		logger.worker = std::thread(log::detail::worker_loop);
		return {};
	}

	void exit_logging() {
		std::thread worker;
		{
			std::unique_lock lock(log::detail::state().mutex);
			log::detail::logger_state& logger = log::detail::state();
			logger.stopping = true;
			logger.wake.notify_one();
			worker = std::move(logger.worker);
		}
		worker.join();
		std::scoped_lock lock(log::detail::state().mutex);
		log::detail::state().sinks.clear();
		log::detail::state().queue.clear();
		log::detail::state().pending_records = 0;
	}
} // namespace lf
