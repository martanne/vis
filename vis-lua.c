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

#if !CONFIG_LUA

void vis_lua_start(Vis *vis) { }
void vis_lua_quit(Vis *vis) { }
void vis_lua_file_open(Vis *vis, File *file) { }
void vis_lua_file_save(Vis *vis, File *file) { }
void vis_lua_file_close(Vis *vis, File *file) { }
void vis_lua_win_open(Vis *vis, Win *win) { }
void vis_lua_win_close(Vis *vis, Win *win) { }
bool vis_theme_load(Vis *vis, const char *name) { return true; }

#else

#if 0
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
	size_t len;
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	msg = lua_tolstring(L, 1, &len);
	vis_message_show(vis, msg);
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

/* expects a lua function at the top of the stack and stores a
 * reference to it in the registry. The return value can be used
 * to look it up.
 *
 *   registry["vis.functions"][(void*)(function)] = function
 */
static const void *func_ref_new(lua_State *L) {
	const void *addr = lua_topointer(L, -1);
	if (!lua_isfunction(L, -1) || !addr)
		return NULL;
	lua_getfield(L, LUA_REGISTRYINDEX, "vis.functions");
	lua_pushlightuserdata(L, (void*)addr);
	lua_pushvalue(L, -3);
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
		lua_pop(L, 1);
		return NULL;
	}
	return luaL_checkudata(L, -1, type);
}

/* expects a userdatum at the top of the stack and sets
 *
 *   registry["vis.objects"][addr] = userdata
 */
static void obj_ref_set(lua_State *L, void *addr) {
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
	lua_pushnil(L);
	obj_ref_set(L, addr);
}

/* creates a new object reference of given type if it does not
 * already exist in the registry */
static void *obj_ref_new(lua_State *L, void *addr, const char *type) {
	if (!addr)
		return NULL;
	void **handle = (void**)obj_ref_get(L, addr, type);
	if (!handle) {
		handle = obj_new(L, sizeof(addr), type);
		obj_ref_set(L, addr);
		*handle = addr;
	}
	return *handle;
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
	return obj;
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

static const char *keymapping(Vis *vis, const char *keys, const Arg *arg) {
	lua_State *L = vis->lua;
	if (!func_ref_get(L, arg->v))
		return keys;
	pcall(vis, L, 0, 0);
	return keys;
}

static int windows_iter(lua_State *L);

static int windows(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis) {
		lua_pushnil(L);
		return 1;
	}
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
	if (!win)
		return 0;
	*handle = win->next;
	return 1;
}

static int files_iter(lua_State *L);

static int files(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis) {
		lua_pushnil(L);
		return 1;
	}
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
	if (!file)
		return 0;
	*handle = file->next;
	return 1;
}

static int command(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis) {
		lua_pushnil(L);
		return 1;
	}
	const char *cmd = luaL_checkstring(L, 2);
	bool ret = vis_cmd(vis, cmd);
	lua_pushboolean(L, ret);
	return 1;
}

static int info(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis) {
		lua_pushnil(L);
		return 1;
	}
	const char *msg = luaL_checkstring(L, 2);
	vis_info_show(vis, "%s", msg);
	return 0;
}

static int open(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis) {
		lua_pushnil(L);
		return 1;
	}
	const char *name = luaL_checkstring(L, 2);
	File *file = NULL;
	if (vis_window_new(vis, name))
		file = obj_ref_new(L, vis->win->file, "vis.file");
	if (!file)
		lua_pushnil(L);
	return 1;
}

static int map(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis) {
		lua_pushnil(L);
		return 1;
	}
	KeyBinding *binding = NULL;
	KeyAction *action = NULL;

	int mode = luaL_checkint(L, 2);
	const char *key = luaL_checkstring(L, 3);

	if (!key || !lua_isfunction(L, 4))
		goto err;
	if (!(binding = calloc(1, sizeof *binding)) || !(action = calloc(1, sizeof *action)))
		goto err;

	/* store reference to function in the registry */
	lua_pushvalue(L, 4);
	const void *func = func_ref_new(L);
	if (!func)
		goto err;

	*action = (KeyAction){
		.name = NULL,
		.help = NULL,
		.func = keymapping,
		.arg = (const Arg){
			.v = func,
		},
	};

	binding->action = action;

	if (!vis_mode_map(vis, mode, key, binding))
		goto err;

	lua_pushboolean(L, true);
	return 1;
