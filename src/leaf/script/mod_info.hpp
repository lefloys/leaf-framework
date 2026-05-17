#pragma once
#include "leaf/core/filesystem.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/core/version.hpp"

namespace lf {
	struct ModCollection {
		void add_privileged_dir(const fs::path& path);
		void add_unprivileged_dir(const fs::path& path);

		vector<fs::path> privileged_dirs;
		vector<fs::path> unprivileged_dirs;
	};

	struct ModDependency {
		static ModDependency parse(string_view str);

		string name;
		enum class Type {
			Required,
			Optional,
			Forbidden,
		} type;
		enum class Operator {
			Any,
			Equal,
			NotEqual,
			Greater,
			GreaterEqual,
			Less,
			LessEqual,
		} op;

		version required_version;
	};

	struct ModInfo {
		string name;
		version mod_version;
		string title;
		string author;
		string contact;
		string homepage;
		string description;
		version game_version;
		vector<ModDependency> dependencies;
		fs::path location;
		bool privileged;
	};

	ModInfo parse_mod_info(string_view path, bool priviledged);
} // namespace lf
