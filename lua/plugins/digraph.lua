-- insert digraphs using vis-digraph(1)

vis:map(vis.modes.INSERT, "<C-k>", function(keys)
	if #keys < 2 then
		return -1 -- need more input
	end
	local file = io.popen(string.format("vis-digraph '%s' 2>&1", keys:gsub("'", "'\\''")))
	local output = file:read('*all')
	local success, msg, status = file:close()
	if success then
		if vis.mode == vis.modes.INSERT then
			vis:insert(output)
		elseif vis.mode == vis.modes.REPLACE then
			vis:replace(output)
		end
	elseif msg == 'exit' then
		if status == 2 then
			return -1 -- prefix need more input
		end
		vis:info(output)
	end
	return #keys
end, "Insert digraph")
