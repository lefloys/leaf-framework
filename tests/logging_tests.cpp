#include <chrono>
#include <memory>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <leaf/core/logging.hpp>

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
	lf::log::Logger& logger = lf::log::Logger::instance();
	logger.clear_sinks();
	logger.set_minimum_level(lf::log::Level::Warning);
	REQUIRE(logger.get_minimum_level() == lf::log::Level::Warning);

	const auto captured = std::make_shared<lf::log::test::captured_log>();
	logger.add_sink(lf::make_unique<lf::log::test::capture_sink>(captured));

	lf::log::Info("ignored {}", 1);
	lf::log::Warning("kept {}", 2);
	logger.flush();

	REQUIRE(captured->records.size() == 1);
	REQUIRE(captured->records[0].level == lf::log::Level::Warning);
	REQUIRE(captured->records[0].message == "kept 2");
	REQUIRE(captured->records[0].timestamp != std::chrono::system_clock::time_point{});
	REQUIRE(captured->lines.size() == 1);
	REQUIRE(captured->lines[0].find("[WARN]") != lf::string::npos);
	REQUIRE(captured->lines[0].find("kept 2") != lf::string::npos);
	REQUIRE(captured->flushes > 0);

	logger.clear_sinks();
	logger.set_minimum_level(lf::log::Level::Info);
}

/// Verifies that record sequence numbers increase monotonically.
TEST_CASE("logging records carry increasing sequence numbers", "[logging]") {
	lf::log::Logger& logger = lf::log::Logger::instance();
	logger.clear_sinks();
	logger.set_minimum_level(lf::log::Level::Info);

	const auto captured = std::make_shared<lf::log::test::captured_log>();
	logger.add_sink(lf::make_unique<lf::log::test::capture_sink>(captured));

	lf::log::Info("first");
	lf::log::Error("second");
	logger.flush();

	REQUIRE(captured->records.size() == 2);
	REQUIRE(captured->records[0].message == "first");
	REQUIRE(captured->records[1].message == "second");
	REQUIRE(captured->records[1].sequence > captured->records[0].sequence);

	logger.clear_sinks();
}
