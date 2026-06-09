#pragma once

#include <leaf/application/application_stats.hpp>
#include <leaf/application/rml_backend.hpp>
#include <leaf/application/scene.hpp>
#include <leaf/core/error.hpp>
#include <leaf/core/memory.hpp>
#include <leaf/core/string.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/graphics/window.hpp>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Rml {
	class Context;
}

namespace lf {
	/*!
	** @ingroup application
	** @brief Default fixed-update rate used by applications.
	*/
	constexpr u32 DefaultApplicationUpdatesPerSecond = 60;

	/*!
	** @ingroup application
	** @brief Parameters used to create a client application window and update loop.
	*/
	struct ClientApplicationCreateInfo {
		/*!
		** @brief Initial window title.
		*/
		string_view title = "leaf-framework";

		/*!
		** @brief Initial window width in pixels.
		*/
		u32 width = 1280;

		/*!
		** @brief Initial window height in pixels.
		*/
		u32 height = 720;

		/*!
		** @brief Number of fixed simulation updates to run per second.
		*/
		u32 updates_per_second = DefaultApplicationUpdatesPerSecond;
	};

	/*!
	** @ingroup application
	** @brief Parameters used to create a headless fixed-update application.
	*/
	struct ServerApplicationCreateInfo {
		/*!
		** @brief Number of fixed simulation updates to run per second.
		*/
		u32 updates_per_second = DefaultApplicationUpdatesPerSecond;
	};

	/*!
	** @ingroup application
	** @brief Owns a headed Leaf application instance.
	**
	** ClientApplication ties together the platform window, graphics queue, RML UI
	** context, scene lifetime, render thread, and fixed-update thread.
	*/
	struct ClientApplication {
		/*!
		** @brief Creates a client application with default create information.
		*/
		ClientApplication();

		/*!
		** @brief Creates a client application and its window from explicit settings.
		** @param create_info Window and update-loop settings.
		*/
		explicit ClientApplication(const ClientApplicationCreateInfo& create_info);

		/*!
		** @brief Creates a client application around an existing window handle.
		** @param display Window ownership to adopt.
		*/
		explicit ClientApplication(handle<lf::window> display);

		/*!
		** @brief Creates a client application around an existing window handle.
		** @param display Window ownership to adopt.
		** @param create_info Window and update-loop settings.
		*/
		ClientApplication(handle<lf::window> display, const ClientApplicationCreateInfo& create_info);
		ClientApplication(const ClientApplication&) = delete;
		ClientApplication& operator=(const ClientApplication&) = delete;
		ClientApplication(ClientApplication&&) = delete;
		ClientApplication& operator=(ClientApplication&&) = delete;

		/*!
		** @brief Stops background work and releases client application resources.
		*/
		~ClientApplication();

		/*!
		** @brief Starts the client application with an initial scene.
		** @param scene_source Source path or identifier for the scene to load.
		** @return An error when startup fails, or an empty error on success.
		*/
		error launch(string_view scene_source);

		/*!
		** @brief Runs main-thread client application work for one iteration.
		** @return True while the client application remains open.
		*/
		bool update();

		/*!
		** @brief Requests client application shutdown.
		*/
		void close();

		/*!
		** @brief Checks whether the client application is currently running.
		*/
		bool is_running() const;

		/*!
		** @brief Releases ownership of the display window.
		** @return The window handle previously owned by the client application.
		*/
		handle<lf::window> release_window();

		/*!
		** @brief Replaces the RML markup for an element.
		** @param id Element id to update.
		** @param rml Replacement RML fragment.
		*/
		void set_rml(string_view id, string_view rml);

		/*!
		** @brief Sets a string attribute on an RML element.
		** @param id Element id to update.
		** @param name Attribute name.
		** @param value Attribute value.
		*/
		void set_attribute(string_view id, string_view name, string_view value);

		/*!
		** @brief Sets a numeric attribute on an RML element.
		** @param id Element id to update.
		** @param name Attribute name.
		** @param value Attribute value.
		*/
		void set_attribute(string_view id, string_view name, f32 value);

		/*!
		** @brief Queues Lua script source for execution in the current scene.
		** @param script Lua source text.
		*/
		void run_script(string_view script);

		/*!
		** @brief Sets the target render frame rate.
		** @param value Maximum frames per second.
		*/
		void set_max_fps(f32 value);

		/*!
		** @brief Gets the target render frame rate.
		*/
		f32 max_fps() const;

		/*!
		** @brief Sets the fixed-update rate.
		** @param value Updates per second.
		*/
		void set_updates_per_second(u32 value);

		/*!
		** @brief Gets the fixed-update rate.
		*/
		u32 updates_per_second() const;

