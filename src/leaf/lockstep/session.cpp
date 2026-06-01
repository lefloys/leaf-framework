#include "session.hpp"

#include "leaf/core/binary.hpp"
#include "leaf/core/error.hpp"
#include "leaf/core/exception.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace lf::lockstep {
	enum class packet_kind : u08 {
		connect_request = 1,
		connect_accept = 2,
		join_request = 3,
		snapshot_begin = 4,
		snapshot_chunk = 5,
		snapshot_end = 6,
		client_heartbeat = 7,
		server_heartbeat = 8,
		missing_packets = 9,
		disconnect = 10,
	};

	struct key {
		std::array<byte, 32> bytes{};
	};

	struct signature {
		std::array<byte, 64> bytes{};
	};

	struct wire_payload {
		PayloadId id = 0;
		SessionId source = 0;
		u64 hash = 0;
		vector<byte> bytes;
	};

	struct tick_closure {
		Tick tick = 0;
		vector<wire_payload> commands;
	};

	struct stored_payload {
		PayloadId id = 0;
		SessionId source = 0;
		u64 hash = 0;
		vector<byte> bytes;
	};

	struct stored_tick {
		Tick tick = 0;
		vector<stored_payload> commands;
	};

	struct sent_packet {
		PacketSequence sequence = 0;
		vector<byte> bytes;
	};

	struct queued_send {
		net::Peer peer;
		vector<byte> bytes;
	};

	struct received_packets {
		PacketSequence next_expected = 1;
		vector<PacketSequence> missing;
	};

	struct packet_record {
		bool fresh = false;
		vector<PacketSequence> missing;
	};

	struct pending_payload {
		PayloadId id = 0;
		u64 hash = 0;
		vector<byte> bytes;
	};

	struct snapshot_download {
		bool active = false;
		bool end_received = false;
		bool load_started = false;
		u64 snapshot_id = 0;
		Tick baseline_tick = 0;
		u16 chunk_size = 1000;
		u32 chunk_count = 0;
		u32 received_count = 0;
		vector<byte> bytes;
		vector<u08> received_chunks;
	};

	struct host_connection {
		host_connection(const net::Peer& peer, SessionId session_id);

		net::Peer peer;
		SessionId session_id = 0;
		state connection_state = state::logging_in;
		bool snapshot_requested = false;
		bool snapshot_sending = false;
		u64 snapshot_id = 0;
		Tick snapshot_baseline_tick = 0;
		u16 snapshot_chunk_size = 1000;
		u32 snapshot_chunk_count = 0;
		u32 next_snapshot_chunk = 0;
		vector<byte> snapshot_bytes;
		PacketSequence next_send_sequence = 1;
		received_packets received;
		vector<sent_packet> sent_packets;
		vector<PayloadId> received_payloads;
	};

	struct Session::Impl {
		Impl(lockstep::mode initial_mode, lockstep::state initial_state, Simulation& simulation, Options options);
		virtual ~Impl() = default;

		virtual void update() = 0;
		virtual void advance() = 0;
		virtual void disconnect() = 0;

		void rebuild_pending_views();

		lockstep::mode session_mode = lockstep::mode::offline;
		lockstep::state session_state = lockstep::state::disconnected;
		Simulation* simulation = nullptr;
		Options options;
		Tick current_tick = 0;
		SessionId local_session = 0;
		PayloadId next_payload_id = 1;
		vector<pending_payload> pending_payloads;
		vector<PendingPayload> pending_views;
		vector<Command> command_views;
	};

	struct collect_submitter final : Submitter::Impl {
		collect_submitter(Session::Impl& session, vector<stored_payload>& out, SessionId source, bool track_pending);

		PayloadId submit(span<const byte> bytes) override;

		Session::Impl& session;
		vector<stored_payload>& out;
		SessionId source = 0;
		bool track_pending = false;
	};

	struct offline_session final : Session::Impl {
		offline_session(Simulation& simulation, Options options);

		void update() override;
		void advance() override;
		void disconnect() override;
		void collect_local();

		vector<stored_payload> staged_commands;
	};

	struct host_session final : Session::Impl {
		host_session(net::Socket socket, Simulation& simulation, Options options);

		void update() override;
		void advance() override;
		void disconnect() override;
		void collect_local();
		void poll_socket();
		void receive_message(const net::Message& message);
		host_connection& find_or_create_connection(const net::Peer& peer);
		host_connection* find_connection(const net::Peer& peer, SessionId session_id);
		void send_connect_accept(host_connection& connection);
		void request_snapshot(host_connection& connection);
		void process_snapshot_requests();
		void begin_snapshot(host_connection& connection);
		void continue_snapshot(host_connection& connection);
		void send_missing_for(host_connection& connection, vector<PacketSequence> missing);
		void resend_missing(host_connection& connection, const vector<PacketSequence>& missing);
		void flush_send_queue();

		template<typename Writer>
		void send_connected(host_connection& connection, packet_kind kind, Writer writer);

		net::Socket socket;
		std::array<byte, 65507> receive_buffer{};
		vector<host_connection> connections;
		vector<queued_send> send_queue;
		vector<stored_payload> staged_commands;
		SessionId next_session_id = 2;
		u64 next_snapshot_id = 1;
	};

	struct client_session final : Session::Impl {
		client_session(net::Socket socket, net::Peer host, Simulation& simulation, Options options);

		void update() override;
		void advance() override;
		void disconnect() override;
		void poll_socket();
		void receive_message(const net::Message& message);
		void collect_local();
		void buffer_tick(const tick_closure& closure);
		bool drain_buffered_ticks();
		void try_finish_snapshot();
		bool valid_session(SessionId packet_session_id) const;
		void send_connect_request();
		void send_join_request();
		void send_client_heartbeat();
		void resend_pending_payloads();
		void send_missing_for(vector<PacketSequence> missing);
		void resend_missing(const vector<PacketSequence>& missing);

		template<typename Writer>
		void send_connected(packet_kind kind, Writer writer);

		net::Socket socket;
		net::Peer host;
		std::array<byte, 65507> receive_buffer{};
		SessionId session_id = 0;
		PacketSequence next_send_sequence = 1;
		received_packets received;
		vector<sent_packet> sent_packets;
		vector<stored_payload> outgoing_commands;
		vector<stored_tick> buffered_ticks;
		snapshot_download snapshot;
		instant last_connect_activity = now();
		instant next_connect_request = now();
		instant next_join_request = now();
		Tick next_pending_resend_tick = 0;
	};
}

