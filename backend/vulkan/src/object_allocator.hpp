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

template<typename Tag, typename Resource>
class resource_pool {
public:
	template<typename... Args>
	lf::handle<Tag> create(Args&&... args) {
		for (auto [index, current] : lf::enumerate(slots)) {
			if (current.resource->has_value()) {
				continue;
			}

			current.resource->emplace(std::forward<Args>(args)...);
			return { static_cast<u32>(index + 1), current.generation };
		}

		slots.emplace_back();
		slot& current = slots.back();
		current.resource->emplace(std::forward<Args>(args)...);
		return { static_cast<u32>(slots.size()), current.generation };
	}

	void destroy(lf::handle<Tag> resource_handle) {
		slot& current = require_slot(resource_handle);
		current.resource->reset();
		++current.generation;
	}

	void clear() {
		for (slot& current : slots) {
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
				<< lf::type_name<Resource>()
				<< " resource(s).\n";
		}

		clear();
	}

	size_t live_resource_count() const {
		std::size_t count = 0;
		for (const auto& current : slots) {
			if (current.resource->has_value()) {
				++count;
			}
		}

		return count;
	}
	Resource& get(lf::view<Tag> resource_view) {
		return **require_slot(resource_view).resource;
	}
	const Resource& get(lf::view<const Tag> resource_view) const {
		return **require_slot(resource_view).resource;
	}

private:
	struct slot {
		u32 generation = 1;
		std::unique_ptr<std::optional<Resource>> resource = std::make_unique<std::optional<Resource>>();
	};

	slot& require_slot(lf::view<Tag> resource_view) {
		if (resource_view.id == 0) {
			std::abort();
		}

		const std::size_t index = static_cast<std::size_t>(resource_view.id - 1);
		if (index >= slots.size()) {
			std::abort();
		}

		slot& current = slots[index];
		if (!current.resource->has_value() || current.generation != resource_view.generation_id) {
			std::abort();
		}

		return current;
	}
	const slot& require_slot(lf::view<const Tag> resource_view) const {
		if (resource_view.id == 0) {
			std::abort();
		}

		const std::size_t index = static_cast<std::size_t>(resource_view.id - 1);
		if (index >= slots.size()) {
			std::abort();
		}

		const slot& current = slots[index];
		if (!current.resource->has_value() || current.generation != resource_view.generation_id) {
			std::abort();
		}

		return current;
	}

	std::vector<slot> slots;
};
