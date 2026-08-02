#pragma once

#include "leaf/core/string.hpp"
#include "leaf/core/unordered_map.hpp"
#include "leaf/core/vector.hpp"

#include <iterator>
#include <utility>

namespace lf {
	template<typename T>
	class indexed_map {
	  public:
		using value_type = T;
		using size_type = vector<value_type>::size_type;
		using iterator = typename vector<value_type>::iterator;
		using const_iterator = typename vector<value_type>::const_iterator;

		void clear() {
			values.clear();
			indices.clear();
		}

		void reserve(size_type count) {
			values.reserve(count);
			indices.reserve(count);
		}

		size_type size() const noexcept {
			return values.size();
		}

		bool empty() const noexcept {
			return values.empty();
		}

		const T& operator[](size_type index) const {
			return values[index];
		}

		T& operator[](size_type index) {
			return values[index];
		}

		const T& at(size_type index) const {
			return values.at(index);
		}

		T& at(size_type index) {
			return values.at(index);
		}

		iterator begin() noexcept {
			return values.begin();
		}

		const_iterator begin() const noexcept {
			return values.begin();
		}

		const_iterator cbegin() const noexcept {
			return values.cbegin();
		}

		iterator end() noexcept {
			return values.end();
		}

		const_iterator end() const noexcept {
			return values.end();
		}

		const_iterator cend() const noexcept {
			return values.cend();
		}

		iterator find(const T& value) {
			const auto index = indices.find(value);
			if (index == indices.end()) {
				return end();
			}
			return begin() + static_cast<std::ptrdiff_t>(index->second);
		}

		const_iterator find(const T& value) const {
			const auto index = indices.find(value);
			if (index == indices.end()) {
				return end();
			}
			return begin() + static_cast<std::ptrdiff_t>(index->second);
		}

		size_type index_of(const_iterator it) const {
			if (it == end()) {
				return size();
			}
			return static_cast<size_type>(std::distance(cbegin(), it));
		}

		std::pair<iterator, bool> insert(const T& value) {
			const size_type index = values.size();
			const auto [map_it, inserted] = indices.emplace(value, index);
			if (!inserted) {
				return { begin() + static_cast<std::ptrdiff_t>(map_it->second), false };
			}
			try {
				values.push_back(value);
			} catch (...) {
				indices.erase(map_it);
				throw;
			}
			return { begin() + static_cast<std::ptrdiff_t>(index), true };
		}

		std::pair<iterator, bool> insert(T&& value) {
			const size_type index = values.size();
			const auto [map_it, inserted] = indices.emplace(value, index);
			if (!inserted) {
				return { begin() + static_cast<std::ptrdiff_t>(map_it->second), false };
			}
			try {
				values.push_back(std::move(value));
			} catch (...) {
				indices.erase(map_it);
				throw;
			}
			return { begin() + static_cast<std::ptrdiff_t>(index), true };
		}

	  private:
		vector<value_type> values;
		unordered_map<value_type, size_type> indices;
	};
} // namespace lf