err:
	free(binding);
	free(action);
	lua_pushboolean(L, false);
	return 1;
}

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
	return luaL_checkunsigned(L, -1);
}

static int motion_register(lua_State *L) {
	int id = -1;
	const void *func;
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis || !lua_isfunction(L, 2) || !(func = func_ref_new(L)))
		goto err;
	id = vis_motion_register(vis, 0, (void*)func, motion_lua);
err:
	lua_pushinteger(L, id);
	return 1;
}

static int textobject(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	enum VisTextObject id = luaL_checkunsigned(L, 2);
	lua_pushboolean(L, vis && vis_textobject(vis, id));
	return 1;
}


static Filerange textobject_lua(Vis *vis, Win *win, void *data, size_t pos) {
	lua_State *L = vis->lua;
	if (!func_ref_get(L, data) || !obj_ref_new(L, win, "vis.window"))
		return text_range_empty();
	lua_pushunsigned(L, pos);
	if (pcall(vis, L, 2, 2) != 0)
		return text_range_empty();
	return text_range_new(luaL_checkunsigned(L, -2), luaL_checkunsigned(L, -1));
}

static int textobject_register(lua_State *L) {
	int id = -1;
	const void *func;
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis || !lua_isfunction(L, 2) || !(func = func_ref_new(L)))
		goto err;
	id = vis_textobject_register(vis, 0, (void*)func, textobject_lua);
err:
	lua_pushinteger(L, id);
	return 1;
}

static int vis_index(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis) {
		lua_pushnil(L);
		return 1;
	}

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "win") == 0) {
			obj_ref_new(L, vis->windows, "vis.window");
			return 1;
		}

		if (strcmp(key, "MODE_NORMAL") == 0) {
			lua_pushunsigned(L, VIS_MODE_NORMAL);
			return 1;
		}

		if (strcmp(key, "MODE_OPERATOR_PENDING") == 0) {
			lua_pushunsigned(L, VIS_MODE_OPERATOR_PENDING);
			return 1;
		}

		if (strcmp(key, "MODE_VISUAL") == 0) {
			lua_pushunsigned(L, VIS_MODE_VISUAL);
			return 1;
		}

		if (strcmp(key, "MODE_VISUAL_LINE") == 0) {
			lua_pushunsigned(L, VIS_MODE_VISUAL_LINE);
			return 1;
		}

		if (strcmp(key, "MODE_INSERT") == 0) {
			lua_pushunsigned(L, VIS_MODE_INSERT);
			return 1;
		}

		if (strcmp(key, "MODE_REPLACE") == 0) {
			lua_pushunsigned(L, VIS_MODE_REPLACE);
			return 1;
		}
	}

	return index_common(L);
}

static int vis_newindex(lua_State *L) {
	Vis *vis = obj_ref_check(L, 1, "vis");
	if (!vis)
		return 0;
	return newindex_common(L);
}

static const struct luaL_Reg vis_lua[] = {
	{ "files", files },
	{ "windows", windows },
	{ "command", command },
	{ "info", info },
	{ "open", open },
	{ "map", map },
	{ "motion", motion },
	{ "motion_register", motion_register },
	{ "textobject", textobject },
	{ "textobject_register", textobject_register },
	{ "__index", vis_index },
	{ "__newindex", vis_newindex },
	{ NULL, NULL },
};

