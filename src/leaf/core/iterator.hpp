#pragma once

#include "cstddef.hpp"
#include "cstdlib.hpp"
#include "utility.hpp"

#include <type_traits>

namespace lf {
	class range_view {
	public:
		class iterator {
		public:
			using value_type = size_t;
			using difference_type = ptrdiff_t;

			iterator(size_t current, size_t end, size_t step, bool finished)
				: current_(current), end_(end), step_(step), finished_(finished) {}

			size_t operator*() const {
				return current_;
			}

			iterator& operator++() {
				if (finished_) {
					return *this;
				}

				current_ += step_;
				finished_ = step_ > 0 ? current_ >= end_ : current_ <= end_;
				return *this;
			}

			bool operator==(const iterator& other) const {
				if (finished_ || other.finished_) {
					return finished_ == other.finished_;
				}

				return current_ == other.current_;
			}

			bool operator!=(const iterator& other) const {
				return !(*this == other);
			}

		private:
			size_t current_;
			size_t end_;
			size_t step_;
			bool finished_;
		};

		range_view(size_t begin, size_t end, size_t step) : begin_(begin), end_(end), step_(step) {
				if (step_ == 0) {
				abort();
			}
		}

		iterator begin() const {
			if (is_empty()) {
				return end();
			}

			return iterator(begin_, end_, step_, false);
		}

		iterator end() const {
			return iterator(begin_, end_, step_, true);
		}

	private:
		bool is_empty() const {
			return begin_ >= end_;
		}

		size_t begin_;
		size_t end_;
		size_t step_;
	};

	class rrange_view {
	public:
		class iterator {
		public:
			using value_type = size_t;
			using difference_type = ptrdiff_t;

			iterator(size_t current, size_t begin, size_t step, bool finished)
				: current_(current), begin_(begin), step_(step), finished_(finished) {}

			size_t operator*() const {
				return current_;
			}

			iterator& operator++() {
				if (finished_) {
					return *this;
				}

				if (current_ < begin_ + step_) {
					finished_ = true;
					return *this;
				}

				current_ -= step_;
				return *this;
			}

			bool operator==(const iterator& other) const {
				if (finished_ || other.finished_) {
					return finished_ == other.finished_;
				}

				return current_ == other.current_;
			}

			bool operator!=(const iterator& other) const {
				return !(*this == other);
			}

		private:
			size_t current_;
			size_t begin_;
			size_t step_;
			bool finished_;
		};

		rrange_view(size_t begin, size_t end, size_t step) : begin_(begin), end_(end), step_(step) {
				if (step_ == 0) {
				abort();
			}
		}

		iterator begin() const {
			if (begin_ >= end_) {
				return end();
			}

			const size_t distance = end_ - begin_;
			const size_t count = (distance + step_ - 1) / step_;
			const size_t last = begin_ + (count - 1) * step_;
			return iterator(last, begin_, step_, false);
		}

		iterator end() const {
			return iterator(begin_, begin_, step_, true);
		}

	private:
		size_t begin_;
		size_t end_;
		size_t step_;
	};

	template<typename Index, typename Reference>
	struct enumerate_item {
		Index index;
		Reference value;
	};

	template<typename Iterator>
	class enumerate_iterator {
	public:
		using difference_type = ptrdiff_t;

		enumerate_iterator(size_t index, Iterator current) : index_(index), current_(current) {}

		auto operator*() const {
			return enumerate_item<size_t, decltype(*current_)>{ index_, *current_ };
		}

		enumerate_iterator& operator++() {
			++index_;
			++current_;
			return *this;
		}

		bool operator==(const enumerate_iterator& other) const {
			return current_ == other.current_;
		}

		bool operator!=(const enumerate_iterator& other) const {
			return !(*this == other);
		}

	private:
		size_t index_;
		Iterator current_;
	};

	template<typename Iterator>
	class renumerate_iterator {
	public:
		using difference_type = ptrdiff_t;

		renumerate_iterator(size_t index, Iterator current) : index_(index), current_(current) {}

		auto operator*() const {
			return enumerate_item<size_t, decltype(*current_)>{ index_, *current_ };
		}

		renumerate_iterator& operator++() {
			--index_;
			++current_;
			return *this;
		}

		bool operator==(const renumerate_iterator& other) const {
			return current_ == other.current_;
		}

		bool operator!=(const renumerate_iterator& other) const {
			return !(*this == other);
		}

	private:
		size_t index_;
		Iterator current_;
	};

	template<typename Range>
	class enumerate_view {
	public:
		explicit enumerate_view(Range& range) : range_(range) {}

		auto begin() const {
			return enumerate_iterator(size_t(0), lf::begin(range_));
		}

		auto end() const {
			return enumerate_iterator(size_t(0), lf::end(range_));
		}

	private:
		Range& range_;
	};

	template<typename Range>
	class renumerate_view {
	public:
		explicit renumerate_view(Range& range) : range_(range) {}

		auto begin() const {
			if (lf::size(range_) == 0) {
				return end();
			}

			return renumerate_iterator(lf::size(range_) - 1, lf::rbegin(range_));
		}

		auto end() const {
			return renumerate_iterator(size_t(0), lf::rend(range_));
		}

	private:
		Range& range_;
	};

	inline range_view range(size_t begin, size_t end, size_t step = 1) {
		return range_view(begin, end, step);
	}

	inline rrange_view rrange(size_t begin, size_t end, size_t step = 1) {
		return rrange_view(begin, end, step);
	}

	template<typename Range>
	enumerate_view<Range> enumerate(Range& range) {
		return enumerate_view<Range>(range);
	}

	template<typename Range>
	enumerate_view<const Range> enumerate(const Range& range) {
		return enumerate_view<const Range>(range);
	}

	template<typename Range>
	renumerate_view<Range> renumerate(Range& range) {
		return renumerate_view<Range>(range);
	}

	template<typename Range>
	renumerate_view<const Range> renumerate(const Range& range) {
		return renumerate_view<const Range>(range);
	}
}
