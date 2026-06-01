#ifndef LEAF_GRAPHICS_QUEUE_HPP
#define LEAF_GRAPHICS_QUEUE_HPP

#include <leaf/graphics/resource.hpp>

#include <mutex>

namespace lf {
	enum class QueueCapability {
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

	namespace detail {
		std::unique_lock<std::mutex> lock_queue(view<queue> queue);
		std::unique_lock<std::mutex> lock_queue(rt_queue queue);
	}
} // namespace lf

#endif /* LEAF_GRAPHICS_QUEUE_HPP */
