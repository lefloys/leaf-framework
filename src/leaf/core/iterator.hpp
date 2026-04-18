#pragma once

#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <type_traits>
#include <utility>

namespace lf {
	template<typename T>
	class range_view {
		static_assert(std::is_integral_v<T>, "range_view requires an integral type");

	public:
		class iterator {
		public:
			using value_type = T;
			using difference_type = std::ptrdiff_t;

			iterator(T current, T end, T step) : current_(current), end_(end), step_(step) {}

			T operator*() const {
				return current_;
			}

			iterator& operator++() {
				current_ += step_;
				return *this;
			}

			bool operator==(const iterator& other) const {
				return current_ == other.current_;
			}

			bool operator!=(const iterator& other) const {
				return !(*this == other);
			}

		private:
			T current_;
			T end_;
			T step_;
		};

		range_view(T begin, T end, T step) : begin_(begin), end_(end), step_(step) {
			if (step_ <= 0) {
				std::abort();
			}
		}

		iterator begin() const {
			if (begin_ >= end_) {
				return end();
			}

			return iterator(begin_, end_, step_);
		}

		iterator end() const {
			const T distance = end_ - begin_;
			const T remainder = distance % step_;
			const T stop = remainder == 0 ? end_ : end_ + (step_ - remainder);
			return iterator(stop, end_, step_);
		}

	private:
		T begin_;
		T end_;
		T step_;
	};

	template<typename T>
	class rrange_view {
		static_assert(std::is_integral_v<T>, "rrange_view requires an integral type");

	public:
		class iterator {
		public:
			using value_type = T;
			using difference_type = std::ptrdiff_t;

			iterator(T current, T begin, T step, bool finished)
				: current_(current), begin_(begin), step_(step), finished_(finished) {}

			T operator*() const {
				return current_;
			}

			iterator& operator++() {
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
			T current_;
			T begin_;
			T step_;
			bool finished_;
		};

		rrange_view(T begin, T end, T step) : begin_(begin), end_(end), step_(step) {
			if (step_ <= 0) {
				std::abort();
			}
		}

		iterator begin() const {
			if (begin_ >= end_) {
				return end();
			}

			const T distance = end_ - begin_;
			const T count = (distance + step_ - 1) / step_;
			const T last = begin_ + (count - 1) * step_;
			return iterator(last, begin_, step_, false);
		}

		iterator end() const {
			return iterator(begin_, begin_, step_, true);
		}

	private:
		T begin_;
		T end_;
		T step_;
	};

	template<typename Index, typename Reference>
	struct enumerate_item {
		Index index;
		Reference value;
	};

	template<typename Iterator>
	class enumerate_iterator {
	public:
		using difference_type = std::ptrdiff_t;

		enumerate_iterator(std::size_t index, Iterator current) : index_(index), current_(current) {}

		auto operator*() const {
			return enumerate_item<std::size_t, decltype(*current_)>{ index_, *current_ };
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
		std::size_t index_;
		Iterator current_;
	};

	template<typename Iterator>
	class renumerate_iterator {
	public:
		using difference_type = std::ptrdiff_t;

		renumerate_iterator(std::size_t index, Iterator current) : index_(index), current_(current) {}

		auto operator*() const {
			return enumerate_item<std::size_t, decltype(*current_)>{ index_, *current_ };
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
		std::size_t index_;
		Iterator current_;
	};

	template<typename Range>
	class enumerate_view {
	public:
		explicit enumerate_view(Range& range) : range_(range) {}

		auto begin() const {
			return enumerate_iterator(std::size_t(0), std::begin(range_));
		}

		auto end() const {
			return enumerate_iterator(std::size_t(0), std::end(range_));
		}

	private:
		Range& range_;
	};

	template<typename Range>
	class renumerate_view {
	public:
		explicit renumerate_view(Range& range) : range_(range) {}

		auto begin() const {
			if (std::size(range_) == 0) {
				return end();
			}

			return renumerate_iterator(std::size(range_) - 1, std::rbegin(range_));
		}

		auto end() const {
			return renumerate_iterator(std::size_t(0), std::rend(range_));
		}

	private:
		Range& range_;
	};

	template<typename T>
	range_view<T> range(T begin, T end) {
		return range_view<T>(begin, end, 1);
	}

	template<typename T>
	range_view<T> range(T begin, T end, T step) {
		return range_view<T>(begin, end, step);
	}

	template<typename T>
	rrange_view<T> rrange(T begin, T end) {
		return rrange_view<T>(begin, end, 1);
	}

	template<typename T>
	rrange_view<T> rrange(T begin, T end, T step) {
		return rrange_view<T>(begin, end, step);
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
