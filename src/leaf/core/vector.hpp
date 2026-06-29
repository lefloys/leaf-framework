#pragma once

#include "exception.hpp"
#include "format.hpp"

#include <vector>

#define IDX_FROM_PTR(array_start_ptr, object_ptr) \
	static_cast<std::size_t>((object_ptr) - (array_start_ptr))

namespace lf {
	template <typename T, typename Alloc = std::allocator<T>>
	class vector : public std::vector<T, Alloc> {
	  public:
		using std::vector<T, Alloc>::vector;
		using std::vector<T, Alloc>::operator=;
		using size_type = typename std::vector<T, Alloc>::size_type;

		T& at(size_type index) {
			if (index >= this->size()) {
				throw out_of_range_exception(
					lf::format("index {} out of range (size is {})", index, this->size()));
			}
			return (*this)[index];
		}
		const T& at(size_type index) const {
			if (index >= this->size()) {
				throw out_of_range_exception(
					lf::format("index {} out of range (size is {})", index, this->size()));
			}
			return (*this)[index];
		}
	};
} // namespace lf
