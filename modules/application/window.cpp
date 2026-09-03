#include "leaf/application/window.hpp"

#include "leaf/core/exception.hpp"
#include "leaf/core/logging.hpp"
#include "leaf/graphics/command_buffer.hpp"
#include "leaf/graphics/queue.hpp"
#include "leaf/graphics/timepoint.hpp"
#include "leaf/platform/platform.hpp"

#include <utility>

namespace lf {
	size_t Window::control_index(rt::input_control control) {
		if (control.type == rt::INPUT_CONTROL_KEY) return control.value < rt::KEY_ENUM_MAX ? control.value : control_count;
		if (control.type == rt::INPUT_CONTROL_BUTTON) return control.value < rt::BUTTON_ENUM_MAX ? rt::KEY_ENUM_MAX + control.value : control_count;
		return control_count;
	}

	Window::Window(string_view title, dim2<u32> size) : window_size(size), windowed_size(size) {
		platform = rt::create_platform_window({ title, size.width, size.height });
		if (!platform) throw runtime_exception("failed to create platform window");
		try {
			swapchain = rtSwapchainCreate();
			rt::detail::check_rutile_error("failed to create swapchain");
			rt::bind_platform_window_swapchain(platform, swapchain);
			queue.reset(rt::Queue::Create(rt::QueueCapability::Graphics));
			frame_command_buffer.reset(rt::Cmd::Create());
			windowed_position = rt::platform_window_position(platform);
			rt::platform_window_owner(platform, this);
			on_resize(rt::platform_window_size(platform), rt::platform_framebuffer_size(platform));
		} catch (...) {
			if (swapchain) rtSwapchainDestroy(swapchain);
			rt::destroy_platform_window(platform);
			platform = nullptr;
			throw;
		}
	}

	Window::~Window() {
		if (!platform) return;
		if (queue) rt::Timepoint::Wait(rt::Queue::Flush(queue));
		frame_command_buffer.reset();
		queue.reset();
		rt::platform_window_cursor(platform, nullptr);
		rt::platform_window_clear_owner(platform);
		if (swapchain) rtSwapchainDestroy(swapchain);
		rt::destroy_platform_window(platform);
	}

	void Window::set_title(string_view title) { rt::platform_window_title(platform, title); }
	void Window::show() { rt::platform_window_show(platform); }
	void Window::set_size(dim2<u32> size) { if (!is_fullscreen) windowed_size = size; rt::platform_window_size(platform, size); on_resize(size, rt::platform_framebuffer_size(platform)); }
	void Window::set_fullscreen(bool enabled) {
		if (is_fullscreen == enabled) return;
		if (enabled) { windowed_position = rt::platform_window_position(platform); windowed_size = rt::platform_window_size(platform); }
		rt::platform_window_fullscreen(platform, enabled, windowed_position, windowed_size);
		is_fullscreen = enabled;
		on_resize(rt::platform_window_size(platform), rt::platform_framebuffer_size(platform));
	}
	bool Window::fullscreen() const { return is_fullscreen; }
	void Window::set_vsync(bool enabled) { vsync = enabled; log::Warning("Swapchain vsync control is not supported by this Rutile version"); }
	void Window::set_cursor(string_view name, rt::PlatformCursor* cursor) { if (current_cursor == name) return; rt::platform_window_cursor(platform, cursor); current_cursor.assign(name); }
	bool Window::drawable() const { return rt::platform_window_drawable(platform); }
	bool Window::should_close() const { return rt::platform_window_should_close(platform); }
	void Window::set_should_close(bool should_close) { rt::platform_window_should_close(platform, should_close); }
	dim2<u32> Window::size() const { std::lock_guard lock(input_mutex); return window_size; }

