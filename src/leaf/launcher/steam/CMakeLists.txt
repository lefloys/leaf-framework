include("Steamworks")

add_files("leaf-framework"
	"launcher.cpp"
)
target_link_libraries("${LEAF_TARGET_NAME}" PRIVATE "Steamworks::Steamworks")

function(leaf_configure_launcher_executable target_name)
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
endfunction()
