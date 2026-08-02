#pragma once

#include "leaf/core/normalized.hpp"
#include "leaf/core/string.hpp"
#include "leaf/core/vector.hpp"

#include <algorithm>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <unordered_map>
#include <utility>
#include <vector>

namespace lf {
	struct ProgressEntry {
		string text;
		f32 value = 0.0f;
	};

	struct Progress {
		std::stop_token stop;

		explicit Progress(string_view label = {}) : shared(std::make_shared<Shared>()), owned(std::make_unique<Node>()) {
			owned->label = string(label);
			node = owned.get();
		}
		Progress(const Progress&) = delete;
		Progress& operator=(const Progress&) = delete;
		Progress(Progress&& other) noexcept : shared(std::move(other.shared)), owned(std::move(other.owned)), node(other.node), notify(other.notify), entries(std::move(other.entries)) {
			other.node = nullptr;
			other.notify = false;
		}
		Progress& operator=(Progress&& other) noexcept {
			if (this != &other) {
				shared = std::move(other.shared);
				owned = std::move(other.owned);
				node = other.node;
				notify = other.notify;
				entries = std::move(other.entries);
				other.node = nullptr;
				other.notify = false;
			}
			return *this;
		}
		~Progress() = default;

		// Defines phase slots. Adding slots changes the denominator immediately.
		void add(string_view label, u64 weight = 1) {
			(void)weight;
			std::scoped_lock lock(shared->mutex);
			node->phases.emplace_back(string(label));
		}

		// Creates the next live child. Its Progress handle owns the child node.
		Progress operator()() {
			std::scoped_lock lock(shared->mutex);
			if (node->next_phase == node->phases.size()) throw std::logic_error("lf::Progress: no phase remains");
			auto child = std::make_unique<Node>();
			child->label = node->phases[node->next_phase++];
			child->parent = node;
			Node* child_ptr = child.get();
			node->active.push_back(child_ptr);
			return Progress(shared, std::move(child), child_ptr, true);
		}

		vector<Progress> split(string_view label, size_t count, u64 weight = 1) {
			(void)weight;
			vector<Progress> result;
			result.reserve(count);
			for (size_t index = 0; index < count; ++index) {
				add(label);
			}
			for (size_t index = 0; index < count; ++index) {
				result.emplace_back((*this)());
			}
			return result;
		}

		void set(normalized<u32> value) {
			std::scoped_lock lock(shared->mutex);
			node->explicit_value = static_cast<f32>(value);
		}

		vector<string> path() const {
			std::scoped_lock lock(shared->mutex);
			vector<string> result;
			const Node* current = node;
			while (current) {
				if (!current->label.empty()) {
					result.emplace_back(current->label);
				}
				if (current->active.empty()) {
					break;
				}
				current = current->active.back();
			}
			return result;
		}

		void dump_tree() const {}

		// This is only a view. It never owns or completes the active node.
		Progress active() const {
			std::scoped_lock lock(shared->mutex);
			if (!node->active.empty()) return Progress(shared, {}, node->active.back(), false);
			return Progress(shared, {}, node, false);
		}

		f32 value() const {
			std::scoped_lock lock(shared->mutex);
			return value_locked(*node);
		}
		string_view label() const { return node->label; }
		bool valid() const { return node != nullptr; }
		bool cancelled() const { return stop.stop_requested(); }

		void set(string_view name, string_view text, f32 value) {
			std::scoped_lock lock(shared->mutex);
			ProgressEntry& entry = entries[string(name)];
			entry.text = text;
			entry.value = std::clamp(value, 0.0f, 1.0f);
		}
		bool try_get(string_view name, ProgressEntry& entry) const {
			std::scoped_lock lock(shared->mutex);
			auto it = entries.find(string(name));
			if (it == entries.end()) return false;
			entry = it->second;
			return true;
		}

		class Scope {
		  public:
			Scope(Progress& owner, string_view name, u32 count) : owner(owner), name(name), count(std::max(count, 1u)) {}
			void major(string_view text, u32 index) {
				current = std::max(current, std::min(index, count));
				owner.set(name, text, static_cast<f32>(current) / static_cast<f32>(count));
			}
			void minor(string_view text, f32 value) { owner.set("process", text, value); }

		  private:
			Progress& owner;
			string name;
			u32 count;
			u32 current = 0;
		};
		Scope scope(string_view name, u32 count) { return Scope(*this, name, count); }

	  private:
		struct Node {
			string label;
			Node* parent = nullptr;
			std::vector<string> phases;
			std::vector<Node*> active;
			size_t next_phase = 0;
			size_t completed = 0;
			f32 explicit_value = 0.0f;

			~Node() {
				if (parent) parent->child_finished(this);
			}

			void child_finished(Node* child) {
				auto it = std::find(active.begin(), active.end(), child);
				if (it != active.end()) {
					active.erase(it);
					++completed;
				}
			}
		};
		struct Shared {
			std::mutex mutex;
		};

		std::shared_ptr<Shared> shared;
		std::unique_ptr<Node> owned;
		Node* node = nullptr;
		bool notify = true;
		std::unordered_map<string, ProgressEntry> entries;

		Progress(std::shared_ptr<Shared> shared, std::unique_ptr<Node> owned, Node* node, bool notify)
			: shared(std::move(shared)), owned(std::move(owned)), node(node), notify(notify) {}

		static f32 value_locked(const Node& value_node) {
			if (value_node.phases.empty()) return value_node.explicit_value;
			f32 result = static_cast<f32>(value_node.completed);
			for (const Node* child : value_node.active)
				result += value_locked(*child);
			return result / static_cast<f32>(value_node.phases.size());
		}
	};
} // namespace lf
