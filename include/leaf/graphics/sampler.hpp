#pragma once

#include <leaf/graphics/resource.hpp>

namespace rt::Sampler {
	handle<sampler> Create();
	void SetFilter(view<sampler> sampler, rt_filter mag_filter, rt_filter min_filter, rt_mip_filter mip_filter);
	void SetAddress(view<sampler> sampler, rt_address_mode address_u, rt_address_mode address_v, rt_address_mode address_w);
	void SetAnisotropy(view<sampler> sampler, u64 max_anisotropy);
	void SetLod(view<sampler> sampler, f32 min_lod, f32 max_lod, f32 lod_bias);
} // namespace rt::Sampler
