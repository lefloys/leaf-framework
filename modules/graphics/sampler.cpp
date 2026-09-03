#include "leaf/graphics/sampler.hpp"

namespace rt::Sampler {
	handle<sampler> Create() {
		rt_sampler sampler = rtSamplerCreate();
		detail::check_rutile_error("failed to create sampler");
		return { sampler };
	}

	void SetFilter(view<sampler> sampler, rt_filter mag_filter, rt_filter min_filter, rt_mip_filter mip_filter) {
		rtSamplerSetFilter(sampler, mag_filter, min_filter, mip_filter);
		detail::check_rutile_error("failed to set sampler filter");
	}

	void SetAddress(view<sampler> sampler, rt_address_mode address_u, rt_address_mode address_v, rt_address_mode address_w) {
		rtSamplerSetAddress(sampler, address_u, address_v, address_w);
		detail::check_rutile_error("failed to set sampler address mode");
	}

	void SetAnisotropy(view<sampler> sampler, u64 max_anisotropy) {
		rtSamplerSetAnisotropy(sampler, max_anisotropy);
		detail::check_rutile_error("failed to set sampler anisotropy");
	}

	void SetLod(view<sampler> sampler, f32 min_lod, f32 max_lod, f32 lod_bias) {
		rtSamplerSetLod(sampler, min_lod, max_lod, lod_bias);
		detail::check_rutile_error("failed to set sampler LOD");
	}
} // namespace rt::Sampler
