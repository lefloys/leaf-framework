#include "leaf/store/network.hpp"

#include "leaf/core/exception.hpp"

namespace lf::net::store_provider {
	void open_channel(u16) {
		throw runtime_exception("store networking is not available");
	}

	void close_channel(u16) {}

	void send(u16, string_view, u16, span<const byte>) {
		throw runtime_exception("store networking is not available");
	}

	optional<Message> recv(u16, span<byte>) {
		throw runtime_exception("store networking is not available");
	}
}
