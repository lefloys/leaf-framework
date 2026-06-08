#include <catch2/catch_test_macros.hpp>

#include "leaf/application/elements/window.hpp"

using namespace lf;

/// Verifies that window element defaults are injected into document head.
TEST_CASE("Window element defaults are injected into document head", "[window-element]") {
	const string source = "<rml><head><title>Test</title></head><body></body></rml>";

	string result = install_window_defaults(source);

	REQUIRE(result.find("window-defaults") != string::npos);
	REQUIRE(result.find("<head>\n<style id=\"window-defaults\">") != string::npos);
}

/// Verifies that window element defaults are not injected twice.
TEST_CASE("Window element defaults are not injected twice", "[window-element]") {
	const string source = "<rml><head></head><body></body></rml>";

	string once = install_window_defaults(source);
	string twice = install_window_defaults(once);

	REQUIRE(twice == once);
}

