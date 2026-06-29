#include "window.hpp"
#include "window_private.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/profiler.hpp"
#include "leaf/graphics/queue.hpp"
#include "leaf/core/logging.hpp"

#include <memory>
#include <utility>

namespace lf {
	namespace detail {
		static constexpr dim2<u32> default_window_size = { 1280, 720 };
		static constexpr string_view default_window_title = "leaf-framework";
	} // namespace detail

	size_t window_t::control_index(input_control control) {
		switch (control.type) {
		case INPUT_CONTROL_KEY:
			return control.value < KEY_ENUM_MAX ? control.value : control_count;
		case INPUT_CONTROL_BUTTON:
			return control.value < BUTTON_ENUM_MAX ? KEY_ENUM_MAX + control.value : control_count;
		default:
			return control_count;
		}
	}

	void window_t::control(input_control control, bool down, input_modifiers modifiers) {
		size_t index = control_index(control);
		if (index >= controls.size()) {
			return;
		}

		std::lock_guard guard(input_mutex);
		this->modifiers = modifiers;
		input_state& state = controls[index];
		if (down && (state == input_state::Up || state == input_state::Released)) {
			state = input_state::Pressed;
			events.push_back({
				.type = INPUT_EVENT_CONTROL,
				.control = control,
				.state = state,
				.modifiers = modifiers,
				.position = pointer_position,
			});
		} else if (!down && (state == input_state::Down || state == input_state::Pressed)) {
			if (state == input_state::Pressed) {
				deferred_releases[index] = true;
				return;
			}
			state = input_state::Released;
			events.push_back({
				.type = INPUT_EVENT_CONTROL,
				.control = control,
				.state = state,
				.modifiers = modifiers,
				.position = pointer_position,
			});
		}
	}

	void window_t::text(u32 character) {
		std::lock_guard guard(input_mutex);
		events.push_back({
			.type = INPUT_EVENT_TEXT,
			.modifiers = modifiers,
			.position = pointer_position,
			.character = character,
		});
	}

	void window_t::pointer(pos2<f32> position) {
		std::lock_guard guard(input_mutex);
		pointer_position = position;
		events.push_back({
			.type = INPUT_EVENT_POINTER_MOVE,
			.modifiers = modifiers,
			.position = pointer_position,
		});
	}

	void window_t::pointer_enter(bool entered) {
		std::lock_guard guard(input_mutex);
		pointer_inside = entered;
		events.push_back({
			.type = INPUT_EVENT_POINTER_ENTER,
			.state = entered ? input_state::Down : input_state::Up,
			.modifiers = modifiers,
			.position = pointer_position,
		});
		if (!entered) {
			for (size_t index = 0; index < controls.size(); ++index) {
				if (index < KEY_ENUM_MAX) {
					continue;
				}
				input_state& state = controls[index];
				if (state == input_state::Down || state == input_state::Pressed) {
					state = input_state::Released;
					deferred_releases[index] = false;
					events.push_back({
						.type = INPUT_EVENT_CONTROL,
						.control = { INPUT_CONTROL_BUTTON, static_cast<u16>(index - KEY_ENUM_MAX) },
						.state = state,
						.modifiers = modifiers,
						.position = pointer_position,
					});
				}
			}
		}
	}

	void window_t::scroll(pos2<f32> delta) {
		std::lock_guard guard(input_mutex);
		events.push_back({
			.type = INPUT_EVENT_SCROLL,
			.modifiers = modifiers,
			.position = pointer_position,
			.delta = delta,
		});
	}

	void window_t::focus(bool focused) {
		std::lock_guard guard(input_mutex);
		events.push_back({
			.type = INPUT_EVENT_FOCUS,
			.state = focused ? input_state::Down : input_state::Up,
			.modifiers = modifiers,
			.position = pointer_position,
		});
		if (!focused) {
			// Synthesize release events for every held key/button so listeners
			// that track "is X held" via keydown/keyup events don't see a
			// permanent down-state when the user alt-tabs mid-press. Without
			// this, controls.fill(Up) below silently zeroes the internal state
			// while every script-side "move_down=true" flag stays stuck.
			for (size_t index = 0; index < controls.size(); ++index) {
				input_state& state = controls[index];
				if (state != input_state::Down && state != input_state::Pressed) {
					continue;
				}
				state = input_state::Released;
				deferred_releases[index] = false;
				events.push_back({
					.type = INPUT_EVENT_CONTROL,
					.control = index < KEY_ENUM_MAX
						? input_control{ INPUT_CONTROL_KEY, static_cast<u16>(index) }
						: input_control{ INPUT_CONTROL_BUTTON, static_cast<u16>(index - KEY_ENUM_MAX) },
					.state = state,
					.modifiers = modifiers,
					.position = pointer_position,
				});
			}
			controls.fill(input_state::Up);
			deferred_releases.fill(false);
			modifiers = {};
		}
	}

