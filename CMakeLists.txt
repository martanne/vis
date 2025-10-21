cmake_minimum_required(VERSION 3.10)
project(vis C)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

enable_testing()

# Default installation directories
set(PREFIX "/usr/local" CACHE PATH "Installation prefix")
set(EXEC_PREFIX "${PREFIX}" CACHE PATH "Installation prefix for executables")
set(BINDIR "${EXEC_PREFIX}/bin" CACHE PATH "User executables directory")
set(SHAREDIR "${PREFIX}/share" CACHE PATH "Share directory")
set(DOCDIR "${SHAREDIR}/doc" CACHE PATH "Documentation directory")
set(MANDIR "${SHAREDIR}/man" CACHE PATH "Man pages directory")

# Optional features
option(ENABLE_CURSES "Build with Curses terminal output" ON)
option(ENABLE_LUA "Build with Lua support" ON)
option(ENABLE_LPEG_STATIC "Build with LPeg static linking" AUTO)
option(ENABLE_TRE "Build with TRE regex support" ON)
option(ENABLE_SELINUX "Build with SELinux support" AUTO)
option(ENABLE_ACL "Build with POSIX ACL support" ON)
option(ENABLE_HELP "Build with built-in help texts" ON)

# Compiler settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_definitions(-D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE -DVIS_PATH="${SHAREDIR}/vis/lua")

# Define VERSION (match Makefile)
find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE GIT_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
set(VERSION_STRING "${GIT_BRANCH}-${GIT_COMMIT_HASH}")
add_definitions(-DVERSION="${VERSION_STRING}")

# Find external tools
find_program(LDOC_EXECUTABLE ldoc)
if(NOT LDOC_EXECUTABLE)
    message(WARNING "ldoc executable not found. luadoc targets will not work.")
endif()
find_program(LUACHECK_EXECUTABLE luacheck)
if(NOT LUACHECK_EXECUTABLE)
    message(WARNING "luacheck executable not found. luacheck target will not work.")
endif()
find_program(MANDOC_EXECUTABLE mandoc)
if(NOT MANDOC_EXECUTABLE)
    message(WARNING "mandoc executable not found. man target will not work.")
endif()

# Detect operating system
if(UNIX AND NOT APPLE)
    if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD|DragonFly")
        add_definitions(-D_BSD_SOURCE -D__BSD_VISIBLE=1)
    elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
        add_definitions(-D_NETBSD_SOURCE)
    endif()
elseif(APPLE)
    add_definitions(-D_DARWIN_C_SOURCE)
elseif(CMAKE_SYSTEM_NAME MATCHES "AIX")
    add_definitions(-D_ALL_SOURCE)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pipe")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE -fstack-protector-all")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,-z,now -Wl,-z,relro -pie -Wl,-O1 -Wl,--sort-common -Wl,--as-needed -Wl,-z,pack-relative-relocs -flto=auto")

# Debug flags
set(CMAKE_C_FLAGS_DEBUG "-U_FORTIFY_SOURCE -UNDEBUG -O0 -g3 -ggdb -Wall -Wextra -pedantic -Wno-missing-field-initializers -Wno-unused-parameter")

# Release flags
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Find pkg-config
find_package(PkgConfig REQUIRED)

# Check for memrchr
include(CheckSymbolExists)
set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_symbol_exists(memrchr "string.h" HAVE_MEMRCHR)
if(HAVE_MEMRCHR)
    add_definitions(-DHAVE_MEMRCHR=1)
else()
    add_definitions(-DHAVE_MEMRCHR=0)
endif()

# Source files (from Makefile)
set(SOURCES
    array.c
    buffer.c
    event-basic.c
    libutf.c
    main.c
    map.c
    sam.c
    text-common.c
    text-io.c
    text-iterator.c
    text-motions.c
    text-objects.c
    text-util.c
    text.c
    ui-terminal.c
    view.c
    vis-lua.c
    vis-marks.c
    vis-modes.c
    vis-motions.c
    vis-operators.c
    vis-prompt.c
    vis-registers.c
    vis-subprocess.c
    vis-text-objects.c
    vis.c
)
if(ENABLE_TRE)
    list(APPEND SOURCES "text-regex-tre.c")
