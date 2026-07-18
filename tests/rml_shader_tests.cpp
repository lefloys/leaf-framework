#include <catch2/catch_test_macros.hpp>

#include <rtsl/hlsl.hpp>
#include <rtsl/sdk/program.hpp>
#include <rtsl/spirv.hpp>

#include <algorithm>

extern "C" const rtsl::ProgramBytes leaf_rml_ui_rtslp;

TEST_CASE("embedded RmlUi shader transpiles for both graphics backends", "[graphics][rtsl]") {
	auto program = rtsl::load_program(leaf_rml_ui_rtslp);
	REQUIRE(program.has_value());

	for (const rtsl::Stage stage : { rtsl::Stage::vertex, rtsl::Stage::fragment }) {
		auto hlsl = rtsl::hlsl::transpile(*program, stage);
		REQUIRE(hlsl.has_value());
		REQUIRE_FALSE(hlsl->source.empty());

		auto spirv = rtsl::spirv::transpile(*program, stage);
		REQUIRE(spirv.has_value());
		REQUIRE_FALSE(spirv->words.empty());
	}

	const auto& resources = program->resources();
	REQUIRE(resources.size() == 2);
	REQUIRE(std::ranges::any_of(resources, [](const rtsl::Resource& resource) {
		return resource.name == "UiDraw" && resource.kind == rtsl::ResourceKind::uniform_buffer &&
			   rtsl::contains(resource.stages, rtsl::Stage::vertex) &&
			   rtsl::contains(resource.stages, rtsl::Stage::fragment);
	}));
	REQUIRE(std::ranges::any_of(resources, [](const rtsl::Resource& resource) {
		return resource.name == "UiTexture" && resource.kind == rtsl::ResourceKind::sampled_texture &&
			   !rtsl::contains(resource.stages, rtsl::Stage::vertex) &&
			   rtsl::contains(resource.stages, rtsl::Stage::fragment);
	}));
}
