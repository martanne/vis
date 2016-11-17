vis.events = {}
vis.events.win_open = function(win)
	-- test.in file passed to vis
	local name = win.file.name
	if name then
		-- use the corresponding test.lua file
		name = string.gsub(name, '%.in$', '')
		local file = assert(io.popen(string.format("cpp -P '%s.keys'", name), 'r'))
		local keys = file:read('*all')
		keys = string.gsub(keys, '%s*\n', '')
		keys = string.gsub(keys, '<Space>', ' ')
		file:close()
		vis:feedkeys(keys..'<Escape>')
		vis:command(string.format("w! '%s.out'", name))
	end
	vis:command('q!')
end
