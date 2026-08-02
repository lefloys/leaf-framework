#pragma once

#include <leaf/store/app.hpp>

#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>

namespace lf::store {
	enum class UserState {
		Unknown,
		Offline,
		Online,
		Busy,
		Away,
		Snooze,
		LookingToTrade,
		LookingToPlay,
		Invisible,
	};

	enum class UserRelationship {
		Unknown,
		None,
		Blocked,
		RequestRecipient,
		Friend,
		RequestInitiator,
		Ignored,
		IgnoredFriend,
		Suggested,
	};

	string_view StateName(UserState state);
	string_view RelationshipName(UserRelationship relationship);

	struct User {
		string id;

		static User Local();
		static User Get(string_view id);
		static vector<User> Friends();

		explicit operator bool() const noexcept {
			return !id.empty();
		}

		string_view display_name() const;
		UserState state() const;
		App current_app() const;
		UserRelationship relationship() const;
		i32 level() const;
	};
} // namespace lf::store
