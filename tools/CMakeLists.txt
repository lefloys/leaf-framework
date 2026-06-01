add_executable(leaf-binary-profiler
    binary_profiler.cpp
)

target_compile_features(leaf-binary-profiler PRIVATE cxx_std_23)
target_link_libraries(leaf-binary-profiler PRIVATE leaf-framework)
