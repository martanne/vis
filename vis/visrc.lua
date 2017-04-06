package.path = '../../lua/?.lua;'..package.path
dofile("../../lua/vis.lua")

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	-- test.in file passed to vis
	local name = win.file.name
	if name then
		-- use the corresponding test.lua file
		name = string.gsub(name, '%.in$', '')
		local file = io.open(string.format("%s.keys", name))
		local keys = file:read('*all')
		keys = string.gsub(keys, '%s*\n', '')
		keys = string.gsub(keys, '<Space>', ' ')
		file:close()
		vis:feedkeys(keys..'<Escape>')
		vis:command(string.format("w! '%s.out'", name))
	end
end)
