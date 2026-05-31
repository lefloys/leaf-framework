#pragma once
#include "leaf/core/error.hpp"
#include "leaf/core/string.hpp"

#include <filesystem>

namespace lf::fs {
	enum class folder {
		appdata,
		install,
		current,
	};

	using path = std::filesystem::path;
	using directory_entry = std::filesystem::directory_entry;
	using directory_iterator = std::filesystem::directory_iterator;

	using std::filesystem::create_directories;
	using std::filesystem::exists;

	path operator/(folder folder, const path& other);
	report<string> ReadTextFile(string_view path);
} // namespace lf::fs
