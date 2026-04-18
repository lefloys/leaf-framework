#pragma once

#include <leaf/core/iterator.hpp>
#include <leaf/core/type_name.hpp>
#include <leaf/graphics/detail/resource.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <cstddef>
#include <utility>
#include <vector>

namespace lf::detail::vk {
	template<typename Tag, typename Resource>
	class resource_pool {
	public:
		template<typename... Args>
		handle<Tag> create(Args&&... args) {
			for (auto [index, current] : enumerate(slots_)) {
				if (current.resource->has_value()) {
					continue;
				}

				current.resource->emplace(std::forward<Args>(args)...);
				return { static_cast<u32>(index + 1), current.generation };
			}

			slots_.emplace_back();
			slot& current = slots_.back();
			current.resource->emplace(std::forward<Args>(args)...);
			return { static_cast<u32>(slots_.size()), current.generation };
		}

		void destroy(handle<Tag> resource_handle) {
			slot& current = require_slot(resource_handle);
			current.resource->reset();
			++current.generation;
		}

		void clear() {
			for (slot& current : slots_) {
				if (!current.resource->has_value()) {
					continue;
				}

				current.resource->reset();
				++current.generation;
			}
		}

		void clear_leaked_resources() {
			const std::size_t leaked_resources = live_resource_count();
			if (leaked_resources != 0) {
				std::cerr
					<< "leaf backend warning: cleaning up "
					<< leaked_resources
					<< " leaked "
					<< type_name<Resource>()
					<< " resource(s).\n";
			}

			clear();
		}

		std::size_t live_resource_count() const {
			std::size_t count = 0;
			for (const auto& current : slots_) {
				if (current.resource->has_value()) {
					++count;
				}
			}

			return count;
		}

		Resource& get(view<Tag> resource_view) {
			return **require_slot(resource_view).resource;
		}

		const Resource& get(view<const Tag> resource_view) const {
			return **require_slot(resource_view).resource;
		}

	private:
		struct slot {
			u32 generation = 1;
			std::unique_ptr<std::optional<Resource>> resource = std::make_unique<std::optional<Resource>>();
		};

		slot& require_slot(view<const Tag> resource_view) {
			return const_cast<slot&>(const_cast<const resource_pool*>(this)->require_slot(resource_view));
		}

		slot& require_slot(handle<Tag> resource_handle) {
			return require_slot(view<const Tag>(resource_handle));
		}

		const slot& require_slot(view<const Tag> resource_view) const {
			if (resource_view.id == 0) {
				std::abort();
			}

			const std::size_t index = static_cast<std::size_t>(resource_view.id - 1);
			if (index >= slots_.size()) {
				std::abort();
			}

			const slot& current = slots_[index];
			if (!current.resource->has_value() || current.generation != resource_view.generation_id) {
				std::abort();
			}

			return current;
		}

		std::vector<slot> slots_;
	};

	template<typename Tag, typename Resource>
	resource_pool<Tag, Resource>& Pool();

	template<typename Resource, typename Tag>
	Resource& Unhandle(view<Tag> resource_view) {
		return Pool<Tag, Resource>().get(resource_view);
	}

	template<typename Resource, typename Tag>
	Resource& Unhandle(handle<Tag> resource_handle) {
		return Pool<Tag, Resource>().get(view<Tag>(resource_handle));
	}

	template<typename Resource, typename Tag>
	const Resource& Unhandle(view<const Tag> resource_view) {
		return Pool<Tag, Resource>().get(resource_view);
	}

	template<typename Resource, typename Tag>
	const Resource& Unhandle(handle<const Tag> resource_handle) {
		return Pool<Tag, Resource>().get(view<const Tag>(resource_handle));
	}
}
