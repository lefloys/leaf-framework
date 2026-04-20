#pragma once


#include "leaf/core/types.hpp"
#include "leaf/core/type_name.hpp"
#include "leaf/core/string.hpp"

namespace lf::resource {
	// Target
	struct window;
	struct framebuffer;

	// Shaders
	struct graphics_pipeline;
	struct compute_pipeline;

	// Command
	struct command_buffer;
	struct command_context;
	struct queue;


	// Buffers
	struct vertex_buffer;
	struct element_buffer;
	struct storage_buffer;

	// Textures
	struct texture_2d;

	struct texture_view_2d;
}



namespace lf::constant {
	enum class buffer_usage : u08;
	enum class texture_format : u08;
	enum class draw_mode : u08;
	enum class queue_flag_bits : u08;
}

namespace lf {
	using namespace resource;
	using namespace constant;


	template<typename T> struct handle;
	template<typename T> struct view;

	template<typename T>
	struct handle {
		u32 id = 0;
		u32 generation_id = 0;

		explicit operator bool() const noexcept { return id != 0; }
		operator view<T>() const { return { id, generation_id }; }
		operator view<const T>() const { return { id, generation_id }; }

	};
	template<typename T>
	struct view {
		u32 id = 0;
		u32 generation_id = 0;

		explicit operator bool() const noexcept { return id != 0; }
		operator view<const T>() const { return { id, generation_id }; }
	};

	template<> struct type_name_trait<window> { static constexpr string_view value = "window"; };
	template<> struct type_name_trait<framebuffer> { static constexpr string_view value = "framebuffer"; };
	template<> struct type_name_trait<command_buffer> { static constexpr string_view value = "command_buffer"; };
	template<> struct type_name_trait<queue> { static constexpr string_view value = "queue"; };

}

namespace lf {
	class QueueTimepoint {
		view<const queue> queue;
		u64 timepoint;
	};
}
