#pragma once

#include <leaf/core/types.hpp>

namespace lf {
	template<typename T> struct handle;
	template<typename T> struct unique;
	template<typename T> struct shared;
	template<typename T> struct weak;
	template<typename T> struct view;

	enum class buffer_usage : u08;
	enum class texture_format : u08;
	enum class draw_mode : u08;
	enum class queue_flag_bits : u08;

	struct Window;
	struct VertexBuffer;
	struct ElementBuffer;
	struct Texture1D;
	struct Texture2D;
	struct Texture3D;
	struct FrameBuffer;
	struct RenderBuffer;
	struct GraphicsPipeline;
	struct ComputePipeline;
	struct CommandPool;
	struct CommandBuffer;
	struct Queue;
	template<typename Vertex>
	struct Canvas;
}