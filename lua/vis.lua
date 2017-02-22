---
-- Vis Lua plugin API standard library
-- @module vis

---
-- @type Vis

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
vis.module_exist = function(vis, name)
	for _, searcher in ipairs(package.searchers or package.loaders) do
		local loader = searcher(name)
		if type(loader) == 'function' then
			return true
		end
	end
	return false
end

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
	THEME_CHANGE = "Event::THEME_CHANGE", -- see @{theme_change}
	WIN_CLOSE = "Event::WIN_CLOSE", -- see @{win_close}
	WIN_HIGHLIGHT = "Event::WIN_HIGHLIGHT", -- see @{win_highlight}
	WIN_OPEN = "Event::WIN_OPEN", -- see @{win_open}
	WIN_STATUS = "Event::WIN_STATUS", -- see @{win_status}
	WIN_SYNTAX = "Event::WIN_SYNTAX", -- see @{win_syntax}
}

events.file_close = function(...) events.emit(events.FILE_CLOSE, ...) end
events.file_open = function(...) events.emit(events.FILE_OPEN, ...) end
events.file_save_post = function(...) events.emit(events.FILE_SAVE_POST, ...) end
events.file_save_pre = function(...) return events.emit(events.FILE_SAVE_PRE, ...) end
events.init = function(...) events.emit(events.INIT, ...) end
events.input = function(...) return events.emit(events.INPUT, ...) end
events.quit = function(...) events.emit(events.QUIT, ...) end
events.start = function(...) events.emit(events.START, ...) end
events.theme_change = function(...) events.emit(events.THEME_CHANGE, ...) end
events.win_close = function(...) events.emit(events.WIN_CLOSE, ...) end
events.win_highlight = function(...) events.emit(events.WIN_HIGHLIGHT, ...) end
events.win_open = function(...) events.emit(events.WIN_OPEN, ...) end
events.win_status = function(...) events.emit(events.WIN_STATUS, ...) end
events.win_syntax = function(...) return events.emit(events.WIN_SYNTAX, ...) end

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

require('vis-std')
