#include <catch2/catch_test_macros.hpp>

#include "leaf/core/filesystem.hpp"
#include "leaf/script/mod_enabled.hpp"
#include "leaf/script/mod_loader.hpp"
#include "leaf/script/prototype.hpp"
#include "leaf/script/registry.hpp"
#include "leaf/system/system.hpp"

#include <chrono>
#include <fstream>
#include <iterator>
#include <unordered_map>

namespace {
	struct TestModPrototype : lf::Prototype<lf::identifier<TestModPrototype, u16, void>> {
		explicit TestModPrototype(const lf::dict& data) : Prototype(data) {
			if (has_field(data, "value")) {
				load_field(data, "value", value);
			}
		}

		static lf::string_view type() {
			return "test";
		}

		lf::string value;
	};

	struct TestModWorkspace {
		lf::fs::path root;
		lf::fs::path appdata;

		TestModWorkspace() {
			auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
			root = std::filesystem::temp_directory_path() / lf::format("ooo-mod-loader-test-{}", stamp);
			appdata = root / "appdata";
			std::filesystem::create_directories(appdata);
			lf::OverwriteAppdataDir(appdata.string());
		}

		~TestModWorkspace() {
			lf::UnloadMods();
			std::error_code ec;
			std::filesystem::remove_all(root, ec);
		}

		void write_file(const lf::fs::path& path, lf::string_view text) {
			std::filesystem::create_directories(path.parent_path());
			std::ofstream out(path, std::ios::binary);
			out << text;
		}

		lf::fs::path mod_dir(lf::string_view name) const {
			return root / "mods" / lf::string(name);
		}

		void write_mod(lf::string_view name, lf::string_view data_lua, lf::string_view extra_files = {}) {
			lf::fs::path dir = mod_dir(name);
			write_file(dir / "info.yaml", lf::format(
				"name: {}\n"
				"mod_version: \"0.1.0\"\n"
				"title: {}\n"
				"author: test\n"
				"game_version: \"0.1.0\"\n",
				name,
				name
			));
			write_file(dir / "data.lua", data_lua);
			if (!extra_files.empty()) {
				write_file(dir / "extra.lua", extra_files);
			}
		}

		void write_core(lf::string_view null_lua = R"(data:extend({{ type = "test", name = "unknown" }})
)") {
			lf::fs::path dir = mod_dir("core");
			write_file(dir / "info.yaml",
				"name: core\n"
				"mod_version: \"0.1.0\"\n"
				"title: core\n"
				"author: test\n"
				"game_version: \"0.1.0\"\n"
			);
			write_file(dir / "null.lua", null_lua);
			write_file(dir / "data.lua",
				"option.scene.main = { path = \"__core__/main.rml\" }\n"
			);
		}

		lf::error load() {
			lf::ModCollection mods;
			mods.add_privileged_dir(root / "mods");
			return lf::LoadMods(mods);
		}
	};

	void register_test_type_once() {
		static bool registered = false;
		if (!registered) {
			lf::PrototypeTypeRegistry::RegisterType<TestModPrototype>();
			registered = true;
		}
	}
} // namespace

namespace lf {
	template <>
	PrototypeFieldList inspect_runtime_fields(const TestModPrototype&) {
		return {};
	}
} // namespace lf

/// Verifies that mod loader data:extend adds prototypes to existing raw tables.
TEST_CASE("mod loader data:extend adds prototypes to existing raw tables", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core();
	workspace.write_mod("base", R"(
data:extend({
  { type = "test", name = "alpha", value = "one" },
  { type = "test", name = "beta", value = "two" },
})
)");

	lf::error err = workspace.load();
	REQUIRE(!err);
	REQUIRE(lf::Database<TestModPrototype>::prototypes.size() == 3);
	REQUIRE(lf::Database<TestModPrototype>::prototypes[1].name == "alpha");
	REQUIRE(lf::Database<TestModPrototype>::prototypes[1].value == "one");
	REQUIRE(lf::Database<TestModPrototype>::prototypes[2].name == "beta");
	REQUIRE(lf::Database<TestModPrototype>::prototypes[2].value == "two");
}

