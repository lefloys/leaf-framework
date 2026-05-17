#pragma once

#include "core/error.hpp"
#include "core/span.hpp"

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
#include <leaf/runtime/runtime.hpp>

namespace lf {
	void RegisterLifetime(lf::error (*init_func)(lf::span<lf::string_view>), void (*exit_func)());

	error Init(span<string_view> args);
	bool Update();
	void Exit();
} // namespace lf
