file(GLOB_RECURSE TPXL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(libtpxl STATIC
    ${TPXL_SOURCES}

    "${CMAKE_SOURCE_DIR}/third_party/miniaudio/miniaudio.c"
    "${CMAKE_SOURCE_DIR}/third_party/base64/lib/libbase64.o"
)

set_target_properties(libtpxl PROPERTIES
    OUTPUT_NAME tpxl
)

target_include_directories(libtpxl
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"

    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/third_party/base64/include"
        "${CMAKE_SOURCE_DIR}/third_party"
)

target_compile_definitions(libtpxl
    PRIVATE
        _POSIX_C_SOURCE=200809L
)

target_link_libraries(libtpxl
    PRIVATE
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