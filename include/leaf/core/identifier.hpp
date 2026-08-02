#pragma once

#include "leaf/core/concepts.hpp"

namespace lf {
	template <typename T, typename vnum, typename gnum>
	class identifier {
	  public:
		using value_type = T;
		using vnum_t = vnum;
		using gnum_t = gnum;

		explicit identifier() = default;
		explicit identifier(vnum_t idx, gnum_t gen) : idx_value(idx), gen_value(gen) {}
		explicit operator bool() const {
			return idx_value;
		}

		vnum get() const {
			return idx_value;
		}
		gnum gen() const {
			return gen_value;
		}
		friend bool operator==(const identifier& lhs, const identifier& rhs) {
			return lhs.idx_value == rhs.idx_value && lhs.gen_value == rhs.gen_value;
		}
		friend bool operator!=(const identifier& lhs, const identifier& rhs) {
			return !(lhs == rhs);
		}

	  private:
		vnum idx_value = 0;
		gnum gen_value = 0;
	};

	template <typename T, typename vnum>
	class identifier<T, vnum, void> {
	  public:
		using value_type = T;
		using vnum_t = vnum;
		using gnum_t = void;

		explicit identifier() = default;
		explicit identifier(vnum idx) : idx_value(idx) {}

		vnum get() const {
			return idx_value;
		}

		explicit operator bool() const {
			return idx_value;
		}
		friend bool operator==(const identifier& lhs, const identifier& rhs) {
			return lhs.idx_value == rhs.idx_value;
		}
		friend bool operator!=(const identifier& lhs, const identifier& rhs) {
			return !(lhs == rhs);
		}

	  private:
		vnum idx_value = 0;
	};
} // namespace lf