static int window_index(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	if (!win) {
		lua_pushnil(L);
		return 1;
	}

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "file") == 0) {
			obj_ref_new(L, win->file, "vis.file");
			return 1;
		}

		if (strcmp(key, "cursor") == 0) {
			obj_ref_new(L, win->view, "vis.window.cursor");
			return 1;
		}

		if (strcmp(key, "syntax") == 0) {
			const char *syntax = view_syntax_get(win->view);
			if (syntax)
				lua_pushstring(L, syntax);
			else
				lua_pushnil(L);
			return 1;
		}
	}

	return index_common(L);
}

static int window_newindex(lua_State *L) {
	Win *win = obj_ref_check(L, 1, "vis.window");
	if (!win)
		return 0;
	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "syntax") == 0) {
			const char *syntax = NULL;
			if (!lua_isnil(L, 3))
				syntax = luaL_checkstring(L, 3);
			view_syntax_set(win->view, syntax);
			return 0;
		}
	}
	return newindex_common(L);
}

static const struct luaL_Reg window_funcs[] = {
	{ "__index", window_index },
	{ "__newindex", window_newindex },
	{ NULL, NULL },
};

static int window_cursor_index(lua_State *L) {
	View *view = obj_ref_check(L, 1, "vis.window.cursor");
	if (!view) {
		lua_pushnil(L);
		return 1;
	}

	if (lua_isstring(L, 2)) {
		Text *txt = view_text(view);
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "pos") == 0) {
			lua_pushunsigned(L, view_cursor_get(view));
			return 1;
		}

		if (strcmp(key, "line") == 0) {
			size_t pos = view_cursor_get(view);
			size_t line = text_lineno_by_pos(txt, pos);
			lua_pushunsigned(L, line);
			return 1;
		}

		if (strcmp(key, "col") == 0) {
			size_t pos = view_cursor_get(view);
			int col = text_line_char_get(txt, pos);
			lua_pushunsigned(L, col);
			return 1;
		}
	}

	return index_common(L);
}

static int window_cursor_newindex(lua_State *L) {
	View *view = obj_ref_check(L, 1, "vis.window.cursor");
	if (!view)
		return 0;
	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "pos") == 0) {
			size_t pos = luaL_checkunsigned(L, 3);
			view_cursor_to(view, pos);
			return 0;
		}
	}
	return newindex_common(L);
}

static int window_cursor_to(lua_State *L) {
	View *view = obj_ref_check(L, 1, "vis.window.cursor");
	if (view) {
		Text *txt = view_text(view);
		size_t line = luaL_checkunsigned(L, 2);
		size_t col = luaL_checkunsigned(L, 3);
		size_t pos = text_pos_by_lineno(txt, line);
		pos = text_line_char_set(txt, pos, col);
		view_cursor_to(view, pos);
	}
	return 0;
}

static const struct luaL_Reg window_cursor_funcs[] = {
	{ "__index", window_cursor_index },
	{ "__newindex", window_cursor_newindex },
	{ "to", window_cursor_to },
	{ NULL, NULL },
};

static int file_index(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	if (!file) {
		lua_pushnil(L);
		return 1;
	}

	if (lua_isstring(L, 2)) {
		const char *key = lua_tostring(L, 2);
		if (strcmp(key, "name") == 0) {
			lua_pushstring(L, file->name);
			return 1;
		}
		if (strcmp(key, "lines") == 0) {
			obj_ref_new(L, file->text, "vis.file.text");
			return 1;
		}
	}

	return index_common(L);
}

static int file_newindex(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	if (!file)
		return 0;
	return newindex_common(L);
}

