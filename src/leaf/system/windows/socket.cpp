#include "leaf/system/socket.hpp"

#include "leaf/core/exception.hpp"

#include <array>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace lf::sys {
	constexpr std::size_t max_udp_payload = 65507;
	constexpr SOCKET invalid_socket_handle = INVALID_SOCKET;

	SOCKET native_socket(socket_udp socket) {
		return static_cast<SOCKET>(socket.handle);
	}

	error init_udp_sockets() {
		WSADATA data{};
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
			return error("failed to initialize Winsock");
		}
		return error::no_error;
	}

	void exit_udp_sockets() {
		WSACleanup();
	}

	socket_udp open_socket_udp(u16 port) {
		SOCKET socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (socket == invalid_socket_handle) {
			throw runtime_exception("failed to create UDP socket");
		}

		try {
			u_long mode = 1;
			if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
				throw runtime_exception("failed to set UDP socket nonblocking");
			}

			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = htonl(INADDR_ANY);
			address.sin_port = htons(port);
			if (bind(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
				throw runtime_exception("failed to bind UDP socket");
			}
		} catch (...) {
			closesocket(socket);
			throw;
		}

		return socket_udp { static_cast<u64>(socket) };
	}

	void close_socket_udp(socket_udp& socket) {
		if (!socket) {
			return;
		}
		closesocket(native_socket(socket));
		socket.handle = 0;
	}

	void send_socket_udp(socket_udp socket, string_view address, u16 port, span<const byte> data) {
		if (data.size() > max_udp_payload) {
			throw runtime_exception("UDP message is too large");
		}

		sockaddr_in remote{};
		remote.sin_family = AF_INET;
		remote.sin_port = htons(port);
		const string address_string(address);
		if (inet_pton(AF_INET, address_string.c_str(), &remote.sin_addr) != 1) {
			addrinfo hints{};
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			hints.ai_protocol = IPPROTO_UDP;

			addrinfo* result = nullptr;
			const string port_string = std::to_string(port);
			if (getaddrinfo(address_string.c_str(), port_string.c_str(), &hints, &result) != 0 || !result) {
				throw runtime_exception("failed to resolve UDP peer");
			}

			std::memcpy(&remote, result->ai_addr, sizeof(remote));
			freeaddrinfo(result);
		}

		const int sent = sendto(
			native_socket(socket),
			reinterpret_cast<const char*>(data.data()),
			static_cast<int>(data.size()),
			0,
			reinterpret_cast<const sockaddr*>(&remote),
			sizeof(remote)
		);
		if (sent < 0) {
			throw runtime_exception("failed to send UDP message");
		}
	}

	optional<message_udp> recv_socket_udp(socket_udp socket, span<byte> buffer) {
		sockaddr_in sender{};
		int sender_size = sizeof(sender);
		const int received = recvfrom(
			native_socket(socket),
			reinterpret_cast<char*>(buffer.data()),
			static_cast<int>(buffer.size()),
			0,
			reinterpret_cast<sockaddr*>(&sender),
			&sender_size
		);
		if (received < 0) {
			return std::nullopt;
		}

		ch08 host[INET_ADDRSTRLEN]{};
		if (!inet_ntop(AF_INET, &sender.sin_addr, host, sizeof(host))) {
			std::strncpy(host, "0.0.0.0", sizeof(host) - 1);
		}

		return message_udp {
			.address = host,
			.port = ntohs(sender.sin_port),
			.data = buffer.subspan(0, static_cast<std::size_t>(received)),
		};
	}
}
