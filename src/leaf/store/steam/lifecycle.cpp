#include "leaf/store/lifecycle.hpp"

#include <leaf/core/format.hpp>
#include <leaf/core/logging.hpp>

#include <steam/steam_api.h>

namespace lf {
	void steam_warning_message_hook(int severity, const char* message) {
		if (severity > 0) {
			log::Warning("{}", lf::format("steam[{}]: {}", severity, message));
		} else {
			log::Debug("{}", lf::format("steam[{}]: {}", severity, message));
		}
	}

	error init_store(span<string_view>) {
		if (!SteamAPI_Init()) {
			return error("failed to initialize Steamworks");
		}

		SteamUtils()->SetWarningMessageHook(steam_warning_message_hook);
		return error::no_error;
	}

	string_view store_backend_name() {
		return "steam";
	}

	void update_store() {
		SteamAPI_RunCallbacks();
	}

	void exit_store() {
		SteamAPI_Shutdown();
	}
} // namespace lf

