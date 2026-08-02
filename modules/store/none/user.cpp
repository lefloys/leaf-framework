#include "leaf/store/user.hpp"

namespace lf::store {
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
		return {};
	}

	User User::Get(string_view id) {
		if (id.empty()) {
			return {};
		}
		return User{ string(id) };
	}

	vector<User> User::Friends() {
		return {};
	}

	string_view User::display_name() const {
		return id;
	}

	UserState User::state() const {
		return UserState::Unknown;
	}

	UserRelationship User::relationship() const {
		return UserRelationship::Unknown;
	}

	i32 User::level() const {
		return 0;
	}

	App App::Current() {
		return {};
	}

	App App::Get(string_view id) {
		if (id.empty()) {
			return {};
		}
		return App{ string(id) };
	}

	App User::current_app() const {
		return {};
	}
}
