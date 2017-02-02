/***
 * Lua Extension API for the [Vis Editor](https://github.com/martanne/vis).
 *
 * *WARNING:* there is no stability guarantee at this time, the API might
 * change without notice!
 *
 * This document might be out of date, run `make luadoc` to regenerate it.
 *
 * @module vis
 * @author Marc André Tanner
 * @license ISC
 * @release RELEASE
 */
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/types.h>
#include <pwd.h>

#include "vis-lua.h"
#include "vis-core.h"
#include "text-motions.h"
#include "util.h"

#ifndef VIS_PATH
#define VIS_PATH "/usr/local/share/vis"
#endif

#ifndef DEBUG_LUA
#define DEBUG_LUA 0
#endif

#if DEBUG_LUA
#define debug(...) do { printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#define debug(...) do { } while (0)
#endif

static void window_status_update(Vis *vis, Win *win) {
	char left_parts[4][255] = { "", "", "", "" };
	char right_parts[4][32] = { "", "", "", "" };
	char left[sizeof(left_parts)+LENGTH(left_parts)*8];
	char right[sizeof(right_parts)+LENGTH(right_parts)*8];
	char status[sizeof(left)+sizeof(right)+1];
	size_t left_count = 0;
	size_t right_count = 0;

	View *view = win->view;
	File *file = win->file;
	Text *txt = file->text;
	int width = vis_window_width_get(win);
	enum UiOption options = view_options_get(view);
	bool focused = vis->win == win;
	const char *filename = file_name_get(file);
	const char *mode = vis->mode->status;

	if (focused && mode)
		strcpy(left_parts[left_count++], mode);

	snprintf(left_parts[left_count], sizeof(left_parts[left_count])-1, "%s%s%s",
	         filename ? filename : "[No Name]",
	         text_modified(txt) ? " [+]" : "",
	         vis_macro_recording(vis) ? " @": "");
	left_count++;

	if (text_newline_type(txt) == TEXT_NEWLINE_CRLF)
		strcpy(right_parts[right_count++], "␍␊");

	int cursor_count = view_cursors_count(view);
	if (cursor_count > 1) {
		Cursor *c = view_cursors_primary_get(view);
		int cursor_number = view_cursors_number(c) + 1;
		snprintf(right_parts[right_count], sizeof(right_parts[right_count])-1,
		         "%d/%d", cursor_number, cursor_count);
		right_count++;
	}

	size_t size = text_size(txt);
	size_t pos = view_cursor_get(view);
	size_t percent = 0;
	if (size > 0) {
		double tmp = ((double)pos/(double)size)*100;
		percent = (size_t)(tmp+1);
	}
	snprintf(right_parts[right_count], sizeof(right_parts[right_count])-1,
	         "%zu%%", percent);
	right_count++;

	if (!(options & UI_OPTION_LARGE_FILE)) {
		Cursor *cur = view_cursors_primary_get(win->view);
		size_t line = view_cursors_line(cur);
		size_t col = view_cursors_col(cur);
		if (col > UI_LARGE_FILE_LINE_SIZE) {
			options |= UI_OPTION_LARGE_FILE;
			view_options_set(win->view, options);
		}
		snprintf(right_parts[right_count], sizeof(right_parts[right_count])-1,
		         "%zu, %zu", line, col);
		right_count++;
	}

	int left_len = snprintf(left, sizeof(left)-1, " %s%s%s%s%s%s%s",
	         left_parts[0],
	         left_parts[1][0] ? " » " : "",
	         left_parts[1],
	         left_parts[2][0] ? " » " : "",
	         left_parts[2],
	         left_parts[3][0] ? " » " : "",
	         left_parts[3]);

	int right_len = snprintf(right, sizeof(right)-1, "%s%s%s%s%s%s%s ",
	         right_parts[0],
	         right_parts[1][0] ? " « " : "",
	         right_parts[1],
	         right_parts[2][0] ? " « " : "",
	         right_parts[2],
	         right_parts[3][0] ? " « " : "",
	         right_parts[3]);

	if (left_len < 0 || right_len < 0)
		return;
	int left_width = text_string_width(left, left_len);
	int right_width = text_string_width(right, right_len);

	int spaces = width - left_width - right_width;
	if (spaces < 1)
		spaces = 1;

	snprintf(status, sizeof(status)-1, "%s%*s%s", left, spaces, " ", right);
	vis_window_status(win, status);
}

#if !CONFIG_LUA

bool vis_lua_path_add(Vis *vis, const char *path) { return true; }
bool vis_lua_paths_get(Vis *vis, char **lpath, char **cpath) { return false; }
void vis_lua_init(Vis *vis) { }
void vis_lua_start(Vis *vis) { }
void vis_lua_quit(Vis *vis) { }
void vis_lua_file_open(Vis *vis, File *file) { }
bool vis_lua_file_save_pre(Vis *vis, File *file, const char *path) { return true; }
void vis_lua_file_save_post(Vis *vis, File *file, const char *path) { }
void vis_lua_file_close(Vis *vis, File *file) { }
void vis_lua_win_open(Vis *vis, Win *win) { }
void vis_lua_win_close(Vis *vis, Win *win) { }
void vis_lua_win_highlight(Vis *vis, Win *win, size_t horizon) { }
bool vis_lua_win_syntax(Vis *vis, Win *win, const char *syntax) { return true; }
bool vis_theme_load(Vis *vis, const char *name) { return true; }
void vis_lua_win_status(Vis *vis, Win *win) { window_status_update(vis, win); }

#else

#if DEBUG_LUA
static void stack_dump_entry(lua_State *L, int i) {
	int t = lua_type(L, i);
	switch (t) {
	case LUA_TNIL:
		printf("nil");
		break;
	case LUA_TBOOLEAN:
		printf(lua_toboolean(L, i) ? "true" : "false");
		break;
	case LUA_TLIGHTUSERDATA:
		printf("lightuserdata(%p)", lua_touserdata(L, i));
		break;
	case LUA_TNUMBER:
		printf("%g", lua_tonumber(L, i));
		break;
	case LUA_TSTRING:
		printf("`%s'", lua_tostring(L, i));
		break;
	case LUA_TTABLE:
		printf("table[");
		lua_pushnil(L); /* first key */
		while (lua_next(L, i > 0 ? i : i - 1)) {
			stack_dump_entry(L, -2);
			printf("=");
			stack_dump_entry(L, -1);
			printf(",");
			lua_pop(L, 1); /* remove value, keep key */
		}
		printf("]");
		break;
	case LUA_TUSERDATA:
		printf("userdata(%p)", lua_touserdata(L, i));
		break;
	default:  /* other values */
		printf("%s", lua_typename(L, t));
		break;
	}
}

static void stack_dump(lua_State *L, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	int top = lua_gettop(L);
	for (int i = 1; i <= top; i++) {
		printf("%d: ", i);
		stack_dump_entry(L, i);
		printf("\n");
	}
	printf("\n\n");
	fflush(stdout);
}

#endif

static int error_function(lua_State *L) {
	Vis *vis = lua_touserdata(L, lua_upvalueindex(1));
	if (vis->errorhandler)
		return 1;
	vis->errorhandler = true;
	size_t len;
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	msg = lua_tolstring(L, 1, &len);
	vis_message_show(vis, msg);
	vis->errorhandler = false;
	return 1;
}

static int pcall(Vis *vis, lua_State *L, int nargs, int nresults) {
	/* insert a custom error function below all arguments */
	int msgh = lua_gettop(L) - nargs;
	lua_pushlightuserdata(L, vis);
	lua_pushcclosure(L, error_function, 1);
	lua_insert(L, msgh);
	int ret = lua_pcall(L, nargs, nresults, msgh);
	lua_remove(L, msgh);
	return ret;
}

/* expects a lua function at stack position `narg` and stores a
 * reference to it in the registry. The return value can be used
 * to look it up.
 *
 *   registry["vis.functions"][(void*)(function)] = function
 */
static const void *func_ref_new(lua_State *L, int narg) {
	const void *addr = lua_topointer(L, narg);
	if (!lua_isfunction(L, narg) || !addr)
		luaL_argerror(L, narg, "function expected");
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.functions");
	lua_pushlightuserdata(L, (void*)addr);
	lua_pushvalue(L, narg);
	lua_settable(L, -3);
	lua_pop(L, 1);
	return addr;
}

/* retrieve function from registry and place it at the top of the stack */
static bool func_ref_get(lua_State *L, const void *addr) {
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.functions");
	lua_pushlightuserdata(L, (void*)addr);
	lua_gettable(L, -2);
	lua_remove(L, -2);
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 1);
		return false;
	}
	return true;
}

/* creates a new metatable for a given type and stores a mapping:
 *
 *   registry["vis.types"][metatable] = type
 *
 * leaves the metatable at the top of the stack.
 */
