find_package(Catch2 CONFIG REQUIRED)
include(Catch)

add_executable("leaf-tests" EXCLUDE_FROM_ALL
    "core/binary.cpp"
    "core/time.cpp"
    "resource_schemas.cpp"
    "script/prototype_lua.cpp"
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
