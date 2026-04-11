# leaf-framework graphics module

This document describes the public types, enums and functions used by the `leaf` graphics module. It assumes modern C++ (C++23) idioms and RAII ownership patterns.

## Types
- `lf::handle<T>`: opaque low-level handle to an implementation object (non-owning view to a resource handle).
- `lf::unique<T>`: RAII-owned, non-copyable owning handle (single ownership). Destruction frees the underlying resource.
- `lf::shared<T>`: reference-counted owning handle. Multiple owners may hold a `shared` to keep a resource alive.
- `lf::weak<T>`: non-owning observation of a `shared` handle. Can be promoted to `shared` if the resource is still alive.
- `lf::view<T>`: non-owning, lightweight view used for API parameters where the callee does not take ownership or extend lifetime.
- `lf::Window`: platform window abstraction.
- `lf::FrameBuffer` : render target abstraction.


### Buffers
- `lf::VertexBuffer`
- `lf::IndexBuffer`
- `lf::UniformBuffer`
- `lf::StorageBuffer`


### Command pipeline
- `lf::CommandBuffer`
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
- `std::span<std::byte> lf::to_bytes(...)` helper: convert typed trivially-copyable data or contiguous containers into `std::span<std::byte>` for passing to `Buffer::Data`. This avoids overload proliferation and keeps the buffer API uniform. The helper requires that the source is safely representable as a contiguous sequence of bytes (e.g., a POD/trivially-copyable type or a std::span).

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
- `void Buffer::Data(lf::view<Buffer> buf, std::span<std::byte> data, lf::buffer_usage usage, size_t offset = 0)`
- `size_t Buffer::GetSize(lf::view<const Buffer> buf)` : returns the size in bytes of the buffer.
- `void Buffer::Reserve(lf::view<Buffer> buf, size_t size, lf::buffer_usage usage)` : allocate or reallocate a buffer to the specified size and usage. if reallocation happens, data will be copied. if size is smaller than before, buffer does not get reallocated


### Framebuffer functions
- `lf::view<const lf::Texture2D> Framebuffer::GetColorAttachment(lf::view<const Framebuffer> fb, u32 index)` : returns a view to the color attachment texture at the specified index. The returned texture is owned by the framebuffer and should not be modified or destroyed by the caller.


### Window functions
- `void Window::Show(lf::view<Window> wnd)` : show the window.
- `void Window::Hide(lf::view<Window> wnd)` : hide the window.
- `void Window::Resize(lf::view<Window> wnd, dim2<u32> dim)` : set window size in pixels. 
- `dim2<u32> Window::GetSize(lf::view<Window> wnd)` : returns the current size of the window in pixels.
- `void Window::SetInputHandler(InputHandler& handler)` : register an input handler. The handler lifetime is managed by the caller; the window stores only a non-owning reference.
- `lf::view<lf::Framebuffer> Window::BeginFrame(lf::view<Window> wnd)` : begin a new frame and return a framebuffer view for rendering. The framebuffer is valid until the next call to `BeginFrame` on the same window. This function may also handle platform-specific frame preparation (e.g., acquiring swapchain images).
- `void Window::EndFrame(lf::view<Window> wnd)` : end the current frame and present the rendered content. This function may also handle platform-specific frame finalization (e.g., presenting swapchain images).)


### Shader and pipeline functions
- `void lf::GraphicsPipeline::Attach(lf::view<GraphicsPipeline> pipeline, lf::view<const Shader> shader)`
- `error lf::GraphicsPipeline::Link(lf::view<GraphicsPipeline> pipeline)`


### Command buffers, queues and synchronization
- `lf::view<lf::CommandBuffer> lf::CommandPool::Allocate(lf::view<lf::CommandPool> pool)` : allocate a command buffer from the command pool.
- `lf::QueueTimepoint lf::Queue::Submit(lf::view<Queue> queue, lf::view<const CommandBuffer> cmd)` : submit a command buffer to the queue and return a `QueueTimepoint` that can be used for synchronization.
- `void CommandBuffer::WaitFor(lf::view<lf::CommandBuffer> cmd, lf::QueueTimepoint timepoint)` : block the command buffer until the specified timepoint has been reached on the GPU. This can be used to synchronize between command buffers or to wait for a submitted command buffer to finish before reusing it.
- `void CommandBuffer::Draw(lf::view<lf::CommandBuffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)` : record a draw command into the command buffer.
- Various other draw functions with similar parameters for indexed draws, indirect draws, etc.
- `void lf::CommandBuffer::Begin(lf::view<lf::CommandBuffer> cmd)`
- `void lf::CommandBuffer::BeginRendering(lf::view<lf::CommandBuffer> cmd, lf::view<Framebuffer> fb)`
- `void lf::CommandBuffer::EndRendering(lf::view<lf::CommandBuffer> cmd)`
- `void lf::CommandBuffer::End(lf::view<lf::CommandBuffer> cmd)`
- `void lf::CommandBuffer::Reset(lf::view<lf::CommandBuffer> cmd)`
- `void lf::QueueTimepoint::wait(QueueTimepoint* this)` : block cpu execution until the timepoint has been reached on the GPU. This can be used to synchronize CPU work with GPU progress, such as waiting for a submitted command buffer to finish before reusing resources.
- `void lf::Queue::Acquire(lf::queue_flag_bits`


### Texture functions
- We have an Image type that can be used for staging texture data on the CPU side, and then a `Texture::Upload` function to copy from an Image to a GPU texture. This keeps the texture API focused on GPU resources and allows for flexible CPU-side image manipulation before uploading.
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
- `void Texture1D::Reserve(lf::view<Texture> tex, u32 extent, u32 layers, u32 mip_levels)`
- `void Texture2D::Reserve(lf::view<Texture> tex, dim2<u32> extent, u32 layers, u32 mip_levels)`
- `void Texture3D::Reserve(lf::view<Texture> tex, dim3<u32> extent, u32 layers, u32 mip_levels)`

- `void Texture1D::Upload(lf::view<Texture> tex, const Image1D& img, u32 mip_level)` : upload image data to the specified mip level of the texture. 
- `void Texture2D::Upload(lf::view<Texture> tex, const Image2D& img, u32 mip_level)` : upload image data to the specified mip level of the texture. 
- `void Texture3D::Upload(lf::view<Texture> tex, const Image3D& img, u32 mip_level)` : upload image data to the specified mip level of the texture. 
- `Image1D Texture1D::Download(lf::view<const Texture1D> tex, u32 mip_level)` : download image data
- `Image2D Texture2D::Download(lf::view<const Texture2D> tex, u32 mip_level)` : download image data
- `Image3D Texture3D::Download(lf::view<const Texture3D> tex, u32 mip_level)` : download image data
