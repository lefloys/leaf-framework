add_executable(leaf-framework-tests
    dynamic_object_tests.cpp
    pmg_tests.cpp
)

target_compile_features(leaf-framework-tests PRIVATE cxx_std_23)
target_link_libraries(leaf-framework-tests PRIVATE Catch2::Catch2WithMain leaf-framework)

add_test(NAME leaf-framework-tests COMMAND leaf-framework-tests)