/// Verifies that mod loader assigns deterministic prototype ids independent of directory iteration.
TEST_CASE("mod loader assigns deterministic prototype ids independent of directory iteration", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core();
	workspace.write_mod("zeta", R"(
data:extend({{ type = "test", name = "zeta", value = "last" }})
)");
	workspace.write_mod("alpha", R"(
data:extend({{ type = "test", name = "alpha", value = "first" }})
)");

	lf::error err = workspace.load();
	REQUIRE(!err);
	REQUIRE(lf::Database<TestModPrototype>::prototypes.size() == 3);
	REQUIRE(lf::Database<TestModPrototype>::prototypes[1].name == "alpha");
	REQUIRE(lf::Database<TestModPrototype>::prototypes[2].name == "zeta");
	REQUIRE(lf::Database<TestModPrototype>::find("alpha").get() == 1);
	REQUIRE(lf::Database<TestModPrototype>::find("zeta").get() == 2);
}

/// Verifies that enabled mods file is written in deterministic name order.
TEST_CASE("enabled mods file is written in deterministic name order", "[mod-loader]") {
	TestModWorkspace workspace;
	std::unordered_map<lf::string, lf::ModEnabledInfo> mods;
	mods["zeta"] = lf::ModEnabledInfo{ true, lf::version{ 1, 0, 0 } };
	mods["alpha"] = lf::ModEnabledInfo{ true, lf::version{ 1, 0, 0 } };

	const lf::fs::path path = workspace.root / "enabled_mods.yaml";
	lf::save_enabled_mods(path, mods);

	std::ifstream in(path, std::ios::binary);
	lf::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	REQUIRE(text.find("alpha:") != lf::string::npos);
	REQUIRE(text.find("zeta:") != lf::string::npos);
	REQUIRE(text.find("alpha:") < text.find("zeta:"));
}

/// Verifies that mod loader data:extend rejects unknown prototype types.
TEST_CASE("mod loader data:extend rejects unknown prototype types", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core();
	workspace.write_mod("base", R"(
data:extend({{ type = "missing-type", name = "alpha" }})
)");

	lf::error err = workspace.load();
	REQUIRE(err);
	REQUIRE(err.message.find("data.raw[missing-type]") != lf::string::npos);
}

/// Verifies that mod loader data:extend rejects duplicate prototypes.
TEST_CASE("mod loader data:extend rejects duplicate prototypes", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core();
	workspace.write_mod("base", R"(
data:extend({{ type = "test", name = "alpha" }})
data:extend({{ type = "test", name = "alpha" }})
)");

	lf::error err = workspace.load();
	REQUIRE(err);
	REQUIRE(err.message.find("duplicate prototype 'test/alpha'") != lf::string::npos);
}

/// Verifies that mod loader keeps direct data.raw assignment behavior.
TEST_CASE("mod loader keeps direct data.raw assignment behavior", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core();
	workspace.write_mod("base", R"(
data.raw["test"]["alpha"] = { value = "first" }
data.raw["test"]["alpha"] = { value = "second" }
)");

	lf::error err = workspace.load();
	REQUIRE(!err);
	REQUIRE(lf::Database<TestModPrototype>::find("alpha"));
	REQUIRE(lf::Database<TestModPrototype>::prototypes[1].value == "second");
}