static void obj_type_new(lua_State *L, const char *type) {
	luaL_newmetatable(L, type);
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.types");
	lua_pushvalue(L, -2);
	lua_pushstring(L, type);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

/* get type of userdatum at the top of the stack:
 *
 *   return registry["vis.types"][getmetatable(userdata)]
 */
const char *obj_type_get(lua_State *L) {
	if (lua_isnil(L, -1))
		return "nil";
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.types");
	lua_getmetatable(L, -2);
	lua_gettable(L, -2);
	// XXX: in theory string might become invalid when poped from stack
	const char *type = lua_tostring(L, -1);
	lua_pop(L, 2);
	return type;
}

static void *obj_new(lua_State *L, size_t size, const char *type) {
	void *obj = lua_newuserdata(L, size);
	luaL_getmetatable(L, type);
	lua_setmetatable(L, -2);
	lua_newtable(L);
	lua_setuservalue(L, -2);
	return obj;
}

/* returns registry["vis.objects"][addr] if it is of correct type */
static void *obj_ref_get(lua_State *L, void *addr, const char *type) {
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.objects");
	lua_pushlightuserdata(L, addr);
	lua_gettable(L, -2);
	lua_remove(L, -2);
	if (lua_isnil(L, -1)) {
		debug("get: vis.objects[%p] = nil\n", addr);
		lua_pop(L, 1);
		return NULL;
	}
	if (DEBUG_LUA) {
		const char *actual_type = obj_type_get(L);
		if (strcmp(type, actual_type) != 0)
			debug("get: vis.objects[%p] = %s (BUG: expected %s)\n", addr, actual_type, type);
		void **handle = luaL_checkudata(L, -1, type);
		if (!handle)
			debug("get: vis.objects[%p] = %s (BUG: invalid handle)\n", addr, type);
		else if (*handle != addr)
			debug("get: vis.objects[%p] = %s (BUG: handle mismatch %p)\n", addr, type, *handle);
	}
	return luaL_checkudata(L, -1, type);
}

/* expects a userdatum at the top of the stack and sets
 *
 *   registry["vis.objects"][addr] = userdata
 */
static void obj_ref_set(lua_State *L, void *addr) {
	//debug("set: vis.objects[%p] = %s\n", addr, obj_type_get(L));
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.objects");
	lua_pushlightuserdata(L, addr);
	lua_pushvalue(L, -3);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

/* invalidates an object reference
 *
 *   registry["vis.objects"][addr] = nil
 */
static void obj_ref_free(lua_State *L, void *addr) {
	if (DEBUG_LUA) {
		lua_getfield(L, LUA_REGISTRYINDEX, "vis.objects");
		lua_pushlightuserdata(L, addr);
		lua_gettable(L, -2);
		lua_remove(L, -2);
		if (lua_isnil(L, -1))
			debug("free-unused: %p\n", addr);
		else
			debug("free: vis.objects[%p] = %s\n", addr, obj_type_get(L));
		lua_pop(L, 1);
	}
	lua_pushnil(L);
	obj_ref_set(L, addr);
}

/* creates a new object reference of given type if it does not already exist in the registry:
 *
 *  if (registry["vis.types"][metatable(registry["vis.objects"][addr])] != type) {
 *      // XXX: should not happen
 *      registry["vis.objects"][addr] = new_obj(addr, type)
 *  }
 *  return registry["vis.objects"][addr];
 */
static void *obj_ref_new(lua_State *L, void *addr, const char *type) {
	if (!addr) {
		lua_pushnil(L);
		return NULL;
	}
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.objects");
	lua_pushlightuserdata(L, addr);
	lua_gettable(L, -2);
	lua_remove(L, -2);
	const char *old_type = obj_type_get(L);
	if (strcmp(type, old_type) == 0) {
		debug("new: vis.objects[%p] = %s (returning existing object)\n", addr, old_type);
		void **handle = luaL_checkudata(L, -1, type);
		if (!handle)
			debug("new: vis.objects[%p] = %s (BUG: invalid handle)\n", addr, old_type);
		else if (*handle != addr)
			debug("new: vis.objects[%p] = %s (BUG: handle mismatch %p)\n", addr, old_type, *handle);
		return addr;
	}
	if (!lua_isnil(L, -1))
		debug("new: vis.objects[%p] = %s (WARNING: changing object type from %s)\n", addr, type, old_type);
	else
		debug("new: vis.objects[%p] = %s (creating new object)\n", addr, type);
	lua_pop(L, 1);
	void **handle = obj_new(L, sizeof(addr), type);
	obj_ref_set(L, addr);
	*handle = addr;
	return addr;
}

/* retrieve object stored in reference at stack location `idx' */
static void *obj_ref_check_get(lua_State *L, int idx, const char *type) {
	void **addr = luaL_checkudata(L, idx, type);
	if (!obj_ref_get(L, *addr, type))
		return NULL;
	return *addr;
}

/* (type) check validity of object reference at stack location `idx' */
static void *obj_ref_check(lua_State *L, int idx, const char *type) {
	void *obj = obj_ref_check_get(L, idx, type);
	if (obj)
		lua_pop(L, 1);
	else
		luaL_argerror(L, idx, "invalid object reference");
	return obj;
}

static void *obj_ref_check_containerof(lua_State *L, int idx, const char *type, size_t offset) {
	void *obj = obj_ref_check(L, idx, type);
	return obj ? ((char*)obj-offset) : obj;
}

static void *obj_lightref_new(lua_State *L, void *addr, const char *type) {
	if (!addr)
		return NULL;
	void **handle = obj_new(L, sizeof(addr), type);
	*handle = addr;
	return addr;
}

static void *obj_lightref_check(lua_State *L, int idx, const char *type) {
	void **addr = luaL_checkudata(L, idx, type);
	return *addr;
}

static int index_common(lua_State *L) {
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	if (lua_isnil(L, -1)) {
		lua_getuservalue(L, 1);
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);
	}
	return 1;
}

static int newindex_common(lua_State *L) {
	lua_getuservalue(L, 1);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_settable(L, -3);
	return 0;
}

static size_t checkpos(lua_State *L, int narg) {
	lua_Number n = luaL_checknumber(L, narg);
	if (n >= 0 && n <= SIZE_MAX && n == (size_t)n)
		return n;
	return luaL_argerror(L, narg, "expected position, got number");
}

static void pushrange(lua_State *L, Filerange *r) {
	if (!text_range_valid(r)) {
		lua_pushnil(L);
		return;
	}
	lua_createtable(L, 0, 2);
	lua_pushstring(L, "start");
	lua_pushunsigned(L, r->start);
	lua_settable(L, -3);
	lua_pushstring(L, "finish");
	lua_pushunsigned(L, r->end);
	lua_settable(L, -3);
}

static Filerange getrange(lua_State *L, int index) {
	Filerange range = text_range_empty();
	if (lua_istable(L, index)) {
		lua_getfield(L, index, "start");
		range.start = checkpos(L, -1);
		lua_pop(L, 1);
		lua_getfield(L, index, "finish");
		range.end = checkpos(L, -1);
		lua_pop(L, 1);
	} else {
		range.start = checkpos(L, index);
		range.end = range.start + checkpos(L, index+1);
	}
	return range;
}

static const char *keymapping(Vis *vis, const char *keys, const Arg *arg) {
	lua_State *L = vis->lua;
	if (!func_ref_get(L, arg->v))
		return keys;
	lua_pushstring(L, keys);
	if (pcall(vis, L, 1, 1) != 0)
		return keys;
	if (lua_type(L, -1) != LUA_TNUMBER)
		return keys; /* invalid or no return value, assume zero */
	lua_Number number = lua_tonumber(L, -1);
	lua_Integer integer = lua_tointeger(L, -1);
	if (number != integer)
		return keys;
	if (integer < 0)
		return NULL; /* need more input */
	size_t len = integer;
	size_t max = strlen(keys);
	return (len <= max) ? keys+len : keys;
}

/***
 * The main editor object.
 * @type Vis
 */

/***
 * Version information.
 * @tfield string VERSION
 * version information in `git describe` format, same as reported by `vis -v`.
 */
/***
 * User interface.
 * @tfield Ui ui the user interface being used
 */
/***
 * Mode constants.
 * @tfield modes modes
 */
/***
 * Events.
 * @tfield events events
 */
/***
 * Registers.
 * @field registers array to access the register by single letter name
 */
/***
 * LPeg lexer support module.
 * @field lexers
 */

// TODO vis.events

/***
 * Create an iterator over all windows.
 * @function windows
 * @return the new iterator
 * @see win
 * @usage
 * for win in vis:windows() do
 * 	-- do something with win
 * end
 */
static int windows_iter(lua_State *L);
static int windows(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	Win **handle = lua_newuserdata(L, sizeof *handle);
	*handle = vis->windows;
	lua_pushcclosure(L, windows_iter, 1);
	return 1;
}

static int windows_iter(lua_State *L) {
	Win **handle = lua_touserdata(L, lua_upvalueindex(1));
	if (!*handle)
		return 0;
	Win *win = obj_ref_new(L, *handle, "vis.window");
	if (win)
		*handle = win->next;
	return 1;
}

/***
 * Create an iterator over all files.
 * @function files
 * @return the new iterator
 * @usage
 * for file in vis:files() do
 * 	-- do something with file
 * end
 */
static int files_iter(lua_State *L);
static int files(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	File **handle = lua_newuserdata(L, sizeof *handle);
	*handle = vis->files;
	lua_pushcclosure(L, files_iter, 1);
	return 1;
}

static int files_iter(lua_State *L) {
	File **handle = lua_touserdata(L, lua_upvalueindex(1));
	if (!*handle)
		return 0;
	File *file = obj_ref_new(L, *handle, "vis.file");
	if (file)
		*handle = file->next;
	return 1;
}

/***
 * Create an iterator over all mark names.
 * @function mark_names
 * @return the new iterator
 * @usage
 * local marks = vis.win.file.marks;
 * for mark in vis:mark_names() do
 * 	local pos = marks[mark]
 * 	if pos then
 * 		-- do something with mark pos
 * 	end
 * end
 */
static int mark_names_iter(lua_State *L);
static int mark_names(lua_State *L) {
	enum VisMark *handle = lua_newuserdata(L, sizeof *handle);
	*handle = 0;
	lua_pushcclosure(L, mark_names_iter, 1);
	return 1;
}

static int mark_names_iter(lua_State *L) {
	char mark = '\0';
	enum VisMark *handle = lua_touserdata(L, lua_upvalueindex(1));
	if (*handle < LENGTH(vis_marks))
		mark = vis_marks[*handle].name;
	else if (VIS_MARK_a <= *handle && *handle <= VIS_MARK_z)
		mark = 'a' + *handle - VIS_MARK_a;
	if (mark) {
		char name[2] = { mark, '\0' };
		lua_pushstring(L, name);
		(*handle)++;
		return 1;
	}
	return 0;
}

/***
 * Create an iterator over all register names.
 * @function register_names
 * @return the new iterator
 * @usage
 * for reg in vis:register_names() do
 * 	local value = vis.registers[reg]
 * 	if value then
 * 		-- do something with register value
 * 	end
 * end
 */
static int register_names_iter(lua_State *L);
static int register_names(lua_State *L) {
	enum VisRegister *handle = lua_newuserdata(L, sizeof *handle);
	*handle = 0;
	lua_pushcclosure(L, register_names_iter, 1);
	return 1;
}

static int register_names_iter(lua_State *L) {
	char reg = '\0';
	enum VisRegister *handle = lua_touserdata(L, lua_upvalueindex(1));
	if (*handle < LENGTH(vis_registers))
		reg = vis_registers[*handle].name;
	else if (VIS_REG_a <= *handle && *handle <= VIS_REG_z)
		reg = 'a' + *handle - VIS_REG_a;
	if (reg) {
		char name[2] = { reg, '\0' };
		lua_pushstring(L, name);
		(*handle)++;
		return 1;
	}
	return 0;
}

/***
 * Execute a `:`-command.
 * @function command
 * @tparam string command the command to execute
 * @treturn bool whether the command succeeded
 * @usage
 * vis:command("set number")
 */
static int command(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const char *cmd = luaL_checkstring(L, 2);
	bool ret = vis_cmd(vis, cmd);
	lua_pushboolean(L, ret);
	return 1;
}

/***
 * Display a short message.
 *
 * The single line message will be displayed at the bottom of
 * the scren and automatically hidden once a key is pressed.
 *
 * @function info
 * @tparam string message the message to display
 */
static int info(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const char *msg = luaL_checkstring(L, 2);
	vis_info_show(vis, "%s", msg);
	return 0;
}

/***
 * Display a multi line message.
 *
 * Opens a new window and displays an arbitrarily long message.
 *
 * @function message
 * @tparam string message the message to display
 */
static int message(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const char *msg = luaL_checkstring(L, 2);
	vis_message_show(vis, msg);
	return 0;
}

/***
 * Open a file.
 *
 * Creates a new window and loads the given file.
 *
 * @function open
 * @tparam string filename the file name to load
 * @treturn File the file object representing the new file, or `nil` on failure
 */
static int open(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const char *name = luaL_checkstring(L, 2);
	if (vis_window_new(vis, name))
		obj_ref_new(L, vis->win->file, "vis.file");
	else
		lua_pushnil(L);
	return 1;
}

/***
 * Register a Lua function as key action.
 * @function action_register
 * @tparam string name the name of the action, can be referred to in key bindings as `<name>` pseudo key
 * @tparam Function func the lua function implementing the key action (see @{keyhandler})
 * @tparam[opt] string help the single line help text as displayed in `:help`
 * @treturn KeyAction action the registered key action
 * @see Vis:map
 * @see Window:map
 */
static int action_register(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const char *name = luaL_checkstring(L, 2);
	const void *func = func_ref_new(L, 3);
	const char *help = luaL_optstring(L, 4, NULL);
	KeyAction *action = vis_action_new(vis, name, help, keymapping, (Arg){ .v = func });
	if (!action)
		goto err;
	if (!vis_action_register(vis, action))
		goto err;
	obj_ref_new(L, action, "vis.keyaction");
	return 1;
err:
	vis_action_free(vis, action);
	lua_pushnil(L);
	return 1;
}

static int keymap(lua_State *L, Vis *vis, Win *win) {
	int mode = luaL_checkint(L, 2);
	const char *key = luaL_checkstring(L, 3);
	const char *help = luaL_optstring(L, 5, NULL);
	KeyBinding *binding = vis_binding_new(vis);
	if (!binding)
		goto err;
	if (lua_isstring(L, 4)) {
		const char *alias = luaL_checkstring(L, 4);
		if (!(binding->alias = strdup(alias)))
			goto err;
	} else if (lua_isfunction(L, 4)) {
		const void *func = func_ref_new(L, 4);
		if (!(binding->action = vis_action_new(vis, NULL, help, keymapping, (Arg){ .v = func })))
			goto err;
	} else if (lua_isuserdata(L, 4)) {
		binding->action = obj_ref_check(L, 4, "vis.keyaction");
	}

	if (win) {
		if (!vis_window_mode_map(win, mode, true, key, binding))
			goto err;
	} else {
		if (!vis_mode_map(vis, mode, true, key, binding))
			goto err;
	}

	lua_pushboolean(L, true);
	return 1;
err:
	vis_binding_free(vis, binding);
	lua_pushboolean(L, false);
	return 1;
}

/***
 * Map a key to a Lua function.
 *
 * Creates a new key mapping in a given mode.
 *
 * @function map
 * @tparam int mode the mode to which the mapping should be added
 * @tparam string key the key to map
 * @tparam function func the Lua function to handle the key mapping (see @{keyhandler})
 * @tparam[opt] string help the single line help text as displayed in `:help`
 * @treturn bool whether the mapping was successfully established
 * @see Window:map
 * @usage
 * vis:map(vis.modes.INSERT, "<C-k>", function(keys)
 * 	if #keys < 2 then
 * 		return -1 -- need more input
 * 	end
 * 	local digraph = keys:sub(1, 2)
 * 	if digraph == "l*" then
 * 		vis:feedkeys('λ')
 * 		return 2 -- consume 2 bytes of input
 * 	end
 * end, "Insert digraph")
 */
/***
 * Setup a key alias.
 *
 * This is equivalent to `vis:command('map! mode key alias')`.
 *
 * Mappings are always recursive!
 * @function map
 * @tparam int mode the mode to which the mapping should be added
 * @tparam string key the key to map
 * @tparam string alias the key to map to
 * @treturn bool whether the mapping was successfully established
 * @see Window:map
 * @usage
 * vis:map(vis.modes.NORMAL, "j", "k")
 */
/***
 * Map a key to a key action.
 *
 * @function map
 * @tparam int mode the mode to which the mapping should be added
 * @tparam string key the key to map
 * @param action the action to map
 * @treturn bool whether the mapping was successfully established
 * @see Window:map
 * @usage
 * local action = vis:action_register("info", function()
 *   vis:info("Mapping works!")
 * end, "Info message help text")
 * vis:map(vis.modes.NORMAL, "gh", action)
 * vis:map(vis.modes.NORMAL, "gl", action)
 */
static int map(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	return keymap(L, vis, NULL);
}

/***
 * Execute a motion.
 *
 * @function motion
 * @tparam int id the id of the motion to execute
 * @treturn bool whether the id was valid
 * @local
 */
static int motion(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	enum VisMotion id = luaL_checkunsigned(L, 2);
	// TODO handle var args?
	lua_pushboolean(L, vis && vis_motion(vis, id));
	return 1;
}

static size_t motion_lua(Vis *vis, Win *win, void *data, size_t pos) {
	lua_State *L = vis->lua;
	if (!func_ref_get(L, data) || !obj_ref_new(L, win, "vis.window"))
		return EPOS;

	lua_pushunsigned(L, pos);
	if (pcall(vis, L, 2, 1) != 0)
		return EPOS;
	return checkpos(L, -1);
}

/***
 * Register a custom motion.
 *
 * @function motion_register
 * @tparam function motion the Lua function implementing the motion
 * @treturn int the associated motion id, or `-1` on failure
 * @see motion, motion_new
 * @local
 * @usage
 * -- custom motion advancing to the next byte
 * local id = vis:motion_register(function(win, pos)
 * 	return pos+1
 * end)
 */
static int motion_register(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const void *func = func_ref_new(L, 2);
	int id = vis_motion_register(vis, 0, (void*)func, motion_lua);
	lua_pushinteger(L, id);
	return 1;
}

/***
 * Execute a text object.
 *
 * @function textobject
 * @tparam int id the id of the text object to execute
 * @treturn bool whether the id was valid
 * @see textobject_register, textobject_new
 * @local
 */
static int textobject(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	enum VisTextObject id = luaL_checkunsigned(L, 2);
	lua_pushboolean(L, vis_textobject(vis, id));
	return 1;
}

static Filerange textobject_lua(Vis *vis, Win *win, void *data, size_t pos) {
	lua_State *L = vis->lua;
	if (!func_ref_get(L, data) || !obj_ref_new(L, win, "vis.window"))
		return text_range_empty();
	lua_pushunsigned(L, pos);
	if (pcall(vis, L, 2, 2) != 0)
		return text_range_empty();
	return text_range_new(checkpos(L, -2), checkpos(L, -1));
}

/***
 * Register a custom text object.
 *
 * @function textobject_register
 * @tparam function textobject the Lua function implementing the text object
 * @treturn int the associated text object id, or `-1` on failure
 * @see textobject, textobject_new
 * @local
 * @usage
 * -- custom text object covering the next byte
 * local id = vis:textobject_register(function(win, pos)
 * 	return pos, pos+1
 * end)
 */
static int textobject_register(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const void *func = func_ref_new(L, 2);
	int id = vis_textobject_register(vis, 0, (void*)func, textobject_lua);
	lua_pushinteger(L, id);
	return 1;
}

static bool command_lua(Vis *vis, Win *win, void *data, bool force, const char *argv[], Cursor *cur, Filerange *range) {
	lua_State *L = vis->lua;
	if (!func_ref_get(L, data))
		return false;
	lua_newtable(L);
	for (size_t i = 0; argv[i]; i++) {
		lua_pushunsigned(L, i);
		lua_pushstring(L, argv[i]);
		lua_settable(L, -3);
	}
	lua_pushboolean(L, force);
	if (!obj_ref_new(L, win, "vis.window"))
		return false;
	if (!cur)
		cur = view_cursors_primary_get(win->view);
	if (!obj_lightref_new(L, cur, "vis.window.cursor"))
		return false;
	pushrange(L, range);
	if (pcall(vis, L, 5, 1) != 0)
		return false;
	return lua_toboolean(L, -1);
}

/***
 * Register a custom `:`-command.
 *
 * @function command_register
 * @tparam string name the command name
 * @tparam function command the Lua function implementing the command
 * @tparam[opt] string help the single line help text as displayed in `:help`
 * @treturn bool whether the command has been successfully registered
 * @usage
 * TODO
 */
static int command_register(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const char *name = luaL_checkstring(L, 2);
	const void *func = func_ref_new(L, 3);
	const char *help = luaL_optstring(L, 4, "");
	bool ret = vis_cmd_register(vis, name, help, (void*)func, command_lua);
	lua_pushboolean(L, ret);
	return 1;
}

/***
 * Push keys to input queue and interpret them.
 *
 * The keys are processed as if they were read from the keyboard.
 *
 * @function feedkeys
 * @tparam string keys the keys to interpret
 */
static int feedkeys(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	const char *keys = luaL_checkstring(L, 2);
	vis_keys_feed(vis, keys);
	return 0;
}

/***
 * Insert keys at all cursor positions of active window.
 *
 * This function behaves as if the keys were entered in insert mode,
 * but in contrast to @{Vis:feedkeys} it bypasses the input queue,
 * meaning mappings do not apply and the keys will not be recorded in macros.
 *
 * @function insert
 * @tparam string keys the keys to insert
 * @see Vis:feedkeys
 */
static int insert(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	size_t len;
	const char *keys = luaL_checklstring(L, 2, &len);
	vis_insert_key(vis, keys, len);
	return 0;
}

/***
 * Replace keys at all cursor positions of active window.
 *
 * This function behaves as if the keys were entered in replace mode,
 * but in contrast to @{Vis:feedkeys} it bypasses the input queue,
 * meaning mappings do not apply and the keys will not be recorded in macros.
 *
 * @function replace
 * @tparam string keys the keys to insert
 * @see Vis:feedkeys
 */
static int replace(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	size_t len;
	const char *keys = luaL_checklstring(L, 2, &len);
	vis_replace_key(vis, keys, len);
	return 0;
}

/***
 * Currently active window.
 * @tfield Window win
 * @see windows
 */
/***
 * Currently active mode.
 * @tfield modes mode
 */
/***
 * Whether a macro is being recorded.
 * @tfield bool recording
 */
// TODO vis.ui
static int vis_index(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "win") == 0) {
			obj_ref_new(L, vis->win, "vis.window");
			return 1;
		}

		if (strcmp(key, "mode") == 0) {
			lua_pushunsigned(L, vis->mode->id);
			return 1;
		}

		if (strcmp(key, "recording") == 0) {
			lua_pushboolean(L, vis_macro_recording(vis));
			return 1;
		}

		if (strcmp(key, "registers") == 0) {
			obj_ref_new(L, vis->ui, "vis.registers");
			return 1;
		}

		if (strcmp(key, "ui") == 0) {
			obj_ref_new(L, vis->ui, "vis.ui");
			return 1;
		}
	}

	return index_common(L);
}