template<>
struct lf::bin::enum_validator<lf::lockstep::packet_kind> {
	static constexpr bool is_valid(lf::lockstep::packet_kind kind) {
		switch (kind) {
		case lf::lockstep::packet_kind::connect_request:
		case lf::lockstep::packet_kind::connect_accept:
		case lf::lockstep::packet_kind::join_request:
		case lf::lockstep::packet_kind::snapshot_begin:
		case lf::lockstep::packet_kind::snapshot_chunk:
		case lf::lockstep::packet_kind::snapshot_end:
		case lf::lockstep::packet_kind::client_heartbeat:
		case lf::lockstep::packet_kind::server_heartbeat:
		case lf::lockstep::packet_kind::missing_packets:
		case lf::lockstep::packet_kind::disconnect:
			return true;
		}
		return false;
	}
};

namespace lf::lockstep {
	constexpr SessionId host_session_id = 1;

	host_connection::host_connection(const net::Peer& peer, SessionId session_id)
		: peer(peer), session_id(session_id) {}

	Session::Impl::Impl(lockstep::mode initial_mode, lockstep::state initial_state, Simulation& simulation, Options options)
		: session_mode(initial_mode), session_state(initial_state), simulation(&simulation), options(options) {}

	void Session::Impl::rebuild_pending_views() {
		pending_views.clear();
		pending_views.reserve(pending_payloads.size());
		for (const pending_payload& pending : pending_payloads) {
			pending_views.emplace_back(PendingPayload {
				.id = pending.id,
				.hash = pending.hash,
				.bytes = span<const byte>(pending.bytes.data(), pending.bytes.size()),
			});
		}
	}

	collect_submitter::collect_submitter(Session::Impl& session, vector<stored_payload>& out, SessionId source, bool track_pending)
		: session(session), out(out), source(source), track_pending(track_pending) {}

	PayloadId collect_submitter::submit(span<const byte> bytes) {
		const PayloadId id = session.next_payload_id;
		++session.next_payload_id;

		stored_payload payload;
		payload.id = id;
		payload.source = source;
		payload.hash = 14695981039346656037ull;
		payload.bytes.assign(bytes.begin(), bytes.end());
		for (byte value : bytes) {
			payload.hash ^= static_cast<u64>(std::to_integer<u08>(value));
			payload.hash *= 1099511628211ull;
		}

		out.emplace_back(payload);
		if (track_pending) {
			session.pending_payloads.emplace_back(pending_payload {
				.id = payload.id,
				.hash = payload.hash,
				.bytes = payload.bytes,
			});
			session.rebuild_pending_views();
		}
		return id;
	}

	template<bin::byte_stream Stream>
	error process(Stream& stream, key& key) {
		return stream.bytes(span(key.bytes.data(), key.bytes.size()));
	}

	template<bin::byte_stream Stream>
	error process(Stream& stream, signature& signature) {
		return stream.bytes(span(signature.bytes.data(), signature.bytes.size()));
	}

	template<bin::byte_stream Stream>
	error process(Stream& stream, wire_payload& payload) {
		return stream(
			bin::field("id", payload.id),
			bin::field("source", payload.source),
			bin::field("hash", payload.hash),
			bin::field("bytes", payload.bytes)
		);
	}

	template<bin::byte_stream Stream>
	error process(Stream& stream, const wire_payload& payload) {
		return stream(
			bin::field("id", payload.id),
			bin::field("source", payload.source),
			bin::field("hash", payload.hash),
			bin::field("bytes", payload.bytes)
		);
	}

	template<bin::byte_stream Stream>
	error process(Stream& stream, tick_closure& closure) {
		return stream(
			bin::field("tick", closure.tick),
			bin::field("commands", closure.commands)
		);
	}

	template<bin::byte_stream Stream>
	error process(Stream& stream, const tick_closure& closure) {
		return stream(
			bin::field("tick", closure.tick),
			bin::field("commands", closure.commands)
		);
	}

	bool same_peer(const net::Peer& lhs, const net::Peer& rhs) {
		return lhs.id() == rhs.id() && lhs.channel() == rhs.channel();
	}

	span<const byte> bytes_view(const vector<byte>& bytes) {
		return span<const byte>(bytes.data(), bytes.size());
	}

	void build_command_views(vector<Command>& out, const vector<stored_payload>& commands) {
		out.clear();
		out.reserve(commands.size());
		for (const stored_payload& command : commands) {
			out.emplace_back(Command {
				.id = command.id,
				.source = command.source,
				.hash = command.hash,
				.bytes = bytes_view(command.bytes),
			});
		}
	}

