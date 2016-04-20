require("utils")

vis.events = {}
vis.events.win_open = function(win)
	in_file = win.file.name
	lua_file = string.gsub(in_file, '%.in$', '')
	require(lua_file)
-- 	These both seem to cause crashes at the moment.
-- 	vis:command('q!')
--
-- 	vis:map(vis.MODE_NORMAL, "Q", function()
-- 		vis:command('q!')
-- 	end)
end
