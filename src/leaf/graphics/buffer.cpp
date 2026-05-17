#include "buffer.hpp"

namespace lf::detail {
	rt_buffer_mode to_rutile(BufferMode mode) {
		switch (mode) {
		case BufferMode::Dynamic: return RT_BUFFER_DYNAMIC;
		case BufferMode::Static:
		default: return RT_BUFFER_STATIC;
		}
	}

	rt_buffer_usage to_rutile(BufferUsage usage) {
		return static_cast<rt_buffer_usage>(static_cast<u32>(usage));
	}
} // namespace lf::detail

namespace lf {
	handle<buffer> Buffer::Create() {
		rt_buffer buffer = rtBufferCreate();
		detail::check_rutile_error("failed to create buffer");
		return { buffer };
	}

	void Buffer::Destroy(handle<buffer> buffer) {
		rtBufferDestroy(buffer);
	}

	timepoint Buffer::Data(view<buffer> buffer, BufferMode mode, BufferUsage usage, u64 size,
						   const void* data) {
		rt_timepoint timepoint =
			rtBufferData(buffer, detail::to_rutile(mode), detail::to_rutile(usage), size, data);
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

	void* Buffer::Map(view<buffer> buffer, u64 offset, u64 size) {
		void* data = rtBufferMap(buffer, offset, size);
		detail::check_rutile_error("failed to map buffer");
		return data;
	}

	void Buffer::Unmap(view<buffer> buffer) {
		rtBufferUnmap(buffer);
		detail::check_rutile_error("failed to unmap buffer");
	}
} // namespace lf
