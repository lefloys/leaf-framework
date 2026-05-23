#pragma once

#include <leaf/core/types.hpp>

namespace lf {
	constexpr f32 default_max_fps = 60.0f;
	constexpr f32 min_max_fps = 0.0f;

	f32 ClampMaxFps(f32 value);
	void SetApplicationMaxFps(f32 value);
	f32 ApplicationMaxFps();
	void RecordApplicationFps(f32 value);
	void RecordApplicationUps(f32 value);
	f32 CurrentApplicationFps();
	f32 CurrentApplicationUps();
	void SetRenderProfileEnabled(bool enabled);
	bool RenderProfileEnabled();
}