	void window_t::drop(string path) {
		std::lock_guard guard(input_mutex);
		events.push_back({
			.type = INPUT_EVENT_DROP,
			.modifiers = modifiers,
			.position = pointer_position,
			.text = std::move(path),
		});
	}

	void window_t::resize(dim2<u32> new_size) {
		if (new_size.width == 0 || new_size.height == 0) {
			return;
		}
		std::lock_guard guard(input_mutex);
		size = new_size;
		pending_resize = new_size;
	}

	window_t::~window_t() {
		if (platform) {
			platform_window_cursor(platform, nullptr);
			platform_window_clear_owner(platform);
		}
		current_cursor.clear();
		if (swapchain) {
			rtSwapchainDestroy(swapchain);
			swapchain = nullptr;
		}
		if (platform) {
			destroy_platform_window(platform);
			platform = nullptr;
		}
	}

	void resource_traits<window>::destroy(native_handle handle) {
		Window::Destroy({ handle });
	}

	handle<window> Window::Create() {
		log::Debug("Creating window title='{}' size={}x{}", detail::default_window_title, detail::default_window_size.width, detail::default_window_size.height);
		std::unique_ptr<window_t> window = std::make_unique<window_t>();
		window->swapchain = rtSwapchainCreate();
		detail::check_rutile_error("failed to create swapchain");
		log::Debug("Created swapchain {}", static_cast<const void*>(window->swapchain));

		window->frame_command_buffer = unique<command_buffer>(CommandBuffer::Create());
		window->platform = create_platform_window({
			detail::default_window_title,
			detail::default_window_size.width,
			detail::default_window_size.height,
		});
		if (!window->platform) {
			throw runtime_exception("failed to create platform window");
		}
		window->size = detail::default_window_size;
		window->windowed_position = platform_window_position(window->platform);
		window->windowed_size = detail::default_window_size;
		platform_window_owner(window->platform, window.get());

		bind_platform_window_swapchain(window->platform, window->swapchain);
		log::Debug("Window created platform={} swapchain={} position={}x{} size={}x{}",
				  static_cast<const void*>(window->platform),
				  static_cast<const void*>(window->swapchain),
				  window->windowed_position.x,
				  window->windowed_position.y,
				  window->windowed_size.width,
				  window->windowed_size.height);
		return { window.release() };
	}

	void Window::Destroy(handle<window> window) {
		log::Debug("Destroying window platform={} swapchain={}",
				  static_cast<const void*>(window.value->platform),
				  static_cast<const void*>(window.value->swapchain));
		window.value->current_framebuffer = {};
		window.value->current_queue = {};
		delete window.value;
	}

	void Window::SetTitle(view<window> window, string_view title) {
		log::Debug("Window title set to '{}'", title);
		platform_window_title(window.value->platform, title);
	}

	void Window::Show(view<window> window) {
		if (!window) {
			return;
		}
		dim2<u32> size = Size(window);
		log::Debug("Showing window size={}x{} fullscreen={}", size.width, size.height, window.value->fullscreen ? "true" : "false");
		platform_window_show(window.value->platform);
	}

	void Window::SetWidth(view<window> window, u32 width) {
		dim2<u32> size = Size(window);
		size.width = width;
		if (!window.value->fullscreen) {
			window.value->windowed_size = size;
		}
		window.value->resize(size);
		platform_window_size(window.value->platform, size);
		log::Debug("Window width set to {}; size={}x{}", width, size.width, size.height);
	}

	void Window::SetHeight(view<window> window, u32 height) {
		dim2<u32> size = Size(window);
		if (size.width == 0) {
			size.width = detail::default_window_size.width;
		}
		size.height = height;
		if (!window.value->fullscreen) {
			window.value->windowed_size = size;
		}
		window.value->resize(size);
		platform_window_size(window.value->platform, size);
		log::Debug("Window height set to {}; size={}x{}", height, size.width, size.height);
	}

