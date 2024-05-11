#ifndef VIS_LUA_H
#define VIS_LUA_H

#if CONFIG_LUA
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
bool vis_lua_path_add(Vis*, const char *path);
/* get semicolon separated list of paths to load lua files
 * (*lpath = package.path) and Lua C modules (*cpath = package.cpath)
 * both these pointers need to be free(3)-ed by the caller */
bool vis_lua_paths_get(Vis*, char **lpath, char **cpath);

/* various event handlers, triggered by the vis core */
#if !CONFIG_LUA
#define vis_event_mode_insert_input  vis_insert_key
#define vis_event_mode_replace_input vis_replace_key
#else
void vis_event_mode_insert_input(Vis*, const char *key, size_t len);
void vis_event_mode_replace_input(Vis*, const char *key, size_t len);
#endif
void vis_lua_process_response(Vis *, const char *, char *, size_t, ResponseType);

#endif
