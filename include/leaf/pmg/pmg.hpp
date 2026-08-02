#pragma once

#include <array>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>

#include <leaf/core/exception.hpp>
#include <leaf/core/format.hpp>
#include <leaf/core/types.hpp>
#include <leaf/core/vector.hpp>

namespace lf::pmg {
	template <typename Vertex>
	struct scratch_triangle;

	struct options {
		u32 staging_capacity = 64 * 1024;
		u32 vertex_history_capacity = 4096;
	};

	struct count_source {
		u32 count = 0;
	};

	struct grid_source {
		u32 width = 0;
		u32 height = 0;
	};

	template <typename Vertex>
	struct triangle_builder {
		Vertex& v0() {
			slot_modes[0] = slot_mode::new_vertex;
			return vertices[0];
		}
		Vertex& v1() {
			slot_modes[1] = slot_mode::new_vertex;
			return vertices[1];
		}
		Vertex& v2() {
			slot_modes[2] = slot_mode::new_vertex;
			return vertices[2];
		}

		const Vertex& v0() const {
			return vertices[0];
		}
		const Vertex& v1() const {
			return vertices[1];
		}
		const Vertex& v2() const {
			return vertices[2];
		}

		void repush_v0(u32 reverse_vertex_index) {
			repush(0, reverse_vertex_index);
		}
		void repush_v1(u32 reverse_vertex_index) {
			repush(1, reverse_vertex_index);
		}
		void repush_v2(u32 reverse_vertex_index) {
			repush(2, reverse_vertex_index);
		}

	  private:
		enum class slot_mode {
			new_vertex,
			repush,
		};

		friend struct scratch_triangle<Vertex>;
		template <typename>
		friend class writer;
		template <typename, typename>
		friend class indexed_writer;

		void repush(u32 slot, u32 reverse_vertex_index) {
			slot_modes[slot] = slot_mode::repush;
			repush_indices[slot] = reverse_vertex_index;
		}

		Vertex* vertices = nullptr;
		std::array<slot_mode, 3> slot_modes{ slot_mode::new_vertex, slot_mode::new_vertex,
											 slot_mode::new_vertex };
		std::array<u32, 3> repush_indices{};
	};

	template <typename Vertex>
	struct scratch_triangle {
		scratch_triangle() {
			construct();
		}
		scratch_triangle(const scratch_triangle&) = delete;
		scratch_triangle& operator=(const scratch_triangle&) = delete;
		~scratch_triangle() {
			destroy();
		}

		triangle_builder<Vertex>& reset() {
			destroy();
			construct();
			return builder;
		}

	  private:
		using storage_type = std::aligned_storage_t<sizeof(Vertex), alignof(Vertex)>;

		void construct() {
			for (storage_type& slot : storage) {
				std::construct_at(reinterpret_cast<Vertex*>(&slot));
			}
			constructed = true;
			builder.vertices = reinterpret_cast<Vertex*>(storage.data());
			builder.slot_modes = { triangle_builder<Vertex>::slot_mode::new_vertex,
								   triangle_builder<Vertex>::slot_mode::new_vertex,
								   triangle_builder<Vertex>::slot_mode::new_vertex };
			builder.repush_indices = {};
		}

		void destroy() {
			if (!constructed) {
				return;
			}
			for (storage_type& slot : storage) {
				std::destroy_at(reinterpret_cast<Vertex*>(&slot));
			}
			constructed = false;
		}

		std::array<storage_type, 3> storage{};
		bool constructed = false;
		triangle_builder<Vertex> builder{};
	};

	namespace detail {
		template <typename Source>
		u32 triangle_count(const Source& source) {
			if constexpr (requires { source.triangle_count(); }) {
				return source.triangle_count();
			} else if constexpr (requires { source.count; }) {
				return source.count;
			} else if constexpr (requires { source.width; source.height; }) {
				return source.width * source.height * 2;
			} else {
				static_assert(sizeof(Source) == 0,
							  "pmg triangle source must expose triangle_count(), count, or width/height");
			}
		}

		template <typename Source, typename Vertex>
		void prepare_triangle(const Source& source, u32 index, triangle_builder<Vertex>& builder) {
			if constexpr (requires { source.prepare_triangle(index, builder); }) {
				source.prepare_triangle(index, builder);
			} else {
				(void)source;
				(void)index;
				(void)builder;
			}
		}

