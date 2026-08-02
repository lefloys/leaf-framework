#pragma once

#include "leaf/core/memory.hpp"
#include "leaf/core/optional.hpp"
#include "leaf/core/span.hpp"
#include "leaf/network/peer.hpp"

#include <utility>

namespace lf::net {
	using Message = std::pair<Peer, span<const byte>>;

	class Socket {
	  public:
		Socket() = default;

		static Socket Port(u16 port);
		static Socket Channel(u16 channel);

		Socket(const Socket&) = delete;
		Socket& operator=(const Socket&) = delete;
		Socket(Socket&& other) noexcept;
		Socket& operator=(Socket&& other) noexcept;
		~Socket();

		explicit operator bool() const noexcept;
		void disconnect();

		optional<Message> recv(span<byte> buffer);
		void send(const Peer& peer, span<const byte> data);

		struct Impl;

	  private:
		explicit Socket(unique_ptr<Impl> impl);

		unique_ptr<Impl> impl;
	};
}
