#ifndef VIS_LUA_H
#define VIS_LUA_H

#if CONFIG_LUA
#define LUA_COMPAT_5_1
#define LUA_COMPAT_5_2
#define LUA_COMPAT_5_3
#define LUA_COMPAT_ALL
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#else
typedef struct lua_State lua_State;
typedef void* lua_CFunction;
#endif

#include "vis.h"
#include "vis-subprocess.h"

/* add a directory to consider when loading lua files */
VIS_INTERNAL bool vis_lua_path_add(Vis*, const char *path);
/* get semicolon separated list of paths to load lua files
 * (*lpath = package.path) and Lua C modules (*cpath = package.cpath)
 * both these pointers need to be free(3)-ed by the caller */
VIS_INTERNAL bool vis_lua_paths_get(Vis*, char **lpath, char **cpath);

/* various event handlers, triggered by the vis core */
#if !CONFIG_LUA
#define vis_event_mode_insert_input  vis_insert_key
#define vis_event_mode_replace_input vis_replace_key
#else
VIS_INTERNAL void vis_event_mode_insert_input(Vis*, const char *key, size_t len);
VIS_INTERNAL void vis_event_mode_replace_input(Vis*, const char *key, size_t len);
#endif
VIS_INTERNAL void vis_lua_process_response(Vis *, const char *, char *, size_t, ResponseType);

#endif