static int vis_newindex(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "mode") == 0) {
			enum VisMode mode = luaL_checkunsigned(L, 3);
			vis_mode_switch(vis, mode);
			return 0;
		}
	}
	return newindex_common(L);
}

static const struct luaL_Reg vis_lua[] = {
	{ "files", files },
	{ "windows", windows },
	{ "mark_names", mark_names },
	{ "register_names", register_names },
	{ "command", command },
	{ "info", info },
	{ "message", message },
	{ "open", open },
	{ "map", map },
	{ "motion", motion },
	{ "motion_register", motion_register },
	{ "textobject", textobject },
	{ "textobject_register", textobject_register },
	{ "command_register", command_register },
	{ "feedkeys", feedkeys },
	{ "insert", insert },
	{ "replace", replace },
	{ "action_register", action_register },
	{ "__index", vis_index },
	{ "__newindex", vis_newindex },
	{ NULL, NULL },
};

static const struct luaL_Reg ui_funcs[] = {
	{ "__index", index_common },
	{ NULL, NULL },
};

static int registers_index(lua_State *L) {
	Vis *vis = lua_touserdata(L, lua_upvalueindex(1));
	const char *symbol = luaL_checkstring(L, 2);
	if (strlen(symbol) != 1)
		goto err;
	enum VisRegister reg = vis_register_from(vis, symbol[0]);
	if (reg >= VIS_REG_INVALID)
		goto err;
	size_t len;
	const char *value = vis_register_get(vis, reg, &len);
	lua_pushlstring(L, value ? value : "" , len);
	return 1;
err:
	lua_pushnil(L);
	return 1;
}

