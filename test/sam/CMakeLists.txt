cmake_minimum_required(VERSION 3.10)

# Add a custom command to run test.sh
add_test(NAME sam-tests COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test.sh WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})