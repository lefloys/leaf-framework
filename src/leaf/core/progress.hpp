#pragma once

#include "string.hpp"

#include <mutex>
#include <stop_token>
#include <unordered_map>

namespace lf {
	struct ProgressEntry {
		string text;
		f32 value = 0.0f;
	};

	struct Progress {
		std::stop_token stop;

		void set(string_view name, string_view text, f32 value) {
			std::lock_guard lock(mutex);
			ProgressEntry& entry = entries[string(name)];
			entry.text = text;
			entry.value = value;
		}

		bool try_get(string_view name, ProgressEntry& entry) const {
			std::lock_guard lock(mutex);
			auto it = entries.find(string(name));
			if (it == entries.end()) {
				return false;
			}
			entry = it->second;
			return true;
		}

		bool cancelled() const {
			return stop.stop_requested();
		}

	  private:
		mutable std::mutex mutex;
		std::unordered_map<string, ProgressEntry> entries;
	};
} // namespace lf
