#pragma once

#include "leaf/core/types.hpp"

namespace lf {
	template<typename T>
	struct handle {
		u32 id;
		u32 generation_id;
	};
	template<typename T>
	struct view {
		u32 id;
		u32 generation_id;
	};
	template<typename T>
	struct unique {

		handle<T> release();
		view<T> get() const;

		handle<T> value;
	};
}

namespace lf::detail {
	template<typename T>
	struct Resource {
		static void Destroy(handle<T> obj);
		struct Backend {
			decltype(Destroy)* destroy = nullptr;
		};
	};
}