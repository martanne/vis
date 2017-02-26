-- standard vis event handlers

vis.events.subscribe(vis.events.INIT, function()
	if os.getenv("TERM_PROGRAM") == "Apple_Terminal" then
		vis:command("set change-256colors false");
	end
	vis:command("set theme ".. (vis.ui.colors <= 16 and "default-16" or "default-256"))
end)

vis.events.subscribe(vis.events.THEME_CHANGE, function(name)
	if name ~= nil then
		local theme = 'themes/'..name
		package.loaded[theme] = nil
		require(theme)
	end

	if vis.lexers then vis.lexers.lexers = {} end

	for win in vis:windows() do
		win.syntax = win.syntax;
	end
end)

vis.events.subscribe(vis.events.WIN_SYNTAX, function(win, name)
	local lexers = vis.lexers
	if not lexers then return false end

	win:style_define(win.STYLE_DEFAULT, lexers.STYLE_DEFAULT or '')
	win:style_define(win.STYLE_CURSOR, lexers.STYLE_CURSOR or '')
	win:style_define(win.STYLE_CURSOR_PRIMARY, lexers.STYLE_CURSOR_PRIMARY or '')
	win:style_define(win.STYLE_CURSOR_LINE, lexers.STYLE_CURSOR_LINE or '')
	win:style_define(win.STYLE_SELECTION, lexers.STYLE_SELECTION or '')
	win:style_define(win.STYLE_LINENUMBER, lexers.STYLE_LINENUMBER or '')
	win:style_define(win.STYLE_COLOR_COLUMN, lexers.STYLE_COLOR_COLUMN or '')

	if name == nil then return true end

	local lexer = lexers.load(name)
	if not lexer then return false end

	for token_name, id in pairs(lexer._TOKENSTYLES) do
		local style = lexers['STYLE_'..string.upper(token_name)] or lexer._EXTRASTYLES[token_name]
		win:style_define(id, style)
	end

	return true
end)

vis.events.subscribe(vis.events.WIN_HIGHLIGHT, function(win, horizon_max)
	if win.syntax == nil or vis.lexers == nil then return end
	local lexer = vis.lexers.load(win.syntax)
	if lexer == nil then return end

	-- TODO: improve heuristic for initial style
	local viewport = win.viewport
	if not viewport then return end
	local horizon = viewport.start < horizon_max and viewport.start or horizon_max
	local view_start = viewport.start
	local lex_start = viewport.start - horizon
	local token_start = lex_start
	viewport.start = token_start
	local data = win.file:content(viewport)
	local token_styles = lexer._TOKENSTYLES
	local tokens = lexer:lex(data, 1)

	for i = 1, #tokens, 2 do
		local token_end = lex_start + tokens[i+1] - 1
		if token_end >= view_start then
			local name = tokens[i]
			local style = token_styles[name]
			if style ~= nil then
				win:style(style, token_start, token_end)
			end
		end
		token_start = token_end
	end
end)

local modes = {
	[vis.modes.NORMAL] = '',
	[vis.modes.OPERATOR_PENDING] = '',
	[vis.modes.VISUAL] = 'VISUAL',
	[vis.modes.VISUAL_LINE] = 'VISUAL-LINE',
	[vis.modes.INSERT] = 'INSERT',
	[vis.modes.REPLACE] = 'REPLACE',
}

vis.events.subscribe(vis.events.WIN_STATUS, function(win)
	local left_parts = {}
	local right_parts = {}
	local file = win.file
	local cursor = win.cursor

	local mode = modes[vis.mode]
	if mode ~= '' and vis.win == win then
		table.insert(left_parts, mode)
	end

	table.insert(left_parts, (file.name or '[No Name]') ..
		(file.modified and ' [+]' or '') .. (vis.recording and ' @' or ''))

	if file.newlines == "crlf" then
		table.insert(right_parts, "␍␊")
	end

	if #win.cursors > 1 then
		table.insert(right_parts, cursor.number..'/'..#win.cursors)
	end

	local size = file.size
	table.insert(right_parts, (size == 0 and "0" or math.ceil(cursor.pos/size*100)).."%")

	if not win.large then
		local col = cursor.col
		table.insert(right_parts, cursor.line..', '..col)
		if size > 33554432 or col > 65536 then
			win.large = true
		end
	end

	local left = ' ' .. table.concat(left_parts, " » ") .. ' '
	local right = ' ' .. table.concat(right_parts, " « ") .. ' '
	win:status(left, right);
end)

-- additional default keybindings

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

vis:map(vis.modes.NORMAL, "gf", function(keys)
	local r = vis.win.file:text_object_longword(vis.win.cursor.pos)
	local filename = vis.win.file:content(r)
	local ok = vis:open(filename)
	if not ok then
		vis:info("Could not open file " .. filename .. " for reading.")
	end
	return #keys
end, "Open file under cursor in a new window")

vis:map(vis.modes.NORMAL, "<C-w>gf", "gf")
