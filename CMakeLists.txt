cmake_minimum_required(VERSION 3.24)

project(tpxl 
    VERSION 1.0.0 
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TPXL_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(TPXL_TSAN "Enable ThreadSanitizer" OFF)

if(TPXL_ASAN AND TPXL_TSAN)
    message(FATAL_ERROR "TPXL_ASAN and TPXL_TSAN cannot both be enabled")
endif()

if(TPXL_ASAN)
    add_compile_options(
        -O1
        -g3
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    add_link_options(
        -fsanitize=address,undefined
    )
elseif(TPXL_TSAN)
    add_compile_options(
        -O1
        -g3
        -fsanitize=thread
        -fno-omit-frame-pointer
    )

    add_link_options(
        -fsanitize=thread
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
        -O3
        -DNDEBUG
    )
else()
    add_compile_options(
        -g3
        -O0
    )

endif()

include(FetchContent)

FetchContent_Declare(
    libsixel
    GIT_REPOSITORY https://github.com/saitoha/libsixel.git
    GIT_TAG v1.8.7
    GIT_SHALLOW TRUE
)

FetchContent_GetProperties(libsixel)

if(NOT libsixel_POPULATED)
    FetchContent_Populate(libsixel)
endif()

set(SIXEL_SOURCE_DIR "${libsixel_SOURCE_DIR}")
set(SIXEL_BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/libsixel-build")
set(SIXEL_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/libsixel-install")

file(MAKE_DIRECTORY
    "${SIXEL_INSTALL_DIR}/include"
    "${SIXEL_INSTALL_DIR}/lib"
)

add_custom_command(
    OUTPUT
        "${SIXEL_INSTALL_DIR}/lib/libsixel.a"
    COMMAND
        ${CMAKE_COMMAND} -E make_directory "${SIXEL_BINARY_DIR}"
    COMMAND
        "${SIXEL_SOURCE_DIR}/configure"
        "--prefix=${SIXEL_INSTALL_DIR}"
        --disable-shared
        --enable-static
        --disable-python
        --disable-tests
        --disable-gcov
        --without-libcurl
        --without-gd
        --without-gdk-pixbuf2
        --without-jpeg
        --without-png
    COMMAND
        ${CMAKE_COMMAND} -E chdir "${SIXEL_SOURCE_DIR}"
        ${CMAKE_MAKE_PROGRAM}
        -j
    COMMAND
        ${CMAKE_COMMAND} -E chdir "${SIXEL_SOURCE_DIR}"
        ${CMAKE_MAKE_PROGRAM}
        install
    DEPENDS
        "${SIXEL_SOURCE_DIR}/configure"
    WORKING_DIRECTORY
        "${SIXEL_SOURCE_DIR}"
    VERBATIM
)

add_custom_target(libsixel_build
    DEPENDS
        "${SIXEL_INSTALL_DIR}/lib/libsixel.a"
)

add_library(libsixel STATIC IMPORTED GLOBAL)

set_target_properties(libsixel PROPERTIES
    IMPORTED_LOCATION
        "${SIXEL_INSTALL_DIR}/lib/libsixel.a"
    INTERFACE_INCLUDE_DIRECTORIES
        "${SIXEL_INSTALL_DIR}/include"
)

add_dependencies(libsixel libsixel_build)

add_subdirectory(libtpxl)
add_subdirectory(cli)