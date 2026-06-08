#pragma once

#include "core/error.hpp"
#include "core/span.hpp"
#include "core/string.hpp"

#include <leaf/application/application.hpp>
#include <leaf/graphics/buffer.hpp>
#include <leaf/graphics/canvas.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/graphics/compute_program.hpp>
#include <leaf/graphics/framebuffer.hpp>
#include <leaf/graphics/graphics_program.hpp>
#include <leaf/graphics/queue.hpp>
#include <leaf/graphics/resource.hpp>
#include <leaf/graphics/texture.hpp>
#include <leaf/graphics/texture_atlas.hpp>
#include <leaf/graphics/texture_view.hpp>
#include <leaf/graphics/timepoint.hpp>
#include <leaf/graphics/uniform_location.hpp>
#include <leaf/graphics/window.hpp>
#include <leaf/pmg/pmg.hpp>

namespace lf {
	struct InitOptions {
		bool portable_user_data = false;
		string_view appdata_folder_name = {};
	};

	/*!
	** @brief Initializes Leaf's global systems.
	** @param args Command line arguments passed to the process.
	** @return An error when initialization fails, or an empty error on success.
	**
	** Call this before creating Leaf applications or using subsystems that need
	** framework-level setup.
	*/
	error Init(span<string_view> args);
	error Init(span<string_view> args, InitOptions options);

	/*!
	** @brief Initializes Leaf without headed window/RML application support.
	** @param args Command line arguments passed to the process.
	** @return An error when initialization fails, or an empty error on success.
	**
	** Headless mode still initializes Rutile/graphics resources; it only skips
	** platform windows and swapchain-backed UI systems.
	*/
	error InitHeadless(span<string_view> args);
	error InitHeadless(span<string_view> args, InitOptions options);

	/*!
	** @brief Advances framework-level work for one host iteration.
	** @return True while the framework should continue running.
	*/
	bool Update();

	/*!
	** @brief Shuts down Leaf's global systems.
	**
	** Call this after application shutdown to release framework-level resources.
	*/
	void Exit();

	/*!
	** @brief Shuts down systems initialized by InitHeadless.
	*/
	void ExitHeadless();
} // namespace lf
