#pragma once
#include "leaf/core/error.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/tags.hpp"
#include "leaf/core/vector.hpp"
	
#include <filesystem>
#include <system_error>

namespace lf::fs {
	using path = std::filesystem::path;
	using directory_entry = std::filesystem::directory_entry;
	using directory_iterator = std::filesystem::directory_iterator;
	using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
	using copy_options = std::filesystem::copy_options;

	using std::filesystem::copy_file;
	using std::filesystem::create_directories;
	using std::filesystem::exists;
	using std::filesystem::relative;

	enum class folder {
		appdata,
		install,
		current,
	};

	path operator/(folder folder, const path& other);

	report<string> Read(string_view path, decltype(tags::String));
	report<vector<byte>> Read(string_view path, decltype(tags::Binary));
	error Write(string_view path, span<const byte> bytes, decltype(tags::Binary));
	error Write(string_view path, string_view text, decltype(tags::String));
} // namespace lf::fs
