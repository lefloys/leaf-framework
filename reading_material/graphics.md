# leaf-framework graphics module

This document describes the public types, enums and functions used by the `leaf` graphics module. It assumes modern C++23.
Everything in this file is assumed to be in the lf:: namespace

---

## Types
* [`handle<T>`](#handle)
* [`view<T>`](#view)
* [`unique<T>`](#unique)
* [`shared<T>`](#shared)
* [`weak<T>`](#weak)
* [`QueueTimepoint`](#queue-timepoint)

---

Resources types are exposed as snake_case type tags (eg. window) usable via a handle type (eg. `handle<window>`)
Resource operations are exposed via CamelCase namespace (eg. Window::) 

## Resources
### Target
* [`Window`](#window)
* [`Framebuffer`](#framebuffer)

### [Buffers](#buffer)
* [`VertexBuffer`](#vertex-buffer)
* [`IndexBuffer`](#index-buffer)
* [`UniformBuffer`](#uniform-buffer)
* [`StorageBuffer`](#storage-buffer)

### [Command Pipeline](#command-pipeline)
* [`CommandBuffer`](#command-buffer)
* [`CommandContext`](#command-context)
* [`Queue`](#queue)

### Shaders and Pipelines
* [`GraphicsPipeline`](#graphics-pipeline)
* [`ComputePipeline`](#compute-pipeline)

### [Textures](#textures)
* [`Texture1D`](#texture1d)
* [`Texture2D`](#texture2d)
* [`Texture3D`](#texture3d)

## [Enums](#enums)
* [`buffer_usage`](#buffer-usage)
* [`texture_format`](#texture-format)
* [`draw_mode`](#draw-mode)
* [`queue_flag_bits`](#queue-flag-bits)

## Helpers
* [`std::span<std::byte> to_bytes(...)`](#to-bytes)

---

## Error handling
Most errors are propogated using exceptions. However, a select few functions that error
in expected ways (eg shader compilation, reading a file) return an `error`, `result` or `report`
`error` is a wrapper around std::error_code + std::string, and 
`result<T>` is a wrapper around `std::expected<T, error_code>`. 
`report` is a wrapper around `std::expected<void, error>`.


Error messages in leaf are constructed incrementally. For example a shader compilation error might look like this:
`error : failed to compile shader : <shader_log>`

Exceptions thrown by leaf will contain a stack trace and a message. The message is constructed incrementally, 
so it may be empty at the point of throwing, but will be filled in as the exception propagates up the call stack.
If you want to use this approach too, use the following pattern:
```cpp
try {
    // code that may throw
} catch (...) {
    throw_with_context("additional context message");
}
```

---
## Types
#### Handle

A `handle<T>` is a lightweight, non-owning token referencing a dynamically 
allocated resource. It is freely copyable but does not manage lifetime.
It is capable of participating in lifecycle operations (invalidate / destroy / 
transfer via API).

Handles do not enforce ownership, but invalid use like double free, use after
free, or other lifetime violations result in a a crash. (std::abort)

For safer usage, handles should be wrapped in `unique<T>` (exclusive ownership)
or `shared<T>` (reference-counted). Direct use is exposed to allow manual management.
Again: Use with caution, as misuse can lead to undefined behavior. Always 
prefer smart handles for safety.

This concept is similar to a dynamically allocated raw pointer.

---
#### View

A `view<T>` is a lightweight, non-owning reference to an existing resource.
It provides temporary access without participating in lifetime management.
It is a pure non-owning borrow, therefor cannot trigger lifecycle operations.

A view does not carry responsibility and must not be used to destroy or
transfer ownership. It is strictly for observing or using a resource owned 
elsewhere.

There is also a `view<const T>` variant for read-only access, which 
enforces const-correctness at compile time.

This concept is similar to `std::string_view` or `std::span`.

---
#### Unique

A `unique<T>` is a move-only smart handle that exclusively owns a resource.
It automatically 
releases the resource when it goes out of scope.

A `unique<T>` cannot be copied, only moved. Moving transfers ownership, 
leaving the source in a
valid but unspecified state.

`unique<T>` is ideal for resources with clear ownership semantics, such as 
buffers, textures, 
or windows that should be automatically cleaned up when no longer needed.

This concept is similar to `std::unique_ptr<T>`.

---
#### Shared
A `shared<T>` is a reference-counted smart handle that allows multiple owners
of a resource. It automatically releases the resource when the last owner goes
out of scope.

A `shared<T>` can be copied, which increments the reference count. Moving a
`shared<T>` transfers ownership without changing the reference count.

`shared<T>` is useful for resources that may be shared across different 
parts of an application, such as a texture used by multiple objects or a window
accessed by various systems.

This concept is similar to `std::shared_ptr<T>`.

---
#### Weak

A `weak<T>` is a non-owning reference to a resource managed by 
`shared<T>`.

It provides temporary access to a resource without affecting its reference 
count and without extending its lifetime.

A `weak<T>` does not guarantee that the resource is still alive. It must be
upgraded to a `shared<T>` before use. If the resource has already been 
destroyed, the upgrade fails and yields an empty or invalid result.

`weak<T>` is used to break reference cycles and to observe shared resources
without contributing to ownership.

---
#### QueueTimepoint
A `QueueTimepoint` represents a specific point in time on a GPU queue. It 
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
All resource types expose a static `Create` function:
```cpp
handle<vertex_buffer> vbo = VertexBuffer::Create();
unique<index_buffer> ibo = unique(IndexBuffer::Create());
```

All resource operations are exposed as free functions via the resource type
and most take a `view<T>` as the first parameter.

---
#### Buffers
```cpp
void Buffer::Data(view<buffer> buf, std::span<std::byte> data, buffer_usage usage, size_t offset = 0);
void Buffer::Reserve(view<buffer> buf, size_t size, buffer_usage usage);
size_t Buffer::GetSize(view<const buffer> buf);
size_t Buffer::GetCapacity(view<const buffer> buf);
```

---
#### Command Buffer

```cpp
void QueueTimepoint::wait(QueueTimepoint* this);

void CommandBuffer::Reset(view<command_buffer> cmd_buf);
CommandBuffer::BindPipeline(view<command_buffer> cmd_buf, view<graphics_pipeline> pipeline);
CommandBuffer::BindVertexBuffer(view<command_buffer> cmd_buf, view<vertex_buffer> vbo, u32 binding);
CommandBuffer::Draw(view<command_buffer> cmd_buf, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance);
CommandBuffer::Wait(view<command_buffer> cmd_buf, QueueTimepoint timepoint);
CommandBuffer::Begin(view<command_buffer> cmd_buf);
CommandBuffer::End(view<command_buffer> cmd_buf);
```


##### Example Command Recording
```cpp
lf::handle<lf::command_buffer> cmd = lf::CommandBuffer::Create();

lf::CommandBuffer::Begin(cmd);
lf::CommandBuffer::Draw(cmd, 3, 1, 0, 0);
lf::CommandBuffer::End(cmd);
```

---
### Command Context
Command contexts are the equivilent of primary command buffers. You might get a
command context from beginning a frame on a window, or by allocating your own
if you want to submit compute commands.
```cpp
void CommandContext::Submit(view<command_context> ctx, view<command_buffer> cmd_buf);
```
---

### Framebuffer
```cpp
void Framebuffer::Submit(view<framebuffer> fb, view<command_buffer> cmd_buf);
void Framebuffer::Flush(view<framebuffer> fb);
```

---
### Graphics Pipeline
WIP
```cpp
void GraphicsPipeline::AttachVertexShader(view<graphics_pipeline> pipeline, string_view source);
void GraphicsPipeline::AttachFragmentShader(view<graphics_pipeline> pipeline, string_view source);
// Other shader stages not yet implemented, but will follow the same pattern.
error GraphicsPipeline::Link(view<graphics_pipeline> pipeline);
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
void Texture1D::Reserve(view<texture1d> tex, u32 extent, u32 layers, u32 mip_levels);
void Texture2D::Reserve(view<texture2d> tex, dim2<u32> extent, u32 layers, u32 mip_levels);
void Texture3D::Reserve(view<texture3d> tex, dim3<u32> extent, u32 layers, u32 mip_levels);

void Texture1D::Upload(view<texture1d> tex, const Image1D& img, u32 mip_level);
void Texture2D::Upload(view<texture2d> tex, const Image2D& img, u32 mip_level);
void Texture3D::Upload(view<texture3d> tex, const Image3D& img, u32 mip_level);

Image1D Texture1D::Download(view<const texture1d> tex, u32 mip_level);
Image2D Texture2D::Download(view<const texture2d> tex, u32 mip_level);
Image3D Texture3D::Download(view<const texture3d> tex, u32 mip_level);
```

---
#### Window
```cpp
handle<window> Window::Create(string_view title, dim2<i32> extent);
void Window::Show(view<window> wnd);
void Window::Hide(view<window> wnd);
void Window::Resize(view<window> wnd, dim2<i32> dim);
dim2<i32> Window::GetSize(view<const window> wnd);
void Window::BeginFrame(view<window> wnd);
void Window::EndFrame(view<window> wnd);
```

Example usage:
```cpp
lf::handle<lf::window> wnd = lf::Window::Create("My Window", {800, 600});
lf::handle<lf::command_buffer> cmd = lf::CommandBuffer::Create();

lf::CommandBuffer::Begin(cmd);
lf::CommandBuffer::Draw(cmd, 3, 1, 0, 0);
lf::CommandBuffer::End(cmd);

lf::view<lf::CommandContext> cmd_ctx = lf::Window::BeginFrame(wnd);
lf::CommandContext::Execute(cmd_ctx, cmd);
lf::Window::EndFrame(wnd);

```

---

## Backend Implementation
The frontend is implemented by calling into the backend. For example, a create 
function will call into the create backend. a function that does more complex
things might call multiple backend functions to express behaviour.

To see the actual backend look at graphics/detail/api.hpp.

You are allowed to call these functions. However, they are poorly documented
so it is not recommended unless you have implemented your own backend already