static int registers_newindex(lua_State *L) {
	Vis *vis = lua_touserdata(L, lua_upvalueindex(1));
	const char *symbol = luaL_checkstring(L, 2);
	if (strlen(symbol) != 1)
		return 0;
	enum VisRegister reg = vis_register_from(vis, symbol[0]);
	if (reg >= VIS_REG_INVALID)
		return 0;
	size_t len;
	const char *value = luaL_checklstring(L, 3, &len);
	register_put(vis, &vis->registers[reg], value, len+1);
	return 0;
}

static int registers_len(lua_State *L) {
	Vis *vis = lua_touserdata(L, lua_upvalueindex(1));
	lua_pushunsigned(L, LENGTH(vis->registers));
	return 1;
}

static const struct luaL_Reg registers_funcs[] = {
	{ "__index", registers_index },
	{ "__newindex", registers_newindex },
	{ "__len", registers_len },
	{ NULL, NULL },
};

/***
 * A window object.
 * @type Window
 */

/***
 * Viewport currently being displayed.
 * @tfield Range viewport
 */
/***
 * The window width.
 * @tfield int width
 */
/***
 * The window height.
 * @tfield int height
 */
/***
 * The file being displayed in this window.
 * @tfield File file
 */
/***
 * The primary cursor of this window.
 * @tfield Cursor cursor
 */
/***
 * The cursors of this window.
 * @tfield Array(Cursor) cursors
 */
/***
 * The file type associated with this window.
 *
 * Setting this field to a valid lexer name will automatically active it.
 * @tfield string syntax the syntax lexer name or `nil` if unset
 */
static int window_index(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);

		if (strcmp(key, "viewport") == 0) {
			Filerange r = view_viewport_get(win->view);
			pushrange(L, &r);
			return 1;
		}

		if (strcmp(key, "width") == 0) {
			lua_pushunsigned(L, vis_window_width_get(win));
			return 1;
		}

		if (strcmp(key, "height") == 0) {
			lua_pushunsigned(L, vis_window_height_get(win));
			return 1;
		}

		if (strcmp(key, "file") == 0) {
			obj_ref_new(L, win->file, "vis.file");
			return 1;
		}

		if (strcmp(key, "cursor") == 0) {
			Cursor *cur = view_cursors_primary_get(win->view);
			obj_lightref_new(L, cur, "vis.window.cursor");
			return 1;
		}

		if (strcmp(key, "cursors") == 0) {
			obj_ref_new(L, win->view, "vis.window.cursors");
			return 1;
		}

		if (strcmp(key, "syntax") == 0) {
			const char *syntax = vis_window_syntax_get(win);
			lua_pushstring(L, syntax);
			return 1;
		}
	}

	return index_common(L);
}

