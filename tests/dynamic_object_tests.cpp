#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "leaf/core/dynamic_object.hpp"

using namespace lf;

TEST_CASE("object numeric conversions", "[object]") {
	object value = 42;

	REQUIRE(value.is<i64>());
	REQUIRE(value.convertible<f64>());
	REQUIRE(value.as<f64>() == 42.0);
	REQUIRE(value.current_type_name() == "i64");
}

TEST_CASE("dict-backed object access", "[dict]") {
	dict entries;
	entries["answer"] = 42;

	object config(entries);
	REQUIRE(config.is<dict>());

	REQUIRE(config.at("answer").as<i64>() == 42);
	config["answer"] = object(i64{ 84 });
	REQUIRE(config["answer"].as<i64>() == 84);
}

TEST_CASE("list-backed object can mutate entries", "[list]") {
	list values;
	values.push_back(1);
	values.push_back(2);

	object list_object(std::move(values));
	REQUIRE(list_object.is<list>());

	REQUIRE(list_object.at(1).as<i64>() == 2);
	list_object[0] = 10;
	REQUIRE(list_object[0].as<i64>() == 10);
}

TEST_CASE("dict operations and iteration", "[dict]") {
	dict store;
	REQUIRE(store.empty());

	store["one"] = 1;
	store["two"] = 2;

	REQUIRE(store.size() == 2);
	REQUIRE(store.contains("one"));
	REQUIRE(store.find("two") != store.end());

	auto [iter, inserted] = store.emplace("three", 3);
	REQUIRE(inserted);
	REQUIRE(iter->second.as<i64>() == 3);

	auto result = store.emplace("two", 4);
	REQUIRE(!result.second);
	REQUIRE(store["two"].as<i64>() == 2);

	REQUIRE(store.erase("two") == 1);
	REQUIRE(!store.contains("two"));

	store.reserve(4);
	store.clear();
	REQUIRE(store.empty());
}

TEST_CASE("list operations and bounds", "[list]") {
	list sequence;
	sequence.reserve(4);

	sequence.emplace_back(1);
	sequence.emplace_back(2);
	sequence.insert(sequence.begin() + 1, 5);

	REQUIRE(sequence.size() == 3);
	REQUIRE(sequence[0].as<i64>() == 1);
	REQUIRE(sequence[1].as<i64>() == 5);
	REQUIRE(sequence[2].as<i64>() == 2);

	sequence.erase(sequence.begin());
	REQUIRE(sequence[0].as<i64>() == 5);

	sequence.resize(2);
	sequence.emplace(sequence.end(), object(i64{ 9 }));
	sequence.push_back(object(i64{ 11 }));

	REQUIRE(sequence[sequence.size() - 1].as<i64>() == 11);
	REQUIRE(sequence.begin() != sequence.end());
}

TEST_CASE("object string and bool handling", "[object]") {
	object text("hello");
	REQUIRE(text.is<string>());
	REQUIRE(text.as<string>() == "hello");
	REQUIRE(!text.convertible<i64>());

	object flag(true);
	REQUIRE(flag.convertible<bool>());
	REQUIRE(flag.as<bool>());

	dict bucket;
	bucket["value"] = 123;
	object config(bucket);
	REQUIRE(config["value"].as<i64>() == 123);
}
