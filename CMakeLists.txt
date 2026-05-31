cmake_minimum_required(VERSION 3.16)

project(Ticket_System_2026_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(code
        src/main.cpp
        include/service/user.h
    include/ticket_system.h
        src/ticket_system.cpp
        include/service/train.h
        include/service/order.h
        src/order.cpp
        src/train.cpp
        src/user.cpp
        include/util/parser.h
        src/parser.cpp
        include/model/data_types.h
        include/storage/bpt.h
        include/storage/file_manager.h
        include/util/internal_utils.h
)

target_include_directories(code PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(code PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wshadow>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(code PRIVATE stdc++fs)
endif()
