#include "fixed_script.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/format.hpp"

#include <cmath>
#include <limits>

namespace lf {
	namespace {
		fixed require_fixed(report<fixed> value) {
			if (!value) {
				throw runtime_exception(value.error().message);
			}
			return *value;
		}

		string lua_number_text(sol::object value) {
			sol::state_view lua(value.lua_state());
			sol::function tostring = lua["tostring"];
			return tostring(value);
		}

		report<fixed> fixed_from_lua_number(sol::object value) {
			if (value.is<i64>()) {
				return fixed::from_integer(value.as<i64>());
			}
			if (value.is<u64>()) {
				u64 integer = value.as<u64>();
				if (integer > static_cast<u64>(std::numeric_limits<i64>::max())) {
					return unexpected(error(generic_errc::input_error, format("value {} out of range for fixed", integer)));
				}
				return fixed::from_integer(static_cast<i64>(integer));
			}
			if (value.is<double>()) {
				const double number = value.as<double>();
				if (!std::isfinite(number)) {
					return unexpected(error(generic_errc::input_error, "non-finite Lua number cannot convert to fixed"));
				}
				return fixed::parse(lua_number_text(value));
			}
			return unexpected(error(generic_errc::type_mismatch, "Lua value is not a number"));
		}

		fixed fixed_add(fixed lhs, fixed rhs) {
			return require_fixed(lhs.checked_add(rhs));
		}

		fixed fixed_subtract(fixed lhs, fixed rhs) {
			return require_fixed(lhs.checked_subtract(rhs));
		}

		fixed fixed_multiply(fixed lhs, fixed rhs) {
			return require_fixed(lhs.checked_multiply(rhs));
		}

		fixed fixed_divide(fixed lhs, fixed rhs) {
			return require_fixed(lhs.checked_divide(rhs));
		}

		fixed fixed_negate(fixed value) {
			return require_fixed(value.checked_negated());
		}

		sol::object coerce_global_value(sol::this_state state, sol::object value) {
			sol::state_view lua(state);
			if (value.is<fixed>() || value.get_type() != sol::type::number) {
				return value;
			}
			report<fixed> parsed = fixed_from_lua_number(value);
			if (!parsed) {
				throw runtime_exception(parsed.error().message);
			}
			return sol::make_object(lua, *parsed);
		}
	}

	report<fixed> FixedFromLua(sol::object value) {
		if (value.is<fixed>()) {
			return value.as<fixed>();
		}
		if (value.get_type() == sol::type::number) {
			return fixed_from_lua_number(value);
		}
		if (value.is<string>()) {
			return fixed::parse(value.as<string>());
		}
		return unexpected(error(generic_errc::type_mismatch, format("cannot convert Lua {} to fixed", sol::type_name(value.lua_state(), value.get_type()))));
	}

	void InstallFixedScript(sol::state& lua, bool install_global_coercion) {
		lua.new_usertype<fixed>(
			"fixed_value",
			"raw", &fixed::raw,
			"to_string", &fixed::to_string,
			sol::meta_function::to_string, &fixed::to_string,
			sol::meta_function::unary_minus, &fixed_negate,
			sol::meta_function::addition, &fixed_add,
			sol::meta_function::subtraction, &fixed_subtract,
			sol::meta_function::multiplication, &fixed_multiply,
			sol::meta_function::division, &fixed_divide,
			sol::meta_function::equal_to, [](fixed lhs, fixed rhs) { return lhs == rhs; },
			sol::meta_function::less_than, [](fixed lhs, fixed rhs) { return lhs < rhs; },
			sol::meta_function::less_than_or_equal_to, [](fixed lhs, fixed rhs) { return lhs <= rhs; }
		);

		lua.set_function("fixed", [](sol::object value) {
			return require_fixed(FixedFromLua(value));
		});
		lua.set_function("fixed_raw", [](i64 raw) {
			return fixed::from_raw(raw);
		});
		lua.set_function("fixed_ratio", [](i64 numerator, i64 denominator) {
			return require_fixed(fixed::from_ratio(numerator, denominator));
		});

		if (!install_global_coercion) {
			return;
		}
		lua.script(R"lua(
local __leaf_global_metatable = getmetatable(_G) or {}
local __leaf_previous_newindex = __leaf_global_metatable.__newindex
__leaf_global_metatable.__newindex = function(table, key, value)
	if type(value) == "number" then
		value = fixed(value)
	end
	if __leaf_previous_newindex ~= nil then
		if type(__leaf_previous_newindex) == "function" then
			return __leaf_previous_newindex(table, key, value)
		end
		__leaf_previous_newindex[key] = value
		return
	end
	rawset(table, key, value)
end
setmetatable(_G, __leaf_global_metatable)
)lua");
	}
} // namespace lf
