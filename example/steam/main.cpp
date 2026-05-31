#include <iostream>

#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/leaf.hpp>
#include <leaf/store/app.hpp>
#include <leaf/store/user.hpp>

void print_user(lf::string_view label, const lf::store::User& user) {
	if (!user) {
		std::cout << label << ": none\n";
		return;
	}

	const lf::store::App app = user.current_app();
	std::cout << label << ": " << user.display_name()
			  << " [" << lf::store::StateName(user.state()) << "]"
			  << " relation=" << lf::store::RelationshipName(user.relationship())
			  << " level=" << user.level();
	if (app) {
		std::cout << " app=" << app.id;
	}
	std::cout << "\n";
}

int main(int argc, char* argv[]) {
	lf::vector<lf::string_view> args;
	args.reserve(argc);
	for (int i = 0; i < argc; ++i) { args.push_back(argv[i]); }

	const lf::error error = lf::Init(args);
	if (error) {
		std::cerr << "leaf init failed: " << error.message << "\n";
		return 1;
	}

	const lf::store::User local_user = lf::store::User::Local();
	print_user("steam local user", local_user);

	const lf::store::User user = lf::store::User::Get("76561199467909261");
	print_user("steam user 76561199467909261", user);

	const lf::vector<lf::store::User> friends = lf::store::User::Friends();
	std::cout << "steam friends: " << friends.size() << "\n";
	for (const lf::store::User& friend_user : friends) {
		print_user(" - friend", friend_user);
	}

	lf::Exit();
	return 0;
}
