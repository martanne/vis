---
-- Vis Lua plugin API standard library
-- @module vis

---
-- @type Vis

--- Map a new operator.
--
-- Sets up a mapping in normal, visual and operator pending mode.
-- The operator function will receive the @{File}, @{Range} and position
-- to operate on and is expected to return the new cursor position.
--
-- @tparam string key the key to associate with the new operator
-- @tparam function operator the operator logic implemented as Lua function
-- @tparam[opt] string help the single line help text as displayed in `:help`
-- @treturn bool whether the new operator could be installed
-- @usage
-- vis:operator_new("gq", function(file, range, pos)
-- 	local status, out, err = vis:pipe(file, range, "fmt")
-- 	if not status then
-- 		vis:info(err)
-- 	else
-- 		file:delete(range)
-- 		file:insert(range.start, out)
-- 	end
-- 	return range.start -- new cursor location
-- end, "Formating operator, filter range through fmt(1)")
--
vis.operator_new = function(vis, key, operator, help)
	local id = vis:operator_register(operator)
	if id < 0 then
		return false
	end
	local binding = function()
		vis:operator(id)
	end
	vis:map(vis.modes.NORMAL, key, binding, help)
	vis:map(vis.modes.VISUAL, key, binding, help)
	vis:map(vis.modes.OPERATOR_PENDING, key, binding, help)
	return true
end

--- Map a new motion.
--
-- Sets up a mapping in normal, visual and operator pending mode.
-- The motion function will receive the @{Window} and an initial position
-- (in bytes from the start of the file) as argument and is expected to
-- return the resulting position.
-- @tparam string key the key to associate with the new mption
-- @tparam function motion the motion logic implemented as Lua function
-- @tparam[opt] string help the single line help text as displayed in `:help`
-- @treturn bool whether the new motion could be installed
-- @usage
-- vis:motion_new("<C-l>", function(win, pos)
-- 	return pos+1
-- end, "Advance to next byte")
--
vis.motion_new = function(vis, key, motion, help)
	local id = vis:motion_register(motion)
	if id < 0 then
		return false
	end
	local binding = function()
		vis:motion(id)
	end
	vis:map(vis.modes.NORMAL, key, binding, help)
	vis:map(vis.modes.VISUAL, key, binding, help)
	vis:map(vis.modes.OPERATOR_PENDING, key, binding, help)
	return true
end

--- Map a new text object.
--
-- Sets up a mapping in visual and operator pending mode.
-- The text object function will receive the @{Window} and an initial
-- position(in bytes from the start of the file) as argument and is
-- expected to return the resulting range or `nil`.
-- @tparam string key the key associated with the new text object
-- @tparam function textobject the text object logic implemented as Lua function
-- @tparam[opt] string help the single line help text as displayed in `:help`
-- @treturn bool whether the new text object could be installed
-- @usage
-- vis:textobject_new("<C-l>", function(win, pos)
-- 	return pos, pos+1
-- end, "Single byte text object")
--
vis.textobject_new = function(vis, key, textobject, help)
	local id = vis:textobject_register(textobject)
	if id < 0 then
		return false
	end
	local binding = function()
		vis:textobject(id)
	end
	vis:map(vis.modes.VISUAL, key, binding, help)
	vis:map(vis.modes.OPERATOR_PENDING, key, binding, help)
	return true
end

--- Check whether a Lua module exists
--
-- Checks whether a subsequent @{require} call will succeed.
-- @tparam string name the module name to check
-- @treturn bool whether the module was found
vis.module_exist = function(_, name)
	for _, searcher in ipairs(package.searchers or package.loaders) do
		local loader = searcher(name)
		if type(loader) == 'function' then
			return true
		end
	end
	return false
end

vis.lexers = {}

if not vis:module_exist('lpeg') then
	vis:info('WARNING: could not find lpeg module')
elseif not vis:module_exist('lexer') then
	vis:info('WARNING: could not find lexer module')
else
	vis.lexers = require('lexer')
	vis.lpeg = require('lpeg')
end

--- Events.
--
-- User scripts can subscribe Lua functions to certain events. Multiple functions
-- can be associated with the same event. They will be called in the order they were
-- registered. The first function which returns a non `nil` value terminates event
-- propagation. The remaining event handler will not be called.
--
-- Keep in mind that the editor is blocked while the event handlers
-- are being executed, avoid long running tasks.
--
-- @section Events

--- Event names.
--- @table events
local events = {
	FILE_CLOSE = "Event::FILE_CLOSE", -- see @{file_close}
	FILE_OPEN = "Event::FILE_OPEN", -- see @{file_open}
	FILE_SAVE_POST = "Event::FILE_SAVE_POST", -- see @{file_save_post}
	FILE_SAVE_PRE = "Event::FILE_SAVE_PRE", -- see @{file_save_pre}
	INIT = "Event::INIT", -- see @{init}
	INPUT = "Event::INPUT", -- see @{input}
	QUIT = "Event::QUIT", -- see @{quit}
	START = "Event::START", -- see @{start}
	WIN_CLOSE = "Event::WIN_CLOSE", -- see @{win_close}
	WIN_HIGHLIGHT = "Event::WIN_HIGHLIGHT", -- see @{win_highlight}
	WIN_OPEN = "Event::WIN_OPEN", -- see @{win_open}
	WIN_STATUS = "Event::WIN_STATUS", -- see @{win_status}
	TERM_CSI = "Event::TERM_CSI", -- see @{term_csi}
}