static int window_newindex(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "syntax") == 0) {
			const char *syntax = NULL;
			if (!lua_isnil(L, 3))
				syntax = luaL_checkstring(L, 3);
			vis_window_syntax_set(win, syntax);
			return 0;
		}
	}
	return newindex_common(L);
}

static int window_cursors_iterator_next(lua_State *L) {
	Cursor **handle = lua_touserdata(L, lua_upvalueindex(1));
	if (!*handle)
		return 0;
	Cursor *cur = obj_lightref_new(L, *handle, "vis.window.cursor");
	if (!cur)
		return 0;
	*handle = view_cursors_next(cur);
	return 1;
}

/***
 * Create an iterator over all cursors of this window.
 * @function cursors_iterator
 * @return the new iterator
 */
static int window_cursors_iterator(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	Cursor **handle = lua_newuserdata(L, sizeof *handle);
	*handle = view_cursors(win->view);
	lua_pushcclosure(L, window_cursors_iterator_next, 1);
	return 1;
}

/***
 * Set up a window local key mapping.
 * The function signatures are the same as for @{Vis:map}.
 * @function map
 * @param ...
 * @see Vis:map
 */
static int window_map(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	return keymap(L, win->vis, win);
}

/***
 * Define a display style.
 * @function style_define
 * @tparam int id the style id to use
 * @tparam string style the style definition 
 * @treturn bool whether the style definition has been successfully
 *  associated with the given id
 * @see style
 * @usage
 * win:style_define(win.STYLE_DEFAULT, "fore:red")
 */
static int window_style_define(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	enum UiStyle id = luaL_checkunsigned(L, 2);
	const char *style = luaL_checkstring(L, 3);
	bool ret = view_style_define(win->view, id, style);
	lua_pushboolean(L, ret);
	return 1;
}

/***
 * Style a window range.
 *
 * The style will be cleared after every window redraw.
 * @function style
 * @tparam int id the display style as registered with @{style_define}
 * @tparam int start the absolute file position in bytes
 * @tparam int len the length in bytes to style
 * @see style_define
 * @usage
 * win:style(win.STYLE_DEFAULT, 0, 10)
 */
static int window_style(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	enum UiStyle style = luaL_checkunsigned(L, 2);
	size_t start = checkpos(L, 3);
	size_t end = checkpos(L, 4);
	view_style(win->view, style, start, end);
	return 0;
}

/***
 * Set window status line.
 *
 * @function status
 * @tparam string left the left aligned part of the status line
 * @tparam[opt] string right the right aligned part of the status line
 */
static int window_status(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	char status[1024] = "";
	int width = vis_window_width_get(win);
	const char *left = luaL_checkstring(L, 2);
	const char *right = luaL_optstring(L, 3, "");
	int left_width = text_string_width(left, strlen(left));
	int right_width = text_string_width(right, strlen(right));
	int spaces = width - left_width - right_width;
	if (spaces < 1)
		spaces = 1;
	snprintf(status, sizeof(status)-1, "%s%*s%s", left, spaces, " ", right);
	vis_window_status(win, status);
	return 0;
}

/***
 * Redraw window content.
 *
 * @function draw
 */
static int window_draw(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	view_draw(win->view);
	return 0;
}

static const struct luaL_Reg window_funcs[] = {
	{ "__index", window_index },
	{ "__newindex", window_newindex },
	{ "cursors_iterator", window_cursors_iterator },
	{ "map", window_map },
	{ "style_define", window_style_define },
	{ "style", window_style },
	{ "status", window_status },
	{ "draw", window_draw },
	{ NULL, NULL },
};

static int window_cursors_index(lua_State *L) {
	View *view = obj_ref_check(L, 1, "vis.window.cursors");
	size_t index = luaL_checkunsigned(L, 2);
	size_t count = view_cursors_count(view);
	if (index == 0 || index > count)
		goto err;
	for (Cursor *c = view_cursors(view); c; c = view_cursors_next(c)) {
		if (!--index) {
			obj_lightref_new(L, c, "vis.window.cursor");
			return 1;
		}
	}
err:
	lua_pushnil(L);
	return 1;
}

static int window_cursors_len(lua_State *L) {
	View *view = obj_ref_check(L, 1, "vis.window.cursors");
	lua_pushunsigned(L, view_cursors_count(view));
	return 1;
}

static const struct luaL_Reg window_cursors_funcs[] = {
	{ "__index", window_cursors_index },
	{ "__len", window_cursors_len },
	{ NULL, NULL },
};

/***
 * A cursor object.
 * @type Cursor
 */

/***
 * The zero based byte position in the file.
 *
 * Setting this field will move the cursor to the given position.
 * @tfield int pos
 */
/***
 * The 1-based line the cursor resides on.
 *
 * @tfield int line
 * @see to
 */
/***
 * The 1-based column position the cursor resides on.
 * @tfield int col
 * @see to
 */
/***
 * The 1-based cursor index.
 * @tfield int number
 */
/***
 * The selection associated with this cursor.
 * @tfield Range selection the selection or `nil` if not in visual mode
 */
static int window_cursor_index(lua_State *L) {
	Cursor *cur = obj_lightref_check(L, 1, "vis.window.cursor");
	if (!cur) {
		lua_pushnil(L);
		return 1;
	}

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "pos") == 0) {
			lua_pushunsigned(L, view_cursors_pos(cur));
			return 1;
		}

		if (strcmp(key, "line") == 0) {
			lua_pushunsigned(L, view_cursors_line(cur));
			return 1;
		}

		if (strcmp(key, "col") == 0) {
			lua_pushunsigned(L, view_cursors_col(cur));
			return 1;
		}

		if (strcmp(key, "number") == 0) {
			lua_pushunsigned(L, view_cursors_number(cur)+1);
			return 1;
		}

		if (strcmp(key, "selection") == 0) {
			Filerange sel = view_cursors_selection_get(cur);
			pushrange(L, &sel);
			return 1;
		}
	}

	return index_common(L);
}

static int window_cursor_newindex(lua_State *L) {
	Cursor *cur = obj_lightref_check(L, 1, "vis.window.cursor");
	if (!cur)
		return 0;
	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "pos") == 0) {
			size_t pos = checkpos(L, 3);
			view_cursors_to(cur, pos);
			return 0;
		}

		if (strcmp(key, "selection") == 0) {
			Filerange sel = getrange(L, 3);
			if (text_range_valid(&sel))
				view_cursors_selection_set(cur, &sel);
			else
				view_cursors_selection_clear(cur);
			return 0;
		}
	}
	return newindex_common(L);
}

/***
 * Move cursor.
 * @function to
 * @tparam int line the 1-based line number
 * @tparam int col the 1-based column number
 */
static int window_cursor_to(lua_State *L) {
	Cursor *cur = obj_lightref_check(L, 1, "vis.window.cursor");
	if (cur) {
		size_t line = checkpos(L, 2);
		size_t col = checkpos(L, 3);
		view_cursors_place(cur, line, col);
	}
	return 0;
}

static const struct luaL_Reg window_cursor_funcs[] = {
	{ "__index", window_cursor_index },
	{ "__newindex", window_cursor_newindex },
	{ "to", window_cursor_to },
	{ NULL, NULL },
};

/***
 * A file object.
 * @type File
 */
/***
 * File name.
 * @tfield string name the file name relative to current working directory or `nil` if not yet named
 */
/***
 * File path.
 * @tfield string path the absolute file path or `nil` if not yet named
 */
/***
 * File content by logical lines.
 *
 * Assigning to array element `0` (`#lines+1`) will insert a new line at
 * the beginning (end) of the file.
 * @tfield Array(string) lines the file content accessible as 1-based array
 * @see content
 * @usage
 * local lines = vis.win.file.lines
 * for i=1, #lines do
 * 	lines[i] = i .. ": " .. lines[i]
 * end
 */
/***
 * Type of line endings.
 * @tfield string newlines the type of line endings either `lf` (the default) or `crlf`
 */
/***
 * File size in bytes.
 * @tfield int size the current file size in bytes
 */
/***
 * File state.
 * @tfield bool modified whether the file contains unsaved changes
 */
/***
 * File marks.
 * @field marks array to access the marks of this file by single letter name
 */
static int file_index(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "name") == 0) {
			lua_pushstring(L, file_name_get(file));
			return 1;
		}

		if (strcmp(key, "path") == 0) {
			lua_pushstring(L, file->name);
			return 1;
		}

		if (strcmp(key, "lines") == 0) {
			obj_ref_new(L, file->text, "vis.file.text");
			return 1;
		}

		if (strcmp(key, "newlines") == 0) {
			switch (text_newline_type(file->text)) {
			case TEXT_NEWLINE_LF:
				lua_pushstring(L, "lf");
				break;
			case TEXT_NEWLINE_CRLF:
				lua_pushstring(L, "crlf");
				break;
			default:
				lua_pushnil(L);
				break;
			}
			return 1;
		}

		if (strcmp(key, "size") == 0) {
			lua_pushunsigned(L, text_size(file->text));
			return 1;
		}

		if (strcmp(key, "modified") == 0) {
			lua_pushboolean(L, text_modified(file->text));
			return 1;
		}

		if (strcmp(key, "marks") == 0) {
			obj_ref_new(L, file->marks, "vis.file.marks");
			return 1;
		}
	}

	return index_common(L);
}

