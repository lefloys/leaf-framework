#pragma once

struct Resource {
	Resource() = default;
	~Resource() = default;

	Resource(const Resource&) = delete;
	Resource& operator=(const Resource&) = delete;
	Resource(Resource&&) = delete;
	Resource& operator=(Resource&&) = delete;
};
