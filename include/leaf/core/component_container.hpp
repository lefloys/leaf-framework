#pragma once

#include "leaf/core/array.hpp"
#include "leaf/core/identifier.hpp"
#include "leaf/core/optional.hpp"
#include "leaf/core/types.hpp"
#include "leaf/core/vector.hpp"

#include <concepts>
#include <tuple>
#include <utility>

namespace lf {
	template<instantiation_of<identifier> Handle, typename... Component>
	class component_container {
	  public:
		using handle = Handle;

		struct bundle {
			template<typename T>
			optional<T>& component() {
				return std::get<optional<T>>(values);
			}

			template<typename T>
			const optional<T>& component() const {
				return std::get<optional<T>>(values);
			}

		  private:
			std::tuple<optional<Component>...> values;
			friend component_container;
		};

		struct slot {
			array<size_t, sizeof...(Component)> components{};
		};

		component_container() : slots(1) {
			(std::get<component_index<Component>>(components).first.emplace_back(), ...);
			(std::get<component_index<Component>>(components).second.emplace_back(), ...);
		}

		Handle create(bundle bundle = {}) {
			size_t index;
			if (free.empty()) {
				index = slots.size();
				slots.emplace_back();
			} else {
				index = free.back();
				free.pop_back();
			}
			slots[index] = {};
			const Handle handle{ static_cast<typename Handle::vnum_t>(index) };
			([&] {
				auto& component = std::get<optional<Component>>(bundle.values);
				if (component) {
					add(handle, std::move(*component));
				}
			}(),
			 ...);
			return handle;
		}

		void destroy(Handle value) {
			(erase<Component>(value), ...);
			slots[value.get()] = {};
			free.push_back(value.get());
		}

		void clear() {
			*this = component_container();
		}

		template<typename T>
		bool has(Handle value) const {
			return slots[value.get()].components[component_index<T>] != 0;
		}

		template<typename T>
		T& add(Handle value, T component = {}) {
			auto& [values, owners] = std::get<component_index<T>>(components);
			size_t& index = slots[value.get()].components[component_index<T>];
			if (index != 0) {
				return values[index];
			}
			index = values.size();
			owners.push_back(value.get());
			return values.emplace_back(std::move(component));
		}

		template<typename T>
		auto find(Handle value) {
			auto& values = std::get<component_index<T>>(components).first;
			const size_t index = slots[value.get()].components[component_index<T>];
			return index == 0 ? values.end() : values.begin() + index;
		}

		template<typename T>
		auto find(Handle value) const {
			const auto& values = std::get<component_index<T>>(components).first;
			const size_t index = slots[value.get()].components[component_index<T>];
			return index == 0 ? values.end() : values.begin() + index;
		}

		template<typename T>
		auto end() {
			return std::get<component_index<T>>(components).first.end();
		}

		template<typename T>
		auto end() const {
			return std::get<component_index<T>>(components).first.end();
		}

		template<typename T, typename F>
		void each(F&& function) {
			auto& [values, owners] = std::get<component_index<T>>(components);
			for (size_t index = 1; index < values.size(); ++index) {
				function(Handle(static_cast<typename Handle::vnum_t>(owners[index])), values[index]);
			}
		}

		template<typename T, typename F>
		void each(F&& function) const {
			const auto& [values, owners] = std::get<component_index<T>>(components);
			for (size_t index = 1; index < values.size(); ++index) {
				function(Handle(static_cast<typename Handle::vnum_t>(owners[index])), values[index]);
			}
		}

		template<typename T>
		void erase(Handle value) {
			size_t index = slots[value.get()].components[component_index<T>];
			if (index == 0) {
				return;
			}

			auto& [values, owners] = std::get<component_index<T>>(components);
			size_t last = values.size() - 1;
			size_t dense = index;
			if (dense != last) {
				values[dense] = std::move(values[last]);
				owners[dense] = owners[last];
				slots[owners[dense]].components[component_index<T>] = index;
			}
			values.pop_back();
			owners.pop_back();
			slots[value.get()].components[component_index<T>] = 0;
		}

	  private:
		template<typename T>
		static constexpr size_t component_index = [] {
			constexpr array matches{ std::same_as<T, Component>... };
			for (size_t index = 0; index < matches.size(); ++index) {
				if (matches[index]) {
					return index;
				}
			}
			return matches.size();
		}();

		vector<slot> slots;
		vector<size_t> free;
		std::tuple<std::pair<vector<Component>, vector<size_t>>...> components;
	};

} // namespace lf
