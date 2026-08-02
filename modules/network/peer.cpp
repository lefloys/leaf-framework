#include "leaf/network/peer.hpp"

namespace lf::net {
	Peer Peer::Address(string_view host, u16 port) {
		return { string(host), port };
	}

	Peer Peer::User(const store::User& user, u16 channel) {
		return { user.id, channel };
	}

	Peer::Peer(string_view host, u16 port) : m_id(host), m_channel(port) {}

	string_view Peer::id() const {
		return m_id;
	}

	u16 Peer::channel() const {
		return m_channel;
	}
} // namespace lf::net
