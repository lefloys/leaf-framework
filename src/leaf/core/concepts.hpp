#pragma once
#include <concepts>

namespace lf {
	template <typename, template <typename...> class>
	struct is_instantiation_of : std::false_type {};
	template <template <typename...> class Template, typename... Args>
	struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};
	template <typename T, template <typename...> class Template>
	concept instantiation_of = is_instantiation_of<T, Template>::value;
} // namespace lf