
if (WIN32)
    add_subdirectory(windows)
else()
    add_subdirectory(posix)
endif()


add_files("leaf-framework"
    "crash_handler.cpp"
    "crash_handler.hpp"
    "socket.hpp"
    "system.hpp"
)
