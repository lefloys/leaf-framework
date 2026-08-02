#pragma once

#include <cassert>
#include <memory>

namespace lf {
	using std::make_unique;
	using std::unique_ptr;

	template <typename T>
	struct view_ptr {
		view_ptr() = default;
		view_ptr(const unique_ptr<T>& uptr) noexcept;
		view_ptr(T* ptr) noexcept;

		T* operator->() const noexcept;
		T& operator*() const noexcept;

		T* ptr = nullptr;
	};

	template <typename T>
	view_ptr<T>::view_ptr(const unique_ptr<T>& uptr) noexcept : ptr(uptr.get()) {}
	template <typename T>
	view_ptr<T>::view_ptr(T* ptr) noexcept : ptr(ptr) {}

	template <typename T>
	T* view_ptr<T>::operator->() const noexcept {
		assert(ptr && "Attempting to access member of null view_ptr");
		return ptr;
	}

	template <typename T>
	T& view_ptr<T>::operator*() const noexcept {
		assert(ptr && "Attempting to dereference null view_ptr");
		return *ptr;
	}
} // namespace lf