option(TPXL_BUILD_SHARED
    "Build the shared libtpxl library"
    OFF
)

include(FetchContent)

# zlib

FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3
    GIT_SHALLOW TRUE
)

set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(zlib)

foreach(target zlib example minigzip example64 minigzip64)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES
            EXCLUDE_FROM_ALL TRUE
        )
    endif()
endforeach()

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

# libpng

FetchContent_Declare(
    libpng
    GIT_REPOSITORY https://github.com/pnggroup/libpng.git
    GIT_TAG v1.6.59
    GIT_SHALLOW TRUE
)

set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON)

FetchContent_MakeAvailable(libpng)

target_include_directories(png_static PRIVATE
    "${zlib_SOURCE_DIR}"
    "${zlib_BINARY_DIR}"
)

if(NOT TARGET PNG::PNG)
    add_library(PNG::PNG ALIAS png_static)
endif()

# base64

FetchContent_Declare(
    base64
    GIT_REPOSITORY https://github.com/aklomp/base64.git
    GIT_TAG v0.5.2
    GIT_SHALLOW TRUE
)

set(BASE64_BUILD_CLI OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(base64)

# Position-independent code for shared TPXL builds
if(TPXL_BUILD_SHARED)
    foreach(target ZLIB::ZLIB PNG::PNG base64)
        if(TARGET ${target})
            set_target_properties(${target} PROPERTIES
                POSITION_INDEPENDENT_CODE ON
            )
        endif()
    endforeach()
endif()

# System dependencies: static libsixel and shared FFmpeg.

if(MSVC)
    # FFmpeg: installed through vcpkg
    find_package(FFMPEG REQUIRED)

    # libsixel: provide its installation prefix with -DSIXEL_ROOT=...
    set(SIXEL_ROOT "" CACHE PATH "libsixel installation prefix")

    find_path(SIXEL_INCLUDE_DIR
        NAMES sixel.h
        HINTS "${SIXEL_ROOT}"
        PATH_SUFFIXES include include/sixel
        REQUIRED
    )

    find_library(SIXEL_LIBRARY
        NAMES sixel libsixel
        HINTS "${SIXEL_ROOT}"
        PATH_SUFFIXES lib lib64
        REQUIRED
    )

    add_library(TPXL_FFMPEG INTERFACE)
    target_include_directories(TPXL_FFMPEG INTERFACE
        ${FFMPEG_INCLUDE_DIRS}
    )
    target_link_directories(TPXL_FFMPEG INTERFACE
        ${FFMPEG_LIBRARY_DIRS}
    )
    target_link_libraries(TPXL_FFMPEG INTERFACE
        ${FFMPEG_LIBRARIES}
    )

    set(SIXEL_INCLUDE_DIRS "${SIXEL_INCLUDE_DIR}")
    set(SIXEL_LINK_LIBRARY "${SIXEL_LIBRARY}")

else()
    # Linux: discover system dependencies through pkg-config
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(SIXEL REQUIRED libsixel)

    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavformat
        libavcodec
        libavutil
        libswscale
        libswresample
    )

    find_library(SIXEL_STATIC
        NAMES libsixel.a
        HINTS ${SIXEL_LIBRARY_DIRS}
        REQUIRED
    )

    set(SIXEL_LINK_LIBRARY "${SIXEL_STATIC}")
    set(FFMPEG_LINK_TARGET PkgConfig::FFMPEG)

    message(STATUS "SIXEL_STATIC: ${SIXEL_STATIC}")
endif()

# Vendored dependencies

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party")
set(MINIAUDIO_DIR "${THIRD_PARTY_DIR}/miniaudio")
set(STB_IMAGE_DIR "${THIRD_PARTY_DIR}/stb")
set(GIFLIB_DIR "${THIRD_PARTY_DIR}/giflib")

# TPXL library

if(TPXL_BUILD_SHARED)
    set(TPXL_LIBRARY_TYPE SHARED)
else()
    set(TPXL_LIBRARY_TYPE STATIC)
endif()

file(GLOB_RECURSE TPXL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(libtpxl ${TPXL_LIBRARY_TYPE}
    ${TPXL_SOURCES}
    "${MINIAUDIO_DIR}/miniaudio.c"
)

add_library(giflib STATIC
    "${GIFLIB_DIR}/dgif_lib.c"
    "${GIFLIB_DIR}/egif_lib.c"
    "${GIFLIB_DIR}/gifalloc.c"
    "${GIFLIB_DIR}/gif_err.c"
    "${GIFLIB_DIR}/gif_font.c"
    "${GIFLIB_DIR}/gif_hash.c"
    "${GIFLIB_DIR}/openbsd-reallocarray.c"
)

target_include_directories(giflib PUBLIC
    "${GIFLIB_DIR}"
)

if(TPXL_BUILD_SHARED)
    set_target_properties(giflib PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

target_compile_features(libtpxl PUBLIC c_std_11)

set_target_properties(libtpxl PROPERTIES
    OUTPUT_NAME tpxl
)

target_include_directories(libtpxl
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${THIRD_PARTY_DIR}"
        "${MINIAUDIO_DIR}"
        "${STB_IMAGE_DIR}"
        ${SIXEL_INCLUDE_DIRS}
)

if(UNIX AND NOT APPLE)
    target_compile_definitions(libtpxl
        PRIVATE
            _POSIX_C_SOURCE=200809L
    )
endif()

# Linking

target_link_libraries(libtpxl
    PRIVATE
        base64
        giflib
        PNG::PNG
        ZLIB::ZLIB
        "${SIXEL_LINK_LIBRARY}"
)

if(MSVC)
    target_link_libraries(libtpxl PRIVATE TPXL_FFMPEG)
else()
    target_link_libraries(libtpxl PRIVATE PkgConfig::FFMPEG)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(libtpxl
        PRIVATE
            m
            dl
            pthread
    )
endif()
