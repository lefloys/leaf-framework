#include "timepoint.hpp"

#include "queue.hpp"

namespace lf {
	void Timepoint::Wait(timepoint timepoint) {
		rtTimepointWait(timepoint);
		detail::check_rutile_error("failed to wait for timepoint");
	}

	bool Timepoint::Reached(timepoint timepoint) {
		bool reached = rtTimepointReached(timepoint);
		detail::check_rutile_error("failed to query timepoint");
		return reached;
	}
} // namespace lf
