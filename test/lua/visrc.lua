package.path = '../../lua/?.lua;'..package.path
dofile("../../lua/vis.lua")

-- redirect output to stdout, stderr is used by the vis UI
io.stderr = io.stdout

-- make sure we gracefully terminate, cleanup terminal state etc.
os.exit = function(status)
	vis:exit(status)
end

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	-- test.in file passed to vis
	local in_file = win.file.name
	if in_file then
		-- use the corresponding test.lua file
		lua_file = string.gsub(in_file, '%.in$', '.lua')
		local ok, msg = pcall(dofile, lua_file)
		if not ok then
			if type(msg) == 'string' then
				print(msg)
			end
			vis:exit(1)
			return
		end
	end
	vis:exit(0)
end)