		/*!
		** @brief Adds RML element types needed by scenes loaded by this client application.
		*/
		using ElementInstaller = std::function<void()>;

		/*!
		** @brief Adds an RML element installer to this client application.
		*/
		void add_rml_element_installer(ElementInstaller installer);

		/*!
		** @brief Adds Lua bindings to scenes loaded by this client application.
		*/
		void add_scene_script_installer(Scene::ScriptInstaller installer);

		/*!
		** @brief Adds fixed-update work to scenes loaded by this client application.
		*/
		void add_scene_fixed_updater(Scene::FixedUpdater updater);

		/*!
		** @brief Gets the measured render frame rate.
		*/
		f32 current_fps() const;

		/*!
		** @brief Gets the measured fixed-update rate.
		*/
		f32 current_ups() const;

		/*!
		** @brief Enables or disables render profiling.
		*/
		void set_render_profile_enabled(bool enabled);

		/*!
		** @brief Checks whether render profiling is enabled.
		*/
		bool render_profile_enabled() const;

		/*!
		** @brief Loads a scene without script arguments.
		** @param scene_source Source path or identifier for the scene to load.
		*/
		void load_scene(string_view scene_source);

		/*!
		** @brief Loads a scene with YAML argument data.
		** @param scene_source Source path or identifier for the scene to load.
		** @param args_yaml YAML data passed to the scene script.
		*/
		void load_scene(string_view scene_source, string_view args_yaml);

		/*!
		** @brief Gets the currently loaded scene.
		*/
		Scene* scene();

		/*!
		** @brief Gets the currently loaded scene.
		*/
		const Scene* scene() const;

		/*!
		** @brief Gets the RML context owned by the client application.
		*/
		Rml::Context* rml_context();

		/*!
		** @brief Gets the RML context owned by the client application.
		*/
		const Rml::Context* rml_context() const;

		/*!
		** @brief Gets a mutable view of the client application window.
		*/
		view<lf::window> window();

		/*!
		** @brief Gets a read-only view of the client application window.
		*/
		view<const lf::window> window() const;

	  private:
		void stop_threads();
		void wait_for_render_idle();

		static void render_thread_main(std::stop_token stop, ClientApplication& app);
		static void update_thread_main(std::stop_token stop, ClientApplication& app);
		static std::chrono::steady_clock::duration update_interval_for(u32 updates_per_second);

		unique<lf::window> display;
		string context_name;
		string window_title;
		Rml::Context* context = nullptr;
		unique_ptr<Scene> loaded_scene;
		std::mutex rml_mutex;
		std::mutex window_mutex;
		std::chrono::steady_clock::duration update_interval = update_interval_for(DefaultApplicationUpdatesPerSecond);
		ApplicationStats stats;
		std::vector<ElementInstaller> rml_element_installers;
		std::vector<Scene::ScriptInstaller> scene_script_installers;
		std::vector<Scene::FixedUpdater> scene_fixed_updaters;
		bool render_frame_active = false;
		std::atomic<bool> running = false;

		std::jthread render_thread;
		std::jthread update_thread;
	};

	/*!
	** @ingroup application
	** @brief Owns a headless fixed-update application lifecycle.
	**
	** ServerApplication intentionally does not create a platform window, RML
	** context, renderer, or scene. It only provides deterministic tick pacing
	** and shutdown state for server-style applications.
	*/
	struct ServerApplication {
		using FixedUpdater = std::function<void()>;

		ServerApplication();
		explicit ServerApplication(const ServerApplicationCreateInfo& create_info);
		ServerApplication(const ServerApplication&) = delete;
		ServerApplication& operator=(const ServerApplication&) = delete;
		ServerApplication(ServerApplication&&) = delete;
		ServerApplication& operator=(ServerApplication&&) = delete;
		~ServerApplication();

		error launch();
		bool update();
		void close();
		bool is_running() const;

		void add_fixed_updater(FixedUpdater updater);
		void set_updates_per_second(u32 value);
		u32 updates_per_second() const;
		f32 current_ups() const;

	  private:
		static std::chrono::steady_clock::duration update_interval_for(u32 updates_per_second);

		std::vector<FixedUpdater> fixed_updaters;
		std::chrono::steady_clock::duration update_interval = update_interval_for(DefaultApplicationUpdatesPerSecond);
		std::chrono::steady_clock::time_point next_update = {};
		std::chrono::steady_clock::time_point ups_sample_start = {};
		u32 ups_sample_updates = 0;
		std::atomic<u32> target_ups = DefaultApplicationUpdatesPerSecond;
		std::atomic<f32> measured_ups = 0.0f;
		std::atomic<bool> running = false;
	};
} // namespace lf
