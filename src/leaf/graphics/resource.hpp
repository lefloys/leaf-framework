#ifndef LEAF_GRAPHICS_RESOURCE_HPP
#define LEAF_GRAPHICS_RESOURCE_HPP

#include <leaf/core/error.hpp>

#include <rt_ext_compute.h>
#include <rutile.h>

#include <type_traits>

namespace lf {

	struct buffer;
	struct texture;
	struct texture_view;
	struct graphics_program;
	struct compute_program;
	struct command_buffer;
	struct framebuffer;
	struct queue;
	using uniform_location = rt_uniform_location;
	using timepoint = rt_timepoint;
	struct window;

	template <typename Resource>
	struct resource_traits;

	namespace detail {

		template <typename NativeHandle>
		constexpr NativeHandle null_handle() {
			if constexpr (std::is_pointer_v<NativeHandle>) {
				return nullptr;
			} else {
				return static_cast<NativeHandle>(RT_NULL_HANDLE);
			}
		}

		template <typename NativeHandle>
		struct const_handle {
			using type = NativeHandle;
		};

		template <typename Value>
		struct const_handle<Value*> {
			using type = const Value*;
		};

		template <typename NativeHandle>
		using const_handle_t = typename const_handle<NativeHandle>::type;

	} // namespace detail

	template <typename Resource>
	struct handle {
		static_assert(!std::is_const_v<Resource>);
		using native_handle = resource_traits<Resource>::native_handle;
		native_handle value = detail::null_handle<native_handle>();

		explicit operator bool() const {
			return value != detail::null_handle<native_handle>();
		}

		operator native_handle() const {
			return value;
		}
	};

	template <typename Resource>
	struct view {
		using base_resource = std::remove_const_t<Resource>;
		using traits = resource_traits<base_resource>;
		using base_native_handle = traits::native_handle;
		using native_handle = std::conditional_t<std::is_const_v<Resource>, detail::const_handle_t<base_native_handle>, base_native_handle>;
		native_handle value = detail::null_handle<native_handle>();

		view() = default;
		view(const view&) = default;
		view& operator=(const view&) = default;
		view(handle<base_resource> handle) : value(handle.value) {}

		template <typename OtherResource>
		view(view<OtherResource> view) requires(std::is_const_v<Resource> && std::is_same_v<OtherResource, base_resource>) : value(view.value) {}

		explicit operator bool() const { return value != detail::null_handle<native_handle>(); }

		operator native_handle() const { return value; }
	};

	template <typename Resource>
	class unique {
	  public:
		unique() = default;
		explicit unique(handle<Resource> handle) : resource(handle) {}
		unique(const unique&) = delete;
		unique& operator=(const unique&) = delete;

		unique(unique&& other) noexcept : resource(other.release()) {}

		unique& operator=(unique&& other) noexcept {
			if (this != &other) {
				reset(other.release());
			}
			return *this;
		}

		~unique() {
			reset();
		}

		handle<Resource> get() const {
			return resource;
		}

		handle<Resource> release() {
			handle<Resource> released = resource;
			resource = {};
			return released;
		}

		void reset(handle<Resource> next = {}) {
			if (resource) {
				resource_traits<Resource>::destroy(resource.value);
			}
			resource = next;
		}

		explicit operator bool() const {
			return static_cast<bool>(resource);
		}

		operator handle<Resource>() const {
			return resource;
		}

		operator view<Resource>() const {
			return view<Resource>(resource);
		}

		operator view<const Resource>() const {
			return view<const Resource>(resource);
		}

	  private:
		handle<Resource> resource = {};
	};

	namespace detail {
		void check_rutile_error(string_view context);
	}

#define LEAF_RESOURCE_TRAITS(resource_name, native_name, destroy_func) \
	template <>                                                        \
	struct resource_traits<resource_name> {                            \
		using native_handle = native_name;                             \
		static void destroy(native_handle handle) {                    \
			destroy_func(handle);                                      \
		}                                                              \
	}

	LEAF_RESOURCE_TRAITS(buffer, rt_buffer, rtBufferDestroy);
	LEAF_RESOURCE_TRAITS(texture, rt_texture, rtTextureDestroy);
	LEAF_RESOURCE_TRAITS(texture_view, rt_texture_view, rtTextureViewDestroy);
	LEAF_RESOURCE_TRAITS(graphics_program, rt_graphics_program, rtGraphicsProgramDestroy);
	LEAF_RESOURCE_TRAITS(compute_program, rt_compute_program, rtComputeProgramDestroy);
	LEAF_RESOURCE_TRAITS(command_buffer, rt_command_buffer, rtCmdDestroy);

#undef LEAF_RESOURCE_TRAITS

	template <>
	struct resource_traits<framebuffer> {
		using native_handle = rt_framebuffer;
		static void destroy(native_handle handle) {
			rtFramebufferDestroy(handle);
		}
	};

	template <>
	struct resource_traits<queue> {
		using native_handle = rt_queue;
	};

} // namespace lf

#endif /* LEAF_GRAPHICS_RESOURCE_HPP */
