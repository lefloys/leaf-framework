#pragma once


namespace lf {
	template<typename T>
	struct handle {};

	template<typename T>
	struct Resource {
		static void Destroy(handle<T> obj);
	};
}