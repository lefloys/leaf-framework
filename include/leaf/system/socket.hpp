#pragma once

#include "leaf/core/error.hpp"
#include "leaf/core/optional.hpp"
#include "leaf/core/span.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/types.hpp"

namespace lf::sys {
	struct socket_udp {
		u64 handle = 0;

		explicit operator bool() const noexcept {
			return handle != 0;
		}
	};

	struct message_udp {
		string address;
		u16 port = 0;
		span<const byte> data;
	};

	error init_udp_sockets();
	void exit_udp_sockets();

	socket_udp open_socket_udp(u16 port);
	void close_socket_udp(socket_udp& socket);
	void send_socket_udp(socket_udp socket, string_view address, u16 port, span<const byte> data);
	optional<message_udp> recv_socket_udp(socket_udp socket, span<byte> buffer);
} // namespace lf::sys
