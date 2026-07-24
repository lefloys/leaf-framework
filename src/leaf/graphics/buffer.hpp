#ifndef LEAF_GRAPHICS_BUFFER_HPP
#define LEAF_GRAPHICS_BUFFER_HPP

#include <leaf/graphics/resource.hpp>

namespace rt {
	enum class BufferMode {
		Static,
		Dynamic,
	};

	enum class BufferUsage : u32 {
		None = 0x00,
		Staging = 0x01,
		Vertex = 0x02,
		Index = 0x04,
		Uniform = 0x08,
		Storage = 0x10,
		TransferSrc = 0x20,
		TransferDst = 0x40,
	};

	// @GPT : Why dont you use the bitfield enum stuff from leaf.
	constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
		return static_cast<BufferUsage>(static_cast<u32>(lhs) | static_cast<u32>(rhs));
	}

	constexpr BufferUsage operator&(BufferUsage lhs, BufferUsage rhs) {
		return static_cast<BufferUsage>(static_cast<u32>(lhs) & static_cast<u32>(rhs));
	}

	namespace Buffer {
		handle<buffer> Create();
		void Destroy(handle<buffer> buffer);
		timepoint Data(view<buffer> buffer, BufferMode mode, BufferUsage usage, u64 size, const void* data);
		timepoint Subdata(view<buffer> buffer, u64 offset, u64 size, const void* data);
		void Read(view<buffer> buffer, u64 offset, u64 size, void* data);
	} // namespace Buffer
} // namespace rt

#endif /* LEAF_GRAPHICS_BUFFER_HPP */
