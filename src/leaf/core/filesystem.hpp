#pragma once
#include <filesystem>

namespace lf::fs {
	enum class folder {
		appdata,
		install,
		current,
	};

	using path = std::filesystem::path;

	path operator/(folder folder, const path& other);

	using namespace std::filesystem;
} // namespace lf::fs
