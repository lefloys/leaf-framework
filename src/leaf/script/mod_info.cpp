#include "leaf/script/mod_info.hpp"
#include <yaml-cpp/yaml.h>

namespace lf {
	// Format: [optional(?) forbidden(!) mod_name [operator version]"
	// Any amount of whitespace is allowed between the mod name, operator, and version
	// Example: Optional mod "ExampleMod" with version >= 1.2.3 would be: "?ExampleMod >= 1.2.3"
	// Example: Required mod "ExampleMod" with version == 1.2.3 would be: "ExampleMod == 1.2.3"
	// Example: Forbidden mod "ExampleMod" would be: "!ExampleMod"
	ModDependency ModDependency::parse(string_view str) {
		ModDependency dep;
		size_t i = 0;

		while (i < str.size() &&
			   (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')) {
			++i;
		}

		ch08 type_char = (i < str.size()) ? str[i] : '\0';
		if (type_char == '?') {
			dep.type = ModDependency::Type::Optional;
			++i;
		} else if (type_char == '!') {
			dep.type = ModDependency::Type::Forbidden;
			++i;
		} else {
			dep.type = ModDependency::Type::Required;
		}

		while (i < str.size() &&
			   (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')) {
			++i;
		}

		struct OpMap {
			string_view op;
			ModDependency::Operator val;
		};
		static const OpMap ops[] = { { "==", ModDependency::Operator::Equal },
									 { "!=", ModDependency::Operator::NotEqual },
									 { ">=", ModDependency::Operator::GreaterEqual },
									 { "<=", ModDependency::Operator::LessEqual },
									 { ">", ModDependency::Operator::Greater },
									 { "<", ModDependency::Operator::Less } };
		size_t op_pos = string_view::npos;
		ModDependency::Operator op_val = ModDependency::Operator::Any;
		string_view op_found;
		for (const auto& op : ops) {
			size_t pos = str.find(op.op, i);
			if (pos != string_view::npos && (op_pos == string_view::npos || pos < op_pos)) {
				op_pos = pos;
				op_found = op.op;
				op_val = op.val;
			}
		}

		// Extract mod name (ignore whitespace)
		size_t name_start = i;
		size_t name_end = (op_pos != string_view::npos) ? op_pos : str.size();

		while (name_end > name_start && (str[name_end - 1] == ' ' || str[name_end - 1] == '\t' ||
										 str[name_end - 1] == '\n' || str[name_end - 1] == '\r')) {
			--name_end;
		}

		dep.name = str.substr(name_start, name_end - name_start);

		if (op_pos != string_view::npos) {
			dep.op = op_val;
			size_t ver_start = op_pos + op_found.size();

			while (ver_start < str.size() && (str[ver_start] == ' ' || str[ver_start] == '\t' ||
											  str[ver_start] == '\n' || str[ver_start] == '\r')) {
				++ver_start;
			}

			dep.required_version = version::from_string(str.substr(ver_start));
		} else {
			dep.op = ModDependency::Operator::Any;
			dep.required_version = version{};
		}

		return dep;
	}

	ModInfo parse_mod_info(string_view path, bool priviledged) {
		ModInfo info;
		info.privileged = priviledged;
		info.location = fs::path(path).parent_path();

		YAML::Node node = YAML::LoadFile(std::string(path));
		if (node["name"]) {
			info.name = node["name"].as<string>();
		}
		if (node["mod_version"]) {
			info.mod_version = version::from_string(node["mod_version"].as<string>());
		}
		if (node["title"]) {
			info.title = node["title"].as<string>();
		}
		if (node["author"]) {
			info.author = node["author"].as<string>();
		}
		if (node["contact"]) {
			info.contact = node["contact"].as<string>();
		}
		if (node["homepage"]) {
			info.homepage = node["homepage"].as<string>();
		}
		if (node["description"]) {
			info.description = node["description"].as<string>();
		}
		if (node["game_version"]) {
			info.game_version = version::from_string(node["game_version"].as<string>());
		}
		if (node["dependencies"]) {
			for (const auto& dep : node["dependencies"]) {
				info.dependencies.push_back(ModDependency::parse(dep.as<string>()));
			}
		}

		return info;
	}
	void ModCollection::add_privileged_dir(const fs::path& path) {
		this->privileged_dirs.push_back(path);
	}
	void ModCollection::add_unprivileged_dir(const fs::path& path) {
		this->unprivileged_dirs.push_back(path);
	}
} // namespace lf
