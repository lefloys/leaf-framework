#pragma once

#include "leaf/core/dynamic_object.hpp"

namespace YAML {
	class Emitter;
	class Node;
} // namespace YAML

namespace lf {
	void EmitYaml(YAML::Emitter& out, const object& value);
	object ObjectFromYaml(const YAML::Node& node);
} // namespace lf
