#pragma once

#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>

namespace lf {
	struct PrototypeField {
		string name;
		string value;
		string kind;
	};

	using PrototypeFieldList = vector<PrototypeField>;
} // namespace lf
