---
-- Vis Lua plugin API standard library
-- @module vis

---
-- @type Vis

local ok, msg = pcall(function()
	vis.lexers = {}
	vis.lexers = require('lexer')
end)

if not ok then
	vis:info('WARNING: could not load lexer module, is lpeg installed?')
end

vis.events = {}

--- Map a new motion.
--
-- Sets up a mapping in normal, visual and operator pending mode.
-- The motion function will receive the @{Window} and an initial position
-- (in bytes from the start of the file) as argument and is expected to
-- return the resulting position.
-- @tparam string key the key to associate with the new mption
-- @tparam function motion the motion logic implemented as Lua function
-- @treturn bool whether the new motion could be installed
-- @usage
-- vis:motion_new("<C-l>", function(win, pos)
-- 	return pos+1
-- end)
--
vis.motion_new = function(vis, key, motion)
	local id = vis:motion_register(motion)
	if id < 0 then
		return false
	end
	local binding = function()
		vis:motion(id)
	end
	vis:map(vis.MODE_NORMAL, key, binding)
	vis:map(vis.MODE_VISUAL, key, binding)
	vis:map(vis.MODE_OPERATOR_PENDING, key, binding)
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
-- @treturn bool whether the new text object could be installed
-- @usage
-- vis:textobject_new("<C-l>", function(win, pos)
-- 	return pos, pos+1
-- end)
--
vis.textobject_new = function(vis, key, textobject)
	local id = vis:textobject_register(textobject)
	if id < 0 then
		return false
	end
	local binding = function()
		vis:textobject(id)
	end
	vis:map(vis.MODE_VISUAL, key, binding)
	vis:map(vis.MODE_OPERATOR_PENDING, key, binding)
	return true
end

require('vis-std')
