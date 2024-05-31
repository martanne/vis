-- complete word at primary selection location using vis-complete(1)

vis:map(vis.modes.INSERT, "<C-n>", function()
	local win = vis.win
	local file = win.file
	local pos = win.selection.pos
	if not pos then return end

	local range = file:text_object_word(pos > 0 and pos-1 or pos);
	if not range then return end
	if range.finish > pos then range.finish = pos end
	if range.start == range.finish then return end
	local prefix = file:content(range)
	if not prefix then return end

	vis:feedkeys("<vis-selections-save><Escape><Escape>")
	-- collect words starting with prefix
	vis:command("x/\\b" .. prefix .. "\\w+/")
	local candidates = {}
	for sel in win:selections_iterator() do
		table.insert(candidates, file:content(sel.range))
	end
	vis:feedkeys("<Escape><Escape><vis-selections-restore>")
	if #candidates == 1 and candidates[1] == "\n" then return end
	candidates = table.concat(candidates, "\n")

	local status, out, err = vis:pipe_buffer(candidates, "sort -u | vis-menu")
	if status ~= 0 or not out then
		if err then vis:info(err) end
		return
	end
	out = out:sub(#prefix + 1, #out - 1)
	file:insert(pos, out)
	win.selection.pos = pos + #out
	-- restore mode to what it was on entry
	vis.mode = vis.modes.INSERT
end, "Complete word in file")
