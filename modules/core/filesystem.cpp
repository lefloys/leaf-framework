#include "leaf/core/filesystem.hpp"

#include "leaf/core/format.hpp"
#include "leaf/system/system.hpp"

#include <fstream>
#include <iterator>

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

	report<string> Read(string_view path, decltype(tags::String)) {
		std::ifstream file{string(path), std::ios::binary};
		if (!file) {
			return unexpected(error(generic_errc::input_error, lf::format("failed to open '{}'", path)));
		}

		auto text = string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		if (!file.eof() && file.fail()) {
			return unexpected(error(generic_errc::input_error, lf::format("failed to read '{}'", path)));
		}
		return text;
	}

	report<vector<byte>> Read(string_view path, decltype(tags::Binary)) {
		std::ifstream file{ string(path), std::ios::binary | std::ios::ate };
		if (!file) {
			return unexpected(error(generic_errc::input_error, lf::format("failed to open file '{}'", path)));
		}
		const auto size = file.tellg();
		if (size < 0) {
			return unexpected(error(generic_errc::input_error, lf::format("failed to size file '{}'", path)));
		}
		vector<byte> bytes{ static_cast<size_t>(size) };
		file.seekg(0);
		file.read(reinterpret_cast<char*>(bytes.data()), size);
		if (!file) {
			return unexpected(error(generic_errc::input_error, lf::format("failed to read file '{}'", path)));
		}
		return bytes;
	}

	error Write(string_view path, span<const byte> bytes, decltype(tags::Binary)) {
		std::ofstream file{string(path), std::ios::binary};
		if (!file) { return error(generic_errc::input_error, lf::format("failed to open file '{}'", path)); }
		file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
		if (!file) { return error(generic_errc::input_error, lf::format("failed to write file '{}'", path)); }
		return {};
	}
} // namespace lf::fs
