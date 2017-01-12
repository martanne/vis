vis.events = {}
vis.events.win_open = function(win)
	-- test.in file passed to vis
	local in_file = win.file.name
	if in_file then
		-- use the corresponding test.cmd file
		local cmd_file_name = string.gsub(in_file, '%.in$', '.cmd');
		local cmd_file = io.open(cmd_file_name)
		local cmd = cmd_file:read('*all')
		vis:command(string.format(",{\n %s\n }", cmd))
		local out_file_name = string.gsub(in_file, '%.in$', '.vis.out')
		vis:command(string.format("wq! %s", out_file_name))
	end
	vis:command('qall!')
end
