find_package(Catch2 CONFIG REQUIRED)
include(Catch)

add_executable("leaf-tests"
    "core/binary_tests.cpp"
    "core/time_tests.cpp"
    "script/prototype_lua_tests.cpp"
)

target_link_libraries("leaf-tests" PRIVATE
    "Catch2::Catch2WithMain"
    "leaf::core"
    "leaf::resource"
    "leaf::script"
)

catch_discover_tests("leaf-tests"
    TEST_PREFIX "leaf::"
)