static int file_insert(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	if (file) {
		size_t pos = luaL_checkunsigned(L, 2);
		size_t len;
		luaL_checkstring(L, 3);
		const char *data = lua_tolstring(L, 3, &len);
 		bool ret = text_insert(file->text, pos, data, len);
		lua_pushboolean(L, ret);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

static int file_delete(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	if (file) {
		size_t pos = luaL_checkunsigned(L, 2);
		size_t len = luaL_checkunsigned(L, 3);
		bool ret = text_delete(file->text, pos, len);
		lua_pushboolean(L, ret);
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}

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

static int file_content(lua_State *L) {
	File *file = obj_ref_check(L, 1, "vis.file");
	if (!file)
		goto err;
	size_t pos = luaL_checkunsigned(L, 2);
	size_t len = luaL_checkunsigned(L, 3);
	char *data = malloc(len);
	if (!data)
		goto err;
	len = text_bytes_get(file->text, pos, len, data);
	lua_pushlstring(L, data, len);
	free(data);
	return 1;
err:
	lua_pushnil(L);
	return 1;
}

static const struct luaL_Reg file_funcs[] = {
	{ "__index", file_index },
	{ "__newindex", file_newindex },
	{ "insert", file_insert },
	{ "delete", file_delete },
	{ "lines_iterator", file_lines_iterator },
	{ "content", file_content },
	{ NULL, NULL },
};

static int file_lines_index(lua_State *L) {
	Text *txt = obj_ref_check(L, 1, "vis.file.text");
	if (!txt)
		goto err;
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
	if (!txt)
		return 0;
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
	if (txt) {
		char lastchar;
		size_t size = text_size(txt);
		if (size > 0)
			lines = text_lineno_by_pos(txt, size);
		if (lines > 1 && text_byte_get(txt, size-1, &lastchar) && lastchar == '\n')
			lines--;
	}
	lua_pushunsigned(L, lines);
	return 1;
}

static const struct luaL_Reg file_lines_funcs[] = {
	{ "__index", file_lines_index },
	{ "__newindex", file_lines_newindex },
	{ "__len", file_lines_len },
	{ NULL, NULL },
};

static void vis_lua_event(Vis *vis, const char *name) {
	lua_State *L = vis->lua;
	lua_getglobal(L, "vis");
	lua_getfield(L, -1, "events");
	if (lua_istable(L, -1)) {
		lua_getfield(L, -1, name);
	}
	lua_remove(L, -2);
}

void vis_lua_start(Vis *vis) {
	lua_State *L = luaL_newstate();
	if (!L)
		return;
	vis->lua = L;
	luaL_openlibs(L);


	/* extends lua's package.path with:
	 * - $VIS_PATH/{,lexers}
	 * - {,lexers} relative to the binary location
	 * - $XDG_CONFIG_HOME/vis/{,lexers} (defaulting to $HOME/.config/vis/{,lexers})
	 * - /usr/local/share/vis/{,lexers}
	 * - /usr/share/vis/{,lexers}
	 * - package.path (standard lua search path)
	 */
	int paths = 3;
	lua_getglobal(L, "package");

	const char *vis_path = getenv("VIS_PATH");
	if (vis_path) {
		lua_pushstring(L, vis_path);
		lua_pushstring(L, "/?.lua;");
		lua_pushstring(L, vis_path);
		lua_pushstring(L, "/lexers/?.lua;");
		lua_concat(L, 4);
		paths++;
	}

	char exe[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe)-1);
	if (len > 0) {
		exe[len] = '\0';
		char *exe_path = dirname(exe);
		lua_pushstring(L, exe_path);
		lua_pushstring(L, "/?.lua;");
		lua_pushstring(L, exe_path);
		lua_pushstring(L, "/lexers/?.lua;");
		lua_concat(L, 4);
		paths++;
	}

	/* try to get users home directory */
	const char *home = getenv("HOME");
	if (!home || !*home) {
		struct passwd *pw = getpwuid(getuid());
		if (pw)
			home = pw->pw_dir;
	}

	const char *xdg_config = getenv("XDG_CONFIG_HOME");
	if (xdg_config) {
		lua_pushstring(L, xdg_config);
		lua_pushstring(L, "/vis/?.lua;");
		lua_pushstring(L, xdg_config);
		lua_pushstring(L, "/vis/lexers/?.lua;");
		lua_concat(L, 4);
		paths++;
	} else if (home && *home) {
		lua_pushstring(L, home);
		lua_pushstring(L, "/.config/vis/?.lua;");
		lua_pushstring(L, home);
		lua_pushstring(L, "/.config/vis/lexers/?.lua;");
		lua_concat(L, 4);
		paths++;
	}

	lua_pushstring(L, "/usr/local/share/vis/?.lua;/usr/local/share/vis/lexers/?.lua;");
	lua_pushstring(L, "/usr/share/vis/?.lua;/usr/share/vis/lexers/?.lua;");
	lua_getfield(L, -paths, "path");
	lua_concat(L, paths);
	lua_setfield(L, -2, "path");
	lua_pop(L, 1); /* package */

	/* table in registry to track lifetimes of C objects */
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "vis.objects");
	/* table in registry to store references to Lua functions */
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "vis.functions");
	/* metatable used to type check user data */
	luaL_newmetatable(L, "vis.file");
	luaL_setfuncs(L, file_funcs, 0);
	luaL_newmetatable(L, "vis.file.text");
	luaL_setfuncs(L, file_lines_funcs, 0);
	luaL_newmetatable(L, "vis.window");
	luaL_setfuncs(L, window_funcs, 0);
	luaL_newmetatable(L, "vis.window.cursor");
	luaL_setfuncs(L, window_cursor_funcs, 0);
	/* vis module table with up value as the C pointer */
	luaL_newmetatable(L, "vis");
	luaL_setfuncs(L, vis_lua, 0);
	obj_ref_new(L, vis, "vis");
	lua_setglobal(L, "vis");

	lua_getglobal(L, "require");
	lua_pushstring(L, "visrc");
	pcall(vis, L, 1, 0);
	vis_lua_event(vis, "start");
	if (lua_isfunction(L, -1))
		pcall(vis, L, 0, 0);
	lua_pop(L, 1);
}