events.file_close = function(...) events.emit(events.FILE_CLOSE, ...) end
events.file_open = function(...) events.emit(events.FILE_OPEN, ...) end
events.file_save_post = function(...) events.emit(events.FILE_SAVE_POST, ...) end
events.file_save_pre = function(...) return events.emit(events.FILE_SAVE_PRE, ...) end
events.init = function(...) events.emit(events.INIT, ...) end
events.input = function(...) return events.emit(events.INPUT, ...) end
events.quit = function(...) events.emit(events.QUIT, ...) end
events.start = function(...) events.emit(events.START, ...) end
events.win_close = function(...) events.emit(events.WIN_CLOSE, ...) end
events.win_highlight = function(...) events.emit(events.WIN_HIGHLIGHT, ...) end
events.win_open = function(...) events.emit(events.WIN_OPEN, ...) end
events.win_status = function(...) events.emit(events.WIN_STATUS, ...) end
events.term_csi = function(...) events.emit(events.TERM_CSI, ...) end

local handlers = {}

--- Subscribe to an event.
--
-- Register an event handler.
-- @tparam string event the event name
-- @tparam function handler the event handler
-- @tparam[opt] int index the index at which to insert the handler (1 is the highest priority)
-- @usage
-- vis.events.subscribe(vis.events.FILE_SAVE_PRE, function(file, path)
-- 	-- do something useful
-- 	return true
-- end)
events.subscribe = function(event, handler, index)
	if not event then error("Invalid event name") end
	if type(handler) ~= 'function' then error("Invalid event handler") end
	if not handlers[event] then handlers[event] = {} end
	events.unsubscribe(event, handler)
	table.insert(handlers[event], index or #handlers[event]+1, handler)
end

--- Unsubscribe from an event.
--
-- Remove a registered event handler.
-- @tparam string event the event name
-- @tparam function handler the event handler to unsubscribe
-- @treturn bool whether the handler was successfully removed
events.unsubscribe = function(event, handler)
	local h = handlers[event]
	if not h then return end
	for i = 1, #h do
		if h[i] == handler then
			table.remove(h, i)
			return true
		end
	end
	return false
end

--- Generate event.
--
-- Invokes all event handlers in the order they were registered.
-- Passes all arguments to the handler. The first handler which returns a non `nil`
-- value terminates the event propagation. The other handlers will not be called.
--
-- @tparam string event the event name
-- @tparam ... ... the remaining paramters are passed on to the handler
events.emit = function(event, ...)
	local h = handlers[event]
	if not h then return end
	for i = 1, #h do
		local ret = h[i](...)
		if type(ret) ~= 'nil' then return ret end
	end
end

vis.events = events

---
-- @type Window

--- The file type associated with this window.
-- @tfield string syntax the syntax lexer name or `nil` if unset

--- Change syntax lexer to use for this window
-- @function set_syntax
-- @tparam string syntax the syntax lexer name or `nil` to disable syntax highlighting
-- @treturn bool whether the lexer could be changed
vis.types.window.set_syntax = function(win, syntax)

	local lexers = vis.lexers

	win:style_define(win.STYLE_DEFAULT, lexers.STYLE_DEFAULT or '')
	win:style_define(win.STYLE_CURSOR, lexers.STYLE_CURSOR or '')
	win:style_define(win.STYLE_CURSOR_PRIMARY, lexers.STYLE_CURSOR_PRIMARY or '')
	win:style_define(win.STYLE_CURSOR_LINE, lexers.STYLE_CURSOR_LINE or '')
	win:style_define(win.STYLE_SELECTION, lexers.STYLE_SELECTION or '')
	win:style_define(win.STYLE_LINENUMBER, lexers.STYLE_LINENUMBER or '')
	win:style_define(win.STYLE_LINENUMBER_CURSOR, lexers.STYLE_LINENUMBER_CURSOR or '')
	win:style_define(win.STYLE_COLOR_COLUMN, lexers.STYLE_COLOR_COLUMN or '')
	win:style_define(win.STYLE_STATUS, lexers.STYLE_STATUS or '')
	win:style_define(win.STYLE_STATUS_FOCUSED, lexers.STYLE_STATUS_FOCUSED or '')
	win:style_define(win.STYLE_SEPARATOR, lexers.STYLE_SEPARATOR or '')
	win:style_define(win.STYLE_INFO, lexers.STYLE_INFO or '')
	win:style_define(win.STYLE_EOF, lexers.STYLE_EOF or '')

	if syntax == nil or syntax == 'off' then
		win.syntax = nil
		return true
	end

	if not lexers.load then return false end
	local lexer = lexers.load(syntax)
	if not lexer then return false end

	for token_name, id in pairs(lexer._TOKENSTYLES) do
		local style = lexers['STYLE_'..string.upper(token_name)] or lexer._EXTRASTYLES[token_name]
		win:style_define(id, style)
	end

	win.syntax = syntax
	return true
end

---
-- @type File

--- Check whether LPeg pattern matches at a given file position.
-- @function match_at
-- @param pattern the LPeg pattern
-- @tparam int pos the absolute file position which should be tested for a match
-- @tparam[opt] int horizon the number of bytes around `pos` to consider (defaults to 1K)
-- @treturn int start,end the range of the matched region or `nil`
vis.types.file.match_at = function(file, pattern, pos, horizon)
	horizon = horizon or 1024
	local lpeg = vis.lpeg
	if not lpeg then return nil end
	local before, after = pos - horizon, pos + horizon
	if before < 0 then before = 0 end
	local data = file:content(before, after - before)
	local string_pos = pos - before + 1

	local I = lpeg.Cp()
	local p = lpeg.P{ I * pattern * I + 1 * lpeg.V(1) }
	local s, e = 1
	while true do
		s, e = p:match(data, s)
		if not s then return nil end
		if s <= string_pos and string_pos < e then
			return before + s - 1, before + e - 1
		end
		s = e
	end
end

require('vis-std')
