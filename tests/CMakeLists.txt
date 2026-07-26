add_executable(leaf-framework-tests
    binary_tests.cpp
    dynamic_object_tests.cpp
    error_tests.cpp
    fixed_tests.cpp
    localization_tests.cpp
    logging_tests.cpp
    math_tests.cpp
    string_time_tests.cpp
    lockstep_network_tests.cpp
    mod_loader_tests.cpp
    pmg_tests.cpp
    window_element_tests.cpp
)

target_compile_features(leaf-framework-tests PRIVATE cxx_std_23)
target_link_libraries(leaf-framework-tests PRIVATE Catch2::Catch2WithMain leaf-framework leaf-store-none)

add_test(NAME leaf-framework-tests COMMAND leaf-framework-tests)

add_executable(leaf-rml-shader-tests
    rml_shader_tests.cpp
    "../Rutile/RTSL/transpilers/hlsl/src/hlsl.cpp"
    "../Rutile/RTSL/transpilers/spirv/src/spirv.cpp"
)
target_compile_features(leaf-rml-shader-tests PRIVATE cxx_std_23)
target_include_directories(leaf-rml-shader-tests PRIVATE
    "../Rutile/RTSL/transpilers/hlsl/include"
    "../Rutile/RTSL/transpilers/spirv/include"
)
target_link_libraries(leaf-rml-shader-tests PRIVATE
    Catch2::Catch2WithMain
    leaf-framework
    rtsl
    rtsl-sdk
)
add_test(NAME leaf-rml-shader-tests COMMAND leaf-rml-shader-tests)
