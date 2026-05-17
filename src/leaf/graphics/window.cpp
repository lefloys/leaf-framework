#include "window.hpp"

#include <leaf/core/exception.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/window/backend.hpp>

#include <rt_ext_swapchain.h>

#include <memory>

namespace lf {

	struct window_t {
		PlatformWindow* platform = nullptr;
		rt_swapchain swapchain = RT_NULL_HANDLE;
		handle<command_buffer> command_buffer;
		handle<framebuffer> current_framebuffer;
		view<queue> current_queue;
	};

	namespace detail {

		static constexpr dim2<u32> default_window_size = { 1280, 720 };
		static constexpr string_view default_window_title = "leaf-framework";

		void resize_swapchain_to_window(window_t* window) {
			dim2<u32> framebuffer_size = platform_window_framebuffer_size(window->platform);
			if (framebuffer_size.width == 0 || framebuffer_size.height == 0) {
				return;
			}

			rtSwapchainResize(window->swapchain, framebuffer_size.width, framebuffer_size.height);
			check_rutile_error("failed to resize swapchain");
		}

	} // namespace detail

	void resource_traits<window>::destroy(native_handle handle) {
		Window::Destroy({ handle });
	}

	handle<window> Window::Create() {
		std::unique_ptr<window_t> window = std::make_unique<window_t>();
		try {
			window->swapchain = rtSwapchainCreate();
			detail::check_rutile_error("failed to create swapchain");
			if (!window->swapchain) {
				throw runtime_exception("failed to create swapchain");
			}

			window->command_buffer = CommandBuffer::Create();
			window->platform = create_platform_window({
				detail::default_window_title,
				detail::default_window_size.width,
				detail::default_window_size.height,
			});

			bind_platform_window_swapchain(window->platform, window->swapchain);
			detail::resize_swapchain_to_window(window.get());
			return { window.release() };
		} catch (...) {
			CommandBuffer::Destroy(window->command_buffer);
			rtSwapchainDestroy(window->swapchain);
			destroy_platform_window(window->platform);
			throw;
		}
	}

	void Window::Destroy(handle<window> window) {
		if (!window.value) {
			return;
		}

		CommandBuffer::Destroy(window.value->command_buffer);
		rtSwapchainDestroy(window.value->swapchain);
		window.value->current_framebuffer = {};
		window.value->current_queue = {};
		destroy_platform_window(window.value->platform);
		delete window.value;
	}

	void Window::SetTitle(view<window> window, string_view title) {
		if (window.value) {
			platform_window_title(window.value->platform, title);
		}
	}

	void Window::SetWidth(view<window> window, u32 width) {
		if (!window.value) {
			return;
		}

		dim2<u32> size = platform_window_size(window.value->platform);
		if (size.height == 0) {
			size.height = detail::default_window_size.height;
		}
		size.width = width;
		platform_window_size(window.value->platform, size);
	}

	void Window::SetHeight(view<window> window, u32 height) {
		if (!window.value) {
			return;
		}

		dim2<u32> size = platform_window_size(window.value->platform);
		if (size.width == 0) {
			size.width = detail::default_window_size.width;
		}
		size.height = height;
		platform_window_size(window.value->platform, size);
	}

	bool Window::ShouldClose(view<const window> window) {
		return window.value && platform_window_should_close(window.value->platform);
	}

	void Window::SetShouldClose(view<window> window, bool should_close) {
		if (window.value) {
			platform_window_should_close(window.value->platform, should_close);
		}
	}

	bool Window::KeyDown(view<const window> window, Key key) {
		return window.value && platform_window_key_down(window.value->platform, key);
	}

	bool Window::KeyPressed(view<const window> window, Key key) {
		return window.value && platform_window_key_pressed(window.value->platform, key);
	}

	bool Window::KeyReleased(view<const window> window, Key key) {
		return window.value && platform_window_key_released(window.value->platform, key);
	}

	bool Window::MouseDown(view<const window> window, MouseButton button) {
		return window.value && platform_window_mouse_down(window.value->platform, button);
	}

	bool Window::MousePressed(view<const window> window, MouseButton button) {
		return window.value && platform_window_mouse_pressed(window.value->platform, button);
	}

	bool Window::MouseReleased(view<const window> window, MouseButton button) {
		return window.value && platform_window_mouse_released(window.value->platform, button);
	}

	pos2<f32> Window::MousePosition(view<const window> window) {
		if (!window.value) {
			return {};
		}
		return platform_window_mouse_position(window.value->platform);
	}

	f32 Window::Scroll(view<const window> window) {
		if (!window.value) {
			return 0.0f;
		}
		return platform_window_scroll(window.value->platform);
	}

	f32 Window::ConsumeScroll(view<window> window) {
		if (!window.value) {
			return 0.0f;
		}
		return platform_window_consume_scroll(window.value->platform);
	}

	dim2<u32> Window::FramebufferSize(view<const window> window) {
		if (!window.value) {
			return {};
		}
		return platform_window_framebuffer_size(window.value->platform);
	}

	handle<framebuffer> Window::CurrentFramebuffer(view<const window> window) {
		if (!window.value) {
			return {};
		}
		return window.value->current_framebuffer;
	}

	view<command_buffer> Window::BeginFrame(view<window> window, view<queue> queue) {
		if (!window.value) {
			return {};
		}

		detail::resize_swapchain_to_window(window.value);
		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(window.value->swapchain);
		detail::check_rutile_error("failed to acquire swapchain framebuffer");
		window.value->current_framebuffer = { acquired.framebuffer };
		if (!window.value->current_framebuffer) {
			return {};
		}
		window.value->current_queue = queue;

		timepoint ready = acquired.timepoint;
		Queue::Wait(queue, ready);
		CommandBuffer::Begin(window.value->command_buffer, queue);
		CommandBuffer::BeginRendering(window.value->command_buffer,
									  window.value->current_framebuffer);
		return window.value->command_buffer;
	}

	void Window::EndFrame(view<window> window) {
		if (!window.value) {
			return;
		}

		CommandBuffer::EndRendering(window.value->command_buffer);
		CommandBuffer::End(window.value->command_buffer);
		timepoint rendered =
			Queue::Submit(window.value->current_queue, window.value->command_buffer);
		rtSwapchainPresent(window.value->swapchain, rendered);
		detail::check_rutile_error("failed to present swapchain");
		window.value->current_framebuffer = {};
		window.value->current_queue = {};
	}

} // namespace lf