	void sort_commands(vector<stored_payload>& commands) {
		std::sort(commands.begin(), commands.end(), [](const stored_payload& lhs, const stored_payload& rhs) {
			if (lhs.source != rhs.source) {
				return lhs.source < rhs.source;
			}
			return lhs.id < rhs.id;
		});
	}

	wire_payload to_wire_payload(const stored_payload& payload) {
		wire_payload wire;
		wire.id = payload.id;
		wire.source = payload.source;
		wire.hash = payload.hash;
		wire.bytes = payload.bytes;
		return wire;
	}

	stored_payload from_wire_payload(const wire_payload& payload, SessionId source_override) {
		stored_payload stored;
		stored.id = payload.id;
		stored.source = source_override == 0 ? payload.source : source_override;
		stored.hash = payload.hash;
		stored.bytes = payload.bytes;
		return stored;
	}

	packet_record record_received_packet(received_packets& received, PacketSequence sequence, const Options& options) {
		packet_record record;
		auto append_missing = [&] {
			for (PacketSequence missing_sequence : received.missing) {
				if (record.missing.size() >= options.max_missing_per_nack) {
					break;
				}
				record.missing.emplace_back(missing_sequence);
			}
		};

		if (sequence == 0) {
			return record;
		}

		if (sequence < received.next_expected) {
			auto missing = std::find(received.missing.begin(), received.missing.end(), sequence);
			if (missing != received.missing.end()) {
				received.missing.erase(missing);
				record.fresh = true;
			}
			append_missing();
			return record;
		}

		record.fresh = true;
		while (received.next_expected < sequence) {
			received.missing.emplace_back(received.next_expected);
			++received.next_expected;
		}
		received.next_expected = sequence + 1;
		append_missing();
		return record;
	}

	void remember_sent_packet(vector<sent_packet>& sent_packets, PacketSequence sequence, const vector<byte>& bytes) {
		sent_packets.emplace_back(sent_packet {
			.sequence = sequence,
			.bytes = bytes,
		});
		if (sent_packets.size() > 2048) {
			sent_packets.erase(sent_packets.begin());
		}
	}

	void erase_confirmed_pending(vector<pending_payload>& pending, SessionId local_session_id, const vector<stored_payload>& commands) {
		for (const stored_payload& command : commands) {
			if (command.source != local_session_id) {
				continue;
			}
			for (size_t index = 0; index < pending.size();) {
				if (pending[index].id == command.id) {
					pending.erase(pending.begin() + static_cast<i64>(index));
				} else {
					++index;
				}
			}
		}
	}

	bool remember_received_payload(host_connection& connection, PayloadId id) {
		for (PayloadId received_id : connection.received_payloads) {
			if (received_id == id) {
				return false;
			}
		}
		connection.received_payloads.emplace_back(id);
		if (connection.received_payloads.size() > 4096) {
			connection.received_payloads.erase(connection.received_payloads.begin());
		}
		return true;
	}

	template<typename Writer>
	vector<byte> write_packet(packet_kind kind, Writer writer) {
		bin::write_stream stream;
		if (error err = stream(bin::field("kind", kind))) {
			throw runtime_exception(err.message);
		}
		if (error err = writer(stream)) {
			throw runtime_exception(err.message);
		}
		return stream.take_written();
	}

	template<typename Writer>
	vector<byte> write_connected_packet(packet_kind kind, SessionId session_id, PacketSequence packet_sequence, Writer writer) {
		return write_packet(kind, [&](bin::write_stream& stream) -> error {
			signature packet_signature;
			IF_ERROR_RETURN_ERROR(stream(
				bin::field("session_id", session_id),
				bin::field("packet_sequence", packet_sequence),
				bin::field("signature", packet_signature)
			));
			return writer(stream);
		});
	}

	error read_connected_header(bin::read_stream& stream, SessionId& session_id, PacketSequence& packet_sequence) {
		signature packet_signature;
		return stream(
			bin::field("session_id", session_id),
			bin::field("packet_sequence", packet_sequence),
			bin::field("signature", packet_signature)
		);
	}

	void resend_packets(net::Socket& socket, const net::Peer& peer, const vector<sent_packet>& sent_packets, const vector<PacketSequence>& missing, u32 max_resends) {
		u32 resent = 0;
		for (PacketSequence sequence : missing) {
			if (resent >= max_resends) {
				break;
			}
			for (const sent_packet& packet : sent_packets) {
				if (packet.sequence == sequence) {
					socket.send(peer, bytes_view(packet.bytes));
					++resent;
					break;
				}
			}
		}
	}

