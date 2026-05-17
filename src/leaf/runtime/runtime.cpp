#include "runtime.hpp"

#include <leaf/core/exception.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/scene/scene.hpp>

#include <chrono>

namespace lf {
	error Init(span<string_view> args);
	bool Update();
	void Exit();

	struct Runtime {
		unique<window> window;
		handle<queue> queue;
		Scene scene;
		RuntimeServices services;
		std::chrono::steady_clock::time_point start{};
		std::chrono::steady_clock::time_point previous{};
		f64 fixed_accumulator = 0.0;
		u64 next_fixed_tick = 1;
		bool should_shutdown = false;
	};

	RuntimeServices registered_services;
	Runtime* active_runtime = nullptr;

	f64 tick_frame(Runtime& leaf_runtime) {
		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<f64> delta = now - leaf_runtime.previous;
		leaf_runtime.previous = now;
		if (delta.count() < 0.0) {
			return 0.0;
		}
		return std::min(delta.count(), 0.05);
	}

	f64 elapsed_seconds(const Runtime& leaf_runtime) {
		return std::chrono::duration<f64>(std::chrono::steady_clock::now() - leaf_runtime.start).count();
	}

	void RegisterRuntimeServices(const RuntimeServices& services) {
		registered_services = services;
	}

	view<window> RuntimeWindow() {
		return active_runtime ? view<window>(active_runtime->window) : view<window>();
	}

	view<queue> RuntimeQueue() {
		return active_runtime ? view<queue>(active_runtime->queue) : view<queue>();
	}

	u64 RuntimeNextFixedTick() {
		return active_runtime ? active_runtime->next_fixed_tick : 0;
	}

	void RuntimeRequestShutdown() {
		if (!active_runtime) {
			return;
		}
		active_runtime->should_shutdown = true;
		if (active_runtime->window) {
			Window::SetShouldClose(active_runtime->window, true);
		}
	}

	void RuntimeSetTitle(string_view title) {
		if (active_runtime && active_runtime->window) {
			Window::SetTitle(active_runtime->window, title);
		}
	}

	void RuntimeSetWindowSize(u32 width, u32 height) {
		if (!active_runtime || !active_runtime->window) {
			return;
		}
		Window::SetWidth(active_runtime->window, width);
		Window::SetHeight(active_runtime->window, height);
	}

	void RuntimeSetText(string_view element_id, string_view text) {
		if (active_runtime) {
			active_runtime->scene.set_text(element_id, text);
		}
	}

	void RuntimeSetProgress(string_view element_id, f32 progress) {
		if (active_runtime) {
			active_runtime->scene.set_progress(element_id, progress);
		}
	}

	error RuntimeLoadScene(string_view rml, string_view source_name, string_view lua_source) {
		if (!active_runtime) {
			return error{ "runtime is not active" };
		}
		return active_runtime->scene.load_memory(rml, source_name, lua_source);
	}

	error RunRuntime(span<string_view> args, const char* entry_rml, const char* entry_lua) {
		(void)args;
		Runtime runtime;
		runtime.services = registered_services;
		runtime.window = unique(Window::Create());
		runtime.queue = Queue::Query(QueueCapability::Graphics);
		runtime.start = std::chrono::steady_clock::now();
		runtime.previous = runtime.start;
		active_runtime = &runtime;

		if (runtime.services.init) {
			error err = runtime.services.init();
			if (err) {
				active_runtime = nullptr;
				return err;
			}
		}

		if (runtime.services.bind_lua) {
			runtime.scene.set_lua_binder([&runtime](sol::state& lua) {
				if (runtime.services.bind_lua) {
					runtime.services.bind_lua(lua);
				}
			});
		}
		if (runtime.services.configure_rml) {
			runtime.scene.set_rml_binder([&runtime]() {
				if (runtime.services.configure_rml) {
					runtime.services.configure_rml();
				}
			});
		}

		error err = runtime.scene.load_memory(entry_rml ? entry_rml : "", "entry.rml", entry_lua ? entry_lua : "");
		if (err) {
			active_runtime = nullptr;
			return err;
		}
		if (runtime.services.start) {
			runtime.services.start();
		}

		constexpr f64 fixed_step = 1.0 / 60.0;
		while (!runtime.should_shutdown && Update()) {
			f64 delta_seconds = tick_frame(runtime);
			if (Window::ShouldClose(runtime.window) || Window::KeyDown(runtime.window, Key::Escape)) {
				break;
			}

			runtime.scene.process_input(runtime.window);
			if (runtime.services.update) {
				runtime.services.update(delta_seconds);
			}
			runtime.scene.update(delta_seconds);

			runtime.fixed_accumulator += delta_seconds;
			u32 fixed_iterations = 0;
			while (runtime.fixed_accumulator >= fixed_step ||
				   (runtime.services.has_fixed_work && runtime.services.has_fixed_work())) {
				if (runtime.services.fixed_update) {
					runtime.services.fixed_update(runtime.next_fixed_tick);
				}
				++runtime.next_fixed_tick;
				if (runtime.fixed_accumulator >= fixed_step) {
					runtime.fixed_accumulator -= fixed_step;
				}
				++fixed_iterations;
				if (fixed_iterations >= 64) {
					runtime.fixed_accumulator = 0.0;
					break;
				}
			}

			dim2<u32> framebuffer_size = Window::FramebufferSize(runtime.window);
			view<command_buffer> command_buffer = Window::BeginFrame(runtime.window, runtime.queue);
			if (!command_buffer) {
				continue;
			}
			CommandBuffer::ClearColor(command_buffer, 0, 0.02f, 0.025f, 0.03f, 1.0f);
			if (runtime.services.render_world) {
				runtime.services.render_world(command_buffer, framebuffer_size, elapsed_seconds(runtime));
			}
			runtime.scene.render(command_buffer, Window::CurrentFramebuffer(runtime.window), framebuffer_size);
			Window::EndFrame(runtime.window);
		}

		Queue::Wait(runtime.queue, Queue::Flush(runtime.queue));
		runtime.scene.shutdown();
		if (runtime.services.shutdown) {
			runtime.services.shutdown();
		}
		active_runtime = nullptr;
		return error::no_error;
	}
} // namespace lf
