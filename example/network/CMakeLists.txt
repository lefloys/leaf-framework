add_executable(leaf-framework-network-example)
add_files(leaf-framework-network-example
	"main.cpp"
)
set_target_properties(leaf-framework-network-example PROPERTIES LEAF_STORE_BACKEND steam)
leaf_framework_configure_example(leaf-framework-network-example)
