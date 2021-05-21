-- complete word at primary selection location using vis-complete(1)
-- all non-primary selections will be completed aswell if their cursor
-- is located at the end of the same word

local get_prefix = function (file, pos)
		if not pos then return "", false end

		local range = file:text_object_word(pos > 0 and pos-1 or pos);
		if not range then return "", false end
		if range.finish > pos then range.finish = pos end
		if range.start == range.finish then return "", false end
		local prefix = file:content(range)
		if not prefix then return "", false end
		return prefix, true
end

vis:map(vis.modes.INSERT, "<C-n>", function()
	local win = vis.win
	local file = win.file
	local pos = win.selection.pos
	prefix, ok = get_prefix(file, pos)
	if not ok then return end

	local cmd = string.format("vis-complete --word '%s'", prefix:gsub("'", "'\\''"))
	local status, out, err = vis:pipe(file, { start = 0, finish = file.size }, cmd)
	if status ~= 0 or not out then
		if err then vis:info(err) end
		return
	end
	for selection in win:selections_iterator() do
		local pos = selection.pos
		localprefix, ok = get_prefix(file, pos)
		if not ok then goto continue end
		if localprefix ~= prefix then goto continue end

		file:insert(pos, out)
		selection.pos = pos + #out
		::continue::
	end
end, "Complete word in file")