/***
 * Insert data at position.
 * @function insert
 * @tparam int pos the 0-based file position in bytes
 * @tparam string data the data to insert
 * @treturn bool whether the file content was successfully changed
 */
static int file_insert(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	size_t pos = checkpos(L, 2);
	size_t len;
	luaL_checkstring(L, 3);
	const char *data = lua_tolstring(L, 3, &len);
	lua_pushboolean(L, text_insert(file->text, pos, data, len));
	return 1;
}

/***
 * Delete data at position.
 *
 * @function delete
 * @tparam int pos the 0-based file position in bytes
 * @tparam int len the length in bytes to delete
 * @treturn bool whether the file content was successfully changed
 */
/***
 * Delete file range.
 *
 * @function delete
 * @tparam Range range the range to delete
 * @treturn bool whether the file content was successfully changed
 */
static int file_delete(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	Filerange range = getrange(L, 2);
	lua_pushboolean(L, text_delete_range(file->text, &range));
	return 1;
}

/***
 * Create an iterator over all lines of the file.
 *
 * For large files this is probably faster than @{lines}.
 * @function lines_iterator
 * @return the new iterator
 * @see lines
 * @usage
 * for line in file:lines_iterator() do
 * 	-- do something with line
 * end
 */
static int file_lines_iterator_it(lua_State *L);
static int file_lines_iterator(lua_State *L) {
	/* need to check second parameter first, because obj_ref_check_get
	 * modifies the stack */
	size_t line = luaL_optunsigned(L, 2, 1);
	File *file = obj_ref_check_get(L, 1, "vis.file");
	size_t *pos = lua_newuserdata(L, sizeof *pos);
	*pos = text_pos_by_lineno(file->text, line);
	lua_pushcclosure(L, file_lines_iterator_it, 2);
	return 1;
}

static int file_lines_iterator_it(lua_State *L) {
	File *file = *(File**)lua_touserdata(L, lua_upvalueindex(1));
	size_t *start = lua_touserdata(L, lua_upvalueindex(2));
	if (*start == text_size(file->text))
		return 0;
	size_t end = text_line_end(file->text, *start);
	size_t len = end - *start;
	char *buf = malloc(len);
	if (!buf && len)
		return 0;
	len = text_bytes_get(file->text, *start, len, buf);
	lua_pushlstring(L, buf, len);
	free(buf);
	*start = text_line_next(file->text, end);
	return 1;
}

/***
 * Get file content of position and length.
 *
 * @function content
 * @tparam int pos the 0-based file position in bytes
 * @tparam int len the length in bytes to read
 * @treturn string the file content corresponding to the range
 * @see lines
 * @usage
 * local file = vis.win.file
 * local text = file:content(0, file.size)
 */
/***
 * Get file content of range.
 *
 * @function content
 * @tparam Range range the range to read
 * @treturn string the file content corresponding to the range
 */
static int file_content(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	Filerange range = getrange(L, 2);
	if (!text_range_valid(&range))
		goto err;
	size_t len = text_range_size(&range);
	char *data = malloc(len);
	if (!data)
		goto err;
	len = text_bytes_get(file->text, range.start, len, data);
	lua_pushlstring(L, data, len);
	free(data);
	return 1;
err:
	lua_pushnil(L);
	return 1;
}

/***
 * Set mark.
 * @function mark_set
 * @tparam int pos the position to set the mark to, must be in [0, file.size]
 * @treturn Mark mark the mark which can be looked up later
 */
static int file_mark_set(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	size_t pos = checkpos(L, 2);
	Mark mark = text_mark_set(file->text, pos);
	if (mark)
		obj_lightref_new(L, (void*)mark, "vis.file.mark");
	else
		lua_pushnil(L);
	return 1;
}

/***
 * Get position of mark.
 * @function mark_get
 * @tparam Mark mark the mark to look up
 * @treturn int pos the position of the mark, or `nil` if invalid
 */
static int file_mark_get(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	Mark mark = (Mark)obj_lightref_check(L, 2, "vis.file.mark");
	size_t pos = text_mark_get(file->text, mark);
	if (pos == EPOS)
		lua_pushnil(L);
	else
		lua_pushunsigned(L, pos);
	return 1;
}

/***
 * Word text object.
 *
 * @function text_object_word
 * @tparam int pos the position which must be part of the word
 * @treturn Range range the range
 */

static int file_text_object(lua_State *L) {
	Filerange range = text_range_empty();
	File *file = obj_ref_check(L, 1, "vis.file");
	size_t pos = checkpos(L, 2);
	size_t idx = lua_tointeger(L, lua_upvalueindex(1));
	if (idx < LENGTH(vis_textobjects)) {
		const TextObject *txtobj = &vis_textobjects[idx];
		if (txtobj->txt)
			range = txtobj->txt(file->text, pos);
	}
	pushrange(L, &range);
	return 1;
}

static const struct luaL_Reg file_funcs[] = {
	{ "__index", file_index },
	{ "__newindex", newindex_common },
	{ "insert", file_insert },
	{ "delete", file_delete },
	{ "lines_iterator", file_lines_iterator },
	{ "content", file_content },
	{ "mark_set", file_mark_set },
	{ "mark_get", file_mark_get },
	{ NULL, NULL },
};

static int file_lines_index(lua_State *L) {
	Text *txt = obj_ref_check(L, 1, "vis.file.text");
	size_t line = luaL_checkunsigned(L, 2);
	size_t start = text_pos_by_lineno(txt, line);
	size_t end = text_line_end(txt, start);
	if (start != EPOS && end != EPOS) {
		size_t size = end - start;
		char *data = malloc(size);
		if (!data && size)
			goto err;
		size = text_bytes_get(txt, start, size, data);
		lua_pushlstring(L, data, size);
		free(data);
		return 1;
	}
err:
	lua_pushnil(L);
	return 1;
}

static int file_lines_newindex(lua_State *L) {
	Text *txt = obj_ref_check(L, 1, "vis.file.text");
	size_t line = luaL_checkunsigned(L, 2);
	size_t size;
	const char *data = luaL_checklstring(L, 3, &size);
	if (line == 0) {
		text_insert(txt, 0, data, size);
		text_insert_newline(txt, size);
		return 0;
	}
	size_t start = text_pos_by_lineno(txt, line);
	size_t end = text_line_end(txt, start);
	if (start != EPOS && end != EPOS) {
		text_delete(txt, start, end - start);
		text_insert(txt, start, data, size);
		if (text_size(txt) == start + size)
			text_insert_newline(txt, text_size(txt));
	}
	return 0;
}

static int file_lines_len(lua_State *L) {
	Text *txt = obj_ref_check(L, 1, "vis.file.text");
	size_t lines = 0;
	char lastchar;
	size_t size = text_size(txt);
	if (size > 0)
		lines = text_lineno_by_pos(txt, size);
	if (lines > 1 && text_byte_get(txt, size-1, &lastchar) && lastchar == '\n')
		lines--;
	lua_pushunsigned(L, lines);
	return 1;
}

static const struct luaL_Reg file_lines_funcs[] = {
	{ "__index", file_lines_index },
	{ "__newindex", file_lines_newindex },
	{ "__len", file_lines_len },
	{ NULL, NULL },
};

static int file_marks_index(lua_State *L) {
	Vis *vis = lua_touserdata(L, lua_upvalueindex(1));
	File *file = obj_ref_check_containerof(L, 1, "vis.file.marks", offsetof(File, marks));
	if (!file)
		goto err;
	const char *symbol = luaL_checkstring(L, 2);
	if (strlen(symbol) != 1)
		goto err;
	enum VisMark mark = vis_mark_from(vis, symbol[0]);
	if (mark == VIS_MARK_INVALID)
		goto err;
	size_t pos = text_mark_get(file->text, file->marks[mark]);
	if (pos == EPOS)
		goto err;
	lua_pushunsigned(L, pos);
	return 1;
err:
	lua_pushnil(L);
	return 1;
}

static int file_marks_newindex(lua_State *L) {
	Vis *vis = lua_touserdata(L, lua_upvalueindex(1));
	File *file = obj_ref_check_containerof(L, 1, "vis.file.marks", offsetof(File, marks));
	if (!file)
		return 0;
	const char *symbol = luaL_checkstring(L, 2);
	if (strlen(symbol) != 1)
		return 0;
	enum VisMark mark = vis_mark_from(vis, symbol[0]);
	size_t pos = luaL_checkunsigned(L, 3);
	if (mark < LENGTH(file->marks))
		file->marks[mark] = text_mark_set(file->text, pos);
	return 0;
}

static int file_marks_len(lua_State *L) {
	File *file = obj_ref_check_containerof(L, 1, "vis.file.marks", offsetof(File, marks));
	lua_pushunsigned(L, file ? LENGTH(file->marks) : 0);
	return 1;
}

static const struct luaL_Reg file_marks_funcs[] = {
	{ "__index", file_marks_index },
	{ "__newindex", file_marks_newindex },
	{ "__len", file_marks_len },
	{ NULL, NULL },
};

/***
 * The user interface.
 *
 * @type Ui
 */
/***
 * Number of available colors.
 * @tfield int colors
 */

