#pragma once

#include <leaf/core/cstddef.hpp>
#include <leaf/core/cstdlib.hpp>
#include <leaf/core/iostream.hpp>
#include <leaf/core/iterator.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/optional.hpp>
#include <leaf/core/type_name.hpp>
#include <leaf/core/utility.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/graphics/detail/resource.hpp>

template<typename Tag, typename Resource>
class resource_pool {
public:
	template<typename... Args>
	lf::handle<Tag> create(Args&&... args) {
		for (auto [index, current] : lf::enumerate(slots)) {
			if (current.resource->has_value()) {
				continue;
			}

			current.resource->emplace(lf::forward<Args>(args)...);
			return { static_cast<u32>(index + 1), current.generation };
		}

		slots.emplace_back();
		slot& current = slots.back();
		current.resource->emplace(lf::forward<Args>(args)...);
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
		const lf::size_t leaked_resources = live_resource_count();
		if (leaked_resources != 0) {
			lf::cerr
				<< "leaf backend warning: cleaning up "
				<< leaked_resources
				<< " leaked "
				<< lf::type_name<Tag>()
				<< " resource(s).\n";
		}

		clear();
	}

	lf::size_t live_resource_count() const {
		lf::size_t count = 0;
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
		lf::unique_ptr<lf::optional<Resource>> resource = lf::make_unique<lf::optional<Resource>>();
	};

	slot& require_slot(lf::view<Tag> resource_view) {
		if (resource_view.id == 0) {
			lf::abort();
		}

		const lf::size_t index = static_cast<lf::size_t>(resource_view.id - 1);
		if (index >= slots.size()) {
			lf::abort();
		}

		slot& current = slots[index];
		if (!current.resource->has_value() || current.generation != resource_view.generation_id) {
			lf::abort();
		}

		return current;
	}
	const slot& require_slot(lf::view<const Tag> resource_view) const {
		if (resource_view.id == 0) {
			lf::abort();
		}

		const lf::size_t index = static_cast<lf::size_t>(resource_view.id - 1);
		if (index >= slots.size()) {
			lf::abort();
		}

		const slot& current = slots[index];
		if (!current.resource->has_value() || current.generation != resource_view.generation_id) {
			lf::abort();
		}

		return current;
	}

	lf::vector<slot> slots;
};
