#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/string.hpp>

namespace lf {
	void ClearVirtualFileSpace();
	void RegisterVirtualRoot(string_view name, fs::path root);
	bool IsVirtualPath(string_view path);
	fs::path ResolveVirtualPath(string_view path, fs::path relative_root = {});
	report<fs::path> ResolveVirtualPathReport(string_view path, fs::path relative_root = {});
	report<string> ReadVirtualTextFile(string_view path, fs::path relative_root = {});
}
