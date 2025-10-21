cmake_minimum_required(VERSION 3.10)

# Define common source files
set(TEXT_FUZZ_SOURCES
    text-fuzzer.c
    ${CMAKE_SOURCE_DIR}/text.c
    ${CMAKE_SOURCE_DIR}/text-common.c
    ${CMAKE_SOURCE_DIR}/text-io.c
    ${CMAKE_SOURCE_DIR}/text-iterator.c
    ${CMAKE_SOURCE_DIR}/text-util.c
    ${CMAKE_SOURCE_DIR}/text-motions.c
    ${CMAKE_SOURCE_DIR}/text-objects.c
    ${CMAKE_SOURCE_DIR}/text-regex.c
    ${CMAKE_SOURCE_DIR}/array.c
)

set(BUFFER_FUZZ_SOURCES
    buffer-fuzzer.c
    ${CMAKE_SOURCE_DIR}/buffer.c
)

# Common include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

# text-fuzzer
add_executable(text-fuzzer ${TEXT_FUZZ_SOURCES})
target_compile_definitions(text-fuzzer PRIVATE BUFFER_SIZE=4 BLOCK_SIZE=4 CONFIG_TRE=0)


# buffer-fuzzer
add_executable(buffer-fuzzer ${BUFFER_FUZZ_SOURCES})
target_compile_definitions(buffer-fuzzer PRIVATE BUFFER_SIZE=4 BLOCK_SIZE=4)