	template <typename Vertex, typename Index>
	struct staging_storage {
		vector<u08> vertex_bytes;
		vector<u08> index_bytes;
	};

		template <typename Vertex, typename Index>
		staging_storage<Vertex, Index>& tls_storage() {
			thread_local staging_storage<Vertex, Index> storage;
			return storage;
		}
	} // namespace detail

	template <typename Vertex>
	class writer {
	  public:
		explicit writer(vector<u08>& vertex_bytes, options opts = {})
			: vertex_bytes(&vertex_bytes), opts(opts) {
			static_assert(std::is_trivially_copyable_v<Vertex>,
						  "pmg writers serialize Vertex with memcpy, so Vertex must be trivially copyable");
		}

		void submit(const triangle_builder<Vertex>& triangle) {
			std::array<std::array<u08, sizeof(Vertex)>, 3> submitted_vertices{};
			const u32 base_history_count = history_count();
			for (u32 slot = 0; slot < 3; ++slot) {
				if (triangle.slot_modes[slot] == triangle_builder<Vertex>::slot_mode::repush) {
					const u08* bytes = history_vertex(base_history_count, triangle.repush_indices[slot]);
					std::memcpy(submitted_vertices[slot].data(), bytes, sizeof(Vertex));
				} else {
					std::memcpy(submitted_vertices[slot].data(), &triangle.vertices[slot],
								sizeof(Vertex));
				}
			}
			ensure_vertex_capacity(3 * sizeof(Vertex));
			auto& storage = detail::tls_storage<Vertex, u32>();
			for (const auto& vertex : submitted_vertices) {
				storage.vertex_bytes.insert(storage.vertex_bytes.end(), vertex.begin(), vertex.end());
			}
			for (const auto& vertex : submitted_vertices) {
				remember_vertex(vertex.data());
			}
		}

		void repush(u32 reverse_vertex_index) {
			submit_repush(reverse_vertex_index);
		}

		void flush() {
			auto& storage = detail::tls_storage<Vertex, u32>();
			if (!storage.vertex_bytes.empty()) {
				vertex_bytes->insert(vertex_bytes->end(), storage.vertex_bytes.begin(),
									 storage.vertex_bytes.end());
				storage.vertex_bytes.clear();
			}
		}

		u32 vertex_count() const {
			auto& storage = detail::tls_storage<Vertex, u32>();
			return static_cast<u32>(vertex_bytes->size() / sizeof(Vertex) +
									storage.vertex_bytes.size() / sizeof(Vertex));
		}

	  private:
		void ensure_vertex_capacity(u32 bytes) {
			auto& storage = detail::tls_storage<Vertex, u32>();
			if (storage.vertex_bytes.size() + bytes > opts.staging_capacity) {
				flush();
			}
			if (storage.vertex_bytes.capacity() < opts.staging_capacity) {
				storage.vertex_bytes.reserve(opts.staging_capacity);
			}
		}

		void remember_vertex(const void* vertex) {
			const u32 vertex_size = sizeof(Vertex);
			const u32 max_bytes = opts.vertex_history_capacity * vertex_size;
			if (vertex_history.size() + vertex_size > max_bytes) {
				const u32 remove_bytes = vertex_history.size() + vertex_size - max_bytes;
				const u32 aligned_remove = ((remove_bytes + vertex_size - 1) / vertex_size) * vertex_size;
				vertex_history.erase(vertex_history.begin(), vertex_history.begin() + aligned_remove);
			}
			const auto* bytes = static_cast<const u08*>(vertex);
			vertex_history.insert(vertex_history.end(), bytes, bytes + vertex_size);
		}

		u32 history_count() const {
			return static_cast<u32>(vertex_history.size() / sizeof(Vertex));
		}

		const u08* history_vertex(u32 history_count, u32 reverse_vertex_index) const {
			const u32 vertex_size = sizeof(Vertex);
			if (reverse_vertex_index >= history_count) {
				throw out_of_range_exception(lf::format("pmg vertex repush index {} out of range ({} vertices in history)",
													reverse_vertex_index, history_count));
			}
			const u32 vertex_index = history_count - reverse_vertex_index - 1;
			return vertex_history.data() + vertex_index * vertex_size;
		}

