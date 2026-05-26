#pragma once

#include <leaf/core/error.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/string.hpp>
#include <leaf/application/scene.hpp>
#include <leaf/application/rml_backend.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/graphics/window.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace Rml {
	class Context;
}

namespace lf {
	constexpr u32 DefaultApplicationUpdatesPerSecond = 60;

	struct ApplicationCreateInfo {
		string_view title = "leaf-framework";
		u32 width = 1280;
		u32 height = 720;
		u32 updates_per_second = DefaultApplicationUpdatesPerSecond;
	};

	struct Application {
		Application();
		explicit Application(const ApplicationCreateInfo& create_info);
		explicit Application(handle<lf::window> display);
		Application(handle<lf::window> display, const ApplicationCreateInfo& create_info);
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
		Application(Application&&) = delete;
		Application& operator=(Application&&) = delete;
		~Application();

		error launch(string_view scene_source);
		bool update();
		void close();
		bool is_running() const;
		handle<lf::window> release_window();
		void set_rml(string_view id, string_view rml);
		void set_attribute(string_view id, string_view name, string_view value);
		void set_attribute(string_view id, string_view name, f32 value);
		void run_script(string_view script);

		void load_scene(string_view scene_source);
		void load_scene(string_view scene_source, string_view args_yaml);
		Scene* scene();
		const Scene* scene() const;
		Rml::Context* rml_context();
		const Rml::Context* rml_context() const;
		view<lf::window> window();
		view<const lf::window> window() const;

	private:
		void stop_threads();

		static void render_thread_main(std::stop_token stop, Application& app);
		static void update_thread_main(std::stop_token stop, Application& app);
		static std::chrono::steady_clock::duration update_interval_for(u32 updates_per_second);

		unique<lf::window> display;
		string context_name;
		string window_title;
		Rml::Context* context = nullptr;
		unique_ptr<Scene> loaded_scene;
		std::mutex rml_mutex;
		std::mutex window_mutex;
		std::chrono::steady_clock::duration update_interval = update_interval_for(DefaultApplicationUpdatesPerSecond);
		bool render_frame_active = false;
		std::atomic<bool> running = false;

		std::jthread render_thread;
		std::jthread update_thread;
	};
} // namespace lf
