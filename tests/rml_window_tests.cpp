#include <catch2/catch_test_macros.hpp>

#include "leaf/application/rml_window.hpp"
#include "leaf/application/rml_window_internal.hpp"

using namespace lf;

TEST_CASE("RML window defaults are injected into document head", "[rml-window]") {
	const string source = "<rml><head><title>Test</title></head><body></body></rml>";

	string result = install_rml_window_defaults(source);

	REQUIRE(result.find("rml-window-defaults") != string::npos);
	REQUIRE(result.find("<head>\n<style id=\"rml-window-defaults\">") != string::npos);
}

TEST_CASE("RML window defaults are not injected twice", "[rml-window]") {
	const string source = "<rml><head></head><body></body></rml>";

	string once = install_rml_window_defaults(source);
	string twice = install_rml_window_defaults(once);

	REQUIRE(twice == once);
}

TEST_CASE("RML window boolean opt-out values are recognized", "[rml-window]") {
	using rml_window_detail::false_attribute_value;

	REQUIRE(false_attribute_value("false"));
	REQUIRE(false_attribute_value("0"));
	REQUIRE(false_attribute_value("disabled"));
	REQUIRE(false_attribute_value("no"));
	REQUIRE(false_attribute_value("off"));

	REQUIRE_FALSE(false_attribute_value(""));
	REQUIRE_FALSE(false_attribute_value("true"));
	REQUIRE_FALSE(false_attribute_value("1"));
	REQUIRE_FALSE(false_attribute_value("yes"));
}

TEST_CASE("RML window resize edge strings are parsed", "[rml-window]") {
	using rml_window_detail::ResizeEdge;
	using rml_window_detail::has_resize_edge;
	using rml_window_detail::resize_edge_from_string;

	ResizeEdge north_east = resize_edge_from_string("ne");
	REQUIRE(has_resize_edge(north_east, ResizeEdge::North));
	REQUIRE(has_resize_edge(north_east, ResizeEdge::East));
	REQUIRE_FALSE(has_resize_edge(north_east, ResizeEdge::South));
	REQUIRE_FALSE(has_resize_edge(north_east, ResizeEdge::West));

	ResizeEdge south_west = resize_edge_from_string("sw");
	REQUIRE(has_resize_edge(south_west, ResizeEdge::South));
	REQUIRE(has_resize_edge(south_west, ResizeEdge::West));
	REQUIRE_FALSE(has_resize_edge(resize_edge_from_string(""), ResizeEdge::North));
}
