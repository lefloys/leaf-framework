set(_miniaudio_INCLUDE_HINTS)

set(_miniaudio_TRIPLET "${VCPKG_TARGET_TRIPLET}")
if(NOT _miniaudio_TRIPLET AND DEFINED CMAKE_VCPKG_TARGET_TRIPLET)
    set(_miniaudio_TRIPLET "${CMAKE_VCPKG_TARGET_TRIPLET}")
endif()

if(NOT _miniaudio_TRIPLET)
    if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_miniaudio_TRIPLET x64-windows)
    elseif(WIN32)
        set(_miniaudio_TRIPLET x86-windows)
    elseif(UNIX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_miniaudio_TRIPLET x64-linux)
    endif()
endif()

if(DEFINED VCPKG_INSTALLED_DIR AND _miniaudio_TRIPLET)
    list(APPEND _miniaudio_INCLUDE_HINTS "${VCPKG_INSTALLED_DIR}/${_miniaudio_TRIPLET}/include")
endif()

if(DEFINED _VCPKG_INSTALLED_DIR AND _miniaudio_TRIPLET)
    list(APPEND _miniaudio_INCLUDE_HINTS "${_VCPKG_INSTALLED_DIR}/${_miniaudio_TRIPLET}/include")
endif()

if(DEFINED CMAKE_TOOLCHAIN_FILE AND _miniaudio_TRIPLET)
    get_filename_component(_miniaudio_TOOLCHAIN_DIR "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
    get_filename_component(_miniaudio_VCPKG_ROOT "${_miniaudio_TOOLCHAIN_DIR}/../.." ABSOLUTE)
    list(APPEND _miniaudio_INCLUDE_HINTS "${_miniaudio_VCPKG_ROOT}/installed/${_miniaudio_TRIPLET}/include")
endif()

if(DEFINED ENV{VCPKG_ROOT} AND _miniaudio_TRIPLET)
    list(APPEND _miniaudio_INCLUDE_HINTS "$ENV{VCPKG_ROOT}/installed/${_miniaudio_TRIPLET}/include")
endif()

if(_miniaudio_TRIPLET)
    list(APPEND _miniaudio_INCLUDE_HINTS
        "${CMAKE_BINARY_DIR}/vcpkg_installed/${_miniaudio_TRIPLET}/include"
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/${_miniaudio_TRIPLET}/include"
    )
endif()

find_path(miniaudio_INCLUDE_DIR
    NAMES miniaudio.h
    HINTS ${_miniaudio_INCLUDE_HINTS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(miniaudio REQUIRED_VARS miniaudio_INCLUDE_DIR)

if(miniaudio_FOUND AND NOT TARGET miniaudio::miniaudio)
    add_library(miniaudio::miniaudio INTERFACE IMPORTED)
    set_target_properties(miniaudio::miniaudio PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${miniaudio_INCLUDE_DIR}"
    )
endif()
