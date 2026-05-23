#pragma once

#include <leaf/core/string.hpp>
#include <leaf/core/types.hpp>
#include <leaf/core/vector.hpp>

#include <chrono>

namespace lf {
	struct ProfileSnapshotEntry {
		string name;
		u64 calls = 0;
		f64 total_ms = 0.0;
		f64 max_ms = 0.0;
	};

	void SetProfilerEnabled(bool enabled);
	bool ProfilerEnabled();
	void ResetProfiler();
	void RecordProfileSample(string_view name, f64 milliseconds);
	vector<ProfileSnapshotEntry> ProfileSnapshot();
	void LogProfileSnapshot(string_view label = "profile");

	class ProfileScope {
	public:
		explicit ProfileScope(string_view name);
		ProfileScope(const ProfileScope&) = delete;
		ProfileScope& operator=(const ProfileScope&) = delete;
		~ProfileScope();

	private:
		string_view name;
		std::chrono::steady_clock::time_point start;
		bool enabled = false;
	};
} // namespace lf

#define LF_CONCAT_IMPL(a, b) a##b
#define LF_CONCAT(a, b) LF_CONCAT_IMPL(a, b)
#define LF_PROFILE_SCOPE(name) ::lf::ProfileScope LF_CONCAT(_lf_profile_scope_, __LINE__)(name)
#define LF_PROFILE_FUNCTION() LF_PROFILE_SCOPE(__FUNCTION__)
