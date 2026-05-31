add_executable(leaf-framework-steam-store-example)
add_files(leaf-framework-steam-store-example
    "main.cpp"
)
set_target_properties(leaf-framework-steam-store-example PROPERTIES LEAF_STORE_BACKEND steam)
leaf_framework_configure_example(leaf-framework-steam-store-example)