/***
 * A file range.
 *
 * For a valid range `start <= finish` holds.
 * An invalid range is represented as `nil`.
 * @type Range
 */
/***
 * The being of the range.
 * @tfield int start
 */
/***
 * The end of the range.
 * @tfield int finish
 */

/***
 * Modes.
 * @section Modes
 */

/***
 * Mode constants.
 * @table modes
 * @tfield int NORMAL
 * @tfield int OPERATOR_PENDING
 * @tfield int INSERT
 * @tfield int REPLACE
 * @tfield int VISUAL
 * @tfield int VISUAL_LINE
 * @see Vis:map
 * @see Window:map
 */

/***
 * Key Handling.
 *
 * This section describes the contract between the editor core and Lua
 * key handling functions mapped to symbolic keys using either @{Vis:map}
 * or @{Window:map}.
 *
 * @section Key_Handling
 */

/***
 * Example of a key handling function.
 *
 * The keyhandler is invoked with the pending content of the input queue
 * given as argument. This might be the empty string if no further input
 * is available.
 *
 * The function is expected to return the number of *bytes* it has
 * consumed from the passed input keys. A negative return value is
 * interpreted as an indication that not enough input was available. The
 * function will be called again once the user has provided more input. A
 * missing return value (i.e. `nil`) is interpreted as zero, meaning
 * no further input was consumed but the function completed successfully.
 *
 * @function keyhandler
 * @tparam string keys the keys following the mapping
 * @treturn int the number of *bytes* being consumed by the function (see above)
 * @see Vis:action_register
 * @see Vis:map
 * @see Window:map
 * @usage
 * vis:map(vis.modes.INSERT, "<C-k>", function(keys)
 * 	if #keys < 2 then
 * 		return -1 -- need more input
 * 	end
 * 	local digraph = keys:sub(1, 2)
 * 	if digraph == "l*" then
 * 		vis:feedkeys('λ')
 * 		return 2 -- consume 2 bytes of input
 * 	end
 * end, "Insert digraph")
 */

/***
 * Core Events.
 *
 * These events are invoked from the editor core.
 * The following functions are invoked if they are registered in the
 * `vis.events` table. Users scripts should generally use the [Events](#events)
 * mechanism instead which multiplexes these core events.
 *
 * @section Core_Events
 */

static void vis_lua_event_get(lua_State *L, const char *name) {
	lua_getglobal(L, "vis");
	lua_getfield(L, -1, "events");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, name);
	}
	lua_remove(L, -2);
}

static void vis_lua_event_call(Vis *vis, const char *name) {
	lua_State *L = vis->lua;
	vis_lua_event_get(L, name);
	if (lua_isfunction(L, -1))
		pcall(vis, L, 0, 0);
	lua_pop(L, 1);
}

static bool vis_lua_path_strip(Vis *vis) {
	lua_State *L = vis->lua;
	lua_getglobal(L, "package");

	for (const char **var = (const char*[]){ "path", "cpath", NULL }; *var; var++) {

		lua_getfield(L, -1, *var);
		const char *path = lua_tostring(L, -1);
		lua_pop(L, 1);
		if (!path)
			return false;

		char *copy = strdup(path), *stripped = calloc(1, strlen(path)+2);
		if (!copy || !stripped) {
			free(copy);
			free(stripped);
			return false;
		}

		for (char *elem = copy, *stripped_elem = stripped, *next; elem; elem = next) {
			if ((next = strstr(elem, ";")))
				*next++ = '\0';
			if (strstr(elem, "./"))
				continue; /* skip relative path entries */
			stripped_elem += sprintf(stripped_elem, "%s;", elem);
		}

		lua_pushstring(L, stripped);
		lua_setfield(L, -2, *var);

		free(copy);
		free(stripped);
	}

	lua_pop(L, 1); /* package */
	return true;
}

bool vis_lua_path_add(Vis *vis, const char *path) {
	if (!path)
		return false;
	lua_State *L = vis->lua;
	lua_getglobal(L, "package");
	lua_pushstring(L, path);
	lua_pushstring(L, "/?.lua;");
	lua_getfield(L, -3, "path");
	lua_concat(L, 3);
	lua_setfield(L, -2, "path");
	lua_pop(L, 1); /* package */
	return true;
}

bool vis_lua_paths_get(Vis *vis, char **lpath, char **cpath) {
	lua_State *L = vis->lua;
	if (!L)
		return false;
	const char *s;
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "path");
	s = lua_tostring(L, -1);
	*lpath = s ? strdup(s) : NULL;
	lua_getfield(L, -2, "cpath");
	s = lua_tostring(L, -1);
	*cpath = s ? strdup(s) : NULL;
	return true;
}

static bool package_exist(Vis *vis, lua_State *L, const char *name) {
	const char lua[] =
		"local name = ...\n"
		"for _, searcher in ipairs(package.searchers or package.loaders) do\n"
			"local loader = searcher(name)\n"
			"if type(loader) == 'function' then\n"
				"return true\n"
			"end\n"
		"end\n"
		"return false\n";
	if (luaL_loadstring(L, lua) != LUA_OK)
		return false;
	lua_pushstring(L, name);
	/* an error indicates package exists */
	bool ret = lua_pcall(L, 1, 1, 0) != LUA_OK || lua_toboolean(L, -1);
	lua_pop(L, 1);
	return ret;
}

/***
 * Editor initialization completed.
 * This event is emitted immediately after `visrc.lua` has been sourced, but
 * before any other events have occured, in particular the command line arguments
 * have not yet been processed.
 *
 * Can be used to set *global* configuration options.
 * @function init
 */
void vis_lua_init(Vis *vis) {
	lua_State *L = luaL_newstate();
	if (!L)
		return;
	vis->lua = L;
	luaL_openlibs(L);

#if CONFIG_LPEG
	extern int luaopen_lpeg(lua_State *L);
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, luaopen_lpeg);
	lua_setfield(L, -2, "lpeg");
	lua_pop(L, 2);
#endif

	/* remove any relative paths from lua's default package.path */
	vis_lua_path_strip(vis);

	/* extends lua's package.path with:
	 * - $VIS_PATH
	 * - ./lua (relative path to the binary location)
	 * - $XDG_CONFIG_HOME/vis (defaulting to $HOME/.config/vis)
	 * - /etc/vis (for system-wide configuration provided by administrator)
	 * - /usr/(local/)?share/vis (or whatever is specified during ./configure)
	 * - package.path (standard lua search path)
	 */
	char path[PATH_MAX];

	vis_lua_path_add(vis, VIS_PATH);

	/* try to get users home directory */
	const char *home = getenv("HOME");
	if (!home || !*home) {
		struct passwd *pw = getpwuid(getuid());
		if (pw)
			home = pw->pw_dir;
	}

	vis_lua_path_add(vis, "/etc/vis");

	const char *xdg_config = getenv("XDG_CONFIG_HOME");
	if (xdg_config) {
		snprintf(path, sizeof path, "%s/vis", xdg_config);
		vis_lua_path_add(vis, path);
	} else if (home && *home) {
		snprintf(path, sizeof path, "%s/.config/vis", home);
		vis_lua_path_add(vis, path);
	}

	ssize_t len = readlink("/proc/self/exe", path, sizeof(path)-1);
	if (len > 0) {
		path[len] = '\0';
		/* some idotic dirname(3) implementations return pointers to statically
		 * allocated memory, hence we use memmove to copy it back */
		char *dir = dirname(path);
		if (dir) {
			size_t len = strlen(dir)+1;
			if (len < sizeof(path) - sizeof("/lua")) {
				memmove(path, dir, len);
				strcat(path, "/lua");
				vis_lua_path_add(vis, path);
			}
		}
	}

	vis_lua_path_add(vis, getenv("VIS_PATH"));

	/* table in registry to lookup object type, stores metatable -> type mapping */
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "vis.types");
	/* table in registry to track lifetimes of C objects */
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "vis.objects");
	/* table in registry to store references to Lua functions */
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "vis.functions");
	/* metatable used to type check user data */
	obj_type_new(L, "vis.file");

	const struct {
		enum VisTextObject id;
		const char *name;
	} textobjects[] = {
		{ VIS_TEXTOBJECT_INNER_WORD, "text_object_word" },
	};

	for (size_t i = 0; i < LENGTH(textobjects); i++) {
		lua_pushunsigned(L, textobjects[i].id);
		lua_pushcclosure(L, file_text_object, 1);
		lua_setfield(L, -2, textobjects[i].name);
	}

	luaL_setfuncs(L, file_funcs, 0);

	obj_type_new(L, "vis.file.text");
	luaL_setfuncs(L, file_lines_funcs, 0);
	obj_type_new(L, "vis.window");
	luaL_setfuncs(L, window_funcs, 0);

	const struct {
		enum UiStyle id;
		const char *name;
	} styles[] = {
		{ UI_STYLE_DEFAULT,        "STYLE_DEFAULT"        },
		{ UI_STYLE_CURSOR,         "STYLE_CURSOR"         },
		{ UI_STYLE_CURSOR_PRIMARY, "STYLE_CURSOR_PRIMARY" },
		{ UI_STYLE_CURSOR_LINE,    "STYLE_CURSOR_LINE"    },
		{ UI_STYLE_SELECTION,      "STYLE_SELECTION"      },
		{ UI_STYLE_LINENUMBER,     "STYLE_LINENUMBER"     },
		{ UI_STYLE_COLOR_COLUMN,   "STYLE_COLOR_COLUMN"   },
	};

	for (size_t i = 0; i < LENGTH(styles); i++) {
		lua_pushunsigned(L, styles[i].id);
		lua_setfield(L, -2, styles[i].name);
	}

	obj_type_new(L, "vis.file.mark");
	obj_type_new(L, "vis.file.marks");
	lua_pushlightuserdata(L, vis);
	luaL_setfuncs(L, file_marks_funcs, 1);

	obj_type_new(L, "vis.window.cursor");
	luaL_setfuncs(L, window_cursor_funcs, 0);
	obj_type_new(L, "vis.window.cursors");
	luaL_setfuncs(L, window_cursors_funcs, 0);

	obj_type_new(L, "vis.ui");
	luaL_setfuncs(L, ui_funcs, 0);
	lua_pushunsigned(L, vis->ui->colors(vis->ui));
	lua_setfield(L, -2, "colors");

	obj_type_new(L, "vis.registers");
	lua_pushlightuserdata(L, vis);
	luaL_setfuncs(L, registers_funcs, 1);

	obj_type_new(L, "vis.keyaction");

	obj_type_new(L, "vis");
	luaL_setfuncs(L, vis_lua, 0);

	lua_pushstring(L, VERSION);
	lua_setfield(L, -2, "VERSION");

	lua_newtable(L);

	static const struct {
		enum VisMode id;
		const char *name;
	} modes[] = {
		{ VIS_MODE_NORMAL,           "NORMAL"           },
		{ VIS_MODE_OPERATOR_PENDING, "OPERATOR_PENDING" },
		{ VIS_MODE_VISUAL,           "VISUAL"           },
		{ VIS_MODE_VISUAL_LINE,      "VISUAL_LINE"      },
		{ VIS_MODE_INSERT,           "INSERT"           },
		{ VIS_MODE_REPLACE,          "REPLACE"          },
	};

	for (size_t i = 0; i < LENGTH(modes); i++) {
		lua_pushunsigned(L, modes[i].id);
		lua_setfield(L, -2, modes[i].name);
	}

	lua_setfield(L, -2, "modes");

	obj_ref_new(L, vis, "vis");
	lua_setglobal(L, "vis");

	if (!package_exist(vis, L, "visrc")) {
		vis_info_show(vis, "WARNING: failed to load visrc.lua");
	} else {
		lua_getglobal(L, "require");
		lua_pushstring(L, "visrc");
		pcall(vis, L, 1, 0);
		vis_lua_event_call(vis, "init");
	}
}

