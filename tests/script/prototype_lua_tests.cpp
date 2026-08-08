#include <catch2/catch_test_macros.hpp>

#include <leaf/core/dynamic_object.hpp>
#include <leaf/core/schema.hpp>
#include <leaf/resource/database.hpp>
#include <leaf/resource/prototype.hpp>
#include <leaf/script/prototype.hpp>

namespace leaf_test::prototype_lua {
	struct TestPrototype final : lf::Prototype<lf::identifier<TestPrototype, u16, void>> {
		static constexpr lf::string_view type() noexcept { return "test-prototype"; }

		explicit TestPrototype(const lf::dict& data) : Prototype(data) {}

		u32 value = 7;
		u64 seed = 11400714819323198485ull;
	};
} // namespace leaf_test::prototype_lua

template<>
struct lf::schema_trait<leaf_test::prototype_lua::TestPrototype> {
	static auto get(auto& value) {
		return lf::group(
			lf::schema(lf::PrototypeBase::base(value)),
			lf::field("value", value.value),
			lf::field("seed", value.seed)
		);
	}
};

TEST_CASE("prototype Lua export provides ordered and named records") {
	using leaf_test::prototype_lua::TestPrototype;

	lf::Database<TestPrototype>::clear();
	lf::Database<TestPrototype>::create("alpha");
	lf::dict data;
	data.emplace("name", "Alpha");
	lf::Database<TestPrototype>::init("alpha", data);

	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
	lf::ExportPrototypeTable<TestPrototype>(lua);

	sol::object prototype_tables = lua["prototypes"];
	REQUIRE(prototype_tables.valid());
	REQUIRE(prototype_tables.is<sol::table>());
	const sol::table prototypes = prototype_tables.as<sol::table>();

	sol::object records_value = prototypes["test-prototype"];
	REQUIRE(records_value.valid());
	REQUIRE(records_value.is<sol::table>());
	const sol::table records = records_value.as<sol::table>();

	sol::object record_by_id_value = records[1];
	REQUIRE(record_by_id_value.valid());
	REQUIRE(record_by_id_value.is<sol::table>());
	const sol::table record_by_id = record_by_id_value.as<sol::table>();

	sol::object record_by_name_value = records["alpha"];
	REQUIRE(record_by_name_value.valid());
	REQUIRE(record_by_name_value.is<sol::table>());
	const sol::table record_by_name = record_by_name_value.as<sol::table>();

	REQUIRE(record_by_id.get<u16>("id") == 1);
	REQUIRE(record_by_id.get<lf::string>("name") == "alpha");
	REQUIRE(record_by_id.get<u32>("value") == 7);
	REQUIRE(record_by_id.get<lf::string>("seed") == "11400714819323198485");
	REQUIRE(record_by_name.get<u16>("id") == 1);

	lf::Database<TestPrototype>::clear();
}
