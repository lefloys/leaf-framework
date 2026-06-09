#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

#ifdef _WIN32
#include <atomic>
#endif

#include <leaf/application/scene.hpp>
#include <leaf/core/binary.hpp>
#include <leaf/core/exception.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/time.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/leaf.hpp>
#include <leaf/lockstep/session.hpp>
#include <leaf/network/peer.hpp>
#include <leaf/network/socket.hpp>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using Tick = lf::lockstep::Tick;
using Player = lf::lockstep::SessionId;

#ifdef _WIN32
std::atomic_bool close_requested = false;

BOOL WINAPI console_handler(DWORD event) {
	switch (event) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		close_requested = true;
		Sleep(1500);
		return TRUE;
	default:
		return FALSE;
	}
}
#endif

constexpr i32 world_width = 8;
constexpr i32 world_height = 6;
constexpr size_t world_cell_count = static_cast<size_t>(world_width * world_height);

enum class action_type : u16 {
	nothing = 0,
	place_building = 1,
};

struct action {
	action_type type = action_type::nothing;
	i32 x = 0;
	i32 y = 0;
};

struct snapshot {
	Tick tick = 0;
	std::array<Player, world_cell_count> cells{};
};

template<>
struct lf::bin::enum_validator<action_type> {
	static constexpr bool is_valid(action_type type) {
		switch (type) {
		case action_type::nothing:
		case action_type::place_building:
			return true;
		}
		return false;
	}
};

