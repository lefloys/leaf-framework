#pragma once
#include <concepts>
#include <type_traits>

namespace lf {
	template <typename, template <typename...> class>
	struct is_instantiation_of : std::false_type {};
	template <template <typename...> class Template, typename... Args>
	struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};
	template <typename T, template <typename...> class Template>
	concept instantiation_of = is_instantiation_of<T, Template>::value;


	template <typename T> struct is_bitfield_enum : std::false_type {};
	template <typename T> constexpr bool is_bitfield_enum_v = is_bitfield_enum<T>::value;
	template <typename T> concept bitfield_enum = std::is_enum_v<T> && is_bitfield_enum_v<T>;
} // namespace lf

template<lf::bitfield_enum E> constexpr E operator|(E, E);
template<lf::bitfield_enum E> constexpr E operator&(E, E);
template<lf::bitfield_enum E> constexpr E operator^(E, E);
template<lf::bitfield_enum E> constexpr E operator~(E);

template<lf::bitfield_enum E> constexpr E& operator|=(E&, E);
template<lf::bitfield_enum E> constexpr E& operator&=(E&, E);
template<lf::bitfield_enum E> constexpr E& operator^=(E&, E);


/*************************************************************************************************/
/*===============================================================================================*/
/*************************************************************************************************/


template<lf::bitfield_enum E>
constexpr E operator|(E a, E b) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(a) | static_cast<U>(b));
}

template<lf::bitfield_enum E>
constexpr E operator&(E a, E b) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(a) & static_cast<U>(b));
}

template<lf::bitfield_enum E>
constexpr E operator^(E a, E b) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(a) ^ static_cast<U>(b));
}

template<lf::bitfield_enum E>
constexpr E operator~(E v) {
	using U = std::underlying_type_t<E>;
	return static_cast<E>(~static_cast<U>(v));
}

template<lf::bitfield_enum E>
constexpr E& operator|=(E& a, E b) {
	return a = a | b;
}

template<lf::bitfield_enum E>
constexpr E& operator&=(E& a, E b) {
	return a = a & b;
}

template<lf::bitfield_enum E>
constexpr E& operator^=(E& a, E b) {
	return a = a ^ b;
}