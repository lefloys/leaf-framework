#include "compute_program.hpp"

namespace lf {
	handle<compute_program> ComputeProgram::Create() {
		rt_compute_program program = rtComputeProgramCreate();
		detail::check_rutile_error("failed to create compute program");
		return { program };
	}

	void ComputeProgram::Destroy(handle<compute_program> program) {
		rtComputeProgramDestroy(program);
	}

	void ComputeProgram::Shader(view<compute_program> program, u64 size, const void* data) {
		rtComputeProgramShader(program, size, data);
		detail::check_rutile_error("failed to set compute shader");
	}

	void ComputeProgram::Link(view<compute_program> program) {
		rtComputeProgramLink(program);
		detail::check_rutile_error("failed to link compute program");
	}
} // namespace lf
