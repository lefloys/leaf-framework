add_files("leaf-framework"
	"app.hpp"
	"lifecycle.hpp"
	"network.hpp"
	"user.hpp"
)

set(LEAF_STORE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "Leaf store source directory")

function(leaf_ensure_store_backend backend_name)
	if(TARGET "leaf-store-${backend_name}")
		return()
	endif()

	if("${backend_name}" STREQUAL "none")
		add_library("leaf-store-none" OBJECT
			"${LEAF_STORE_SOURCE_DIR}/none/lifecycle.cpp"
			"${LEAF_STORE_SOURCE_DIR}/none/network.cpp"
			"${LEAF_STORE_SOURCE_DIR}/none/user.cpp"
		)
		target_compile_features("leaf-store-none" PRIVATE "cxx_std_23")
		target_include_directories("leaf-store-none" PRIVATE "${LEAF_STORE_SOURCE_DIR}/../..")
	elseif("${backend_name}" STREQUAL "steam")
		include("Steamworks")
		add_library("leaf-store-steam" OBJECT
			"${LEAF_STORE_SOURCE_DIR}/steam/lifecycle.cpp"
			"${LEAF_STORE_SOURCE_DIR}/steam/network.cpp"
			"${LEAF_STORE_SOURCE_DIR}/steam/user.cpp"
		)
		target_compile_features("leaf-store-steam" PRIVATE "cxx_std_23")
		target_include_directories("leaf-store-steam" PRIVATE "${LEAF_STORE_SOURCE_DIR}/../..")
		target_link_libraries("leaf-store-steam" PRIVATE "Steamworks::Steamworks")
	else()
		message(FATAL_ERROR "Unsupported store backend '${backend_name}'.")
	endif()
endfunction()

function(leaf_configure_store_executable target_name)
	get_target_property(store_backend "${target_name}" LEAF_STORE_BACKEND)
	if(NOT store_backend)
		set(store_backend "${store}")
	endif()

	leaf_ensure_store_backend("${store_backend}")
	target_link_libraries("${target_name}" PRIVATE "leaf-store-${store_backend}")

	if("${store_backend}" STREQUAL "steam")
		target_link_libraries("${target_name}" PRIVATE "Steamworks::Steamworks")
		file(GENERATE
			OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target_name}-steam_appid.txt"
			CONTENT "${LEAF_STEAM_APP_ID}\n"
		)
		add_custom_command(TARGET "${target_name}" POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${CMAKE_CURRENT_BINARY_DIR}/${target_name}-steam_appid.txt"
				"$<TARGET_FILE_DIR:${target_name}>/steam_appid.txt"
		)
		add_custom_target("${target_name}-steam-runtime"
			COMMAND ${CMAKE_COMMAND} -E make_directory
				"$<TARGET_FILE_DIR:${target_name}>"
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${LEAF_STEAMWORKS_RUNTIME_LIBRARY}"
				"$<TARGET_FILE_DIR:${target_name}>"
		)
		add_dependencies("${target_name}" "${target_name}-steam-runtime")
	endif()
endfunction()
