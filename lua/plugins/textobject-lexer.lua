-- text object matching a lexer token

local MAX_CONTEXT = 32768

vis:textobject_new("ii", function(win, pos)

	if win.syntax == nil or not vis.lexers then
		return pos, pos
	end

	local before, after = pos - MAX_CONTEXT, pos + MAX_CONTEXT
	if before < 0 then
		before = 0
	end
	-- TODO make sure we start at a line boundary?

	local lexer = vis.lexers.load(win.syntax)
	local data = win.file:content(before, after - before)
	local tokens = lexer:lex(data)
	local cur = before
	-- print(before..", "..pos..", ".. after)

	for i = 1, #tokens, 2 do
		local name = tokens[i]
		local token_next = before + tokens[i+1] - 1
		-- print(name..": ["..cur..", "..token_next.."] pos: "..pos)
		if cur <= pos and pos < token_next then
			return cur, token_next
		end
		cur = token_next
	end

	return pos, pos
end)
