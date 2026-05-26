#include "leaf/launcher/launcher.hpp"

#include <steam/steam_api.h>


#include <iostream>


namespace lf {
	error init_launcher(span<string_view>) {
		if (!SteamAPI_Init()) {
			return error("failed to initialize Steamworks");
		}
		std::cout << "launcher : steam\n";
		return error::no_error;
	}



	void update_launcher() {
		SteamAPI_RunCallbacks();
	}

	void exit_launcher() {
		SteamAPI_Shutdown();
	}
} // namespace lf
