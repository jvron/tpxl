cmake_minimum_required(VERSION 3.24)

project(TPXL
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TPXL_ASAN "Enable AddressSanitizer" OFF)
option(TPXL_TSAN "Enable ThreadSanitizer" OFF)

if(TPXL_ASAN AND TPXL_TSAN)
    message(FATAL_ERROR "TPXL_ASAN and TPXL_TSAN cannot both be enabled")
endif()

if(TPXL_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
elseif(TPXL_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

add_subdirectory(libtpxl)
add_subdirectory(cli)