#include <catch2/catch_test_macros.hpp>

#include <leaf/core/binary.hpp>
#include <leaf/core/exception.hpp>
#include <leaf/lockstep/session.hpp>
#include <leaf/system/socket.hpp>

#include <algorithm>
#include <cstddef>
#include <thread>

namespace {
	struct udp_runtime {
		udp_runtime() {
			if (lf::error err = lf::sys::init_udp_sockets()) {
				throw lf::runtime_exception(err.message);
			}
		}

		~udp_runtime() {
			lf::sys::exit_udp_sockets();
		}
	};

	struct test_simulation {
		lf::vector<lf::byte> state;
		lf::lockstep::Tick last_tick = 0;
		bool allow_collect = false;
		u32 collected = 0;
		u32 applied = 0;
		u32 disconnected_count = 0;

		explicit test_simulation(std::size_t snapshot_bytes = 0) {
			state.resize(snapshot_bytes);
			for (std::size_t index = 0; index < state.size(); ++index) {
				state[index] = static_cast<lf::byte>((index * 31u) & 0xffu);
			}
		}

		lf::vector<lf::byte> save_snapshot() {
			return state;
		}

		void load_snapshot(lf::span<const lf::byte> bytes) {
			state.assign(bytes.begin(), bytes.end());
		}

		void submit_pending(lf::lockstep::Session& session) {
			if (!allow_collect || collected != 0) {
				return;
			}
			const std::array<lf::byte, 4> payload{
				lf::byte{0x10},
				lf::byte{0x20},
				lf::byte{0x30},
				lf::byte{0x40},
			};
			session.submit(lf::span<const lf::byte>(payload.data(), payload.size()));
			++collected;
		}

		void step(lf::lockstep::Tick tick, const lf::vector<lf::lockstep::Command>& commands) {
			last_tick = tick;
			for (const lf::lockstep::Command& command : commands) {
				for (lf::byte value : command.bytes) {
					state.emplace_back(value);
				}
				++applied;
			}
		}

		void disconnected() {
			++disconnected_count;
		}
	};

	void service(lf::lockstep::Session& session, test_simulation& simulation) {
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
		for (const lf::lockstep::ReadyTick& ready : ready_ticks) {
			simulation.step(ready.tick, ready.commands);
		}
	}

	void pump(lf::lockstep::Session& host, test_simulation& host_sim, lf::lockstep::Session& client, test_simulation& client_sim) {
		host.update();
		service(host, host_sim);
		client.update();
		service(client, client_sim);
	}

	void drive(lf::lockstep::Session& host, test_simulation& host_sim, lf::lockstep::Session& client, test_simulation& client_sim, u32 iterations) {
		for (u32 index = 0; index < iterations; ++index) {
			host_sim.submit_pending(host);
			client_sim.submit_pending(client);
			pump(host, host_sim, client, client_sim);
			host.advance();
			service(host, host_sim);
			pump(host, host_sim, client, client_sim);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void join_loop(lf::lockstep::Session& host, test_simulation& host_sim, lf::lockstep::Session& client, test_simulation& client_sim) {
		for (u32 index = 0; index < 2000 && !client.joined(); ++index) {
			pump(host, host_sim, client, client_sim);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

/// Verifies that lockstep loopback recovers snapshot and exchanges delayed commands.
TEST_CASE("lockstep loopback recovers snapshot and exchanges delayed commands", "[lockstep][network]") {
	udp_runtime udp;

	constexpr u16 host_port = 42142;
	constexpr u16 client_port = 42143;

	test_simulation host_sim(128 * 1024);
	test_simulation client_sim;

	lf::lockstep::Options options;
	options.snapshot_chunk_bytes = 1024;
	options.max_snapshot_chunks_per_update = 2;
	options.snapshot_request_window = 8;
	options.snapshot_request_retry_delay = 2;
	options.command_latency_ticks = 2;
	options.handshake_interval = lf::duration::from_quantum(10'000'000);
	options.nack_interval = lf::duration::from_quantum(5'000'000);
	options.connect_timeout = lf::duration::from_quantum(5'000'000'000);

	auto host = lf::lockstep::Session::Host(lf::net::Socket::Port(host_port), options);
	auto client = lf::lockstep::Session::Client(
		lf::net::Socket::Port(client_port),
		lf::net::Peer::Address("127.0.0.1", host_port),
		options);

	join_loop(host, host_sim, client, client_sim);
	REQUIRE(client.joined());
	REQUIRE(client_sim.state == host_sim.state);

	host_sim.allow_collect = true;
	client_sim.allow_collect = true;
	drive(host, host_sim, client, client_sim, 96);

	CHECK(host_sim.applied >= 1);
	CHECK(client_sim.applied >= 1);
	CHECK(client_sim.state == host_sim.state);
}

/// Verifies that lockstep reports disconnects as explicit events.
TEST_CASE("lockstep disconnects are explicit events", "[lockstep][network]") {
	udp_runtime udp;

	constexpr u16 host_port = 42144;
	constexpr u16 client_port = 42145;

	test_simulation host_sim(16 * 1024);
	test_simulation client_sim;

	lf::lockstep::Options options;
	options.snapshot_chunk_bytes = 1024;
	options.max_snapshot_chunks_per_update = 8;
	options.handshake_interval = lf::duration::from_quantum(10'000'000);
	options.nack_interval = lf::duration::from_quantum(5'000'000);
	options.connect_timeout = lf::duration::from_quantum(5'000'000'000);

	auto host = lf::lockstep::Session::Host(lf::net::Socket::Port(host_port), options);
	auto client = lf::lockstep::Session::Client(
		lf::net::Socket::Port(client_port),
		lf::net::Peer::Address("127.0.0.1", host_port),
		options);

	join_loop(host, host_sim, client, client_sim);
	REQUIRE(client.joined());

	client.disconnect();
	service(client, client_sim);
	host.update();
	service(host, host_sim);

	CHECK(client_sim.disconnected_count == 1);
}
