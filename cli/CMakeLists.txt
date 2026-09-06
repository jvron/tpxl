file(GLOB CLI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(tpxl ${CLI_SOURCES})

target_include_directories(tpxl
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/libtpxl/include"
        "${CMAKE_SOURCE_DIR}/libtpxl/src"
        "${CMAKE_SOURCE_DIR}/third_party/base64/include"
        "${CMAKE_SOURCE_DIR}/third_party"
)

target_link_libraries(tpxl 
    PRIVATE
        libtpxl
)