	template<bin::readable_byte_stream Stream>
	error process(Stream& stream, host_session& session, const net::Peer& peer) {
		packet_kind kind = {};
		IF_ERROR_RETURN_ERROR(stream(bin::field("kind", kind)));

		switch (kind) {
		case packet_kind::connect_request: {
			PacketSequence packet_sequence = 0;
			key client_public_key;
			IF_ERROR_RETURN_ERROR(stream(
				bin::field("packet_sequence", packet_sequence),
				bin::field("client_public_key", client_public_key)
			));

			host_connection& connection = session.find_or_create_connection(peer);
			packet_record record = record_received_packet(connection.received, packet_sequence, session.options);
			session.send_missing_for(connection, std::move(record.missing));
			session.send_connect_accept(connection);
			return {};
		}
		case packet_kind::join_request: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			vector<byte> login_payload;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(bin::field("login_payload", login_payload)));

			host_connection* connection = session.find_connection(peer, session_id);
			if (!connection) {
				return {};
			}
			packet_record record = record_received_packet(connection->received, packet_sequence, session.options);
			session.send_missing_for(*connection, std::move(record.missing));
			if (connection->connection_state == state::joined ||
				connection->connection_state == state::downloading_snapshot ||
				!record.fresh) {
				return {};
			}
			u16 joined_clients = 0;
			for (const host_connection& existing : session.connections) {
				if (existing.connection_state == state::joined) {
					++joined_clients;
				}
			}
			if (session.options.max_clients != 0 && joined_clients >= session.options.max_clients) {
				session.send_connected(*connection, packet_kind::disconnect, [](bin::write_stream&) -> error {
					return {};
				});
				return {};
			}
			if (!session.simulation->accept_login(span<const byte>(login_payload.data(), login_payload.size()))) {
				session.send_connected(*connection, packet_kind::disconnect, [](bin::write_stream&) -> error {
					return {};
				});
				return {};
			}
			session.request_snapshot(*connection);
			return {};
		}
		case packet_kind::client_heartbeat: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			vector<wire_payload> commands;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(bin::field("commands", commands)));

			host_connection* connection = session.find_connection(peer, session_id);
			if (!connection || connection->connection_state != state::joined) {
				return {};
			}
			packet_record record = record_received_packet(connection->received, packet_sequence, session.options);
			session.send_missing_for(*connection, std::move(record.missing));
			if (!record.fresh) {
				return {};
			}
			for (const wire_payload& payload : commands) {
				if (!remember_received_payload(*connection, payload.id)) {
					continue;
				}
				session.staged_commands.emplace_back(from_wire_payload(payload, connection->session_id));
			}
			return {};
		}
		case packet_kind::missing_packets: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			vector<PacketSequence> missing;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(bin::field("missing", missing)));

			host_connection* connection = session.find_connection(peer, session_id);
			if (connection) {
				session.resend_missing(*connection, missing);
			}
			return {};
		}
		case packet_kind::disconnect: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));

			for (size_t index = 0; index < session.connections.size();) {
				if (session.connections[index].session_id == session_id && same_peer(session.connections[index].peer, peer)) {
					session.connections.erase(session.connections.begin() + static_cast<i64>(index));
				} else {
					++index;
				}
			}
			return {};
		}
		default:
			return {};
		}
	}

	template<bin::readable_byte_stream Stream>
	error process(Stream& stream, client_session& session, const net::Peer& peer) {
		if (!same_peer(peer, session.host)) {
			return {};
		}

		packet_kind kind = {};
		IF_ERROR_RETURN_ERROR(stream(bin::field("kind", kind)));

		switch (kind) {
		case packet_kind::connect_accept: {
			PacketSequence packet_sequence = 0;
			SessionId accepted_session_id = 0;
			key server_public_key;
			signature packet_signature;
			IF_ERROR_RETURN_ERROR(stream(
				bin::field("packet_sequence", packet_sequence),
				bin::field("session_id", accepted_session_id),
				bin::field("server_public_key", server_public_key),
				bin::field("signature", packet_signature)
			));

			packet_record record = record_received_packet(session.received, packet_sequence, session.options);
			session.send_missing_for(std::move(record.missing));
			session.session_id = accepted_session_id;
			session.local_session = accepted_session_id;
			if (session.session_state == state::connecting) {
				session.session_state = state::logging_in;
				session.next_join_request = now();
			}
			return {};
		}
		case packet_kind::snapshot_begin: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			u64 snapshot_id = 0;
			Tick baseline_tick = 0;
			u64 total_bytes = 0;
			u32 chunk_count = 0;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(
				bin::field("snapshot_id", snapshot_id),
				bin::field("baseline_tick", baseline_tick),
				bin::field("total_bytes", total_bytes),
				bin::field("chunk_count", chunk_count)
			));

			if (!session.valid_session(session_id)) {
				return {};
			}
			packet_record record = record_received_packet(session.received, packet_sequence, session.options);
			session.send_missing_for(std::move(record.missing));
			if (session.session_state == state::catching_up || session.session_state == state::joined) {
				return {};
			}
			if (session.snapshot.active && session.snapshot.snapshot_id == snapshot_id) {
				return {};
			}
			session.session_state = state::downloading_snapshot;
			session.snapshot = {};
			session.snapshot.active = true;
			session.snapshot.snapshot_id = snapshot_id;
			session.snapshot.baseline_tick = baseline_tick;
			session.snapshot.chunk_size = session.options.snapshot_chunk_bytes == 0 ? 1000 : session.options.snapshot_chunk_bytes;
			session.snapshot.chunk_count = chunk_count;
			session.snapshot.bytes.resize(static_cast<size_t>(total_bytes));
			session.snapshot.received_chunks.resize(chunk_count);
			return {};
		}
		case packet_kind::snapshot_chunk: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			u64 snapshot_id = 0;
			u32 chunk_index = 0;
			vector<byte> bytes;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(
				bin::field("snapshot_id", snapshot_id),
				bin::field("chunk_index", chunk_index),
				bin::field("bytes", bytes)
			));

			if (!session.valid_session(session_id) || !session.snapshot.active || session.snapshot.snapshot_id != snapshot_id) {
				return {};
			}
			packet_record record = record_received_packet(session.received, packet_sequence, session.options);
			session.send_missing_for(std::move(record.missing));
			if (chunk_index >= session.snapshot.chunk_count || session.snapshot.received_chunks[chunk_index] != 0) {
				return {};
			}
			const size_t offset = static_cast<size_t>(chunk_index) * session.snapshot.chunk_size;
			if (offset + bytes.size() > session.snapshot.bytes.size()) {
				return {};
			}
			std::copy(bytes.begin(), bytes.end(), session.snapshot.bytes.begin() + static_cast<i64>(offset));
			session.snapshot.received_chunks[chunk_index] = 1;
			++session.snapshot.received_count;
			session.try_finish_snapshot();
			return {};
		}
		case packet_kind::snapshot_end: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			u64 snapshot_id = 0;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(bin::field("snapshot_id", snapshot_id)));

			if (!session.valid_session(session_id) || !session.snapshot.active || session.snapshot.snapshot_id != snapshot_id) {
				return {};
			}
			packet_record record = record_received_packet(session.received, packet_sequence, session.options);
			session.send_missing_for(std::move(record.missing));
			session.snapshot.end_received = true;
			session.try_finish_snapshot();
			return {};
		}
		case packet_kind::server_heartbeat: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			vector<tick_closure> tick_closures;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(bin::field("tick_closures", tick_closures)));

			if (!session.valid_session(session_id)) {
				return {};
			}
			packet_record record = record_received_packet(session.received, packet_sequence, session.options);
			session.send_missing_for(std::move(record.missing));
			if (!record.fresh) {
				return {};
			}
			for (const tick_closure& closure : tick_closures) {
				session.buffer_tick(closure);
			}
			if (session.session_state != state::connecting &&
				session.session_state != state::logging_in &&
				session.session_state != state::downloading_snapshot) {
				if (session.drain_buffered_ticks() && session.session_state == state::catching_up) {
					session.session_state = state::joined;
				}
			}
			return {};
		}
		case packet_kind::missing_packets: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			vector<PacketSequence> missing;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			IF_ERROR_RETURN_ERROR(stream(bin::field("missing", missing)));

			if (session.valid_session(session_id)) {
				session.resend_missing(missing);
			}
			return {};
		}
		case packet_kind::disconnect: {
			SessionId session_id = 0;
			PacketSequence packet_sequence = 0;
			IF_ERROR_RETURN_ERROR(read_connected_header(stream, session_id, packet_sequence));
			if (session.valid_session(session_id)) {
				session.socket.disconnect();
				session.session_state = state::disconnected;
				session.simulation->disconnected();
			}
			return {};
		}
		default:
			return {};
		}
	}

	offline_session::offline_session(Simulation& simulation, Options options)
		: Impl(mode::offline, state::joined, simulation, options) {
		local_session = host_session_id;
	}

	void offline_session::update() {
		collect_local();
	}

	void offline_session::advance() {
		if (session_state == state::disconnected) {
			throw runtime_exception("lockstep session is disconnected");
		}
		++current_tick;
		sort_commands(staged_commands);
		build_command_views(command_views, staged_commands);
		simulation->step(current_tick, span<const Command>(command_views.data(), command_views.size()));
		erase_confirmed_pending(pending_payloads, host_session_id, staged_commands);
		rebuild_pending_views();
		staged_commands.clear();
	}

	void offline_session::disconnect() {
		if (session_state == state::disconnected) {
			return;
		}
		session_state = state::disconnected;
		simulation->disconnected();
	}

	void offline_session::collect_local() {
		if (session_state != state::joined) {
			return;
		}
		Submitter submitter(make_unique<collect_submitter>(*this, staged_commands, host_session_id, true));
		simulation->collect(submitter);
	}

	host_session::host_session(net::Socket socket, Simulation& simulation, Options options)
		: Impl(mode::host, state::joined, simulation, options), socket(std::move(socket)) {
		local_session = host_session_id;
	}

	void host_session::update() {
		if (session_state == state::disconnected) {
			return;
		}
		process_snapshot_requests();
		poll_socket();
		collect_local();
		flush_send_queue();
	}

	void host_session::advance() {
		if (session_state == state::disconnected) {
			throw runtime_exception("lockstep session is disconnected");
		}
		for (const host_connection& connection : connections) {
			if (connection.snapshot_requested || connection.connection_state == state::downloading_snapshot) {
				return;
			}
		}
		++current_tick;
		sort_commands(staged_commands);
		build_command_views(command_views, staged_commands);
		simulation->step(current_tick, span<const Command>(command_views.data(), command_views.size()));
		erase_confirmed_pending(pending_payloads, host_session_id, staged_commands);
		rebuild_pending_views();

		tick_closure closure;
		closure.tick = current_tick;
		for (const stored_payload& command : staged_commands) {
			closure.commands.emplace_back(to_wire_payload(command));
		}
		for (host_connection& connection : connections) {
			if (connection.connection_state != state::joined) {
				continue;
			}
			send_connected(connection, packet_kind::server_heartbeat, [&](bin::write_stream& stream) -> error {
				vector<tick_closure> closures;
				closures.emplace_back(closure);
				return stream(bin::field("tick_closures", closures));
			});
		}
		staged_commands.clear();
	}

	void host_session::disconnect() {
		if (session_state == state::disconnected) {
			return;
		}
		bool was_sending_snapshot = false;
		for (host_connection& connection : connections) {
			was_sending_snapshot = was_sending_snapshot || connection.snapshot_sending;
			send_connected(connection, packet_kind::disconnect, [](bin::write_stream&) -> error {
				return {};
			});
		}
		flush_send_queue();
		socket.disconnect();
		session_state = state::disconnected;
		if (was_sending_snapshot) {
			simulation->snapshot_save_finished();
		}
		simulation->disconnected();
	}

	void host_session::collect_local() {
		Submitter submitter(make_unique<collect_submitter>(*this, staged_commands, host_session_id, true));
		simulation->collect(submitter);
	}

	void host_session::poll_socket() {
		u32 received = 0;
		while (received < options.max_incoming_packets_per_update) {
			optional<net::Message> message = socket.recv(receive_buffer);
			if (!message) {
				break;
			}
			try {
				receive_message(*message);
			} catch (const exception&) {
			}
			++received;
		}
	}

	void host_session::receive_message(const net::Message& message) {
		bin::read_stream stream(message.second);
		if (error err = process(stream, *this, message.first)) {
			throw runtime_exception(err.message);
		}
	}

	host_connection& host_session::find_or_create_connection(const net::Peer& peer) {
		for (host_connection& connection : connections) {
			if (same_peer(connection.peer, peer)) {
				return connection;
			}
		}
		const SessionId session_id = next_session_id;
		++next_session_id;
		connections.emplace_back(peer, session_id);
		return connections.back();
	}

	host_connection* host_session::find_connection(const net::Peer& peer, SessionId session_id) {
		for (host_connection& connection : connections) {
			if (connection.session_id == session_id && same_peer(connection.peer, peer)) {
				return &connection;
			}
		}
		return nullptr;
	}

	void host_session::send_connect_accept(host_connection& connection) {
		const PacketSequence packet_sequence = connection.next_send_sequence;
		++connection.next_send_sequence;
		vector<byte> bytes = write_packet(packet_kind::connect_accept, [&](bin::write_stream& stream) -> error {
			key server_public_key;
			signature packet_signature;
			return stream(
				bin::field("packet_sequence", packet_sequence),
				bin::field("session_id", connection.session_id),
				bin::field("server_public_key", server_public_key),
				bin::field("signature", packet_signature)
			);
		});
		socket.send(connection.peer, bytes_view(bytes));
		flush_send_queue();
		remember_sent_packet(connection.sent_packets, packet_sequence, bytes);
	}

	void host_session::request_snapshot(host_connection& connection) {
		connection.connection_state = state::downloading_snapshot;
		connection.snapshot_requested = true;
		simulation->snapshot_save_started();
	}

	void host_session::process_snapshot_requests() {
		for (host_connection& connection : connections) {
			if (connection.snapshot_sending) {
				continue_snapshot(connection);
				return;
			}
			if (!connection.snapshot_requested) {
				continue;
			}
			connection.snapshot_requested = false;
			try {
				begin_snapshot(connection);
				continue_snapshot(connection);
			} catch (...) {
				connection.connection_state = state::disconnected;
				connection.snapshot_sending = false;
				connection.snapshot_bytes.clear();
				simulation->snapshot_save_finished();
				throw;
			}
			return;
		}
	}

	void host_session::begin_snapshot(host_connection& connection) {
		connection.snapshot_bytes = simulation->save_snapshot();
		const u16 chunk_size = options.snapshot_chunk_bytes == 0 ? 1000 : options.snapshot_chunk_bytes;
		const u32 chunk_count = static_cast<u32>((connection.snapshot_bytes.size() + chunk_size - 1u) / chunk_size);
		connection.snapshot_id = next_snapshot_id;
		++next_snapshot_id;
		connection.snapshot_baseline_tick = current_tick;
		connection.snapshot_chunk_size = chunk_size;
		connection.snapshot_chunk_count = chunk_count;
		connection.next_snapshot_chunk = 0;
		connection.snapshot_sending = true;

		send_connected(connection, packet_kind::snapshot_begin, [&](bin::write_stream& stream) -> error {
			const u64 total_bytes = static_cast<u64>(connection.snapshot_bytes.size());
			return stream(
				bin::field("snapshot_id", connection.snapshot_id),
				bin::field("baseline_tick", connection.snapshot_baseline_tick),
				bin::field("total_bytes", total_bytes),
				bin::field("chunk_count", connection.snapshot_chunk_count)
			);
		});
	}

	void host_session::continue_snapshot(host_connection& connection) {
		u32 sent = 0;
		while (connection.next_snapshot_chunk < connection.snapshot_chunk_count && sent < options.max_snapshot_chunks_per_update) {
			const u32 chunk_index = connection.next_snapshot_chunk;
			++connection.next_snapshot_chunk;
			const size_t offset = static_cast<size_t>(chunk_index) * connection.snapshot_chunk_size;
			const size_t remaining = connection.snapshot_bytes.size() - offset;
			const size_t count = std::min(static_cast<size_t>(connection.snapshot_chunk_size), remaining);
			send_connected(connection, packet_kind::snapshot_chunk, [&](bin::write_stream& stream) -> error {
				vector<byte> bytes;
				bytes.assign(connection.snapshot_bytes.begin() + static_cast<i64>(offset), connection.snapshot_bytes.begin() + static_cast<i64>(offset + count));
				return stream(
					bin::field("snapshot_id", connection.snapshot_id),
					bin::field("chunk_index", chunk_index),
					bin::field("bytes", bytes)
				);
			});
			++sent;
		}

		if (connection.next_snapshot_chunk < connection.snapshot_chunk_count) {
			return;
		}

		send_connected(connection, packet_kind::snapshot_end, [&](bin::write_stream& stream) -> error {
			return stream(bin::field("snapshot_id", connection.snapshot_id));
		});
		connection.snapshot_sending = false;
		connection.snapshot_bytes.clear();
		connection.connection_state = state::joined;
		simulation->snapshot_save_finished();
	}

	void host_session::send_missing_for(host_connection& connection, vector<PacketSequence> missing) {
		if (missing.empty()) {
			return;
		}
		send_connected(connection, packet_kind::missing_packets, [&](bin::write_stream& stream) -> error {
			return stream(bin::field("missing", missing));
		});
	}

	void host_session::resend_missing(host_connection& connection, const vector<PacketSequence>& missing) {
		resend_packets(socket, connection.peer, connection.sent_packets, missing, options.max_resends_per_update);
	}

	void host_session::flush_send_queue() {
		u32 sent = 0;
		while (!send_queue.empty() && sent < options.max_outgoing_packets_per_update) {
			queued_send outgoing = std::move(send_queue.front());
			send_queue.erase(send_queue.begin());
			socket.send(outgoing.peer, bytes_view(outgoing.bytes));
			++sent;
		}
	}

	template<typename Writer>
	void host_session::send_connected(host_connection& connection, packet_kind kind, Writer writer) {
		const PacketSequence packet_sequence = connection.next_send_sequence;
		++connection.next_send_sequence;
		vector<byte> bytes = write_connected_packet(kind, connection.session_id, packet_sequence, writer);
		send_queue.emplace_back(queued_send {
			.peer = connection.peer,
			.bytes = bytes,
		});
		remember_sent_packet(connection.sent_packets, packet_sequence, bytes);
	}

	client_session::client_session(net::Socket socket, net::Peer host, Simulation& simulation, Options options)
		: Impl(mode::client, state::connecting, simulation, options), socket(std::move(socket)), host(std::move(host)) {}

	void client_session::update() {
		if (session_state == state::disconnected) {
			return;
		}
		poll_socket();
		if (session_state == state::downloading_snapshot && snapshot.load_started) {
			try_finish_snapshot();
		}
		if (session_state == state::catching_up) {
			if (drain_buffered_ticks()) {
				session_state = state::joined;
			}
		}
		const instant current_time = now();
		if ((session_state == state::connecting ||
			 session_state == state::logging_in ||
			 session_state == state::downloading_snapshot ||
			 session_state == state::catching_up) &&
			options.connect_timeout.quantum_count() > 0 &&
			duration::from_quantum(current_time.quantum_count() - last_connect_activity.quantum_count()) >= options.connect_timeout) {
			socket.disconnect();
			session_state = state::disconnected;
			simulation->disconnected();
			return;
		}
		if (session_state == state::connecting && current_time >= next_connect_request) {
			send_connect_request();
			next_connect_request = current_time + options.handshake_interval;
		}
		if ((session_state == state::logging_in || session_state == state::downloading_snapshot) && current_time >= next_join_request) {
			send_join_request();
			session_state = state::downloading_snapshot;
			next_join_request = current_time + options.handshake_interval;
		}
		if (session_state == state::joined) {
			collect_local();
			send_client_heartbeat();
			resend_pending_payloads();
		}
	}

	void client_session::advance() {
		throw runtime_exception("client lockstep sessions advance only from server ticks");
	}

	void client_session::disconnect() {
		if (session_state == state::disconnected) {
			return;
		}
		if (session_id != 0) {
			send_connected(packet_kind::disconnect, [](bin::write_stream&) -> error {
				return {};
			});
		}
		socket.disconnect();
		session_state = state::disconnected;
		simulation->disconnected();
	}

	void client_session::poll_socket() {
		u32 received = 0;
		while (received < options.max_incoming_packets_per_update) {
			optional<net::Message> message = socket.recv(receive_buffer);
			if (!message) {
				break;
			}
			try {
				receive_message(*message);
			} catch (const exception&) {
			}
			++received;
		}
	}

	void client_session::receive_message(const net::Message& message) {
		bin::read_stream stream(message.second);
		last_connect_activity = now();
		if (error err = process(stream, *this, message.first)) {
			throw runtime_exception(err.message);
		}
	}

	void client_session::collect_local() {
		Submitter submitter(make_unique<collect_submitter>(*this, outgoing_commands, session_id, true));
		simulation->collect(submitter);
	}

	void client_session::buffer_tick(const tick_closure& closure) {
		if (closure.tick <= current_tick) {
			return;
		}
		for (stored_tick& existing : buffered_ticks) {
			if (existing.tick == closure.tick) {
				return;
			}
		}
		stored_tick tick;
		tick.tick = closure.tick;
		for (const wire_payload& payload : closure.commands) {
			tick.commands.emplace_back(from_wire_payload(payload, 0));
		}
		sort_commands(tick.commands);
		buffered_ticks.emplace_back(std::move(tick));
	}

	bool client_session::drain_buffered_ticks() {
		u32 stepped = 0;
		while (stepped < options.max_tick_steps_per_update) {
			const Tick next_tick = current_tick + 1;
			size_t found_index = buffered_ticks.size();
			for (size_t index = 0; index < buffered_ticks.size(); ++index) {
				if (buffered_ticks[index].tick == next_tick) {
					found_index = index;
					break;
				}
			}
			if (found_index == buffered_ticks.size()) {
				return true;
			}
			stored_tick tick = std::move(buffered_ticks[found_index]);
			buffered_ticks.erase(buffered_ticks.begin() + static_cast<i64>(found_index));
			build_command_views(command_views, tick.commands);
			simulation->step(tick.tick, span<const Command>(command_views.data(), command_views.size()));
			erase_confirmed_pending(pending_payloads, session_id, tick.commands);
			rebuild_pending_views();
			current_tick = tick.tick;
			++stepped;
		}
		const Tick next_tick = current_tick + 1;
		for (const stored_tick& tick : buffered_ticks) {
			if (tick.tick == next_tick) {
				return false;
			}
		}
		return true;
	}

	void client_session::try_finish_snapshot() {
		if (!snapshot.active || !snapshot.end_received || snapshot.received_count != snapshot.chunk_count) {
			return;
		}
		if (!snapshot.load_started) {
			snapshot.load_started = true;
			simulation->start_load_snapshot(bytes_view(snapshot.bytes));
			return;
		}
		if (!simulation->snapshot_load_finished()) {
			return;
		}
		current_tick = snapshot.baseline_tick;
		pending_payloads.clear();
		rebuild_pending_views();
		snapshot = {};
		session_state = state::catching_up;
		if (drain_buffered_ticks() && session_state == state::catching_up) {
			session_state = state::joined;
		}
	}

	bool client_session::valid_session(SessionId packet_session_id) const {
		return session_id != 0 && packet_session_id == session_id;
	}

	void client_session::send_connect_request() {
		const PacketSequence packet_sequence = next_send_sequence;
		++next_send_sequence;
		vector<byte> bytes = write_packet(packet_kind::connect_request, [&](bin::write_stream& stream) -> error {
			key client_public_key;
			return stream(
				bin::field("packet_sequence", packet_sequence),
				bin::field("client_public_key", client_public_key)
			);
		});
		socket.send(host, bytes_view(bytes));
		remember_sent_packet(sent_packets, packet_sequence, bytes);
	}

	void client_session::send_join_request() {
		if (session_id == 0) {
			return;
		}
		send_connected(packet_kind::join_request, [&](bin::write_stream& stream) -> error {
			vector<byte> payload = simulation->login_payload();
			return stream(bin::field("login_payload", payload));
		});
	}

	void client_session::send_client_heartbeat() {
		if (outgoing_commands.empty()) {
			return;
		}
		vector<wire_payload> commands;
		commands.reserve(outgoing_commands.size());
		for (const stored_payload& command : outgoing_commands) {
			commands.emplace_back(to_wire_payload(command));
		}
		send_connected(packet_kind::client_heartbeat, [&](bin::write_stream& stream) -> error {
			return stream(bin::field("commands", commands));
		});
		outgoing_commands.clear();
	}

	void client_session::resend_pending_payloads() {
		if (pending_payloads.empty() || current_tick < next_pending_resend_tick) {
			return;
		}
		if (!outgoing_commands.empty()) {
			return;
		}
		for (const pending_payload& pending : pending_payloads) {
			outgoing_commands.emplace_back(stored_payload {
				.id = pending.id,
				.source = session_id,
				.hash = pending.hash,
				.bytes = pending.bytes,
			});
		}
		send_client_heartbeat();
		next_pending_resend_tick = current_tick + 5;
	}

	void client_session::send_missing_for(vector<PacketSequence> missing) {
		if (missing.empty() || session_id == 0) {
			return;
		}
		send_connected(packet_kind::missing_packets, [&](bin::write_stream& stream) -> error {
			return stream(bin::field("missing", missing));
		});
	}

	void client_session::resend_missing(const vector<PacketSequence>& missing) {
		resend_packets(socket, host, sent_packets, missing, options.max_resends_per_update);
	}

	template<typename Writer>
	void client_session::send_connected(packet_kind kind, Writer writer) {
		const PacketSequence packet_sequence = next_send_sequence;
		++next_send_sequence;
		vector<byte> bytes = write_connected_packet(kind, session_id, packet_sequence, writer);
		socket.send(host, bytes_view(bytes));
		remember_sent_packet(sent_packets, packet_sequence, bytes);
	}

	Submitter::Submitter(unique_ptr<Impl> impl) : impl(std::move(impl)) {}
	Submitter::Submitter(Submitter&& other) noexcept = default;
	Submitter& Submitter::operator=(Submitter&& other) noexcept = default;
	Submitter::~Submitter() = default;

	PayloadId Submitter::submit(span<const byte> bytes) {
		if (!impl) {
			throw runtime_exception("lockstep submitter is not active");
		}
		return impl->submit(bytes);
	}

	void Session::ImplDeleter::operator()(Impl* impl) const noexcept {
		delete impl;
	}

	Session::Session(unique_ptr<Impl> impl) : impl(impl.release()) {}
	Session::Session(Session&& other) noexcept = default;
	Session& Session::operator=(Session&& other) noexcept = default;

	Session::~Session() {
		if (impl) {
			try {
				impl->disconnect();
			} catch (const exception&) {
			}
		}
	}

	Session Session::Offline(Simulation& simulation, Options options) {
		return Session(make_unique<offline_session>(simulation, options));
	}

	Session Session::Host(net::Socket socket, Simulation& simulation, Options options) {
		return Session(make_unique<host_session>(std::move(socket), simulation, options));
	}

	Session Session::Client(net::Socket socket, net::Peer host, Simulation& simulation, Options options) {
		return Session(make_unique<client_session>(std::move(socket), std::move(host), simulation, options));
	}

	Session::operator bool() const noexcept {
		return impl && impl->session_state != state::disconnected;
	}

	void Session::update() {
		if (!impl) {
			throw runtime_exception("lockstep session is not open");
		}
		impl->update();
	}

	void Session::advance() {
		if (!impl) {
			throw runtime_exception("lockstep session is not open");
		}
		impl->advance();
	}

	void Session::disconnect() {
		if (impl) {
			impl->disconnect();
		}
	}

	mode Session::mode() const {
		return impl ? impl->session_mode : mode::offline;
	}

	state Session::state() const {
		return impl ? impl->session_state : state::disconnected;
	}

	SessionId Session::local_session_id() const {
		return impl ? impl->local_session : 0;
	}

	Tick Session::tick() const {
		return impl ? impl->current_tick : 0;
	}

	bool Session::joined() const {
		return impl && impl->session_state == state::joined;
	}

	span<const PendingPayload> Session::pending() const {
		if (!impl) {
			return {};
		}
		return span<const PendingPayload>(impl->pending_views.data(), impl->pending_views.size());
	}
}
