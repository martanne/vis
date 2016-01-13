#ifndef VIS_LUA_H
#define VIS_LUA_H

#if CONFIG_LUA
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#else
typedef struct lua_State lua_State;
#endif

#include "vis.h"

void vis_lua_start(Vis*);
void vis_lua_quit(Vis*);
void vis_lua_file_open(Vis*, File*);
void vis_lua_file_save(Vis*, File*);
void vis_lua_file_close(Vis*, File*);
void vis_lua_win_open(Vis*, Win*);
void vis_lua_win_close(Vis*, Win*);

#endif
