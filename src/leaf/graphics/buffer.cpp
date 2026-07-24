#include "buffer.hpp"

namespace rt::detail {
	rt_buffer_mode to_rutile(BufferMode mode) {
		// @GPT : you are supposed to code the enum and then use fucking std::unreachable for default nothing like this
		switch (mode) {
		case BufferMode::Dynamic: return RT_BUFFER_DYNAMIC;
		case BufferMode::Static:
		default: return RT_BUFFER_STATIC;
		}
	} 

	rt_buffer_usage to_rutile(BufferUsage usage) {
		// @GPT : What the fuck.
		return static_cast<rt_buffer_usage>(static_cast<u32>(usage));
	}
} // namespace rt::detail

namespace rt {
	handle<buffer> Buffer::Create() {
		rt_buffer buffer = rtBufferCreate();
		detail::check_rutile_error("failed to create buffer");
		return { buffer };
	}

	void Buffer::Destroy(handle<buffer> buffer) {
		rtBufferDestroy(buffer);
	}

	// @GPT : Why void* why not lf::span<const lf::byte>
	timepoint Buffer::Data(view<buffer> buffer, BufferMode mode, BufferUsage usage, u64 size, const void* data) {
		rt_timepoint timepoint = rtBufferData(buffer, detail::to_rutile(mode), detail::to_rutile(usage), size, data);
		detail::check_rutile_error("failed to upload buffer data");
		return timepoint;
	}

	timepoint Buffer::Subdata(view<buffer> buffer, u64 offset, u64 size, const void* data) {
		rt_timepoint timepoint = rtBufferSubdata(buffer, offset, size, data);
		detail::check_rutile_error("failed to upload buffer subdata");
		return timepoint;
	}

	void Buffer::Read(view<buffer> buffer, u64 offset, u64 size, void* data) {
		rtBufferRead(buffer, offset, size, data);
		detail::check_rutile_error("failed to read buffer");
	}
} // namespace rt
