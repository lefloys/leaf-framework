#include "sound.hpp"

#include <leaf/core/format.hpp>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

namespace lf {
	error PlaySoundFile(const fs::path& path, f32 volume) {
		if (volume <= 0.0f) {
			return error::no_error;
		}
		if (!fs::exists(path)) {
			return error(generic_errc::input_error, format("missing sound '{}'", path.string()));
		}

#ifdef _WIN32
		if (!PlaySoundA(path.string().c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT)) {
			return error(generic_errc::input_error, format("failed to play sound '{}'", path.string()));
		}
#else
		(void)path;
#endif
		return error::no_error;
	}
}