	std::vector<rt::input_event> Window::input_events() { std::vector<rt::input_event> result; std::lock_guard lock(input_mutex); result.swap(events); return result; }
	rt::input_state Window::input_state(rt::input_control control) const { const size_t index = control_index(control); if (index >= controls.size()) return rt::input_state::Up; std::lock_guard lock(input_mutex); return controls[index]; }
	void Window::update_input() {
		std::lock_guard lock(input_mutex);
		for (size_t index = 0; index < controls.size(); ++index) {
			rt::input_state& state = controls[index];
			if (state == rt::input_state::Pressed && deferred_releases[index]) {
				state = rt::input_state::Released;
				events.push_back({ .type = rt::INPUT_EVENT_CONTROL, .control = index < rt::KEY_ENUM_MAX ? rt::input_control{ rt::INPUT_CONTROL_KEY, static_cast<u16>(index) } : rt::input_control{ rt::INPUT_CONTROL_BUTTON, static_cast<u16>(index - rt::KEY_ENUM_MAX) }, .state = state, .modifiers = modifiers, .position = pointer_position });
				deferred_releases[index] = false;
			} else if (state == rt::input_state::Pressed) state = rt::input_state::Down;
			else if (state == rt::input_state::Released) state = rt::input_state::Up;
		}
	}
	bool Window::mouse_down(rt::input_button button) const { const auto state = input_state({ rt::INPUT_CONTROL_BUTTON, static_cast<u16>(button) }); return state == rt::input_state::Down || state == rt::input_state::Pressed; }
	bool Window::mouse_pressed(rt::input_button button) const { return input_state({ rt::INPUT_CONTROL_BUTTON, static_cast<u16>(button) }) == rt::input_state::Pressed; }
	bool Window::mouse_released(rt::input_button button) const { return input_state({ rt::INPUT_CONTROL_BUTTON, static_cast<u16>(button) }) == rt::input_state::Released; }
	bool Window::key_down(rt::input_key key) const { const auto state = input_state({ rt::INPUT_CONTROL_KEY, static_cast<u16>(key) }); return state == rt::input_state::Down || state == rt::input_state::Pressed; }
	bool Window::key_pressed(rt::input_key key) const { return input_state({ rt::INPUT_CONTROL_KEY, static_cast<u16>(key) }) == rt::input_state::Pressed; }
	bool Window::key_released(rt::input_key key) const { return input_state({ rt::INPUT_CONTROL_KEY, static_cast<u16>(key) }) == rt::input_state::Released; }
	rt::view<rt::framebuffer> Window::current_framebuffer() { return frame_buffer; }
	rt::view<const rt::framebuffer> Window::current_framebuffer() const { return frame_buffer; }

	rt::view<rt::command_buffer> Window::begin_frame() {
		dim2<u32> resize; { std::lock_guard lock(input_mutex); resize = pending_resize; pending_resize = {}; }
		if (resize.width && resize.height) { rtSwapchainResize(swapchain, resize.width, resize.height); rt::detail::check_rutile_error("failed to resize swapchain"); }
		auto lock = rt::detail::lock_queue(queue);
		const rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		rt::detail::check_rutile_error("failed to acquire swapchain framebuffer");
		frame_buffer.value = acquired.framebuffer;
		if (!frame_buffer) return {};
		rtQueueWait(queue, acquired.timepoint);
		rt::detail::check_rutile_error("failed to wait for swapchain acquire");
		rt::Cmd::Reset(frame_command_buffer);
		rt::Cmd::Begin(frame_command_buffer);
		rt::Cmd::BeginRendering(frame_command_buffer, frame_buffer);
		rt::Cmd::ClearColor(frame_command_buffer, nullptr, 0.0f, 0.0f, 0.0f, 1.0f);
		rt::Cmd::Clear(frame_command_buffer, RT_CLEAR_COLOR);
		const dim2<u32> framebuffer_size = resize.width && resize.height ? resize : rt::platform_framebuffer_size(platform);
		rt::Cmd::SetViewport(frame_command_buffer, 0, 0, framebuffer_size.width, framebuffer_size.height, 0.0f, 1.0f);
		rt::Cmd::SetScissor(frame_command_buffer, 0, 0, framebuffer_size.width, framebuffer_size.height);
		return frame_command_buffer;
	}
	void Window::end_frame() {
		if (!frame_buffer) return;
		auto lock = rt::detail::lock_queue(queue);
		rt::Cmd::EndRendering(frame_command_buffer);
		rt::Cmd::End(frame_command_buffer);
		const rt::timepoint rendered = rtQueueSubmit(queue, frame_command_buffer);
		rt::detail::check_rutile_error("failed to submit swapchain frame");
		rtSwapchainPresent(swapchain, rendered);
		rt::detail::check_rutile_error("failed to present swapchain");
		frame_buffer = {};
	}

