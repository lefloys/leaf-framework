#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <thread>

#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/leaf.hpp>
#include <leaf/network/socket.hpp>
#include <leaf/store/user.hpp>

lf::span<const lf::byte> bytes(std::string_view text) {
	return {
		reinterpret_cast<const lf::byte*>(text.data()),
		text.size()
	};
}

std::string_view text(lf::span<const lf::byte> data) {
	return {
		reinterpret_cast<const char*>(data.data()),
		data.size()
	};
}

struct console_input {
	std::mutex mutex;
	std::queue<std::string> lines;
	bool done = false;

	void push(std::string line) {
		std::lock_guard lock(mutex);
		lines.push(std::move(line));
	}

	std::optional<std::string> pop() {
		std::lock_guard lock(mutex);
		if (lines.empty()) {
			return std::nullopt;
		}
		std::string line = std::move(lines.front());
		lines.pop();
		return line;
	}
};

int main(int argc, char* argv[]) {
	if (argc < 2 || (std::string_view(argv[1]) != "server" && std::string_view(argv[1]) != "client")) {
		std::cerr << "usage:\n"
			<< "  leaf-framework-network-example server\n"
			<< "  leaf-framework-network-example client <server-steam-id>\n";
		return 1;
	}
	const bool is_server = std::string_view(argv[1]) == "server";
	if (!is_server && argc < 3) {
		std::cerr << "client mode needs a server SteamID64\n";
		return 1;
	}

	lf::vector<lf::string_view> args;
	args.reserve(argc);
	for (int i = 0; i < argc; ++i) { args.push_back(argv[i]); }

	const lf::error error = lf::Init(args);
	if (error) { std::cerr << "leaf init failed: " << error.message << "\n"; return 1; }

	constexpr u16 channel = 42042;
	lf::net::Socket socket = lf::net::Socket::Channel(channel);
	std::optional<lf::net::Peer> peer;
	if (!is_server) {
		peer = lf::net::Peer::User(lf::store::User::Get(argv[2]), channel);
	}

	const lf::store::User local_user = lf::store::User::Local();
	std::cout << (is_server ? "server" : "client") << " steam id: "
		<< (local_user ? local_user.id : "none") << "\n";
	if (is_server) {
		std::cout << "waiting for first message on channel " << channel << "\n";
	} else {
		std::cout << "sending to " << peer->id() << ":" << peer->channel() << "\n";
	}
	std::cout << "type messages. /quit exits.\n";

	console_input input;
	std::thread input_thread([&input] {
		std::string line;
		while (std::getline(std::cin, line)) {
			if (line == "/quit") {
				std::lock_guard lock(input.mutex);
				input.done = true;
				return;
			}
			input.push(std::move(line));
		}
		std::lock_guard lock(input.mutex);
		input.done = true;
	});

	std::array<lf::byte, 2048> buffer;
	bool running = true;
	while (running) {
		lf::Update();

		if (lf::optional<lf::net::Message> message = socket.recv(buffer)) {
			const auto& [sender, data] = *message;
			peer = sender;
			std::cout << sender.id() << ":" << sender.channel() << " > " << text(data) << "\n";
		}

		while (std::optional<std::string> line = input.pop()) {
			if (!peer) {
				std::cout << "no peer yet\n";
				continue;
			}
			socket.send(*peer, bytes(*line));
		}

		{
			std::lock_guard lock(input.mutex);
			running = !input.done;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	socket.disconnect();
	lf::Exit();
	if (input_thread.joinable()) {
		input_thread.join();
	}
	return 0;
}
