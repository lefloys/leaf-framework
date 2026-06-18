#ifndef LEAF_GRAPHICS_COMPUTE_PROGRAM_HPP
#define LEAF_GRAPHICS_COMPUTE_PROGRAM_HPP

#include <leaf/graphics/resource.hpp>

namespace lf {
	namespace ComputeProgram {
		handle<compute_program> Create();
		void Destroy(handle<compute_program> program);
		void Shader(view<compute_program> program, u64 size, const void* data);
		void Link(view<compute_program> program);
	} // namespace ComputeProgram
} // namespace lf

#endif
