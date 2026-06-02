if(NOT LEAF_HEADLESS)
	add_subdirectory("application")
endif()
add_subdirectory("core")
add_subdirectory("graphics")
add_subdirectory("logging")
add_subdirectory("lockstep")
add_subdirectory("network")
add_subdirectory("store")
add_subdirectory("math")
add_subdirectory("pmg")
add_subdirectory("script")
add_subdirectory("system")
if(NOT LEAF_HEADLESS)
	add_subdirectory("platform")
endif()
add_subdirectory("vendor")

add_files("leaf-framework"
	"leaf.cpp"
	"leaf.hpp"
)
