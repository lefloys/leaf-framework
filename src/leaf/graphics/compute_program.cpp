#include "compute_program.hpp"

#include <leaf/core/exception.hpp>

namespace lf {
	void RequireComputeProgramExtension() {
		if (!rt_rtComputeProgramCreate) {
			throw runtime_exception("Rutile compute extension is not available");
		}
	}

	handle<compute_program> ComputeProgram::Create() {
		RequireComputeProgramExtension();
		rt_compute_program program = rtComputeProgramCreate();
		detail::check_rutile_error("failed to create compute program");
		return { program };
	}

	void ComputeProgram::Destroy(handle<compute_program> program) {
		RequireComputeProgramExtension();
		rtComputeProgramDestroy(program);
	}

	void ComputeProgram::Shader(handle<compute_program> program, u64 size, const void* data) {
		RequireComputeProgramExtension();
		rtComputeProgramShader(program, size, data);
		detail::check_rutile_error("failed to set compute shader");
	}

	void ComputeProgram::Link(handle<compute_program> program) {
		RequireComputeProgramExtension();
		rtComputeProgramLink(program);
		detail::check_rutile_error("failed to link compute program");
	}
} // namespace lf
