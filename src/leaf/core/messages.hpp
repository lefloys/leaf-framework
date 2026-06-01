#pragma once

#include "format.hpp"
#include "string.hpp"

#include "memory.hpp"
#include "vector.hpp"

#include <source_location>
#include <utility>

namespace lf {
	string make_missing_field_error_message(string_view field_name);
	string make_type_mismatch_error_message(string_view expected, string_view actual);

	namespace log {
		enum class Level {
			Trace,
			Debug,
			Info,
			Warning,
			Error,
			Critical,
			Assert,
			None,
		};

		struct Record {
			Level level = Level::Info;
			string category = "leaf";
			string message;
			std::source_location location = std::source_location::current();
			u64 sequence = 0;
			string timestamp;
		};

		struct TextSegment {
			string text;
			string field;
			bool annotated = false;
			bool structured_field = false;
		};

		struct Sink {
			virtual ~Sink() = default;
			virtual void Write(const Record& record, string_view rendered_line) = 0;
			virtual void Flush() {}
		};

		struct ConsoleSink : Sink {
			void Write(const Record& record, string_view rendered_line) override;
		};

		struct FileSink : Sink {
			explicit FileSink(string_view path);
			~FileSink() override;
			void Write(const Record& record, string_view rendered_line) override;
			void Flush() override;

		private:
			struct Impl;
			unique<Impl> impl;
		};

		string_view LevelName(Level level);
		bool IsStructuredFieldKey(string_view field);
		string EscapeAnnotationText(string_view text);
		string Field(string_view field, string_view display);
		string Field(string_view field, const char* display);
		string Field(string_view field, const string& display);
		vector<TextSegment> ParseAnnotatedText(string_view text);
		string PlainText(string_view text);
		void SetMinimumLevel(Level level);
		Level GetMinimumLevel();
		void AddSink(unique<Sink> sink);
		void ClearSinks();
		void Flush();
		void Shutdown();
		void ConfigureFromEnvironment();
		void Write(Record record);

		template <typename T>
		string Field(string_view field, const T& value) {
			return Field(field, format("{}", value));
		}

		template <typename... Args>
		void Trace(string_view category, string_view text, Args&&... args) {
			Write({Level::Trace, string(category), format(text, std::forward<Args>(args)...), std::source_location::current()});
		}

		template <typename... Args>
		void Debug(string_view category, string_view text, Args&&... args) {
			Write({Level::Debug, string(category), format(text, std::forward<Args>(args)...), std::source_location::current()});
		}

		template <typename... Args>
		void Info(string_view category, string_view text, Args&&... args) {
			Write({Level::Info, string(category), format(text, std::forward<Args>(args)...), std::source_location::current()});
		}

		template <typename... Args>
		void Warning(string_view category, string_view text, Args&&... args) {
			Write({Level::Warning, string(category), format(text, std::forward<Args>(args)...), std::source_location::current()});
		}

		template <typename... Args>
		void Error(string_view category, string_view text, Args&&... args) {
			Write({Level::Error, string(category), format(text, std::forward<Args>(args)...), std::source_location::current()});
		}

		template <typename... Args>
		void Critical(string_view category, string_view text, Args&&... args) {
			Write({Level::Critical, string(category), format(text, std::forward<Args>(args)...), std::source_location::current()});
		}

		template <typename... Args>
		void Assert(string_view category, string_view text, Args&&... args) {
			Write({Level::Assert, string(category), format(text, std::forward<Args>(args)...), std::source_location::current()});
		}
	} // namespace log

	using log_level = log::Level;
	inline string_view log_level_name(log_level level) { return log::LevelName(level); }
	inline void set_log_min_level(log_level level) { log::SetMinimumLevel(level); }
	inline log_level get_log_min_level() { return log::GetMinimumLevel(); }
	inline void set_log_file(string_view path) { log::AddSink(make_unique<log::FileSink>(path)); }
	inline void close_log_file() { log::Shutdown(); }
	inline void configure_logging_from_environment() { log::ConfigureFromEnvironment(); }
	inline void log_message(log::Record record) { log::Write(std::move(record)); }

	void log_info(string_view message);
	void log_warning(string_view message);
	void log_error(string_view message);
} // namespace lf

#define LF_LOG_TRACE(category, message) \
	::lf::log::Write({::lf::log::Level::Trace, ::lf::string(category), ::lf::string(message), std::source_location::current()})
#define LF_LOG_DEBUG(category, message) \
	::lf::log::Write({::lf::log::Level::Debug, ::lf::string(category), ::lf::string(message), std::source_location::current()})
#define LF_LOG_INFO(category, message) \
	::lf::log::Write({::lf::log::Level::Info, ::lf::string(category), ::lf::string(message), std::source_location::current()})
#define LF_LOG_WARNING(category, message) \
	::lf::log::Write({::lf::log::Level::Warning, ::lf::string(category), ::lf::string(message), std::source_location::current()})
#define LF_LOG_ERROR(category, message) \
	::lf::log::Write({::lf::log::Level::Error, ::lf::string(category), ::lf::string(message), std::source_location::current()})
#define LF_LOG_FATAL(category, message) \
	::lf::log::Write({::lf::log::Level::Critical, ::lf::string(category), ::lf::string(message), std::source_location::current()})
