dofile("utils.lua")

vis.events = {}
vis.events.win_open = function(win)
	-- test.in file passed to vis
	local in_file = win.file.name
	if in_file then
		-- use the corresponding test.lua file
		lua_file = string.gsub(in_file, '%.in$', '.lua')
		dofile(lua_file)
	end
	vis:command('q!')
end
