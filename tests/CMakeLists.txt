add_executable(leaf-framework-tests
    binary_tests.cpp
    dynamic_object_tests.cpp
    logging_tests.cpp
    lockstep_network_tests.cpp
    mod_loader_tests.cpp
    pmg_tests.cpp
    window_element_tests.cpp
)

if(COMMAND leaf_ensure_store_backend)
    leaf_ensure_store_backend("none")
endif()

target_compile_features(leaf-framework-tests PRIVATE cxx_std_23)
target_link_libraries(leaf-framework-tests PRIVATE Catch2::Catch2WithMain leaf-framework leaf-store-none)

add_test(NAME leaf-framework-tests COMMAND leaf-framework-tests)
