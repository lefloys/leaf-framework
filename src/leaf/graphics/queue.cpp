#include "queue.hpp"

#include <memory>
#include <vector>

namespace lf::detail {
	namespace {
		struct queue_lock_entry {
			rt_queue queue = RT_NULL_HANDLE;
			std::unique_ptr<std::mutex> mutex;
		};

		struct queue_lock_registry {
			std::mutex mutex;
			std::vector<queue_lock_entry> entries;

			std::mutex& get(rt_queue queue) {
				std::lock_guard lock(mutex);
				for (queue_lock_entry& entry : entries) {
					if (entry.queue == queue) {
						return *entry.mutex;
					}
				}

				queue_lock_entry& entry = entries.emplace_back();
				entry.queue = queue;
				entry.mutex = std::make_unique<std::mutex>();
				return *entry.mutex;
			}
		};

		queue_lock_registry& queue_locks() {
			static queue_lock_registry registry;
			return registry;
		}
	}

	rt_queue_capability to_rutile(QueueCapability capability) {
		switch (capability) {
		case QueueCapability::Transfer: return RT_QUEUE_TRANSFER;
		case QueueCapability::Compute: return RT_QUEUE_COMPUTE;
		case QueueCapability::Graphics:
		default: return RT_QUEUE_GRAPHICS;
		}
	}

	std::unique_lock<std::mutex> lock_queue(view<queue> queue) {
		return std::unique_lock(queue_locks().get(queue));
	}
} // namespace lf::detail

namespace lf {
	handle<queue> Queue::Query(QueueCapability capability) {
		rt_queue queue = rtQueueQuery(detail::to_rutile(capability));
		detail::check_rutile_error("failed to query queue");
		return { queue };
	}

	void Queue::Wait(view<queue> queue, timepoint timepoint) {
		auto lock = detail::lock_queue(queue);
		rtQueueWait(queue, timepoint);
		detail::check_rutile_error("failed to wait for queue");
	}

	timepoint Queue::Submit(view<queue> queue, view<command_buffer> command_buffer) {
		auto lock = detail::lock_queue(queue);
		rt_timepoint timepoint = rtQueueSubmit(queue, command_buffer);
		detail::check_rutile_error("failed to submit command buffer");
		return timepoint;
	}

	timepoint Queue::Flush(view<queue> queue) {
		auto lock = detail::lock_queue(queue);
		rt_timepoint timepoint = rtQueueFlush(queue);
		detail::check_rutile_error("failed to flush queue");
		return timepoint;
	}
} // namespace lf
