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

	struct test_simulation final : lf::lockstep::Simulation {
		lf::vector<lf::byte> state;
		lf::lockstep::Tick last_tick = 0;
		bool allow_collect = false;
		u32 collected = 0;
		u32 applied = 0;
		u32 checksum_calls = 0;
		u32 desyncs = 0;
		u32 disconnected_count = 0;
		bool corrupt_checksum = false;

		explicit test_simulation(std::size_t snapshot_bytes = 0) {
			state.resize(snapshot_bytes);
			for (std::size_t index = 0; index < state.size(); ++index) {
				state[index] = static_cast<lf::byte>((index * 31u) & 0xffu);
			}
		}

		lf::vector<lf::byte> save_snapshot() override {
			return state;
		}

		void load_snapshot(lf::span<const lf::byte> bytes) override {
			state.assign(bytes.begin(), bytes.end());
		}

		u64 checksum(lf::lockstep::Tick) override {
			++checksum_calls;
			u64 hash = 14695981039346656037ull;
			for (lf::byte value : state) {
				hash ^= static_cast<u64>(std::to_integer<u08>(value));
				hash *= 1099511628211ull;
			}
			hash ^= static_cast<u64>(last_tick);
			hash *= 1099511628211ull;
			if (corrupt_checksum) {
				++hash;
			}
			return hash == 0 ? 1 : hash;
		}

		void desync_detected(lf::lockstep::Tick, u64, u64) override {
			++desyncs;
		}

		void collect(lf::lockstep::Submitter& submitter) override {
			if (!allow_collect || collected != 0) {
				return;
			}
			const std::array<lf::byte, 4> payload{
				lf::byte{0x10},
				lf::byte{0x20},
				lf::byte{0x30},
				lf::byte{0x40},
			};
			submitter.submit(lf::span<const lf::byte>(payload.data(), payload.size()));
			++collected;
		}

		void step(lf::lockstep::Tick tick, lf::span<const lf::lockstep::Command> commands) override {
			last_tick = tick;
			for (const lf::lockstep::Command& command : commands) {
				for (lf::byte value : command.bytes) {
					state.emplace_back(value);
				}
				++applied;
			}
		}

		void disconnected() override {
			++disconnected_count;
		}
	};

	void drive(lf::lockstep::Session& host, lf::lockstep::Session& client, u32 iterations) {
		for (u32 index = 0; index < iterations; ++index) {
			host.update();
			client.update();
			host.advance();
			host.update();
			client.update();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	void join_loop(lf::lockstep::Session& host, lf::lockstep::Session& client) {
		for (u32 index = 0; index < 2000 && !client.joined(); ++index) {
			host.update();
			client.update();
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
	options.checksum_interval_ticks = 8;
	options.handshake_interval = lf::duration::from_quantum(10'000'000);
	options.nack_interval = lf::duration::from_quantum(5'000'000);
	options.connect_timeout = lf::duration::from_quantum(5'000'000'000);

	auto host = lf::lockstep::Session::Host(lf::net::Socket::Port(host_port), host_sim, options);
	auto client = lf::lockstep::Session::Client(
		lf::net::Socket::Port(client_port),
		lf::net::Peer::Address("127.0.0.1", host_port),
		client_sim,
		options);

	for (u32 index = 0; index < 2000 && !client.joined(); ++index) {
		host.update();
		client.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	REQUIRE(client.joined());
	REQUIRE(client_sim.state == host_sim.state);

	host_sim.allow_collect = true;
	client_sim.allow_collect = true;
	drive(host, client, 96);

	CHECK(host_sim.applied >= 1);
	CHECK(client_sim.applied >= 1);
	CHECK(client_sim.state == host_sim.state);
	CHECK(host_sim.checksum_calls < host_sim.last_tick);
	CHECK(client_sim.checksum_calls == host_sim.checksum_calls);
	CHECK(client_sim.desyncs == 0);
}

/// Verifies that sync checks are sparse heartbeat records, not per-tick checksum work.
TEST_CASE("lockstep sync checks are sparse", "[lockstep][network]") {
	udp_runtime udp;

	constexpr u16 host_port = 42144;
	constexpr u16 client_port = 42145;

	test_simulation host_sim(16 * 1024);
	test_simulation client_sim;

	lf::lockstep::Options options;
	options.snapshot_chunk_bytes = 1024;
	options.max_snapshot_chunks_per_update = 8;
	options.checksum_interval_ticks = 10;
	options.handshake_interval = lf::duration::from_quantum(10'000'000);
	options.nack_interval = lf::duration::from_quantum(5'000'000);
	options.connect_timeout = lf::duration::from_quantum(5'000'000'000);

	auto host = lf::lockstep::Session::Host(lf::net::Socket::Port(host_port), host_sim, options);
	auto client = lf::lockstep::Session::Client(
		lf::net::Socket::Port(client_port),
		lf::net::Peer::Address("127.0.0.1", host_port),
		client_sim,
		options);

	join_loop(host, client);
	REQUIRE(client.joined());

	drive(host, client, 45);

	CHECK(host_sim.last_tick >= 40);
	CHECK(host_sim.checksum_calls >= 4);
	CHECK(host_sim.checksum_calls < host_sim.last_tick);
	CHECK(client_sim.checksum_calls == host_sim.checksum_calls);
	CHECK(client_sim.desyncs == 0);
}

/// Verifies that checksum mismatch reports desync only when a sparse sync check is received.
TEST_CASE("lockstep desync detection uses sparse sync checks", "[lockstep][network]") {
	udp_runtime udp;

	constexpr u16 host_port = 42146;
	constexpr u16 client_port = 42147;

	test_simulation host_sim(16 * 1024);
	test_simulation client_sim;
	client_sim.corrupt_checksum = true;

	lf::lockstep::Options options;
	options.snapshot_chunk_bytes = 1024;
	options.max_snapshot_chunks_per_update = 8;
	options.checksum_interval_ticks = 12;
	options.handshake_interval = lf::duration::from_quantum(10'000'000);
	options.nack_interval = lf::duration::from_quantum(5'000'000);
	options.connect_timeout = lf::duration::from_quantum(5'000'000'000);

	auto host = lf::lockstep::Session::Host(lf::net::Socket::Port(host_port), host_sim, options);
	auto client = lf::lockstep::Session::Client(
		lf::net::Socket::Port(client_port),
		lf::net::Peer::Address("127.0.0.1", host_port),
		client_sim,
		options);

	join_loop(host, client);
	REQUIRE(client.joined());

	drive(host, client, 30);

	CHECK(host_sim.checksum_calls >= 2);
	CHECK(client_sim.checksum_calls == host_sim.checksum_calls);
	CHECK(client_sim.desyncs == client_sim.checksum_calls);
	CHECK(client_sim.desyncs < client_sim.last_tick);
}