else()
    list(APPEND SOURCES "text-regex.c")
endif()

# Check source files exist
foreach(src ${SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        message(FATAL_ERROR "Source file ${src} not found!")
    endif()
endforeach()

# Check additional executables' sources
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vis-menu.c")
    message(WARNING "vis-menu.c not found, skipping vis-menu")
else()
    set(BUILD_VIS_MENU 1)
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vis-digraph.c")
    message(WARNING "vis-digraph.c not found, skipping vis-digraph")
else()
    set(BUILD_VIS_DIGRAPH 1)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

# Find mandatory libtermkey
pkg_check_modules(TERMKEY REQUIRED termkey)
list(APPEND LIBS ${TERMKEY_LIBRARIES})
include_directories(${TERMKEY_INCLUDE_DIRS})
link_directories(${TERMKEY_LIBRARY_DIRS})

# Find curses
set(CONFIG_CURSES 0)
if(ENABLE_CURSES)
    foreach(lib ncursesw ncurses curses)
        pkg_check_modules(CURSES QUIET ${lib})
        if(CURSES_FOUND)
            set(CONFIG_CURSES 1)
            list(APPEND LIBS ${CURSES_LIBRARIES})
            include_directories(${CURSES_INCLUDE_DIRS})
            link_directories(${CURSES_LIBRARY_DIRS})
            break()
        endif()
    endforeach()
    if(ENABLE_CURSES AND NOT CONFIG_CURSES)
        message(FATAL_ERROR "Curses library is required but not found")
    endif()
endif()

# Find TRE
set(CONFIG_TRE 0)
if(ENABLE_TRE)
    pkg_check_modules(TRE QUIET tre)
    if(TRE_FOUND)
        set(CONFIG_TRE 1)
        list(APPEND LIBS ${TRE_LIBRARIES})
        include_directories(${TRE_INCLUDE_DIRS})
        link_directories(${TRE_LIBRARY_DIRS})
        message(STATUS "Using TRE regex support")
    else()
        message(FATAL_ERROR "TRE library not found but ENABLE_TRE=ON")
    endif()
endif()

# Find Lua
set(CONFIG_LUA 0)
if(ENABLE_LUA)
    foreach(lib lua lua5.4 lua5.3 lua5.2 lua-5.4 lua-5.3 lua-5.2 lua54 lua53 lua52)
        pkg_check_modules(LUA QUIET ${lib})
        if(LUA_FOUND)
            set(CONFIG_LUA 1)
            add_definitions(-DLUA_COMPAT_5_1 -DLUA_COMPAT_5_2 -DLUA_COMPAT_5_3 -DLUA_COMPAT_ALL)
            list(APPEND LIBS ${LUA_LIBRARIES} m)
            include_directories(${LUA_INCLUDE_DIRS})
            link_directories(${LUA_LIBRARY_DIRS})
            break()
        endif()
    endforeach()
    if(ENABLE_LUA AND NOT CONFIG_LUA)
        message(FATAL_ERROR "Lua library (>= 5.2) not found but ENABLE_LUA=ON")
    endif()
endif()

# Find LPeg (requires Lua)
set(CONFIG_LPEG 0)
if(ENABLE_LPEG_STATIC AND CONFIG_LUA)
    foreach(lib lpeg lua5.4-lpeg lua5.3-lpeg lua5.2-lpeg)
        pkg_check_modules(LPEG QUIET ${lib})
        if(LPEG_FOUND)
            set(CONFIG_LPEG 1)
            list(APPEND LIBS ${LPEG_LIBRARIES})
            include_directories(${LPEG_INCLUDE_DIRS})
            link_directories(${LPEG_LIBRARY_DIRS})
            break()
        endif()
    endforeach()
    if(ENABLE_LPEG_STATIC AND NOT CONFIG_LPEG)
        message(WARNING "LPeg library not found, disabling LPeg support")
    endif()
endif()

# Find ACL (Linux only)
set(CONFIG_ACL 0)
if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND ENABLE_ACL)
    pkg_check_modules(ACL QUIET libacl)
    if(ACL_FOUND)
        set(CONFIG_ACL 1)
        list(APPEND LIBS ${ACL_LIBRARIES})
        include_directories(${ACL_INCLUDE_DIRS})
        link_directories(${ACL_LIBRARY_DIRS})
    else()
        message(FATAL_ERROR "ACL library not found but ENABLE_ACL=ON")
    endif()
endif()

# Find SELinux (Linux only)
set(CONFIG_SELINUX 0)
if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND ENABLE_SELINUX)
    pkg_check_modules(SELINUX QUIET libselinux)
    if(SELINUX_FOUND)
        set(CONFIG_SELINUX 1)
        list(APPEND LIBS ${SELINUX_LIBRARIES})
        include_directories(${SELINUX_INCLUDE_DIRS})
        link_directories(${SELINUX_LIBRARY_DIRS})
    else()
        message(WARNING "SELinux library not found, disabling SELinux support")
    endif()
