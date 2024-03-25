local complete_filename = function(expand)
	local win = vis.win
	local file = win.file
	local pos = win.selection.pos
	if not pos then return end

	-- TODO do something clever here
	local range = file:text_object_longword(pos > 0 and pos-1 or pos);
	if not range then return end
	if range.finish > pos then range.finish = pos end

	local prefix = file:content(range)
	if not prefix then return end

	-- Strip leading delimiters for some programming languages
	local _, j = prefix:find(".*[{[(<'\"]+")
	if not expand and j then prefix = prefix:sub(j + 1) end

	if prefix:match("^%s*$") then
		prefix = ""
		range.start = pos
		range.finish = pos
	end

	local cmdfmt = "vis-complete --file '%s'"
	if expand then cmdfmt = "vis-open -- '%s'*" end
	local status, out, err = vis:pipe(cmdfmt:format(prefix:gsub("'", "'\\''")))
	if status ~= 0 or not out then
		if err then vis:info(err) end
		return
	end
	out = out:gsub("\n$", "")

	if expand then
		file:delete(range)
		pos = range.start
	end

	file:insert(pos, out)
	win.selection.pos = pos + #out
end

-- complete file path at primary selection location using vis-complete(1)
vis:map(vis.modes.INSERT, "<C-x><C-f>", function()
	complete_filename(false);
end, "Complete file name")

-- complete file path at primary selection location using vis-open(1)
vis:map(vis.modes.INSERT, "<C-x><C-o>", function()
	complete_filename(true);
end, "Complete file name (expands path)")
