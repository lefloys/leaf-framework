include("Steamworks")

add_files("leaf-framework"
	"launcher.cpp"
)
target_link_libraries("${LEAF_TARGET_NAME}" PRIVATE "Steamworks::Steamworks")