endif()

# Define help configuration
set(CONFIG_HELP ${ENABLE_HELP})

# Add configuration definitions
add_definitions(

)

# Executables
add_executable(vis ${SOURCES})
target_compile_definitions(vis PRIVATE
    CONFIG_HELP=${CONFIG_HELP}
    CONFIG_CURSES=${CONFIG_CURSES}
    CONFIG_TRE=${CONFIG_TRE}
    CONFIG_LUA=${CONFIG_LUA}
    CONFIG_LPEG=${CONFIG_LPEG}
    CONFIG_ACL=${CONFIG_ACL}
    CONFIG_SELINUX=${CONFIG_SELINUX}
)
target_link_libraries(vis PRIVATE ${LIBS})

if(BUILD_VIS_MENU)
    add_executable(vis-menu vis-menu.c)
    target_link_libraries(vis-menu PRIVATE -lc)
endif()

if(BUILD_VIS_DIGRAPH)
    add_executable(vis-digraph vis-digraph.c)
    target_link_libraries(vis-digraph PRIVATE -lc)
endif()

# Copy Lua files to the build directory
add_custom_command(TARGET vis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/lua"
    "${CMAKE_BINARY_DIR}/lua"
    COMMENT "Copying Lua files to build directory"
)

# Copy vis-clipboard script to the build directory
add_custom_command(TARGET vis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_SOURCE_DIR}/vis-clipboard"
    "${CMAKE_BINARY_DIR}/vis-clipboard"
    COMMENT "Copying vis-clipboard script to build directory"
)

# Copy vis-open script to the build directory
add_custom_command(TARGET vis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_SOURCE_DIR}/vis-open"
    "${CMAKE_BINARY_DIR}/vis-open"
    COMMENT "Copying vis-open script to build directory"
)

# Copy vis-complete script to the build directory
add_custom_command(TARGET vis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_SOURCE_DIR}/vis-complete"
    "${CMAKE_BINARY_DIR}/vis-complete"
    COMMENT "Copying vis-complete script to build directory"
)

# Installation
install(TARGETS vis DESTINATION ${BINDIR})
if(BUILD_VIS_MENU)
    install(TARGETS vis-menu DESTINATION ${BINDIR})
endif()
if(BUILD_VIS_DIGRAPH)
    install(TARGETS vis-digraph DESTINATION ${BINDIR})
endif()
install(PROGRAMS ${CMAKE_SOURCE_DIR}/vis-clipboard ${CMAKE_SOURCE_DIR}/vis-open DESTINATION ${BINDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/lua DESTINATION ${SHAREDIR}/vis)
if(EXISTS "${CMAKE_SOURCE_DIR}/doc")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/doc/ DESTINATION ${DOCDIR})
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/man")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/man/ DESTINATION ${MANDIR})
endif()

# Generate config.h
configure_file(
    ${CMAKE_SOURCE_DIR}/config.def.h
    ${CMAKE_BINARY_DIR}/config.h
    COPYONLY
)

# Print configuration summary
message(STATUS "Configuration Summary:")
message(STATUS "  Prefix: ${PREFIX}")
message(STATUS "  Bindir: ${BINDIR}")
message(STATUS "  Sources found: ${SOURCES}")
message(STATUS "  Curses: ${CONFIG_CURSES}")
message(STATUS "  Lua: ${CONFIG_LUA}")
message(STATUS "  LPeg: ${CONFIG_LPEG}")
message(STATUS "  TRE: ${CONFIG_TRE}")
message(STATUS "  ACL: ${CONFIG_ACL}")
message(STATUS "  SELinux: ${CONFIG_SELINUX}")
message(STATUS "  Help: ${CONFIG_HELP}")
message(STATUS "  memrchr: ${HAVE_MEMRCHR}")