	void Window::on_control(rt::input_control control, bool down, rt::input_modifiers next_modifiers) {
		const size_t index = control_index(control); if (index >= controls.size()) return;
		std::lock_guard lock(input_mutex); modifiers = next_modifiers; rt::input_state& state = controls[index];
		if (down && (state == rt::input_state::Up || state == rt::input_state::Released)) { state = rt::input_state::Pressed; events.push_back({ .type = rt::INPUT_EVENT_CONTROL, .control = control, .state = state, .modifiers = modifiers, .position = pointer_position }); }
		else if (!down && (state == rt::input_state::Down || state == rt::input_state::Pressed)) { if (state == rt::input_state::Pressed) { deferred_releases[index] = true; return; } state = rt::input_state::Released; events.push_back({ .type = rt::INPUT_EVENT_CONTROL, .control = control, .state = state, .modifiers = modifiers, .position = pointer_position }); }
	}
	void Window::on_text(u32 character) { std::lock_guard lock(input_mutex); events.push_back({ .type = rt::INPUT_EVENT_TEXT, .modifiers = modifiers, .position = pointer_position, .character = character }); }
	void Window::on_pointer(pos2<f32> position) { std::lock_guard lock(input_mutex); pointer_position = position; events.push_back({ .type = rt::INPUT_EVENT_POINTER_MOVE, .modifiers = modifiers, .position = pointer_position }); }
	void Window::on_pointer_enter(bool entered) {
		std::lock_guard lock(input_mutex);
		pointer_inside = entered;
		events.push_back({ .type = rt::INPUT_EVENT_POINTER_ENTER, .state = entered ? rt::input_state::Down : rt::input_state::Up, .modifiers = modifiers, .position = pointer_position });
		if (entered) return;
		for (size_t index = rt::KEY_ENUM_MAX; index < controls.size(); ++index) {
			rt::input_state& state = controls[index];
			if (state != rt::input_state::Down && state != rt::input_state::Pressed) continue;
			state = rt::input_state::Released;
			deferred_releases[index] = false;
			events.push_back({ .type = rt::INPUT_EVENT_CONTROL, .control = { rt::INPUT_CONTROL_BUTTON, static_cast<u16>(index - rt::KEY_ENUM_MAX) }, .state = state, .modifiers = modifiers, .position = pointer_position });
		}
	}
	void Window::on_scroll(pos2<f32> delta) { std::lock_guard lock(input_mutex); events.push_back({ .type = rt::INPUT_EVENT_SCROLL, .modifiers = modifiers, .position = pointer_position, .delta = delta }); }
	void Window::on_focus(bool focused) {
		std::lock_guard lock(input_mutex);
		events.push_back({ .type = rt::INPUT_EVENT_FOCUS, .state = focused ? rt::input_state::Down : rt::input_state::Up, .modifiers = modifiers, .position = pointer_position });
		if (focused) return;
		for (size_t index = 0; index < controls.size(); ++index) {
			rt::input_state& state = controls[index];
			if (state != rt::input_state::Down && state != rt::input_state::Pressed) continue;
			state = rt::input_state::Released;
			deferred_releases[index] = false;
			events.push_back({ .type = rt::INPUT_EVENT_CONTROL, .control = index < rt::KEY_ENUM_MAX ? rt::input_control{ rt::INPUT_CONTROL_KEY, static_cast<u16>(index) } : rt::input_control{ rt::INPUT_CONTROL_BUTTON, static_cast<u16>(index - rt::KEY_ENUM_MAX) }, .state = state, .modifiers = modifiers, .position = pointer_position });
		}
		modifiers = {};
	}
	void Window::on_drop(string path) { std::lock_guard lock(input_mutex); events.push_back({ .type = rt::INPUT_EVENT_DROP, .modifiers = modifiers, .position = pointer_position, .text = std::move(path) }); }
	void Window::on_resize(dim2<u32> next_window_size, dim2<u32> framebuffer_size) { if (!next_window_size.width || !next_window_size.height || !framebuffer_size.width || !framebuffer_size.height) return; std::lock_guard lock(input_mutex); window_size = next_window_size; pending_resize = framebuffer_size; }
} // namespace lf
