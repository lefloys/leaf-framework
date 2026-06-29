#pragma once

#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/platform/platform.hpp>

#include <rt_ext_swapchain.h>

#include <array>
#include <mutex>
#include <string>
#include <vector>

namespace lf {
	struct window_t {
		static constexpr size_t control_count = KEY_ENUM_MAX + BUTTON_ENUM_MAX;

		PlatformWindow* platform = nullptr;
		std::string current_cursor;
		rt_swapchain swapchain = RT_NULL_HANDLE;
		unique<command_buffer> frame_command_buffer;
		view<framebuffer> current_framebuffer;
		view<queue> current_queue;
		dim2<u32> size = { 1280, 720 };
		dim2<u32> pending_resize = {};
		pos2<i32> windowed_position = { 100, 100 };
		dim2<u32> windowed_size = { 1280, 720 };
		std::array<input_state, control_count> controls = {};
		std::array<bool, control_count> deferred_releases = {};
		std::vector<input_event> events;
		pos2<f32> pointer_position;
		bool pointer_inside = false;
		bool fullscreen = false;
		bool fullscreen_change_requested = false;
		bool requested_fullscreen = false;
		bool vsync = false;
		input_modifiers modifiers;
		mutable std::mutex input_mutex;

		static size_t control_index(input_control control);
		void control(input_control control, bool down, input_modifiers modifiers);
		void text(u32 character);
		void pointer(pos2<f32> position);
		void pointer_enter(bool entered);
		void scroll(pos2<f32> delta);
		void focus(bool focused);
		void drop(string path);
		void resize(dim2<u32> size);

		~window_t();
	};
} // namespace lf
