#include "filesystem.hpp"

#include "leaf/system/system.hpp"

namespace lf::fs {
	path operator/(folder folder, const path& other) {
		path base_path;
		switch (folder) {
		case folder::appdata: return GetAppdataDir() / other;
		case folder::install: return GetInstallDir() / other;
		case folder::current: return std::filesystem::current_path() / other;
		}
		return base_path;
	}
} // namespace lf::fs
