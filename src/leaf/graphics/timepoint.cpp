#include "timepoint.hpp"

#include "queue.hpp"

namespace lf {
	void Timepoint::Wait(timepoint timepoint) {
		auto lock = detail::lock_queue(timepoint.queue);
		rtTimepointWait(timepoint);
		detail::check_rutile_error("failed to wait for timepoint");
	}

	bool Timepoint::Reached(timepoint timepoint) {
		auto lock = detail::lock_queue(timepoint.queue);
		bool reached = rtTimepointReached(timepoint);
		detail::check_rutile_error("failed to query timepoint");
		return reached;
	}
} // namespace lf
