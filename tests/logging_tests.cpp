#include <array>
#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <leaf/logging/logging.hpp>

namespace lf::log::test {
	struct captured_log {
		std::vector<Record> records;
		std::vector<string> lines;
		int flushes = 0;
	};

	struct capture_sink : Sink {
		explicit capture_sink(std::shared_ptr<captured_log> captured) : captured(std::move(captured)) {}

		void write(const Record& record, string_view line) override {
			captured->records.push_back(record);
			captured->lines.emplace_back(line);
		}

		void flush() override {
			++captured->flushes;
		}

		std::shared_ptr<captured_log> captured;
	};
} // namespace lf::log::test

/// Verifies that logging sink receives formatted filtered records.
TEST_CASE("logging sink receives formatted filtered records", "[logging]") {
	std::array<lf::string_view, 0> args{};
	REQUIRE(!lf::init_logging(args));
	lf::log::clear_sinks();
	lf::log::set_minimum_level(lf::log::Level::Warning);

	const auto captured = std::make_shared<lf::log::test::captured_log>();
	lf::log::add_sink(lf::make_unique<lf::log::test::capture_sink>(captured));

	lf::log::Info("ignored {}", 1);
	lf::log::Warning("kept {}", 2);
	lf::log::flush();

	REQUIRE(captured->records.size() == 1);
	REQUIRE(captured->records[0].level == lf::log::Level::Warning);
	REQUIRE(captured->records[0].message == "kept 2");
	REQUIRE(captured->records[0].sequence > 0);
	REQUIRE(captured->records[0].timestamp != std::chrono::system_clock::time_point{});
	REQUIRE(captured->lines.size() == 1);
	REQUIRE(captured->lines[0].find("[WARNING]") != lf::string::npos);
	REQUIRE(captured->flushes > 0);

	lf::log::set_minimum_level(lf::log::Level::Info);
	lf::exit_logging();
}
