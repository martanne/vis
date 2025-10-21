cmake_minimum_required(VERSION 3.10)
project(core_tests C)

# Create empty config.h for core tests
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/config.h
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/config.h
    COMMENT "Creating empty config.h for core tests"
)
add_custom_target(create_empty_config_h ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

# Include directories (exclude ${CMAKE_BINARY_DIR} to avoid main config.h)
include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/test/core
    NO_CMAKE_SYSTEM_PATH
)

# Common source files from parent directory
set(PARENT_SOURCES
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

# CCAN sources
file(GLOB CCAN_SOURCES "${CMAKE_SOURCE_DIR}/test/core/ccan/*/*.c")

# Common compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DBUFFER_SIZE=4 -DBLOCK_SIZE=4")

# Generate ccan_config.h for CCAN
add_executable(ccan-config ccan-config.c)
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/test/core/ccan_config.h
    COMMAND ./ccan-config "${CMAKE_C_COMPILER}" "${CMAKE_C_FLAGS}" > ${CMAKE_BINARY_DIR}/test/core/ccan_config.h
    DEPENDS ccan-config
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating CCAN ccan_config.h"
)
add_custom_target(generate_ccan_config_h DEPENDS ${CMAKE_BINARY_DIR}/test/core/ccan_config.h)

# Buffer test target
add_executable(buffer-test buffer-test.c ${CMAKE_SOURCE_DIR}/buffer.c ${CCAN_SOURCES})
target_compile_definitions(buffer-test PRIVATE BUFFER_SIZE=4 BLOCK_SIZE=4)
target_link_libraries(buffer-test PRIVATE ${LIBS})
add_dependencies(buffer-test generate_ccan_config_h create_empty_config_h)

# Map test target
add_executable(map-test map-test.c ${CMAKE_SOURCE_DIR}/map.c ${CCAN_SOURCES})
target_compile_definitions(map-test PRIVATE BUFFER_SIZE=4 BLOCK_SIZE=4)
target_link_libraries(map-test PRIVATE ${LIBS})
add_dependencies(map-test generate_ccan_config_h create_empty_config_h)

# Array test target
add_executable(array-test array-test.c ${CMAKE_SOURCE_DIR}/array.c ${CCAN_SOURCES})
target_compile_definitions(array-test PRIVATE BUFFER_SIZE=4 BLOCK_SIZE=4)
target_link_libraries(array-test PRIVATE ${LIBS})
add_dependencies(array-test generate_ccan_config_h create_empty_config_h)

# Text test target
add_executable(text-test text-test.c ${PARENT_SOURCES} ${CCAN_SOURCES})
target_compile_definitions(text-test PRIVATE BUFFER_SIZE=4 BLOCK_SIZE=4 CONFIG_TRE=0)
target_link_libraries(text-test PRIVATE ${LIBS})
add_dependencies(text-test generate_ccan_config_h create_empty_config_h)

# Register tests with CTest
set(TESTS buffer-test map-test array-test text-test)
foreach(test ${TESTS})
    add_test(NAME ${test} COMMAND ${test})
endforeach()

# Run all tests target
add_custom_target(run_core_tests
    COMMAND ./buffer-test
    COMMAND ./map-test
    COMMAND ./array-test
    COMMAND ./text-test
    DEPENDS ${TESTS}
    COMMENT "Running core tests"
)

# Debug target
# add_custom_target(debug
#     COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR}/test/core
#     COMMAND ${CMAKE_COMMAND} --build . --target run_core_tests
#     DEPENDS ${TESTS}
#     COMMENT "Building and running tests in debug mode"
# )

# Coverage target
# add_custom_target(coverage
#     COMMAND ${CMAKE_COMMAND} -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} --coverage" ${CMAKE_SOURCE_DIR}/test/core
#     COMMAND ${CMAKE_COMMAND} --build . --target run_core_tests
#     DEPENDS ${TESTS}
#     COMMENT "Building and running tests with coverage"
# )

# AddressSanitizer target
# add_custom_target(asan
#     COMMAND ${CMAKE_COMMAND} -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} -fsanitize=address" ${CMAKE_SOURCE_DIR}/test/core
#     COMMAND ${CMAKE_COMMAND} --build . --target run_core_tests
#     DEPENDS ${TESTS}
#     COMMENT "Building and running tests with AddressSanitizer"
# )

# UndefinedBehaviorSanitizer target
# add_custom_target(ubsan
#     COMMAND ${CMAKE_COMMAND} -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} -fsanitize=undefined" ${CMAKE_SOURCE_DIR}/test/core
#     COMMAND ${CMAKE_COMMAND} --build . --target run_core_tests
#     DEPENDS ${TESTS}
#     COMMENT "Building and running tests with UndefinedBehaviorSanitizer"
# )

# MemorySanitizer target
# add_custom_target(msan
#     COMMAND ${CMAKE_COMMAND} -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins" ${CMAKE_SOURCE_DIR}/test/core
#     COMMAND ${CMAKE_COMMAND} --build . --target run_core_tests
#     DEPENDS ${TESTS}
#     COMMENT "Building and running tests with MemorySanitizer"
# )

# Valgrind target
add_custom_target(valgrind
    COMMAND valgrind --leak-check=full --log-file=buffer-test.valgrind ./buffer-test && grep -q LEAK buffer-test.valgrind && exit 1 || true
    COMMAND valgrind --leak-check=full --log-file=map-test.valgrind ./map-test && grep -q LEAK map-test.valgrind && exit 1 || true
    COMMAND valgrind --leak-check=full --log-file=array-test.valgrind ./array-test && grep -q LEAK array-test.valgrind && exit 1 || true
    COMMAND valgrind --leak-check=full --log-file=text-test.valgrind ./text-test && grep -q LEAK text-test.valgrind && exit 1 || true
    DEPENDS ${TESTS}
    COMMENT "Running tests with Valgrind"
)

# TIS target
# add_custom_target(tis
#     COMMAND ${CMAKE_COMMAND}
#             -DCMAKE_C_COMPILER="tis-interpreter.sh --cc"
#             -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS} -DHAVE_MEMRCHR=0 -DTIS_INTERPRETER=1"
#             -DCMAKE_C_FLAGS_STD=""
#             -DCMAKE_C_FLAGS_LIBC=""
#             -- ${CMAKE_SOURCE_DIR}/test/core
#     COMMAND ${CMAKE_COMMAND} --build . --target run_core_tests
#     DEPENDS ${TESTS}
#     COMMENT "Building and running tests with TIS interpreter"
# )

# Clean target
add_custom_target(clean_core
    COMMAND ${CMAKE_COMMAND} -E remove -f ccan-config ${CMAKE_BINARY_DIR}/test/core/ccan_config.h
    COMMAND ${CMAKE_COMMAND} -E remove -f data symlink hardlink
    COMMAND ${CMAKE_COMMAND} -E remove -f ${TESTS}
    COMMAND ${CMAKE_COMMAND} -E remove -f *.gcov *.gcda *.gcno *.valgrind
    COMMENT "Cleaning core test directory"
)
