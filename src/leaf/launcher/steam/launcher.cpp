#include "leaf/launcher/launcher.hpp"

#include <leaf/core/format.hpp>
#include <leaf/core/messages.hpp>

#include <steam/steam_api.h>

namespace lf {
	namespace {
		bool steam_ready = false;

		void steam_warning_message_hook(int severity, const char* message) {
			log_info(format("steam[{}]: {}", severity, message));
		}
	}

	error init_launcher(span<string_view>) {
		if (!SteamAPI_Init()) {
			return error("failed to initialize Steamworks");
		}

		steam_ready = true;
		SteamUtils()->SetWarningMessageHook(steam_warning_message_hook);
		return error::no_error;
	}

	void update_launcher() {
		if (steam_ready) {
			SteamAPI_RunCallbacks();
		}
	}

	void exit_launcher() {
		if (steam_ready) {
			SteamAPI_Shutdown();
			steam_ready = false;
		}
	}
} // namespace lf
