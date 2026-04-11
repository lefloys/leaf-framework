#pragma once

#include <leaf/graphics/forward.hpp>

#include <vector>
#include <cassert>
#include <memory>
#include <optional>
#include <utility>

namespace lf::vk {

    template<typename T>
    class ResourceManager {
    public:
        ResourceManager() {
            resources.emplace_back();
            resources[0].resource = std::make_unique<std::optional<private_t>>();
        }

        using public_t = typename T::front_t;
        using private_t = T;

        template<typename... Args>
        handle<public_t> create(Args&&... args);

        void destroy(handle<public_t> obj);   // optional: just validation hook
        void recycle(u32 id);                 // immediate destruction

        private_t& get(view<public_t> obj);
        const private_t& get(view<const public_t> obj) const;

    private:
        u32 allocate_id();
        void release_id(u32 id);

    private:
        struct resource_entry {
            std::unique_ptr<std::optional<private_t>> resource;
            u32 generation = 0;
        };

        std::vector<resource_entry> resources;
        std::vector<u32> free_ids;
    };

    template<typename T>
    u32 ResourceManager<T>::allocate_id() {
        if (!free_ids.empty()) {
            u32 id = free_ids.back();
            free_ids.pop_back();

            if (!resources[id].resource) {
                resources[id].resource = std::make_unique<std::optional<private_t>>();
            }

            return id;
        }

        resources.emplace_back();
        resources.back().resource = std::make_unique<std::optional<private_t>>();

        return static_cast<u32>(resources.size() - 1);
    }

    template<typename T>
    void ResourceManager<T>::release_id(u32 id) {
        assert(id > 0 && id < resources.size());
        assert(resources[id].resource);

        resources[id].resource->reset();
        resources[id].generation++;

        free_ids.push_back(id);
    }

    template<typename T>
    template<typename... Args>
    handle<typename ResourceManager<T>::public_t>
        ResourceManager<T>::create(Args&&... args) {
        u32 id = allocate_id();

        auto& entry = resources[id];
        entry.resource->emplace(std::forward<Args>(args)...);

        return handle<public_t>{ id, entry.generation };
    }

    template<typename T>
    void ResourceManager<T>::destroy(handle<public_t> obj) {
        // optional validation only (no deferred system anymore)
        auto& entry = resources[obj.id];

        assert(entry.resource);
        assert(entry.generation == obj.generation);
    }

    template<typename T>
    void ResourceManager<T>::recycle(u32 id) {
        assert(id > 0 && id < resources.size());
        release_id(id);
    }

    template<typename T>
    typename ResourceManager<T>::private_t&
        ResourceManager<T>::get(view<public_t> obj) {
        auto& entry = resources[obj.id];

        assert(entry.resource);
        assert(entry.resource->has_value());
        assert(entry.generation == obj.generation);

        return **entry.resource;
    }

    template<typename T>
    const typename ResourceManager<T>::private_t&
        ResourceManager<T>::get(view<const public_t> obj) const {
		const auto& entry = resources[obj.id]; // Can technically reference an invalid entry, but the assertions will catch it

        assert(entry.resource);
        assert(entry.resource->has_value());
        assert(entry.generation == obj.generation);

        return **entry.resource;
    }
}
