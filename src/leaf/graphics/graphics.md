# leaf-framework graphics module

This document describes the public types, enums and functions used by the `leaf` graphics module. 
It assumes modern C++23.

## Types

Objects

- [`lf::handle<T>`](#raw-handle)
- [`lf::view<T>`](#view-handle)
- [`lf::unique<T>`](#unique-handle)
- [`lf::shared<T>`](#shared-handle)
- [`lf::weak<T>`](#weak-handle)

---

Resources

- [`lf::Window`](#window)


---

#### [`lf::handle<T>`](#raw-handle)
A `lf::handle<T>` is a lightweight, non-owning token referencing a dynamically
allocated resource. It is freely copyable but does not manage lifetime.

Returning a `handle<T>` transfers responsibility to the caller, which must be
passed along through the API until the resource is ultimately consumed.
Passing a `handle<T>` to a function transfers responsibility to the callee,
and the caller must not use it afterward unless explicitly documented
otherwise.

Handles do not enforce ownership, but invalid usage—such as double consumption,
use-after-release, or other lifetime violations result in an exception.

For safer usage, handles should be wrapped in `lf::unique<T>` (exclusive
ownership) or `lf::shared<T>` (reference-counted). Direct use is exposed to
allow manual management.

This concept is similar to a dynamically allocated raw pointer.
#### [`lf::Window`](#window)

---

#### [`lf::view<T>`](#view-handle)
A `lf::view<T>` is a lightweight, non-owning reference to an existing resource.
It provides temporary access without participating in lifetime management.

A view does not carry responsibility and must not be used to destroy or
transfer ownership. It is strictly for observing or using a resource owned
elsewhere.

Views are intended for short-lived, transient use, such as passing resources to
functions (e.g., command recording or pipeline setup).

The caller must ensure the underlying resource remains valid for the entire
duration of the view’s use. A view must not outlive the resource it refers to
and should not be stored beyond the scope in which validity is guaranteed.

There is also a `lf::view<const T>` variant for read-only access, which enforces
const-correctness at compile time.

This concept is similar to `std::string_view` or `std::span`.

---

#### [`lf::unique<T>`](#unique-handle)
A `lf::unique<T>` is a move-only smart handle that exclusively owns a resource.
It automatically releases the resource when it goes out of scope, ensuring
proper cleanup.

A `unique<T>` cannot be copied, only moved. Moving transfers ownership,
leaving the source in a valid but unspecified state (typically empty or null).

`unique<T>` is ideal for resources with clear ownership semantics, such as
buffers, textures, or windows that should be automatically cleaned up when no
longer needed.

This concept is similar to `std::unique_ptr<T>`

---

#### [`lf::shared<T>`](#shared-handle)
A `lf::shared<T>` is a reference-counted smart handle that allows multiple
owners of a resource. It automatically releases the resource when the last
owner goes out of scope.

A `shared<T>` can be copied, which increments the reference count. Moving a
`shared<T>` transfers ownership without changing the reference count.

`shared<T>` is useful for resources that may be shared across different parts
of an application, such as a texture used by multiple objects or a window
accessed by various systems.

This concept is similar to `std::shared<T>`

---

#### [`lf::weak<T>`](#weak-handle)
A `lf::weak<T>` is a non-owning reference to a resource managed by `lf::shared<T>`.

It provides temporary access to a resource without affecting its reference
count and without extending its lifetime.

A `weak<T>` does not guarantee that the resource is still alive. It must be
upgraded to a `lf::shared<T>` before use. If the resource has already been
destroyed, the upgrade fails and yields an empty or invalid result.

`weak<T>` is used to break reference cycles and to observe shared resources
without contributing to ownership.---




- `lf::Window`: platform window abstraction.
- `lf::Framebuffer`: render target abstraction.

### Buffers
- `lf::VertexBuffer`
- `lf::IndexBuffer`
- `lf::UniformBuffer`
- `lf::StorageBuffer`


### Command pipeline
- `lf::Commandbuffer`
- `lf::CommandPool`
- `lf::Queue`
- `lf::QueueTimepoint`: lightweight timeline value returned from `Queue::Submit` used for synchronization. A `QueueTimepoint` can be waited on from any `lf::Queue` on the same device or consumed by a `CommandBuffer` for GPU-side waits.


### Shaders
- `lf::VertexShader`
- `lf::FragmentShader`
- `lf::GeometryShader`
- `lf::GraphicsPipeline`
- `lf::ComputePipeline`


### Textures
- `lf::Texture1D`
- `lf::Texture2D`
- `lf::Texture3D`


### Rendering Helpers
- `lf::Canvas` : a high-level rendering abstraction that provides a simplified API for common 2D drawing operations (e.g., drawing shapes, text, images) on top of the lower-level graphics primitives. The `Canvas` can be used for UI rendering or simple 2D graphics without needing to manage shaders, buffers, and draw calls directly. It internally manages its own resources and command buffers to provide an easy-to-use interface for 2D rendering tasks.
 

## Enums
- `lf::buffer_usage`: enum describing intended GPU usage (`static_draw, dynamic_draw, stream_draw`). Note: `Buffer::Data` is allowed to change the usage flags of an existing buffer — usage is not necessarily immutable.
- `lf::texture_format`: enum describing texture formats (e.g., `rgba8_unorm`, `rgba16_float`, `depth24_stencil8`, etc.). The API should use a consistent set of format enums across texture creation and data upload/download to avoid confusion.
- `lf::draw_mode`: enum describing primitive types for draw calls (e.g., `triangles`, `lines`, `points`, etc.).
- `lf::queue_flag_bits`: enum describing queue capabilities (e.g., `graphics`, `compute`, `transfer`).


## Helpers
- `std::span<std::byte> lf::to_bytes(...)`

Example:
```cpp
auto data_bytes = lf::to_bytes(vertices); // returns std::span<std::byte>
lf::VertexBuffer::Data(vbo, data_bytes, lf::buffer_usage::dynamic_draw);
```


## Error handling
- Some functions may return an `error` type to indicate failure. This is for expected errors.
- Most functions may throw exceptions. This is for unexpected errors.
- The API is designed to never make exception handling explicit unless in the handler. error results must be handled


## Functions and behavior notes
Most resource types expose a static `Create` factory that returns a handle
```cpp
lf::handle<lf::VertexBuffer> vbo = lf::VertexBuffer::Create();
lf::unique<lf::IndexBuffer> uebo = lf::unique(lf::IndexBuffer::Create())`
```


### Buffer functions
- `static void Buffer::Data(lf::view<Buffer> buf, std::span<std::byte> data, lf::buffer_usage usage, size_t offset = 0)`
- `static size_t Buffer::GetSize(lf::view<const Buffer> buf)` : returns the size in bytes of the buffer.
- `static size_t Buffer::GetCapacity(lf::view<const Buffer> buf)` : returns the capacity in bytes of the buffer.
- `static void Buffer::Reserve(lf::view<Buffer> buf, size_t size, lf::buffer_usage usage)`


### Framebuffer functions
- `static lf::view<const lf::Texture2D> Framebuffer::GetColorAttachment(lf::view<const Framebuffer> fb, u32 index)` : returns a view to the color attachment texture at the specified index. The returned texture is owned by the framebuffer and should not be modified or destroyed by the caller.


### Window functions
- `static void Window::Show(lf::view<Window> wnd)` : show the window.
- `static void Window::Hide(lf::view<Window> wnd)` : hide the window.
- `static void Window::Resize(lf::view<Window> wnd, dim2<u32> dim)` : set window size in pixels. 
- `static dim2<u32> Window::GetSize(lf::view<Window> wnd)` : returns the current size of the window in pixels.
- `static void Window::SetInputHandler(InputHandler& handler)` : register an input handler. The handler lifetime is managed by the caller; the window stores only a non-owning reference.
- `static lf::view<lf::Framebuffer> Window::BeginFrame(lf::view<Window> wnd)` : begin a new frame and return a framebuffer view for rendering. The framebuffer is valid until the next call to `BeginFrame` on the same window. This function may also handle platform-specific frame preparation (e.g., acquiring swapchain images).
- `static void Window::EndFrame(lf::view<Window> wnd)` : end the current frame and present the rendered content. This function may also handle platform-specific frame finalization (e.g., presenting swapchain images).)


### Shader and pipeline functions
- `static void lf::GraphicsPipeline::Attach(lf::view<GraphicsPipeline> pipeline, lf::view<const Shader> shader)`
- `static error lf::GraphicsPipeline::Link(lf::view<GraphicsPipeline> pipeline)`


### Command buffers, queues and synchronization
- `static lf::view<lf::CommandBuffer> lf::CommandPool::Allocate(lf::view<lf::CommandPool> pool)` : allocate a command buffer from the command pool.
- `static lf::QueueTimepoint lf::Queue::Submit(lf::view<Queue> queue, lf::view<const CommandBuffer> cmd)` : submit a command buffer to the queue and return a `QueueTimepoint` that can be used for synchronization.
- `static void lf::CommandBuffer::WaitFor(lf::view<lf::CommandBuffer> cmd, lf::QueueTimepoint timepoint)` : block the command buffer until the specified timepoint has been reached on the GPU. This can be used to synchronize between command buffers or to wait for a submitted command buffer to finish before reusing it.
- `static void lf::CommandBuffer::Draw(lf::view<lf::CommandBuffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)` : record a draw command into the command buffer.
- Various other draw functions with similar parameters for indexed draws, indirect draws, etc.
- `static void lf::CommandBuffer::Begin(lf::view<lf::CommandBuffer> cmd)`
- `static void lf::CommandBuffer::BeginRendering(lf::view<lf::CommandBuffer> cmd, lf::view<Framebuffer> fb)`
- `static void lf::CommandBuffer::EndRendering(lf::view<lf::CommandBuffer> cmd)`
- `static void lf::CommandBuffer::End(lf::view<lf::CommandBuffer> cmd)`
- `static void lf::CommandBuffer::Reset(lf::view<lf::CommandBuffer> cmd)`
- `void lf::QueueTimepoint::wait(QueueTimepoint* this)` : block cpu execution until the timepoint has been reached on the GPU. This can be used to synchronize CPU work with GPU progress, such as waiting for a submitted command buffer to finish before reusing resources.
- `static void lf::Queue::Acquire(lf::queue_flag_bits`


### Texture functions
We have an Image type that can be used for staging texture data on the CPU side,
and then a `Texture::Upload` function to copy from an Image to a GPU texture. 
This keeps the texture API focused on GPU resources and allows for flexible 
CPU-side image manipulation before uploading.
```cpp

struct Image1D {
	texture_format format;
	std::vector<std::byte> data;
};
struct Image2D {
	dim2<u32> size;
	texture_format format;
	std::vector<std::byte> data;
};
struct Image3D {
	dim3<u32> size;
	texture_format format;
	std::vector<std::byte> data;
}
```
- `static void Texture1D::Reserve(lf::view<Texture> tex, u32 extent, u32 layers, u32 mip_levels)`
- `static void Texture2D::Reserve(lf::view<Texture> tex, dim2<u32> extent, u32 layers, u32 mip_levels)`
- `static void Texture3D::Reserve(lf::view<Texture> tex, dim3<u32> extent, u32 layers, u32 mip_levels)`

- `static void Texture1D::Upload(lf::view<Texture> tex, const Image1D& img, u32 mip_level)` : upload image data to the specified mip level of the texture. 
- `static void Texture2D::Upload(lf::view<Texture> tex, const Image2D& img, u32 mip_level)` : upload image data to the specified mip level of the texture. 
- `static void Texture3D::Upload(lf::view<Texture> tex, const Image3D& img, u32 mip_level)` : upload image data to the specified mip level of the texture. 
- `static Image1D Texture1D::Download(lf::view<const Texture1D> tex, u32 mip_level)` : download image data
- `static Image2D Texture2D::Download(lf::view<const Texture2D> tex, u32 mip_level)` : download image data
- `static Image3D Texture3D::Download(lf::view<const Texture3D> tex, u32 mip_level)` : download image data
