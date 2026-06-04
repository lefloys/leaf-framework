#pragma once

#include "leaf/core/memory.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/time.hpp"
#include "leaf/core/types.hpp"
#include "leaf/core/vector.hpp"
#include "leaf/network/peer.hpp"
#include "leaf/network/socket.hpp"

namespace lf::lockstep {
	using Tick = u64;
	using SessionId = u64;
	using PayloadId = u64;
	using PacketSequence = u64;

	enum class mode : u08 {
		offline = 0,
		host = 1,
		client = 2,
	};

	enum class state : u08 {
		connecting = 0,
		logging_in = 1,
		downloading_snapshot = 2,
		catching_up = 3,
		joined = 4,
		disconnected = 5,
	};

	struct Command {
		PayloadId id = 0;
		SessionId source = 0;
		u64 hash = 0;
		span<const byte> bytes;
	};

	struct PendingPayload {
		PayloadId id = 0;
		u64 hash = 0;
		span<const byte> bytes;
	};

	class Submitter {
	  public:
		struct Impl {
			virtual ~Impl() = default;
			virtual PayloadId submit(span<const byte> bytes) = 0;
		};

		explicit Submitter(unique_ptr<Impl> impl);

		Submitter(const Submitter&) = delete;
		Submitter& operator=(const Submitter&) = delete;
		Submitter(Submitter&& other) noexcept;
		Submitter& operator=(Submitter&& other) noexcept;
		~Submitter();

		PayloadId submit(span<const byte> bytes);

	  private:
		unique_ptr<Impl> impl;
	};

	class Simulation {
	  public:
		virtual ~Simulation() = default;

		virtual vector<byte> save_snapshot() = 0;
		virtual void load_snapshot(span<const byte> bytes) = 0;
		virtual void start_load_snapshot(span<const byte> bytes) { load_snapshot(bytes); }
		virtual bool snapshot_load_finished() { return true; }
		virtual void snapshot_save_started() {}
		virtual void snapshot_save_finished() {}
		virtual vector<byte> login_payload() { return {}; }
		virtual bool accept_login(SessionId, span<const byte>) { return true; }
		virtual u64 checksum(Tick) { return 0; }
		virtual void desync_detected(Tick, u64, u64) {}
		virtual void collect(Submitter& submitter) = 0;
		virtual void step(Tick tick, span<const Command> commands) = 0;
		virtual void disconnected() {}
	};

	struct Options {
		u16 snapshot_chunk_bytes = 1000;
		u16 max_clients = 0;
		duration handshake_interval = duration::from_quantum(500'000'000);
		duration nack_interval = duration::from_quantum(50'000'000);
		duration connect_timeout = duration::from_quantum(15'000'000'000);
		u32 max_missing_per_nack = 64;
		u32 heartbeat_request_min_delay = 3;
		u32 heartbeat_request_max_delay = 32;
		u32 heartbeat_request_interval = 10;
		u32 max_resends_per_update = 128;
		u32 max_outgoing_packets_per_update = 128;
		u32 max_incoming_packets_per_update = 128;
		u32 max_snapshot_chunks_per_update = 16;
		u32 snapshot_request_window = 32;
		u32 snapshot_request_retry_delay = 4;
		u32 command_latency_ticks = 2;
		u32 checksum_interval_ticks = 60;
		u32 max_tick_steps_per_update = 4;
	};

	class Session {
	  public:
		struct Impl;
		struct ImplDeleter {
			void operator()(Impl* impl) const noexcept;
		};

		Session() = default;

		static Session Offline(Simulation& simulation, Options options = {});
		static Session Host(net::Socket socket, Simulation& simulation, Options options = {});
		static Session Client(net::Socket socket, net::Peer host, Simulation& simulation, Options options = {});

		Session(const Session&) = delete;
		Session& operator=(const Session&) = delete;
		Session(Session&& other) noexcept;
		Session& operator=(Session&& other) noexcept;
		~Session();

		explicit operator bool() const noexcept;

		void update();
		void advance();
		void disconnect();

		lockstep::mode mode() const;
		lockstep::state state() const;
		SessionId local_session_id() const;
		Tick tick() const;
		bool joined() const;
		bool waiting_for_response() const;
		span<const PendingPayload> pending() const;

	  private:
		explicit Session(unique_ptr<Impl> impl);

		std::unique_ptr<Impl, ImplDeleter> impl;
	};
}
