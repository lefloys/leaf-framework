#include "leaf/store/network.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/vector.hpp"

#include <steam/steam_api.h>

#include <array>
#include <charconv>
#include <cstring>
#include <system_error>

namespace lf::net::steam_network_detail {
	constexpr std::array<byte, 4> header_magic {
		static_cast<byte>('L'),
		static_cast<byte>('F'),
		static_cast<byte>('N'),
		static_cast<byte>('1'),
	};
	constexpr std::size_t header_size = header_magic.size() + sizeof(u16);

	bool user_id_from_string(string_view id, u64& value) {
		const char* first = id.data();
		const char* last = first + id.size();
		const std::from_chars_result result = std::from_chars(first, last, value);
		return result.ec == std::errc{} && result.ptr == last && value != 0;
	}

	SteamNetworkingIdentity identity_from_user_id(u64 user_id) {
		SteamNetworkingIdentity identity;
		identity.Clear();
		identity.SetSteamID64(user_id);
		return identity;
	}

	void accept_session_request(SteamNetworkingMessagesSessionRequest_t* request) {
		if (!request || !SteamNetworkingMessages()) {
			return;
		}
		SteamNetworkingMessages()->AcceptSessionWithUser(request->m_identityRemote);
	}

	void write_header(vector<byte>& message, u16 source_channel) {
		message.insert(message.end(), header_magic.begin(), header_magic.end());
		message.push_back(static_cast<byte>(source_channel & 0xff));
		message.push_back(static_cast<byte>((source_channel >> 8) & 0xff));
	}

	bool read_header(span<const byte> message, u16& source_channel) {
		if (message.size() < header_size) {
			return false;
		}
		for (std::size_t i = 0; i < header_magic.size(); ++i) {
			if (message[i] != header_magic[i]) {
				return false;
			}
		}

		source_channel = static_cast<u16>(message[4]) |
			(static_cast<u16>(message[5]) << 8);
		return true;
	}
}

namespace lf::net::store_provider {
	void open_channel(u16) {
		if (SteamNetworkingUtils()) {
			SteamNetworkingUtils()->SetGlobalCallback_MessagesSessionRequest(steam_network_detail::accept_session_request);
		}
	}

	void close_channel(u16) {}

	void send(u16 local_channel, string_view user_id, u16 remote_channel, span<const byte> data) {
		if (!SteamNetworkingMessages()) {
			throw runtime_exception("Steam networking messages are not available");
		}

		u64 steam_id = 0;
		if (!steam_network_detail::user_id_from_string(user_id, steam_id)) {
			throw invalid_argument_exception("Steam peer user id is invalid");
		}

		vector<byte> message;
		message.reserve(steam_network_detail::header_size + data.size());
		steam_network_detail::write_header(message, local_channel);
		message.insert(message.end(), data.begin(), data.end());

		SteamNetworkingIdentity identity = steam_network_detail::identity_from_user_id(steam_id);
		const EResult result = SteamNetworkingMessages()->SendMessageToUser(
			identity,
			message.data(),
			static_cast<u32>(message.size()),
			k_nSteamNetworkingSend_UnreliableNoDelay | k_nSteamNetworkingSend_AutoRestartBrokenSession,
			static_cast<int>(remote_channel)
		);
		if (result != k_EResultOK) {
			throw runtime_exception("failed to send Steam networking message");
		}
	}

	optional<Message> recv(u16 local_channel, span<byte> buffer) {
		if (!SteamNetworkingMessages()) {
			throw runtime_exception("Steam networking messages are not available");
		}

		SteamNetworkingMessage_t* message = nullptr;
		const int count = SteamNetworkingMessages()->ReceiveMessagesOnChannel(static_cast<int>(local_channel), &message, 1);
		if (count <= 0 || !message) {
			return std::nullopt;
		}

		const span<const byte> raw {
			reinterpret_cast<const byte*>(message->m_pData),
			static_cast<std::size_t>(message->m_cbSize),
		};

		u16 source_channel = local_channel;
		if (!steam_network_detail::read_header(raw, source_channel)) {
			message->Release();
			throw runtime_exception("received malformed Steam networking message");
		}

		const span<const byte> payload = raw.subspan(steam_network_detail::header_size);
		if (payload.size() > buffer.size()) {
			message->Release();
			throw runtime_exception("receive buffer is too small for Steam networking message");
		}

		if (!payload.empty()) {
			std::memcpy(buffer.data(), payload.data(), payload.size());
		}

		const u64 sender_id = message->m_identityPeer.GetSteamID64();
		message->Release();

		return Message {
			Peer::User(store::User::Get(std::to_string(sender_id)), source_channel),
			buffer.subspan(0, payload.size()),
		};
	}
}
