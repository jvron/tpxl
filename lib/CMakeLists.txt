include(FetchContent)

FetchContent_Declare(
    base64
    GIT_REPOSITORY https://github.com/aklomp/base64.git
    GIT_TAG v0.5.2
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(base64)

set_target_properties(base64 PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

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
set(SIXEL_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/libsixel-build")
set(SIXEL_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/libsixel-install")

file(MAKE_DIRECTORY
    "${SIXEL_INSTALL_DIR}/include"
    "${SIXEL_INSTALL_DIR}/lib"
)

add_custom_command(
    OUTPUT
        "${SIXEL_INSTALL_DIR}/lib/libsixel.a"

    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "${SIXEL_BINARY_DIR}"

    COMMAND
        "${SIXEL_SOURCE_DIR}/configure"
        "--prefix=${SIXEL_INSTALL_DIR}"
        "CFLAGS=-fPIC"
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
        ${CMAKE_COMMAND} -E chdir
        "${SIXEL_SOURCE_DIR}"
        ${CMAKE_MAKE_PROGRAM}
        -j

    COMMAND
        ${CMAKE_COMMAND} -E chdir
        "${SIXEL_SOURCE_DIR}"
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


file(GLOB_RECURSE TPXL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

set(TPXL_SOURCES
    ${TPXL_SOURCES}

    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/miniaudio/miniaudio.c"
)

# ---------------------------------------------------------------------------
# TPXL object library
# ---------------------------------------------------------------------------

add_library(tpxl_objects OBJECT
    ${TPXL_SOURCES}
)

set_target_properties(tpxl_objects PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_compile_features(tpxl_objects
    PUBLIC
        c_std_11
)

target_include_directories(tpxl_objects
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../third_party"
)

target_compile_definitions(tpxl_objects
    PRIVATE
        _POSIX_C_SOURCE=200809L
)

target_link_libraries(tpxl_objects
    PRIVATE
        libsixel
        base64
)

# ---------------------------------------------------------------------------
# Static library
# ---------------------------------------------------------------------------

add_library(libtpxl STATIC
    $<TARGET_OBJECTS:tpxl_objects>
)

target_include_directories(libtpxl
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(libtpxl PROPERTIES
    OUTPUT_NAME tpxl
)

target_link_libraries(libtpxl
    PRIVATE
        base64
        libsixel
        m
        dl
        pthread
        avformat
        avcodec
        avutil
        swscale
        swresample
        gif
        z
)


# ---------------------------------------------------------------------------
# Shared library
# ---------------------------------------------------------------------------

add_library(libtpxl_shared SHARED
    $<TARGET_OBJECTS:tpxl_objects>
)

target_include_directories(libtpxl_shared
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(libtpxl_shared PROPERTIES
    OUTPUT_NAME tpxl
)

target_link_libraries(libtpxl_shared
    PRIVATE
        base64
        libsixel
        m
        dl
        pthread
        avformat
        avcodec
        avutil
        swscale
        swresample
        gif
        z
)