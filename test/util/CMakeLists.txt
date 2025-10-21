cmake_minimum_required(VERSION 3.10)

# Build the keys utility
add_executable(keys keys.c)
target_link_libraries(keys PRIVATE termkey unibilium) # Assuming termkey is needed based on keys.c