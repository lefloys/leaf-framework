#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/types.hpp>
#include <leaf/core/vector.hpp>

namespace lf::net {
	struct packet {
		vector<byte> bytes;

		explicit operator bool() const noexcept;
	};

	enum class connection_state : u08 {
		connecting,
		connected,
		disconnected,
	};

	enum class provider_feature : u08 {
		direct_ip,
		relay,
		p2p,
		lobbies,
		invites,
	};

	struct provider_info {
		string id;
		string display_name;
		vector<provider_feature> features;
	};

	struct direct_ip_listen {
		u16 port = 0;
		string host;
	};

	struct direct_ip_connect {
		string host;
		u16 port = 0;
	};

	class listen_target {
	  public:
		static listen_target direct_ip(u16 port);
		static listen_target direct_ip(direct_ip_listen target);
		static listen_target provider_data(string provider_id, vector<byte> data);

		string_view provider_id() const noexcept;
		const direct_ip_listen* direct_ip() const noexcept;
		span<const byte> data() const noexcept;

	  private:
		string provider;
		direct_ip_listen direct;
		vector<byte> opaque_data;
		bool has_direct = false;
	};

	class connect_target {
	  public:
		static connect_target direct_ip(string host, u16 port);
		static connect_target direct_ip(direct_ip_connect target);
		static connect_target provider_data(string provider_id, vector<byte> data);

		string_view provider_id() const noexcept;
		const direct_ip_connect* direct_ip() const noexcept;
		span<const byte> data() const noexcept;

	  private:
		string provider;
		direct_ip_connect direct;
		vector<byte> opaque_data;
		bool has_direct = false;
	};

	class connection_impl;
	class listener_impl;

	class connection {
	  public:
		connection() = default;
		explicit connection(unique_ptr<connection_impl> impl);
		~connection();

		connection(connection&&) noexcept;
		connection& operator=(connection&&) noexcept;

		connection(const connection&) = delete;
		connection& operator=(const connection&) = delete;

		explicit operator bool() const noexcept;

		connection_state state() const;
		packet receive();
		error send(span<const byte> bytes);
		void close();

	  private:
		unique_ptr<connection_impl> impl;
	};

	class listener {
	  public:
		listener() = default;
		explicit listener(unique_ptr<listener_impl> impl);
		~listener();

		listener(listener&&) noexcept;
		listener& operator=(listener&&) noexcept;

		listener(const listener&) = delete;
		listener& operator=(const listener&) = delete;

		explicit operator bool() const noexcept;

		connection accept();
		void close();

	  private:
		unique_ptr<listener_impl> impl;
	};

	report<listener> listen(const listen_target& target);
	report<connection> connect(const connect_target& target);

	vector<provider_info> providers();
	bool supports(provider_feature feature);
	bool supports(string_view provider_id, provider_feature feature);
} // namespace lf::net
