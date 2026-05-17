#include "queue.hpp"

namespace lf::detail {
	rt_queue_capability to_rutile(QueueCapability capability) {
		switch (capability) {
		case QueueCapability::Transfer: return RT_QUEUE_TRANSFER;
		case QueueCapability::Compute: return RT_QUEUE_COMPUTE;
		case QueueCapability::Graphics:
		default: return RT_QUEUE_GRAPHICS;
		}
	}
} // namespace lf::detail

namespace lf {
	handle<queue> Queue::Query(QueueCapability capability) {
		rt_queue queue = rtQueueQuery(detail::to_rutile(capability));
		detail::check_rutile_error("failed to query queue");
		return { queue };
	}

	void Queue::Wait(view<queue> queue, timepoint timepoint) {
		rtQueueWait(queue, timepoint);
		detail::check_rutile_error("failed to wait for queue");
	}

	timepoint Queue::Submit(view<queue> queue, view<command_buffer> command_buffer) {
		rt_timepoint timepoint = rtQueueSubmit(queue, command_buffer);
		detail::check_rutile_error("failed to submit command buffer");
		return timepoint;
	}

	timepoint Queue::Flush(view<queue> queue) {
		rt_timepoint timepoint = rtQueueFlush(queue);
		detail::check_rutile_error("failed to flush queue");
		return timepoint;
	}
} // namespace lf
