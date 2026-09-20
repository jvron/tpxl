file(GLOB CLI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(tpxl ${CLI_SOURCES})

target_link_libraries(tpxl
    PRIVATE
        libtpxl
)