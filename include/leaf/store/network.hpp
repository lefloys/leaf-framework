#pragma once

#include "leaf/network/socket.hpp"

namespace lf::net::store_provider {
	void open_channel(u16 channel);
	void close_channel(u16 channel);
	void send(u16 local_channel, string_view user_id, u16 remote_channel, span<const byte> data);
	optional<Message> recv(u16 local_channel, span<byte> buffer);
} // namespace lf::net::store_provider
