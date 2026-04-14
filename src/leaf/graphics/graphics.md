# leaf-framework graphics module

This document describes the public types, enums and functions used by the `leaf` graphics module. It assumes modern C++23.

---

## Types
* [`lf::handle<T>`](#handle)
* [`lf::view<T>`](#view)
* [`lf::unique<T>`](#unique)
* [`lf::shared<T>`](#shared)
* [`lf::weak<T>`](#weak)
* [`lf::QueueTimepoint`](#queue-timepoint)

---

## Resources
### Target
* [`lf::Window`](#window)
* [`lf::Framebuffer`](#framebuffer)

### [Buffers](#buffer)
* [`lf::VertexBuffer`](#vertex-buffer)
* [`lf::IndexBuffer`](#index-buffer)
* [`lf::UniformBuffer`](#uniform-buffer)
* [`lf::StorageBuffer`](#storage-buffer)

### [Command Pipeline](#command-pipeline)
* [`lf::CommandBuffer`](#command-buffer)
* [`lf::CommandPool`](#command-pool)
* [`lf::Queue`](#queue)

### Shaders and Pipelines
* [`lf::GraphicsPipeline`](#graphics-pipeline)
* [`lf::ComputePipeline`](#compute-pipeline)

### [Textures](#textures)
* [`lf::Texture1D`](#texture1d)
* [`lf::Texture2D`](#texture2d)
* [`lf::Texture3D`](#texture3d)

## [Enums](#enums)
* [`lf::buffer_usage`](#buffer-usage)
* [`lf::texture_format`](#texture-format)
* [`lf::draw_mode`](#draw-mode)
* [`lf::queue_flag_bits`](#queue-flag-bits)

## Helpers
* [`std::span<std::byte> lf::to_bytes(...)`](#to-bytes)

---

## Error handling
Most errors are propogated using exceptions. However, a select few functions that error
in expected ways (eg shader compilation, reading a file) return an `lf::error`, `lf::result` or `lf::report`
`lf::error` is a wrapper around std::error_code + std::string, and 
`lf::result<T>` is a wrapper around `std::expected<T, lf::error_code>`. 
`lf::report` is a wrapper around `std::expected<void, lf::error>`.


Error messages in leaf are constructed incrementally. For example a shader compilation error might look like this:
`error : failed to compile shader : <shader_log>`

Exceptions thrown by leaf will contain a stack trace and a message. The message is constructed incrementally, 
so it may be empty at the point of throwing, but will be filled in as the exception propagates up the call stack.
If you want to use this approach too, use the following pattern:
```cpp
try {
    // code that may throw
} catch (...) {
    lf::throw_with_context("additional context message");
}
```

---
## Types
#### Handle

A `lf::handle<T>` is a lightweight, non-owning token referencing a dynamically 
allocated resource. It is freely copyable but does not manage lifetime.

Returning a `lf::handle<T>` transfers responsibility to the caller, which must
be passed along through the API until the resource is ultimately consumed. 
Passing a `lf::handle<T>` to a function transfers responsibility to the callee,
and the caller must not use it afterward unless explicitly documented otherwise.
This is not enforced by the type system. However, the leaf API is designed to
follow this convention consistently.

This seems like a duplication of `lf::view<T>` but it is not. A handle is a 
token that represents ownership, while a view is a reference that represents 
access. A handle can be used to create views, but a view cannot be used to 
create handles. Again, this is not enforced by the type system, but it is a 
convention that should be followed.

Handles do not enforce ownership, but invalid usage—such as double consumption,
use-after-release, or other lifetime violations result in an exception.


For safer usage, handles should be wrapped in `lf::unique<T>` (exclusive ownership)
or `lf::shared<T>` (reference-counted). Direct use is exposed to allow manual management.


This concept is similar to a dynamically allocated raw pointer.

---
#### View

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

There is also a `lf::view<const T>` variant for read-only access, which 
enforces
const-correctness at compile time.

This concept is similar to `std::string_view` or `std::span`.

---
#### Unique

A `lf::unique<T>` is a move-only smart handle that exclusively owns a resource.
It automatically 
releases the resource when it goes out of scope.

A `lf::unique<T>` cannot be copied, only moved. Moving transfers ownership, 
leaving the source in a
valid but unspecified state.

`lf::unique<T>` is ideal for resources with clear ownership semantics, such as 
buffers, textures, 
or windows that should be automatically cleaned up when no longer needed.

This concept is similar to `std::unique_ptr<T>`.

---
#### Shared
A `lf::shared<T>` is a reference-counted smart handle that allows multiple owners
of a resource. It automatically releases the resource when the last owner goes
out of scope.

A `lf::shared<T>` can be copied, which increments the reference count. Moving a
`lf::shared<T>` transfers ownership without changing the reference count.

`lf::shared<T>` is useful for resources that may be shared across different 
parts of an application, such as a texture used by multiple objects or a window
accessed by various systems.

This concept is similar to `std::shared_ptr<T>`.

---
#### Weak

A `lf::weak<T>` is a non-owning reference to a resource managed by 
`lf::shared<T>`.

It provides temporary access to a resource without affecting its reference 
count and without extending its lifetime.

A `lf::weak<T>` does not guarantee that the resource is still alive. It must be
upgraded to a `lf::shared<T>` before use. If the resource has already been 
destroyed, the upgrade fails and yields an empty or invalid result.

`lf::weak<T>` is used to break reference cycles and to observe shared resources
without contributing to ownership.

---
#### QueueTimepoint
A `lf::QueueTimepoint` represents a specific point in time on a GPU queue. It 
is used for synchronization, allowing the CPU or GPU to wait for certain 
operations to complete before proceeding.

A CPU wait can be achieved by calling `timepoint.wait()`, which blocks the CPU 
until the associated GPU operations have finished. This is useful for ensuring 
that resources are not modified or accessed while they are still in use by the 
GPU.

A GPU wait can be inserted into a command buffer at any point, allowing 
subsequent commands to wait until the timepoint is reached. This is useful for 
synchronizing GPU operations without stalling the CPU.

---

## Resources
Most resource types expose a static `Create` function:
```cpp
lf::handle<lf::VertexBuffer> vbo = lf::VertexBuffer::Create();
lf::unique<lf::IndexBuffer> ibo = lf::unique(lf::IndexBuffer::Create());
```

All resource operations are exposed as static functions via the resource type
and most take a `lf::view<T>` as the first parameter.

---
#### Buffers
```cpp
static void lf::Buffer::Data(lf::view<Buffer> buf, std::span<std::byte> data, lf::buffer_usage usage, size_t offset = 0);
// Ensures the buffer has at least 'size' bytes of capacity.
static void lf::Buffer::Reserve(lf::view<Buffer> buf, size_t size, lf::buffer_usage usage);
static size_t lf::Buffer::GetSize(lf::view<const Buffer> buf);
static size_t lf::Buffer::GetCapacity(lf::view<const Buffer> buf);
```

---
#### Command Pipeline

```cpp
static lf::view<lf::CommandBuffer> lf::CommandPool::Allocate(lf::view<lf::CommandPool> pool);
static lf::QueueTimepoint lf::Queue::Submit(lf::view<Queue> queue, lf::view<const CommandBuffer> cmd);
static void lf::CommandBuffer::WaitFor(lf::view<lf::CommandBuffer> cmd, lf::QueueTimepoint timepoint);
static void lf::CommandBuffer::Draw(lf::view<lf::CommandBuffer> cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
static void lf::CommandBuffer::Begin(lf::view<lf::CommandBuffer> cmd);
static void lf::CommandBuffer::BeginRendering(lf::view<lf::CommandBuffer> cmd, lf::view<Framebuffer> fb);
static void lf::CommandBuffer::EndRendering(lf::view<lf::CommandBuffer> cmd);
static void lf::CommandBuffer::End(lf::view<lf::CommandBuffer> cmd);
static void lf::CommandBuffer::Reset(lf::view<lf::CommandBuffer> cmd);
void lf::QueueTimepoint::wait(QueueTimepoint* this);
```

---
### Graphics Pipeline
```cpp
static void lf::GraphicsPipeline::AttachVertexShader(lf::view<GraphicsPipeline> pipeline, lf::string_view source);
static void lf::GraphicsPipeline::AttachFragmentShader(lf::view<GraphicsPipeline> pipeline, lf::string_view source);
// Other shader stages not yet implemented, but will follow the same pattern.
static lf::error lf::GraphicsPipeline::Link(lf::view<GraphicsPipeline> pipeline);
```

---
#### Textures
##### Images
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
};
```
##### Texture Functions
```cpp
static void lf::Texture1D::Reserve(lf::view<Texture> tex, u32 extent, u32 layers, u32 mip_levels);
static void lf::Texture2D::Reserve(lf::view<Texture> tex, dim2<u32> extent, u32 layers, u32 mip_levels);
static void lf::Texture3D::Reserve(lf::view<Texture> tex, dim3<u32> extent, u32 layers, u32 mip_levels);

static void lf::Texture1D::Upload(lf::view<Texture> tex, const Image1D& img, u32 mip_level);
static void lf::Texture2D::Upload(lf::view<Texture> tex, const Image2D& img, u32 mip_level);
static void lf::Texture3D::Upload(lf::view<Texture> tex, const Image3D& img, u32 mip_level);

static Image1D lf::Texture1D::Download(lf::view<const Texture1D> tex, u32 mip_level);
static Image2D lf::Texture2D::Download(lf::view<const Texture2D> tex, u32 mip_level);
static Image3D lf::Texture3D::Download(lf::view<const Texture3D> tex, u32 mip_level);
```

---
#### Window
```cpp
static void lf::Window::Show(lf::view<Window> wnd);
static void lf::Window::Hide(lf::view<Window> wnd);
static void lf::Window::Resize(lf::view<Window> wnd, dim2<u32> dim);
static dim2<u32> lf::Window::GetSize(lf::view<Window> wnd);
// The framebuffer object is valid until the call to EndFrame, and must not be used afterward.
static lf::view<lf::Framebuffer> lf::Window::BeginFrame(lf::view<Window> wnd);
static void lf::Window::EndFrame(lf::view<Window> wnd);
```