	void Window::SetFullscreen(view<window> window, bool fullscreen) {
		if (window.value->fullscreen == fullscreen) {
			return;
		}
		dim2<u32> before = platform_window_size(window.value->platform);
		log::Debug("Changing fullscreen {} -> {} from size={}x{}",
				  window.value->fullscreen ? "true" : "false",
				  fullscreen ? "true" : "false",
				  before.width,
				  before.height);
		if (fullscreen) {
			window.value->windowed_position = platform_window_position(window.value->platform);
			window.value->windowed_size = platform_window_size(window.value->platform);
		}
		platform_window_fullscreen(
			window.value->platform,
			fullscreen,
			window.value->windowed_position,
			window.value->windowed_size);
		window.value->resize(platform_window_size(window.value->platform));
		window.value->fullscreen = fullscreen;
		dim2<u32> after = Window::Size(window);
		log::Debug("Fullscreen change applied; size={}x{}", after.width, after.height);
	}

	void Window::SetVsync(view<window> window, bool enabled) {
		if (!window || window.value->vsync == enabled) {
			return;
		}
		window.value->vsync = enabled;
		log::Warning("Swapchain vsync control is not supported by this Rutile version");
	}

	void Window::RequestFullscreen(view<window> window, bool fullscreen) {
		std::lock_guard lock(window.value->input_mutex);
		window.value->requested_fullscreen = fullscreen;
		window.value->fullscreen_change_requested = true;
		log::Debug("Fullscreen request queued: {}", fullscreen ? "true" : "false");
	}

	bool Window::FullscreenRequestPending(view<window> window) {
		std::lock_guard lock(window.value->input_mutex);
		return window.value->fullscreen_change_requested;
	}

	bool Window::ApplyFullscreenRequest(view<window> window) {
		bool requested = false;
		{
			std::lock_guard lock(window.value->input_mutex);
			if (!window.value->fullscreen_change_requested) {
				return false;
			}
			requested = window.value->requested_fullscreen;
			window.value->fullscreen_change_requested = false;
		}
		SetFullscreen(window, requested);
		return true;
	}

	bool Window::Fullscreen(view<const window> window) {
		return window.value->fullscreen;
	}

	void Window::ApplyCursor(view<window> window, string_view name, PlatformCursor* cursor) {
		if (window.value->current_cursor == name) {
			return;
		}
		platform_window_cursor(window.value->platform, cursor);
		window.value->current_cursor.assign(name);
	}

	bool Window::Drawable(view<const window> window) {
		return platform_window_drawable(window.value->platform);
	}

	bool Window::ShouldClose(view<const window> window) {
		return platform_window_should_close(window.value->platform);
	}

	void Window::SetShouldClose(view<window> window, bool should_close) {
		platform_window_should_close(window.value->platform, should_close);
	}

	dim2<u32> Window::Size(view<const window> window) {
		std::lock_guard lock(window.value->input_mutex);
		return window.value->size;
	}

	std::vector<input_event> Window::InputEvents(view<window> window) {
		std::vector<input_event> events;
		std::lock_guard lock(window.value->input_mutex);
		events.swap(window.value->events);
		return events;
	}

	input_state Window::InputState(view<const window> window, input_control control) {
		size_t index = window_t::control_index(control);
		if (index >= window.value->controls.size()) {
			return input_state::Up;
		}

		std::lock_guard lock(window.value->input_mutex);
		return window.value->controls[index];
	}

	void Window::UpdateInput(view<window> window) {
		std::lock_guard lock(window.value->input_mutex);
		for (size_t index = 0; index < window.value->controls.size(); ++index) {
			input_state& state = window.value->controls[index];
			if (state == input_state::Pressed) {
				if (window.value->deferred_releases[index]) {
					state = input_state::Released;
					window.value->events.push_back({
						.type = INPUT_EVENT_CONTROL,
						.control = index < KEY_ENUM_MAX
							? input_control{ INPUT_CONTROL_KEY, static_cast<u16>(index) }
							: input_control{ INPUT_CONTROL_BUTTON, static_cast<u16>(index - KEY_ENUM_MAX) },
						.state = state,
						.modifiers = window.value->modifiers,
						.position = window.value->pointer_position,
					});
					window.value->deferred_releases[index] = false;
				} else {
					state = input_state::Down;
				}
			} else if (state == input_state::Released) {
				state = input_state::Up;
			}
		}
	}