template<lf::bin::byte_stream Stream, lf::bin::data<action> Action>
lf::error process(Stream& stream, Action& action) {
	return stream(
		lf::bin::field("type", action.type),
		lf::bin::field("x", action.x),
		lf::bin::field("y", action.y)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<snapshot> Snapshot>
lf::error process(Stream& stream, Snapshot& snapshot) {
	return stream(
		lf::bin::field("tick", snapshot.tick),
		lf::bin::field("cells", snapshot.cells)
	);
}

struct toy_world {
	Tick tick = 0;
	std::array<Player, world_cell_count> cells{};

	void apply(Player player, const action& action) {
		if (action.type != action_type::place_building) {
			return;
		}
		if (action.x < 0 || action.x >= world_width || action.y < 0 || action.y >= world_height) {
			return;
		}
		cells[static_cast<size_t>(action.y * world_width + action.x)] = player;
	}

	snapshot save() const {
		return snapshot {
			.tick = tick,
			.cells = cells,
		};
	}

	void load(const snapshot& snapshot) {
		tick = snapshot.tick;
		cells = snapshot.cells;
	}
};

struct shared_demo_state {
	std::mutex mutex;
	lf::vector<action> local_actions;
	std::array<Player, world_cell_count> predicted_cells{};
	lf::vector<lf::string> log;
	Player local_player = 1;
	Tick local_tick = 0;
	bool disconnected = false;

	void place_building(i32 x, i32 y) {
		std::lock_guard lock(mutex);
		local_actions.emplace_back(action {
			.type = action_type::place_building,
			.x = x,
			.y = y,
		});
		if (x >= 0 && x < world_width && y >= 0 && y < world_height) {
			predicted_cells[static_cast<size_t>(y * world_width + x)] = local_player;
		}
		log.emplace_back(lf::format("predicted {}, {} at local tick {}", x, y, local_tick));
		if (log.size() > 12) {
			log.erase(log.begin());
		}
	}

	lf::vector<action> take_actions() {
		std::lock_guard lock(mutex);
		lf::vector<action> result = std::move(local_actions);
		local_actions.clear();
		return result;
	}

	void confirm(const action& action) {
		std::lock_guard lock(mutex);
		if (action.type != action_type::place_building) {
			return;
		}
		if (action.x < 0 || action.x >= world_width || action.y < 0 || action.y >= world_height) {
			return;
		}
		const size_t index = static_cast<size_t>(action.y * world_width + action.x);
		predicted_cells[index] = 0;
	}

	void clear_predictions() {
		std::lock_guard lock(mutex);
		predicted_cells = {};
		local_actions.clear();
	}

	void set_tick(Tick tick) {
		std::lock_guard lock(mutex);
		local_tick = tick;
	}

	void push_log(lf::string line) {
		std::lock_guard lock(mutex);
		log.emplace_back(std::move(line));
		if (log.size() > 12) {
			log.erase(log.begin());
		}
	}

	void mark_disconnected() {
		std::lock_guard lock(mutex);
		disconnected = true;
		log.emplace_back("session disconnected");
	}

	bool is_disconnected() {
		std::lock_guard lock(mutex);
		return disconnected;
	}

	std::array<Player, world_cell_count> copy_predictions() {
		std::lock_guard lock(mutex);
		return predicted_cells;
	}

	lf::vector<lf::string> copy_log() {
		std::lock_guard lock(mutex);
		return log;
	}
};

shared_demo_state* active_demo_state = nullptr;

struct demo_simulation {
	demo_simulation(toy_world& world, shared_demo_state& state) : world(world), state(state) {}

	lf::vector<lf::byte> save_snapshot() {
		lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(world.save());
		if (!bytes) {
			throw lf::runtime_exception(bytes.error().message);
		}
		state.push_log(lf::format("saved snapshot at tick {}", world.tick));
		return std::move(*bytes);
	}

	void load_snapshot(lf::span<const lf::byte> bytes) {
		lf::report<snapshot> loaded = lf::bin::read<snapshot>(bytes);
		if (!loaded) {
			throw lf::runtime_exception(loaded.error().message);
		}
		world.load(*loaded);
		state.clear_predictions();
		state.set_tick(world.tick);
		state.push_log(lf::format("loaded snapshot at tick {}", world.tick));
	}

	void submit_pending(lf::lockstep::Session& session) {
		if (!session.joined()) {
			return;
		}
		lf::vector<action> actions = state.take_actions();
		for (const action& action : actions) {
			lf::report<lf::vector<lf::byte>> bytes = lf::bin::write(action);
			if (!bytes) {
				throw lf::runtime_exception(bytes.error().message);
			}
			session.submit(lf::span<const lf::byte>(bytes->data(), bytes->size()));
		}
	}

	void step(Tick tick, const lf::vector<lf::lockstep::Command>& commands) {
		world.tick = tick;
		for (const lf::lockstep::Command& command : commands) {
			lf::report<action> read_action = lf::bin::read<action>(command.bytes);
			if (!read_action) {
				state.push_log(lf::format("bad action payload: {}", read_action.error().message));
				continue;
			}
			world.apply(command.source, *read_action);
			state.confirm(*read_action);
		}
		state.set_tick(world.tick);
		state.push_log(lf::format("stepped tick {} with {} command(s)", world.tick, commands.size()));
	}

	void disconnected() {
		state.mark_disconnected();
	}

	toy_world& world;
	shared_demo_state& state;
};

void service_session(lf::lockstep::Session& session, demo_simulation& simulation) {
	lf::vector<lf::lockstep::SessionEvent> events = session.take_events();
	for (const lf::lockstep::SessionEvent& event : events) {
		switch (event.kind) {
		case lf::lockstep::SessionEventKind::login_requested: {
			lf::vector<lf::byte> snapshot = simulation.save_snapshot();
			session.accept_login(event.session_id, lf::span<const lf::byte>(snapshot.data(), snapshot.size()));
			break;
		}
		case lf::lockstep::SessionEventKind::snapshot_received:
			simulation.load_snapshot(lf::span<const lf::byte>(event.bytes.data(), event.bytes.size()));
			session.finish_snapshot_load();
			break;
		case lf::lockstep::SessionEventKind::peer_disconnected:
			break;
		case lf::lockstep::SessionEventKind::disconnected:
			simulation.disconnected();
			break;
		}
	}

	lf::vector<lf::lockstep::ReadyTick> ready_ticks = session.take_ready_ticks();
	for (const lf::lockstep::ReadyTick& ready_tick : ready_ticks) {
		simulation.step(ready_tick.tick, ready_tick.commands);
	}
}

lf::string html_escape(std::string_view text) {
	lf::string result;
	for (char value : text) {
		switch (value) {
		case '&': result += "&amp;"; break;
		case '<': result += "&lt;"; break;
		case '>': result += "&gt;"; break;
		case '"': result += "&quot;"; break;
		default: result += value; break;
		}
	}
	return result;
}

lf::string scene_source(bool server) {
	const char* title = server ? "Leaf lockstep host" : "Leaf lockstep client";
	return lf::format(R"rml(
<rml>
<head>
	<title>{}</title>
	<style>
		body {{
			background: #111318;
			color: #e8edf2;
			font-family: sans-serif;
			font-size: 18px;
			margin: 0;
		}}
		#root {{
			padding: 24px;
		}}
		#status {{
			color: #aab6c3;
			margin-bottom: 18px;
		}}
		#world {{
			display: flex;
			flex-wrap: wrap;
			width: 384px;
			margin-bottom: 18px;
		}}
		.cell {{
			width: 44px;
			height: 44px;
			margin: 2px;
			background: #202833;
			border-width: 1px;
			border-color: #344150;
		}}
		.cell:hover {{
			background: #38485c;
		}}
		.p0 {{ background: #202833; }}
		.p1 {{ background: #43a047; }}
		.p2 {{ background: #d98c2b; }}
		.p3 {{ background: #4f7fd8; }}
		.p4 {{ background: #7e57c2; }}
		.pending {{ background: #c62828; }}
		#log {{
			color: #bac6d3;
			font-size: 14px;
			line-height: 20px;
		}}
	</style>
</head>
<body>
	<div id="root">
		<div id="status"></div>
		<div id="world"></div>
		<div id="log"></div>
	</div>
</body>
</rml>
)rml", title);
}

Player color_player(Player player) {
	if (player == 0) {
		return 0;
	}
	return ((player - 1) % 4) + 1;
}

lf::string world_rml(const toy_world& world, const std::array<Player, world_cell_count>& predicted_cells) {
	lf::string out;
	for (i32 y = 0; y < world_height; ++y) {
		for (i32 x = 0; x < world_width; ++x) {
			const size_t index = static_cast<size_t>(y * world_width + x);
			Player player = world.cells[index];
			bool pending = false;
			if (predicted_cells[index] != 0) {
				player = predicted_cells[index];
				pending = true;
			}
			out += lf::format("<div class=\"cell p{}{}\" mousedown=\"place_building({}, {})\"></div>", color_player(player), pending ? " pending" : "", x, y);
		}
	}
	return out;
}

lf::string log_rml(const lf::vector<lf::string>& log) {
	lf::string out;
	for (const lf::string& line : log) {
		out += "<div>";
		out += html_escape(line);
		out += "</div>";
	}
	return out;
}

u16 parse_port(const char* text, u16 fallback) {
	if (!text) {
		return fallback;
	}
	const unsigned long value = std::strtoul(text, nullptr, 10);
	if (value == 0 || value > 65535) {
		return fallback;
	}
	return static_cast<u16>(value);
}

u16 parse_auto_port(const char* text) {
	if (!text) {
		return 0;
	}
	const unsigned long value = std::strtoul(text, nullptr, 10);
	if (value > 65535) {
		return 0;
	}
	return static_cast<u16>(value);
}

const char* state_name(lf::lockstep::state state) {
	switch (state) {
	case lf::lockstep::state::connecting: return "connecting";
	case lf::lockstep::state::logging_in: return "logging in";
	case lf::lockstep::state::downloading_snapshot: return "downloading snapshot";
	case lf::lockstep::state::catching_up: return "catching up";
	case lf::lockstep::state::joined: return "joined";
	case lf::lockstep::state::disconnected: return "disconnected";
	}
	return "unknown";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
	SetConsoleCtrlHandler(console_handler, TRUE);
#endif

	const std::string_view command = argc >= 2 ? argv[1] : "";
	const bool headless_mode = command == "headless" || command == "headless-server";
	const bool host_mode = command == "server" || headless_mode;
	const bool client_mode = command == "client";
	if (!host_mode && !client_mode) {
		std::cerr << "usage:\n"
			<< "  leaf-framework-lockstep-example server [port]\n"
			<< "  leaf-framework-lockstep-example headless [port]\n"
			<< "  leaf-framework-lockstep-example client [host] [server-port] [local-port]\n"
			<< "client local-port defaults to 0, which lets the OS pick one\n";
		return 1;
	}

	const u16 server_port = host_mode ? parse_port(argc > 2 ? argv[2] : nullptr, 43000) : parse_port(argc > 3 ? argv[3] : nullptr, 43000);
	const u16 local_port = host_mode ? server_port : parse_auto_port(argc > 4 ? argv[4] : nullptr);
	const lf::string host = (!host_mode && argc > 2) ? argv[2] : "127.0.0.1";

	lf::vector<lf::string_view> args;
	args.reserve(static_cast<size_t>(argc));
	for (int index = 0; index < argc; ++index) {
		args.emplace_back(argv[index]);
	}

	const lf::error init_error = lf::Init(args);
	if (init_error) {
		std::cerr << "leaf init failed: " << init_error.message << "\n";
		return 1;
	}

	shared_demo_state state;
	state.local_player = host_mode ? 1 : 2;
	active_demo_state = &state;

	if (!headless_mode) {
		lf::Scene::RegisterScriptInstaller([](sol::state& lua, Rml::ElementDocument&) {
			lua.set_function("place_building", [](i32 x, i32 y) {
				if (active_demo_state) {
					active_demo_state->place_building(x, y);
				}
			});
		});
	}

	std::unique_ptr<lf::ClientApplication> app;
	if (!headless_mode) {
		app = std::make_unique<lf::ClientApplication>(lf::ClientApplicationCreateInfo {
			.title = host_mode ? "Leaf lockstep host" : "Leaf lockstep client",
			.width = 720,
			.height = 520,
		});
		if (lf::error launch_error = app->launch(scene_source(host_mode))) {
			std::cerr << "launch failed: " << launch_error.message << "\n";
			lf::Exit();
			return 1;
		}
	}

	toy_world world;
	demo_simulation simulation(world, state);
	lf::net::Socket socket = lf::net::Socket::Port(local_port);
	lf::lockstep::Options options;
	options.handshake_interval = lf::duration::from_quantum(250'000'000);
	options.snapshot_chunk_bytes = 700;

	lf::lockstep::Session session;
	if (host_mode) {
		session = lf::lockstep::Session::Host(std::move(socket), options);
		state.push_log(lf::format("host listening on port {}", server_port));
	} else {
		lf::net::Peer peer = lf::net::Peer::Address(host, server_port);
		session = lf::lockstep::Session::Client(std::move(socket), peer, options);
		state.push_log(lf::format("client connecting to {}:{}", host, server_port));
	}

	lf::instant next_host_tick = lf::now() + lf::duration::from_quantum(500'000'000);
	lf::string displayed_status;
	lf::string displayed_world;
	lf::string displayed_log;
	bool running = true;

	while (running) {
#ifdef _WIN32
		if (close_requested) {
			break;
		}
#endif
		if (app) {
			running = app->update();
		}
		lf::Update();
		simulation.submit_pending(session);
		session.update();
		service_session(session, simulation);

		if (host_mode && lf::now() >= next_host_tick) {
			session.advance();
			service_session(session, simulation);
			next_host_tick = next_host_tick + lf::duration::from_quantum(500'000'000);
		}

		if (state.is_disconnected()) {
			running = false;
		}

		if (app) {
			lf::string next_status = lf::format("{} | {} | tick {} | port {}", host_mode ? "host" : "client", state_name(session.state()), session.tick(), local_port);
			if (displayed_status != next_status) {
				displayed_status = std::move(next_status);
				app->set_rml("status", displayed_status);
			}

			lf::string next_world = world_rml(world, state.copy_predictions());
			if (displayed_world != next_world) {
				displayed_world = std::move(next_world);
				app->set_rml("world", displayed_world);
			}

			lf::string next_log = log_rml(state.copy_log());
			if (displayed_log != next_log) {
				displayed_log = std::move(next_log);
				app->set_rml("log", displayed_log);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	session.disconnect();
	active_demo_state = nullptr;
	if (app) {
		app->close();
	}
	lf::Exit();
	return 0;
}
