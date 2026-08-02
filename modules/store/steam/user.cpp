#include "leaf/store/user.hpp"

#include <charconv>

#include <steam/steam_api.h>

namespace lf::store {
	namespace {
		CSteamID steam_id_from_string(string_view id) {
			if (id.empty()) {
				return {};
			}

			uint64 value = 0;
			const char* begin = id.data();
			const char* end = id.data() + id.size();
			const auto parsed = std::from_chars(begin, end, value);
			if (parsed.ec != std::errc{} || parsed.ptr != end) {
				return {};
			}

			CSteamID steam_id(value);
			return steam_id.IsValid() ? steam_id : CSteamID();
		}

		bool is_local_user(CSteamID steam_id) {
			return SteamUser() && SteamUser()->BLoggedOn() && steam_id == SteamUser()->GetSteamID();
		}

		UserState user_state_from_steam(EPersonaState state) {
			switch (state) {
			case k_EPersonaStateOffline: return UserState::Offline;
			case k_EPersonaStateOnline: return UserState::Online;
			case k_EPersonaStateBusy: return UserState::Busy;
			case k_EPersonaStateAway: return UserState::Away;
			case k_EPersonaStateSnooze: return UserState::Snooze;
			case k_EPersonaStateLookingToTrade: return UserState::LookingToTrade;
			case k_EPersonaStateLookingToPlay: return UserState::LookingToPlay;
			case k_EPersonaStateInvisible: return UserState::Invisible;
			default: return UserState::Unknown;
			}
		}

		UserRelationship user_relationship_from_steam(EFriendRelationship relationship) {
			switch (relationship) {
			case k_EFriendRelationshipNone: return UserRelationship::None;
			case k_EFriendRelationshipBlocked: return UserRelationship::Blocked;
			case k_EFriendRelationshipRequestRecipient: return UserRelationship::RequestRecipient;
			case k_EFriendRelationshipFriend: return UserRelationship::Friend;
			case k_EFriendRelationshipRequestInitiator: return UserRelationship::RequestInitiator;
			case k_EFriendRelationshipIgnored: return UserRelationship::Ignored;
			case k_EFriendRelationshipIgnoredFriend: return UserRelationship::IgnoredFriend;
			case k_EFriendRelationshipSuggested_DEPRECATED: return UserRelationship::Suggested;
			default: return UserRelationship::Unknown;
			}
		}
	} // namespace

	string_view StateName(UserState state) {
		switch (state) {
		case UserState::Offline: return "offline";
		case UserState::Online: return "online";
		case UserState::Busy: return "busy";
		case UserState::Away: return "away";
		case UserState::Snooze: return "snooze";
		case UserState::LookingToTrade: return "looking-to-trade";
		case UserState::LookingToPlay: return "looking-to-play";
		case UserState::Invisible: return "invisible";
		default: return "unknown";
		}
	}

	string_view RelationshipName(UserRelationship relationship) {
		switch (relationship) {
		case UserRelationship::None: return "none";
		case UserRelationship::Blocked: return "blocked";
		case UserRelationship::RequestRecipient: return "request-recipient";
		case UserRelationship::Friend: return "friend";
		case UserRelationship::RequestInitiator: return "request-initiator";
		case UserRelationship::Ignored: return "ignored";
		case UserRelationship::IgnoredFriend: return "ignored-friend";
		case UserRelationship::Suggested: return "suggested";
		default: return "unknown";
		}
	}

	User User::Local() {
		if (!SteamUser() || !SteamUser()->BLoggedOn()) {
			return {};
		}
		return User{ std::to_string(SteamUser()->GetSteamID().ConvertToUint64()) };
	}

	User User::Get(string_view id) {
		CSteamID steam_id = steam_id_from_string(id);
		if (!steam_id.IsValid()) {
			return {};
		}
		if (SteamFriends()) {
			SteamFriends()->RequestUserInformation(steam_id, false);
		}
		return User{ std::to_string(steam_id.ConvertToUint64()) };
	}

	vector<User> User::Friends() {
		if (!SteamFriends()) {
			return {};
		}

		const int count = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
		vector<User> friends;
		friends.reserve(static_cast<size_t>(count));
		for (int index = 0; index < count; ++index) {
			CSteamID steam_id = SteamFriends()->GetFriendByIndex(index, k_EFriendFlagImmediate);
			if (steam_id.IsValid()) {
				SteamFriends()->RequestUserInformation(steam_id, false);
				friends.push_back(User{ std::to_string(steam_id.ConvertToUint64()) });
			}
		}
		return friends;
	}

	string_view User::display_name() const {
		CSteamID steam_id = steam_id_from_string(id);
		if (!steam_id.IsValid() || !SteamFriends()) {
			return id;
		}

		const char* name = SteamFriends()->GetFriendPersonaName(steam_id);
		return name && *name ? string_view(name) : string_view(id);
	}

	UserState User::state() const {
		CSteamID steam_id = steam_id_from_string(id);
		if (!steam_id.IsValid() || !SteamFriends()) {
			return UserState::Unknown;
		}
		return user_state_from_steam(SteamFriends()->GetFriendPersonaState(steam_id));
	}

	UserRelationship User::relationship() const {
		CSteamID steam_id = steam_id_from_string(id);
		if (!steam_id.IsValid() || !SteamFriends()) {
			return UserRelationship::Unknown;
		}
		return user_relationship_from_steam(SteamFriends()->GetFriendRelationship(steam_id));
	}

	i32 User::level() const {
		CSteamID steam_id = steam_id_from_string(id);
		if (!steam_id.IsValid()) {
			return 0;
		}
		if (is_local_user(steam_id)) {
			return SteamUser()->GetPlayerSteamLevel();
		}
		if (!SteamFriends()) {
			return 0;
		}
		SteamFriends()->RequestUserInformation(steam_id, false);
		return SteamFriends()->GetFriendSteamLevel(steam_id);
	}

	App App::Current() {
		if (!SteamUtils()) {
			return {};
		}
		return App{ std::to_string(SteamUtils()->GetAppID()) };
	}

	App App::Get(string_view id) {
		if (id.empty()) {
			return {};
		}
		return App{ string(id) };
	}

	App User::current_app() const {
		CSteamID steam_id = steam_id_from_string(id);
		if (!steam_id.IsValid() || !SteamFriends()) {
			return {};
		}

		FriendGameInfo_t game_info{};
		if (!SteamFriends()->GetFriendGamePlayed(steam_id, &game_info) || game_info.m_gameID.AppID() == 0) {
			return {};
		}
		return App{ std::to_string(game_info.m_gameID.AppID()) };
	}
} // namespace lf::store