	bool Window::MouseDown(view<const window> window, input_button button) {
		input_state state = InputState(window, { INPUT_CONTROL_BUTTON, static_cast<u16>(button) });
		return state == input_state::Down || state == input_state::Pressed;
	}

	bool Window::MousePressed(view<const window> window, input_button button) {
		return InputState(window, { INPUT_CONTROL_BUTTON, static_cast<u16>(button) }) == input_state::Pressed;
	}

	bool Window::MouseReleased(view<const window> window, input_button button) {
		return InputState(window, { INPUT_CONTROL_BUTTON, static_cast<u16>(button) }) == input_state::Released;
	}

	bool Window::KeyDown(view<const window> window, input_key key) {
		input_state state = InputState(window, { INPUT_CONTROL_KEY, static_cast<u16>(key) });
		return state == input_state::Down || state == input_state::Pressed;
	}

	bool Window::KeyPressed(view<const window> window, input_key key) {
		return InputState(window, { INPUT_CONTROL_KEY, static_cast<u16>(key) }) == input_state::Pressed;
	}

	bool Window::KeyReleased(view<const window> window, input_key key) {
		return InputState(window, { INPUT_CONTROL_KEY, static_cast<u16>(key) }) == input_state::Released;
	}

	view<framebuffer> Window::CurrentFramebuffer(view<window> window) {
		return window.value->current_framebuffer;
	}

	view<const framebuffer> Window::CurrentFramebuffer(view<const window> window) {
		return window.value->current_framebuffer;
	}

	view<command_buffer> Window::BeginFrame(view<window> window, view<queue> queue) {
		LF_PROFILE_SCOPE("Window::BeginFrame");
		dim2<u32> resize = {};
		{
			std::lock_guard lock(window.value->input_mutex);
			resize = window.value->pending_resize;
			window.value->pending_resize = {};
		}
		if (resize.width != 0 && resize.height != 0) {
			LF_PROFILE_SCOPE("Window::ResizeSwapchain");
			log::Debug("Resizing swapchain to {}x{}", resize.width, resize.height);
			rtSwapchainResize(window.value->swapchain, resize.width, resize.height);
			detail::check_rutile_error("failed to resize swapchain");
		}

		rt_swapchain_acquire_result acquired = {};
		auto queue_lock = detail::lock_queue(queue);
		{
			LF_PROFILE_SCOPE("Window::AcquireSwapchain");
			acquired = rtSwapchainAcquire(window.value->swapchain);
		}
		detail::check_rutile_error("failed to acquire swapchain framebuffer");
		window.value->current_framebuffer.value = acquired.framebuffer;
		if (!window.value->current_framebuffer) {
			return {};
		}
		window.value->current_queue = queue;

		timepoint ready = acquired.timepoint;
		{
			LF_PROFILE_SCOPE("Window::WaitForAcquire");
			rtQueueWait(queue, ready);
			detail::check_rutile_error("failed to wait for swapchain acquire");
		}
		{
			LF_PROFILE_SCOPE("Window::BeginCommandBuffer");
			CommandBuffer::Begin(window.value->frame_command_buffer, queue);
			CommandBuffer::BeginRendering(window.value->frame_command_buffer,
										  window.value->current_framebuffer);
		}
		return window.value->frame_command_buffer;
	}

	void Window::EndFrame(view<window> window) {
		LF_PROFILE_SCOPE("Window::EndFrame");
		timepoint rendered;
		auto queue_lock = detail::lock_queue(window.value->current_queue);
		{
			LF_PROFILE_SCOPE("Window::SubmitFrame");
			CommandBuffer::EndRendering(window.value->frame_command_buffer);
			CommandBuffer::End(window.value->frame_command_buffer);
			rendered = rtQueueSubmit(window.value->current_queue, window.value->frame_command_buffer.get().value);
			detail::check_rutile_error("failed to submit swapchain frame");
		}
		{
			LF_PROFILE_SCOPE("Window::PresentSwapchain");
			rtSwapchainPresent(window.value->swapchain, rendered);
		}
		detail::check_rutile_error("failed to present swapchain");
		window.value->current_framebuffer = {};
		window.value->current_queue = {};
	}

} // namespace lf
