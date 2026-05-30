#pragma once

#include "network.hpp"

namespace lf::net {
	class connection_impl {
	  public:
		virtual ~connection_impl() = default;

		virtual connection_state state() const = 0;
		virtual packet receive() = 0;
		virtual error send(span<const byte> bytes) = 0;
		virtual void close() = 0;
	};

	class listener_impl {
	  public:
		virtual ~listener_impl() = default;

		virtual connection accept() = 0;
		virtual void close() = 0;
	};

	class provider {
	  public:
		virtual ~provider() = default;

		virtual provider_info info() const = 0;
		virtual report<listener> listen(const listen_target& target) = 0;
		virtual report<connection> connect(const connect_target& target) = 0;
	};

	error register_provider(unique_ptr<provider> provider);
	bool unregister_provider(string_view provider_id);
	void clear_providers();
} // namespace lf::net
