vis.events.subscribe(vis.events.FILE_SAVE_PRE, function(file)
	local lines = file.lines
	for i=1, #lines do
		lines[i] = lines[i]..' bigger'
	end
	return true
end)
