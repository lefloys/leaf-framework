#include "leaf/script/virtual_filesystem.hpp"

#include <leaf/core/exception.hpp>
#include <leaf/core/format.hpp>

#include <mutex>
#include <unordered_map>

namespace lf {
	namespace {
		std::unordered_map<string, fs::path>& virtual_roots() {
			static std::unordered_map<string, fs::path> roots;
			return roots;
		}

		std::mutex& virtual_roots_mutex() {
			static std::mutex mutex;
			return mutex;
		}

		fs::path checked_relative_path(string_view path) {
			fs::path relative = fs::path(path).lexically_normal();
			if (relative.is_absolute()) {
				throw runtime_exception(lf::format("virtual path tail '{}' must be relative", path));
			}
			for (const fs::path& part : relative) {
				if (part == "..") {
					throw runtime_exception(lf::format("virtual path tail '{}' escapes its mod root", path));
				}
			}
			return relative;
		}

		report<fs::path> checked_relative_path_report(string_view path) {
			fs::path relative = fs::path(path).lexically_normal();
			if (relative.is_absolute()) {
				return unexpected(error(
					generic_errc::input_error,
					lf::format("virtual path tail '{}' must be relative", path)));
			}
			for (const fs::path& part : relative) {
				if (part == "..") {
					return unexpected(error(
						generic_errc::input_error,
						lf::format("virtual path tail '{}' escapes its mod root", path)));
				}
			}
			return relative;
		}
	}

	void ClearVirtualFileSpace() {
		std::lock_guard lock(virtual_roots_mutex());
		virtual_roots().clear();
	}

	void RegisterVirtualRoot(string_view name, fs::path root) {
		if (name.empty()) {
			throw runtime_exception("virtual root name cannot be empty");
		}

		std::lock_guard lock(virtual_roots_mutex());
		virtual_roots()[string(name)] = std::move(root);
	}

	bool IsVirtualPath(string_view path) {
		return path.size() >= 4 && path.starts_with("__");
	}
	fs::path ResolveVirtualPath(string_view path, fs::path relative_root) {
		if (path.empty()) {
			return {};
		}

		fs::path raw_path = fs::path(path);
		if (raw_path.is_absolute()) {
			return raw_path;
		}

		if (!IsVirtualPath(path)) {
			return relative_root.empty() ? raw_path : relative_root / raw_path;
		}

		size_t end = path.find("__", 2);
		if (end == string_view::npos || end == 2) {
			throw runtime_exception(lf::format("invalid virtual path '{}'", path));
		}

		string mod_name(path.substr(2, end - 2));
		string_view tail = path.substr(end + 2);
		if (!tail.empty() && (tail.front() == '/' || tail.front() == '\\')) {
			tail.remove_prefix(1);
		}
		fs::path relative_tail = checked_relative_path(tail);

		std::lock_guard lock(virtual_roots_mutex());
		auto it = virtual_roots().find(mod_name);
		if (it == virtual_roots().end()) {
			throw runtime_exception(lf::format("virtual path '{}' references unknown mod '{}'", path, mod_name));
		}
		return it->second / relative_tail;
	}

	report<fs::path> ResolveVirtualPathReport(string_view path, fs::path relative_root) {
		if (path.empty()) {
			return fs::path();
		}

		fs::path raw_path = fs::path(path);
		if (raw_path.is_absolute()) {
			return raw_path;
		}

		if (!IsVirtualPath(path)) {
			return relative_root.empty() ? raw_path : relative_root / raw_path;
		}

		size_t end = path.find("__", 2);
		if (end == string_view::npos || end == 2) {
			return unexpected(error(
				generic_errc::input_error,
				lf::format("invalid virtual path '{}'", path)));
		}

		string mod_name(path.substr(2, end - 2));
		string_view tail = path.substr(end + 2);
		if (!tail.empty() && (tail.front() == '/' || tail.front() == '\\')) {
			tail.remove_prefix(1);
		}

		report<fs::path> relative_tail = checked_relative_path_report(tail);
		if (!relative_tail) {
			return unexpected(relative_tail.error().add_context(lf::format("resolving virtual path '{}'", path)));
		}

		std::lock_guard lock(virtual_roots_mutex());
		auto it = virtual_roots().find(mod_name);
		if (it == virtual_roots().end()) {
			return unexpected(error(
				generic_errc::input_error,
				lf::format("virtual path '{}' references unknown mod '{}'", path, mod_name)));
		}
		return it->second / *relative_tail;
	}

	report<string> ReadVirtualTextFile(string_view path, fs::path relative_root) {
		report<fs::path> resolved_path = ResolveVirtualPathReport(path, std::move(relative_root));
		if (!resolved_path) {
			return unexpected(resolved_path.error().add_context(lf::format("resolving text file '{}'", path)));
		}

		auto text = fs::ReadTextFile(resolved_path->string());
		if (!text) {
			return unexpected(text.error().add_context(lf::format("reading text file '{}'", path)));
		}
		return text;
	}
}

