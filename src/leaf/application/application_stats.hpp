#pragma once

#include <leaf/core/types.hpp>

namespace lf {
	constexpr f32 default_max_fps = 60.0f;
	constexpr f32 min_max_fps = 0.0f;
	constexpr f32 max_max_fps = 240.0f;

	f32 ClampMaxFps(f32 value);
	void SetApplicationMaxFps(f32 value);
	f32 ApplicationMaxFps();
	void SetApplicationUpdatesPerSecond(u32 value);
	u32 ApplicationUpdatesPerSecond();
	void RecordApplicationFps(f32 value);
	void RecordApplicationUps(f32 value);
	f32 CurrentApplicationFps();
	f32 CurrentApplicationUps();
	void SetRenderProfileEnabled(bool enabled);
	bool RenderProfileEnabled();
}