/***
 * Editor startup completed.
 * This event is emitted immediately before the main loop starts.
 * At this point all files are loaded and corresponding windows are created.
 * We are about to process interactive keyboard input.
 * @function start
 */
void vis_lua_start(Vis *vis) {
	vis_lua_event_call(vis, "start");
}

/**
 * Editor is about to terminate.
 * @function quit
 */
void vis_lua_quit(Vis *vis) {
	vis_lua_event_call(vis, "quit");
	lua_close(vis->lua);
	vis->lua = NULL;
}

/***
 * Input key event in either input or replace mode.
 * @function input
 * @tparam string key
 * @treturn bool whether the key was cosumed or not
 */
static bool vis_lua_input(Vis *vis, const char *key, size_t len) {
	if (vis->win->file->internal)
		return false;
	lua_State *L = vis->lua;
	bool ret = false;
	vis_lua_event_get(L, "input");
	if (lua_isfunction(L, -1)) {
		lua_pushlstring(L, key, len);
		if (pcall(vis, L, 1, 1) != 0) {
			ret = lua_isboolean(L, -1) && lua_toboolean(L, -1);
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	return ret;
}

void vis_lua_mode_insert_input(Vis *vis, const char *key, size_t len) {
	if (!vis_lua_input(vis, key, len))
		vis_insert_key(vis, key, len);
}

void vis_lua_mode_replace_input(Vis *vis, const char *key, size_t len) {
	if (!vis_lua_input(vis, key, len))
		vis_replace_key(vis, key, len);
}

/***
 * File open.
 * @function file_open
 * @tparam File file the file to be opened
 */
void vis_lua_file_open(Vis *vis, File *file) {
	debug("event: file-open: %s %p %p\n", file->name ? file->name : "unnamed", (void*)file, (void*)file->text);
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "file_open");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, file, "vis.file");
		pcall(vis, L, 1, 0);
	}
	lua_pop(L, 1);
}

/***
 * File pre save.
 * Triggered *before* the file is being written.
 * @function file_save_pre
 * @tparam File file the file being written
 * @tparam string path the absolute path to which the file will be written, `nil` if standard output
 * @treturn bool whether the write operation should be proceeded
 */
bool vis_lua_file_save_pre(Vis *vis, File *file, const char *path) {
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "file_save_pre");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, file, "vis.file");
		lua_pushstring(L, path);
		if (pcall(vis, L, 2, 1) != 0)
			return false;
		return !lua_isboolean(L, -1) || lua_toboolean(L, -1);
	}
	lua_pop(L, 1);
	return true;
}

/***
 * File post save.
 * Triggered *after* a successfull write operation.
 * @function file_save_post
 * @tparam File file the file which was written
 * @tparam string path the absolute path to which it was written, `nil` if standard output
 */
void vis_lua_file_save_post(Vis *vis, File *file, const char *path) {
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "file_save_post");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, file, "vis.file");
		lua_pushstring(L, path);
		pcall(vis, L, 2, 0);
	}
	lua_pop(L, 1);
}

/***
 * File close.
 * The last window displaying the file has been closed.
 * @function file_close
 * @tparam File file the file being closed
 */
void vis_lua_file_close(Vis *vis, File *file) {
	debug("event: file-close: %s %p %p\n", file->name ? file->name : "unnamed", (void*)file, (void*)file->text);
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "file_close");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, file, "vis.file");
		pcall(vis, L, 1, 0);
	}
	obj_ref_free(L, file->marks);
	obj_ref_free(L, file->text);
	obj_ref_free(L, file);
	lua_pop(L, 1);
}

/***
 * Window open.
 * A new window has been created.
 * @function win_open
 * @tparam Window win the window being opened
 */
void vis_lua_win_open(Vis *vis, Win *win) {
	debug("event: win-open: %s %p %p\n", win->file->name ? win->file->name : "unnamed", (void*)win, (void*)win->view);
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "win_open");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, win, "vis.window");
		pcall(vis, L, 1, 0);
	}
	lua_pop(L, 1);
}

/***
 * Window close.
 * An window is being closed.
 * @function win_close
 * @tparam Window win the window being closed
 */
void vis_lua_win_close(Vis *vis, Win *win) {
	debug("event: win-close: %s %p %p\n", win->file->name ? win->file->name : "unnamed", (void*)win, (void*)win->view);
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "win_close");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, win, "vis.window");
		pcall(vis, L, 1, 0);
	}
	obj_ref_free(L, win->view);
	obj_ref_free(L, win);
	lua_pop(L, 1);
}

/**
 * Window highlight.
 * The window has been redrawn and the syntax highlighting needs to be performed.
 * @function win_highlight
 * @tparam Window win the window being redrawn
 * @tparam int horizon the maximal number of bytes the lexer should look behind to synchronize parsing state
 * @see style
 */
void vis_lua_win_highlight(Vis *vis, Win *win, size_t horizon) {
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "win_highlight");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, win, "vis.window");
		lua_pushunsigned(L, horizon);
		pcall(vis, L, 2, 0);
	}
	lua_pop(L, 1);
}

/***
 * Window syntax/filetype change.
 * @function win_syntax
 * @tparam Window win the affected window
 * @tparam string syntax the lexer name or `nil` if syntax highlighting should be disabled for this window
 * @treturn bool whether the syntax change was successful
 */
bool vis_lua_win_syntax(Vis *vis, Win *win, const char *syntax) {
	lua_State *L = vis->lua;
	bool ret = false;
	vis_lua_event_get(L, "win_syntax");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, win, "vis.window");
		if (syntax)
			lua_pushstring(L, syntax);
		else
			lua_pushnil(L);
		pcall(vis, L, 2, 1);
		ret = lua_toboolean(L, -1);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return ret;
}

/***
 * Window status bar redraw.
 * @function win_status
 * @tparam Window win the affected window
 * @see status
 */
void vis_lua_win_status(Vis *vis, Win *win) {
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "win_status");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, win, "vis.window");
		pcall(vis, L, 1, 0);
	} else {
		window_status_update(vis, win);
	}
	lua_pop(L, 1);
}

/***
 * Theme change.
 * @function theme_change
 * @tparam string theme the name of the new theme to load
 */
bool vis_theme_load(Vis *vis, const char *name) {
	lua_State *L = vis->lua;
	vis_lua_event_get(L, "theme_change");
	if (lua_isfunction(L, -1)) {
		lua_pushstring(L, name);
		pcall(vis, L, 1, 0);
	}
	lua_pop(L, 1);
	/* package.loaded['themes/'..name] = nil
	 * require 'themes/'..name */
	return true;
}

#endif