# Add uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY
)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    COMMENT "Uninstalling project"
)

add_subdirectory(test/core)
add_subdirectory(test/lua)
add_subdirectory(test/fuzz)
add_subdirectory(test/sam)
add_subdirectory(test/util)
add_subdirectory(test/vim)
add_subdirectory(test/vis)

add_custom_target(dist
    COMMAND ${CMAKE_COMMAND} -E echo "creating dist tarball"
    COMMAND ${GIT_EXECUTABLE} archive --prefix=vis-${VERSION_STRING}/ -o ${CMAKE_BINARY_DIR}/vis-${VERSION_STRING}.tar.gz HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Creating distribution tarball"
)

# Man page generation
file(GLOB MANUALS "${CMAKE_SOURCE_DIR}/man/*.1")
add_custom_target(man)
set(MAN_HTML_TARGETS)
if(MANDOC_EXECUTABLE)
foreach(man_file ${MANUALS})
    get_filename_component(man_name ${man_file} NAME)
    get_filename_component(man_base ${man_file} NAME_WE)
    set(HTML_OUTPUT_FILE "${CMAKE_BINARY_DIR}/man/${man_base}.html")
    add_custom_target(man_html_${man_base} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/man"
        COMMAND sed -e "s/VERSION/${VERSION_STRING}/" "${man_file}" | ${MANDOC_EXECUTABLE} -W warning -T utf8 -T html -O man=%N.%S.html -O style=mandoc.css 1> ${HTML_OUTPUT_FILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating HTML for ${man_name}"
    )
    list(APPEND MAN_HTML_TARGETS man_html_${man_base})
endforeach()
add_dependencies(man ${MAN_HTML_TARGETS})
else()
    add_custom_command(TARGET man POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "mandoc executable not found. man target skipped."
    )
endif()

add_custom_target(luadoc
    COMMENT "Generating Lua documentation"
)
if(LDOC_EXECUTABLE)
    add_custom_command(TARGET luadoc POST_BUILD
        COMMAND cd ${CMAKE_SOURCE_DIR}/lua/doc && ${LDOC_EXECUTABLE} . && sed -e "s/RELEASE/${VERSION_STRING}/" -i index.html
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
    add_custom_command(TARGET luadoc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "ldoc executable not found. luadoc target skipped."
    )
endif()

add_custom_target(luadoc-all
    COMMENT "Generating all Lua documentation"
)
if(LDOC_EXECUTABLE)
    add_custom_command(TARGET luadoc-all POST_BUILD
        COMMAND cd ${CMAKE_SOURCE_DIR}/lua/doc && ${LDOC_EXECUTABLE} -a . && sed -e "s/RELEASE/${VERSION_STRING}/" -i index.html
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
    add_custom_command(TARGET luadoc-all POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "ldoc executable not found. luadoc-all target skipped."
    )
endif()

add_custom_target(luacheck
    COMMENT "Running luacheck"
)
if(LUACHECK_EXECUTABLE)
    add_custom_command(TARGET luacheck POST_BUILD
        COMMAND ${LUACHECK_EXECUTABLE} --config ${CMAKE_SOURCE_DIR}/.luacheckrc ${CMAKE_SOURCE_DIR}/lua ${CMAKE_SOURCE_DIR}/test/lua
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
    add_custom_command(TARGET luacheck POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "luacheck executable not found. luacheck target skipped."
    )
endif()

add_custom_target(debug
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/debug
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/debug -DCMAKE_BUILD_TYPE=Debug
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/debug
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Configuring and building a debug version in 'debug' subdirectory"
)

add_custom_target(profile
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/profile
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/profile
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/profile -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-pg;-O2" -DCMAKE_EXE_LINKER_FLAGS="-pg"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/profile
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Configuring and building a profiling version in 'profile' subdirectory"
)

add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/coverage -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/coverage
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Configuring and building a coverage version in 'coverage' subdirectory"
)