/// Verifies that mod loader include resolves relative and nested files.
TEST_CASE("mod loader include resolves relative and nested files", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core();
	workspace.write_mod("base",
		"include(\"prototypes/first.lua\")\n"
	);
	workspace.write_file(workspace.mod_dir("base") / "prototypes" / "first.lua",
		"data:extend({{ type = \"test\", name = \"first\", value = \"one\" }})\n"
		"include(\"nested/second.lua\")\n"
	);
	workspace.write_file(workspace.mod_dir("base") / "prototypes" / "nested" / "second.lua",
		"data:extend({{ type = \"test\", name = \"second\", value = \"two\" }})\n"
	);

	lf::error err = workspace.load();
	REQUIRE(!err);
	REQUIRE(lf::Database<TestModPrototype>::find("first"));
	REQUIRE(lf::Database<TestModPrototype>::find("second"));
}

/// Verifies that mod loader include supports virtual paths.
TEST_CASE("mod loader include supports virtual paths", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core();
	workspace.write_mod("base",
		"include(\"__base__/prototypes/virtual.lua\")\n"
	);
	workspace.write_file(workspace.mod_dir("base") / "prototypes" / "virtual.lua",
		"data:extend({{ type = \"test\", name = \"virtual\" }})\n"
	);

	lf::error err = workspace.load();
	REQUIRE(!err);
	REQUIRE(lf::Database<TestModPrototype>::find("virtual"));
}

/// Verifies that mod loader null prototype script uses the regular include pipeline.
TEST_CASE("mod loader null prototype script uses the regular include pipeline", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core("include(\"null/unknown.lua\")\n");
	workspace.write_file(workspace.mod_dir("core") / "null" / "unknown.lua",
		"data:extend({{ type = \"test\", name = \"unknown\", value = \"null-include\" }})\n"
	);
	workspace.write_mod("base", R"(
data:extend({{ type = "test", name = "alpha", value = "runtime" }})
)");

	lf::error err = workspace.load();
	REQUIRE(!err);
	REQUIRE(lf::Database<TestModPrototype>::prototypes.size() == 2);
	REQUIRE(lf::Database<TestModPrototype>::prototypes[0].name == "unknown");
	REQUIRE(lf::Database<TestModPrototype>::prototypes[0].value == "null-include");
	REQUIRE(lf::Database<TestModPrototype>::prototypes[1].name == "alpha");
}

/// Verifies that mod loader include reports missing and escaping files.
TEST_CASE("mod loader include reports missing and escaping files", "[mod-loader]") {
	register_test_type_once();
	{
		TestModWorkspace workspace;
		workspace.write_core();
		workspace.write_mod("base", "include(\"missing.lua\")\n");

		lf::error err = workspace.load();
		REQUIRE(err);
		REQUIRE(err.message.find("missing.lua") != lf::string::npos);
	}
	{
		TestModWorkspace workspace;
		workspace.write_core();
		workspace.write_mod("base", "include(\"../outside.lua\")\n");

		lf::error err = workspace.load();
		REQUIRE(err);
		REQUIRE(err.message.find("escapes") != lf::string::npos);
	}
}

/// Verifies that mod loader null prototype script reports include errors.
TEST_CASE("mod loader null prototype script reports include errors", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	workspace.write_core("include(\"missing-null.lua\")\n");

	lf::error err = workspace.load();
	REQUIRE(err);
	REQUIRE(err.message.find("core/null.lua") != lf::string::npos);
	REQUIRE(err.message.find("missing-null.lua") != lf::string::npos);
}

/// Verifies that mod loader requires the core null prototype script.
TEST_CASE("mod loader requires the core null prototype script", "[mod-loader]") {
	register_test_type_once();
	TestModWorkspace workspace;
	lf::fs::path core_dir = workspace.mod_dir("core");
	workspace.write_file(core_dir / "info.yaml",
		"name: core\n"
		"mod_version: \"0.1.0\"\n"
		"title: core\n"
		"author: test\n"
		"game_version: \"0.1.0\"\n"
	);
	workspace.write_file(core_dir / "data.lua",
		"option.scene.main = { path = \"__core__/main.rml\" }\n"
	);

	lf::error err = workspace.load();
	REQUIRE(err);
	REQUIRE(err.message.find("core/null.lua") != lf::string::npos);
}
