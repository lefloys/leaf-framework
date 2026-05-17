#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/graphics/resource.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/math/dim.hpp>

#include <sol/sol.hpp>

namespace lf {
	struct Runtime;

	struct RuntimeServices {
		error (*init)() = nullptr;
		void (*start)() = nullptr;
		void (*shutdown)() = nullptr;
		void (*configure_rml)() = nullptr;
		void (*bind_lua)(sol::state& lua) = nullptr;
		bool (*has_fixed_work)() = nullptr;
		void (*fixed_update)(u64 tick) = nullptr;
		void (*update)(f64 delta_seconds) = nullptr;
		void (*render_world)(view<command_buffer> command_buffer, dim2<u32> framebuffer_size,
							 f64 time_seconds) = nullptr;
	};

	void RegisterRuntimeServices(const RuntimeServices& services);

	view<window> RuntimeWindow();
	view<queue> RuntimeQueue();
	u64 RuntimeNextFixedTick();
	void RuntimeRequestShutdown();
	void RuntimeSetTitle(string_view title);
	void RuntimeSetWindowSize(u32 width, u32 height);
	void RuntimeSetText(string_view element_id, string_view text);
	void RuntimeSetProgress(string_view element_id, f32 progress);
	error RuntimeLoadScene(string_view rml, string_view source_name, string_view lua_source);

	error RunRuntime(span<string_view> args, const char* entry_rml, const char* entry_lua);
} // namespace lf