		void submit_vertex(const Vertex* vertex) {
			const u32 vertex_size = sizeof(Vertex);
			ensure_vertex_capacity(vertex_size);
			auto& storage = detail::tls_storage<Vertex, u32>();
			const auto* bytes = reinterpret_cast<const u08*>(vertex);
			storage.vertex_bytes.insert(storage.vertex_bytes.end(), bytes, bytes + vertex_size);
			remember_vertex(vertex);
		}

		void submit_repush(u32 reverse_vertex_index) {
			const u32 vertex_size = sizeof(Vertex);
			ensure_vertex_capacity(vertex_size);
			auto& storage = detail::tls_storage<Vertex, u32>();
			const u08* bytes = history_vertex(history_count(), reverse_vertex_index);
			storage.vertex_bytes.insert(storage.vertex_bytes.end(), bytes, bytes + vertex_size);
			remember_vertex(bytes);
		}

		vector<u08>* vertex_bytes = nullptr;
		vector<u08> vertex_history;
		options opts{};
	};

	template <typename Vertex, typename Index = u32>
	class indexed_writer {
	  public:
		indexed_writer(vector<u08>& vertex_bytes, vector<u08>& index_bytes, options opts = {})
			: vertex_bytes(&vertex_bytes), index_bytes(&index_bytes), opts(opts) {
			static_assert(std::is_trivially_copyable_v<Vertex>,
						  "pmg writers serialize Vertex with memcpy, so Vertex must be trivially copyable");
			static_assert(std::is_integral_v<Index>, "pmg index type must be integral");
		}

		void submit(const triangle_builder<Vertex>& triangle) {
			ensure_capacity(3 * sizeof(Vertex), 3 * sizeof(Index));
			const u32 base_history_count = history_count();
			const Index base_vertex = static_cast<Index>(vertex_count());
			u32 new_vertex_count = 0;
			std::array<Index, 3> submitted_indices{};
			for (u32 slot = 0; slot < 3; ++slot) {
				if (triangle.slot_modes[slot] == triangle_builder<Vertex>::slot_mode::repush) {
					submitted_indices[slot] =
						history_index(base_history_count, triangle.repush_indices[slot]);
				} else {
					submitted_indices[slot] = static_cast<Index>(base_vertex + new_vertex_count);
					stage_vertex(&triangle.vertices[slot]);
					++new_vertex_count;
				}
			}
			for (Index index : submitted_indices) {
				stage_index(index);
			}
			for (Index index : submitted_indices) {
				remember_index(index);
			}
		}

		void repush(u32 reverse_vertex_index) {
			submit_repush(reverse_vertex_index);
		}

		void flush() {
			auto& storage = detail::tls_storage<Vertex, Index>();
			if (!storage.vertex_bytes.empty()) {
				vertex_bytes->insert(vertex_bytes->end(), storage.vertex_bytes.begin(),
									 storage.vertex_bytes.end());
				storage.vertex_bytes.clear();
			}
			if (!storage.index_bytes.empty()) {
				index_bytes->insert(index_bytes->end(), storage.index_bytes.begin(),
									storage.index_bytes.end());
				storage.index_bytes.clear();
			}
		}

		u32 vertex_count() const {
			auto& storage = detail::tls_storage<Vertex, Index>();
			return static_cast<u32>(vertex_bytes->size() / sizeof(Vertex) +
									storage.vertex_bytes.size() / sizeof(Vertex));
		}

		u32 index_count() const {
			auto& storage = detail::tls_storage<Vertex, Index>();
			return static_cast<u32>(index_bytes->size() / sizeof(Index) +
									storage.index_bytes.size() / sizeof(Index));
		}

	  private:
		void ensure_capacity(u32 vertex_bytes_needed, u32 index_bytes_needed) {
			auto& storage = detail::tls_storage<Vertex, Index>();
			if (storage.vertex_bytes.size() + vertex_bytes_needed > opts.staging_capacity ||
				storage.index_bytes.size() + index_bytes_needed > opts.staging_capacity) {
				flush();
			}
			if (storage.vertex_bytes.capacity() < opts.staging_capacity) {
				storage.vertex_bytes.reserve(opts.staging_capacity);
			}
			if (storage.index_bytes.capacity() < opts.staging_capacity) {
				storage.index_bytes.reserve(opts.staging_capacity);
			}
		}

