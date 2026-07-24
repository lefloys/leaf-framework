#ifndef LEAF_GRAPHICS_QUEUE_HPP
#define LEAF_GRAPHICS_QUEUE_HPP

#include <leaf/graphics/resource.hpp>

#include <mutex>

namespace rt {
	enum class QueueCapability : u08 {
		Transfer,
		Compute,
		Graphics,
	};

	namespace Queue {
		handle<queue> Query(QueueCapability capability);
		void Wait(view<queue> queue, timepoint timepoint);
		timepoint Submit(view<queue> queue, view<command_buffer> command_buffer);
		timepoint Flush(view<queue> queue);
	} // namespace Queue
	// @GPT : again why the fuck... do you have these lock queue things. thats just stupid...
	namespace detail {
		std::unique_lock<std::mutex> lock_queue(view<queue> queue);
		std::unique_lock<std::mutex> lock_queue(rt_queue queue);
	}
} // namespace rt

#endif /* LEAF_GRAPHICS_QUEUE_HPP */
