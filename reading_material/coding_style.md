# leaf-framework coding style

This file captures the project conventions established so far. Update it when
the codebase direction changes.

## Naming

- Exposed API uses `CamelCase`.
- Functions placed into backend function tables also use `CamelCase`.
- Internal helpers, private functions, local variables, globals, and members use
  `snake_case`.
- Prefer `Resource` over `Object` when naming backend-managed GPU-facing data.

## General C++ style

- Do not silence unused variables/parameters with `(void)x;`.
- Ignore unused variable warnings in WIP code.

## Backend architecture

- The framework does not choose a graphics backend by itself.
- The user selects a backend explicitly through `lf::SetGraphicsAPI(...)`.
- Frontend code should call the selected graphics table directly through
  `Graphics`.
- Platform-specific windowing details stay behind the platform abstraction.
- Graphics backends should consume the platform layer instead of calling GLFW
  window APIs directly from resource code.

## Resource lifetime

- Backend resources should be real types such as `WindowVK`.
- Backend resources should derive from a shared `Resource` base when they need
  common deleted copy/move semantics.
- Resource creation logic belongs inside constructors when practical.
- Resource cleanup belongs inside destructors instead of separate helper
  functions.
- Avoid manual release helpers when the destructor can own the same logic.
- Prefer rule-of-0 style in concrete resources; avoid hand-written move/copy
  operations unless there is a compelling reason.
- Context access should fail hard if it is used while missing; do not continue
  with invalid state.
- In destructor-sensitive code paths, prefer aborting over throwing.
- Backend shutdown should clean up leaked resources while the context is still
  valid, and warn the user when this happens.

## Pools and handles

- Resource pools belong to the backend context.
- The pool itself should be stored inline in the context, not heap-allocated.
- Pool slots store `std::unique_ptr<std::optional<Resource>>`.
- Resource lookup should go through a generic helper such as
  `Unhandle<Resource>(...)` instead of repeating pool access code.

## Context ownership

- Backend contexts should be owned by `std::unique_ptr`.
- `nullptr` means "not initialized"; avoid extra boolean state when pointer
  lifetime already expresses it.
- When checking pointers, prefer implicit conversion to `bool` over explicit
  comparisons such as `== nullptr` or `!= nullptr`.
- Prefer `if (ptr)` / `if (!ptr)` over `if (ptr != nullptr)` /
  `if (ptr == nullptr)`.
- Apply the same readability rule to Vulkan handles: prefer
  `if (handle)` / `if (!handle)` over comparisons against `VK_NULL_HANDLE`
  when the type allows it.

## Iteration

- Prefer `size_t` over `u32` for indexing containers.
- Prefer project iterators over manual index loops when the code becomes
  clearer.
- Use `range(begin, end)` or `range(begin, end, step)`.
- Use `rrange(begin, end)` or `rrange(begin, end, step)` for reverse numeric
  iteration.
- Use `enumerate(range)` and `renumerate(range)` for indexed iteration.
- Do not provide or use single-argument `range(end)` or `rrange(end)`.

## Layout

- Prefer a real module layout over large monolithic backend files.
- Backend code should be split into focused files.
- Typical backend structure should separate:
  `entrypoints`, `context/state`, and `per-resource headers/sources`.

## Vulkan struct initialization

- Do not rely on designated brace initialization for Vulkan structs.
- Initialize `sType` inline in the declaration, then assign the remaining fields
  explicitly.
- Preferred style:
  `VkInstanceCreateInfo create_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };`
- vulkan types should be prefixed with vk_ : `VkInstance vk_instance;`
- There should not be an explicit null handle check before destruction, unless 
  you want to avoid the API call because you expect something to be null