void vis_lua_quit(Vis *vis) {
	lua_State *L = vis->lua;
	if (!L)
		return;
	vis_lua_event(vis, "quit");
	if (lua_isfunction(L, -1))
		pcall(vis, L, 0, 0);
	lua_pop(L, 1);
	lua_close(L);
}

void vis_lua_file_open(Vis *vis, File *file) {

}

void vis_lua_file_save(Vis *vis, File *file) {

}

void vis_lua_file_close(Vis *vis, File *file) {
	lua_State *L = vis->lua;
	vis_lua_event(vis, "file_close");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, file, "vis.file");
		pcall(vis, L, 1, 0);
	}
	obj_ref_free(L, file->text);
	obj_ref_free(L, file);
	lua_pop(L, 1);
}

void vis_lua_win_open(Vis *vis, Win *win) {
	lua_State *L = vis->lua;
	vis_lua_event(vis, "win_open");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, win, "vis.window");
		pcall(vis, L, 1, 0);
	}
	lua_pop(L, 1);
}

void vis_lua_win_close(Vis *vis, Win *win) {
	lua_State *L = vis->lua;
	vis_lua_event(vis, "win_close");
	if (lua_isfunction(L, -1)) {
		obj_ref_new(L, win, "vis.window");
		pcall(vis, L, 1, 0);
	}
	obj_ref_free(L, win->view);
	obj_ref_free(L, win);
	lua_pop(L, 1);
}

bool vis_theme_load(Vis *vis, const char *name) {
	lua_State *L = vis->lua;
	if (!L)
		return false;
	/* package.loaded['themes/'..name] = nil
	 * require 'themes/'..name */
	lua_pushstring(L, "themes/");
	lua_pushstring(L, name);
	lua_concat(L, 2);
	lua_getglobal(L, "package");
	lua_getfield(L, -1, "loaded");
	lua_pushvalue(L, -3);
	lua_pushnil(L);
	lua_settable(L, -3);
	lua_pop(L, 2);
	lua_getglobal(L, "require");
	lua_pushvalue(L, -2);
	if (pcall(vis, L, 1, 0))
		return false;
	for (Win *win = vis->windows; win; win = win->next)
		view_syntax_set(win->view, view_syntax_get(win->view));
	return true;
}

#endif