		void remember_index(Index index) {
			if (index_history.size() == opts.vertex_history_capacity) {
				index_history.erase(index_history.begin());
			}
			index_history.push_back(index);
		}

		u32 history_count() const {
			return static_cast<u32>(index_history.size());
		}

		Index history_index(u32 history_count, u32 reverse_vertex_index) const {
			if (reverse_vertex_index >= history_count) {
				throw out_of_range_exception(lf::format("pmg vertex repush index {} out of range ({} vertices in history)",
													reverse_vertex_index, history_count));
			}
			return index_history[history_count - reverse_vertex_index - 1];
		}

		void stage_index(Index index) {
			auto& storage = detail::tls_storage<Vertex, Index>();
			const auto* bytes = reinterpret_cast<const u08*>(&index);
			storage.index_bytes.insert(storage.index_bytes.end(), bytes, bytes + sizeof(Index));
		}

		void stage_vertex(const Vertex* vertex) {
			auto& storage = detail::tls_storage<Vertex, Index>();
			const auto* bytes = reinterpret_cast<const u08*>(vertex);
			storage.vertex_bytes.insert(storage.vertex_bytes.end(), bytes, bytes + sizeof(Vertex));
		}

		void submit_vertex(const Vertex* vertex) {
			ensure_capacity(sizeof(Vertex), sizeof(Index));
			const Index index = static_cast<Index>(vertex_count());
			stage_vertex(vertex);
			stage_index(index);
			remember_index(index);
		}

		void submit_repush(u32 reverse_vertex_index) {
			ensure_capacity(0, sizeof(Index));
			const Index index = history_index(history_count(), reverse_vertex_index);
			stage_index(index);
			remember_index(index);
		}

		vector<u08>* vertex_bytes = nullptr;
		vector<u08>* index_bytes = nullptr;
		vector<Index> index_history;
		options opts{};
	};

	template <typename Vertex, typename Writer, typename Source>
	class triangle_range {
	  public:
		triangle_range(Writer& writer, const Source& source, options opts)
			: writer(&writer), source(&source), opts(opts), count(detail::triangle_count(source)) {}

		triangle_range(const triangle_range&) = delete;
		triangle_range& operator=(const triangle_range&) = delete;
		triangle_range(triangle_range&&) = delete;
		triangle_range& operator=(triangle_range&&) = delete;

		~triangle_range() {
			writer->flush();
		}

		class iterator {
		  public:
			iterator() = default;
			iterator(triangle_range* range, u32 index) : range(range), index(index) {
				if (range != nullptr && index < range->count) {
					prepare();
				}
			}

			triangle_builder<Vertex>& operator*() {
				return *builder;
			}

			iterator& operator++() {
				range->writer->submit(*builder);
				++index;
				if (index < range->count) {
					prepare();
				} else {
					range = nullptr;
					builder = nullptr;
				}
				return *this;
			}

			bool operator!=(const iterator& other) const {
				return range != other.range || index != other.index;
			}

		  private:
			void prepare() {
				builder = &range->scratch.reset();
				detail::prepare_triangle(*range->source, index, *builder);
			}

			triangle_range* range = nullptr;
			u32 index = 0;
			triangle_builder<Vertex>* builder = nullptr;
		};

		iterator begin() {
			if (count == 0) {
				return end();
			}
			return iterator(this, 0);
		}

		iterator end() {
			return iterator(nullptr, count);
		}

	  private:
		Writer* writer = nullptr;
		const Source* source = nullptr;
		options opts{};
		u32 count = 0;
		scratch_triangle<Vertex> scratch{};
	};

	template <typename Vertex, typename Writer, typename Source>
	triangle_range<Vertex, Writer, Source> triangles(Writer& writer, const Source& source,
													 options opts = {}) {
		return triangle_range<Vertex, Writer, Source>(writer, source, opts);
	}
} // namespace lf::pmg

