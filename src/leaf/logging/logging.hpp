#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>

#include <chrono>
#include <cstddef>
#include <format>
#include <fstream>
#include <source_location>
#include <utility>

namespace lf::log {
	struct Text {
		string_view value;
		std::source_location location;

		template <std::size_t N>
		consteval Text(const char (&value)[N], std::source_location location = std::source_location::current()) : value(value, N - 1), location(location) {}
	};

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
		string message;
		std::source_location location = std::source_location::current();
		u64 sequence = 0;
		std::chrono::system_clock::time_point timestamp;
	};

	struct Sink {
		virtual ~Sink() = default;
		virtual void write(const Record& record, string_view line) = 0;
		virtual void flush() {}
	};

	struct ConsoleSink : Sink {
		void write(const Record& record, string_view line) override;
	};

	struct FileSink : Sink {
		explicit FileSink(string_view path);
		~FileSink() override;
		void write(const Record& record, string_view line) override;
		void flush() override;

	private:
		std::ofstream file;
	};

	void set_minimum_level(Level level);
	Level get_minimum_level();
	void add_sink(unique_ptr<Sink> sink);
	void clear_sinks();
	void flush();
	void write(Record record);

	template <typename... Args>
	void log(Level level, Text text, Args&&... args) {
		string message;
		if constexpr (sizeof...(Args) == 0) {
			message = string(text.value);
		} else {
			message = std::vformat(text.value, std::make_format_args(args...));
		}
		write({level, std::move(message), text.location});
	}

	template <typename... Args>
	void Trace(Text text, Args&&... args) {
		log(Level::Trace, text, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Debug(Text text, Args&&... args) {
		log(Level::Debug, text, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Info(Text text, Args&&... args) {
		log(Level::Info, text, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Warning(Text text, Args&&... args) {
		log(Level::Warning, text, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Error(Text text, Args&&... args) {
		log(Level::Error, text, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Critical(Text text, Args&&... args) {
		log(Level::Critical, text, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Assert(Text text, Args&&... args) {
		log(Level::Assert, text, std::forward<Args>(args)...);
	}
} // namespace lf::log

namespace lf {
	error init_logging(span<string_view> args);
	void exit_logging();
} // namespace lf
