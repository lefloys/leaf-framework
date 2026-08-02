#pragma once

#include <leaf/core/span.hpp>
#include <leaf/core/types.hpp>
#include <leaf/core/vector.hpp>
#include <leaf/graphics/buffer.hpp>
#include <leaf/graphics/command_buffer.hpp>
#include <leaf/math/pos.hpp>

#include <type_traits>

namespace rt {
	struct canvas_color {
		f32 r = 1.0f;
		f32 g = 1.0f;
		f32 b = 1.0f;
		f32 a = 1.0f;
	};

	template <typename Vertex, typename = void>
	struct canvas_vertex_traits {
		static Vertex make(pos2<f32> position, pos2<f32> uv, canvas_color color) {
			Vertex vertex{};
			if constexpr (requires { vertex.position = position; }) {
				vertex.position = position;
			} else if constexpr (requires { vertex.pos = position; }) {
				vertex.pos = position;
			} else {
				static_assert(sizeof(Vertex) == 0, "Canvas vertex needs rt::canvas_vertex_traits specialization or a position/pos field assignable from lf::pos2<f32>");
			}
			if constexpr (requires { vertex.uv = uv; }) {
				vertex.uv = uv;
			}
			if constexpr (requires { vertex.color = color; }) {
				vertex.color = color;
			}
			return vertex;
		}
	};

	template <typename Vertex>
	class Canvas {
	  public:
		void clear() {
			vertices.clear();
		}

		void reserve(u32 vertex_count) {
			vertices.reserve(vertex_count);
		}

		bool empty() const {
			return vertices.empty();
		}

		u32 vertex_count() const {
			return static_cast<u32>(vertices.size());
		}

		span<const Vertex> vertex_span() const {
			return vertices;
		}

		const vector<Vertex>& vertex_data() const {
			return vertices;
		}

		vector<Vertex>& vertex_data() {
			return vertices;
		}

		void push_vertex(const Vertex& vertex) {
			vertices.push_back(vertex);
		}

		void push_triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
			vertices.push_back(v0);
			vertices.push_back(v1);
			vertices.push_back(v2);
		}

		void draw_rect(f32 x, f32 y, f32 width, f32 height, canvas_color color = {}) {
			draw_rect({ x, y }, { x + width, y + height }, color);
		}

		void draw_rect(pos2<f32> min, pos2<f32> max, canvas_color color = {}) {
			Vertex v0 = canvas_vertex_traits<Vertex>::make({ min.x, min.y }, { 0.0f, 0.0f }, color);
			Vertex v1 = canvas_vertex_traits<Vertex>::make({ max.x, min.y }, { 1.0f, 0.0f }, color);
			Vertex v2 = canvas_vertex_traits<Vertex>::make({ max.x, max.y }, { 1.0f, 1.0f }, color);
			Vertex v3 = canvas_vertex_traits<Vertex>::make({ min.x, max.y }, { 0.0f, 1.0f }, color);
			push_triangle(v0, v1, v2);
			push_triangle(v0, v2, v3);
		}

	  private:
		vector<Vertex> vertices;
	};

	template <typename Vertex>
	class CompiledCanvas {
	  public:
		CompiledCanvas() = default;
		explicit CompiledCanvas(const Canvas<Vertex>& canvas, BufferMode mode = BufferMode::Static) {
			build(canvas, mode);
		}

		void build(const Canvas<Vertex>& canvas, BufferMode mode = BufferMode::Static) {
			build(canvas.vertex_span(), mode);
		}

		void build(span<const Vertex> vertices, BufferMode mode = BufferMode::Static) {
			vertex_count_value = static_cast<u32>(vertices.size());
			if (vertices.empty()) {
				return;
			}
			if (!vertex_buffer) {
				vertex_buffer = unique(Buffer::Create());
			}
			u64 byte_size = static_cast<u64>(vertices.size_bytes());
			Buffer::Data(vertex_buffer, mode, BufferUsage::Vertex, byte_size, vertices.data());
		}

		bool empty() const {
			return vertex_count_value == 0;
		}

		u32 vertex_count() const {
			return vertex_count_value;
		}

		view<buffer> vertices() const {
			return vertex_buffer;
		}

	  private:
		unique<buffer> vertex_buffer;
		u32 vertex_count_value = 0;
	};

	template <typename Vertex>
	CompiledCanvas<Vertex> compile_canvas(const Canvas<Vertex>& canvas, BufferMode mode = BufferMode::Static) {
		return CompiledCanvas<Vertex>(canvas, mode);
	}

	namespace Cmd {
		template <typename Vertex>
		void Draw(view<command_buffer> command_buffer, const CompiledCanvas<Vertex>& canvas) {
			if (canvas.empty()) {
				return;
			}
			BindVertexBuffer(command_buffer, canvas.vertices(), 0);
			Draw(command_buffer, canvas.vertex_count(), 0);
		}
	} // namespace Cmd
} // namespace rt

