#pragma once

#include "leaf/core/string.hpp"

#include <tuple>
#include <type_traits>
#include <utility>

namespace lf {

	enum class field_presence : u08 {
		absent,
		present
	};

	template<typename T, typename Default = void>
	struct field_node;

	template<typename... Fields>
	struct group_node;

	template<typename Controller, typename Condition, typename... Children>
	struct conditional_node;

	template<typename T>
	struct is_schema_node : std::false_type {};

	template<typename T, typename Default>
	struct is_schema_node<field_node<T, Default>> : std::true_type {};

	template<typename... Fields>
	struct is_schema_node<group_node<Fields...>> : std::true_type {};

	template<typename Controller, typename Condition, typename... Children>
	struct is_schema_node<conditional_node<Controller, Condition, Children...>> : std::true_type {};

	template<typename T>
	concept schema_node = is_schema_node<std::remove_cvref_t<T>>::value;

	template<typename T>
	concept non_schema_node = !schema_node<T>;

	struct present_condition {
		bool operator()(field_presence value) const { return value == field_presence::present; }
	};

	struct absent_condition {
		bool operator()(field_presence value) const { return value == field_presence::absent; }
	};

	template<typename T, typename Default>
	struct field_storage;

	template<typename T>
	struct field_storage<T, void> {
		string_view name;
		T& value;
		field_storage(string_view name, T& value) : name(name), value(value) {}
	};

	template<typename T, typename Default>
	struct field_storage {
		string_view name;
		T& value;
		Default default_value;
		field_storage(string_view name, T& value, Default default_value) : name(name), value(value), default_value(std::move(default_value)) {}
	};

	template<typename T, typename Default>
	struct field_node : field_storage<T, Default> {
		using field_storage<T, Default>::field_storage;
		using field_storage<T, Default>::name;
		using field_storage<T, Default>::value;

		template<typename... Children>
		auto present(Children&&... children) const { return conditional_node{ *this, present_condition{}, std::forward<Children>(children)... }; }
		template<typename... Children>
		auto absent(Children&&... children) const { return conditional_node{ *this, absent_condition{}, std::forward<Children>(children)... }; }
		template<typename Predicate, typename... Children>
		auto when(Predicate&& predicate, Children&&... children) const { return conditional_node{ *this, std::forward<Predicate>(predicate), std::forward<Children>(children)... }; }
	};

	template<typename... Fields>
	struct group_node {
		std::tuple<Fields...> fields;

		template<typename... Children>
		auto present(Children&&... children) const { return conditional_node{ *this, present_condition{}, std::forward<Children>(children)... }; }
		template<typename... Children>
		auto absent(Children&&... children) const { return conditional_node{ *this, absent_condition{}, std::forward<Children>(children)... }; }
		template<typename Predicate, typename... Children>
		auto when(Predicate&& predicate, Children&&... children) const { return conditional_node{ *this, std::forward<Predicate>(predicate), std::forward<Children>(children)... }; }
	};

	template<typename Controller, typename Condition, typename... Children>
	struct conditional_node {
		Controller controller;
		Condition condition;
		std::tuple<Children...> children;

		template<typename... ChildArgs>
		conditional_node(Controller controller, Condition condition, ChildArgs&&... children) : controller(std::move(controller)), condition(std::move(condition)), children(std::forward<ChildArgs>(children)...) {}
	};

	template<typename Controller, typename Condition, typename... Children>
	conditional_node(Controller, Condition, Children&&...) -> conditional_node<Controller, Condition, std::remove_cvref_t<Children>...>;

	template<typename T>
	field_node<T> field(string_view name, T& value) { return { name, value }; }

	template<typename T, typename Default>
	field_node<T, T> field(string_view name, T& value, Default&& default_value) { return { name, value, T{ std::forward<Default>(default_value) } }; }

	template<typename... Fields>
	group_node<std::remove_cvref_t<Fields>...> group(Fields&&... fields) { return { { std::forward<Fields>(fields)... } }; }

} // namespace lf
