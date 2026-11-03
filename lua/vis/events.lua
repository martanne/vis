
-- standard vis event handlers

--[[
	Order of operations
	INIT
	FILE_OPEN
	WIN_HIGHLIGHT -- if lexer exists
	WIN_STATUS
	WIN_OPEN
--]]

-- On init
vis.events.subscribe(vis.events.INIT, function()
	if os.getenv("TERM_PROGRAM") == "Apple_Terminal" then
		vis:command("set change256colors false")
	end

	vis:command('set theme default')
end)



-- WIN_HIGHLIGHT changes when the file changed, or when the viewport changed
if lpeg and vis.lexers.load then
	vis.events.subscribe(vis.events.WIN_HIGHLIGHT, function(win)
		local lexer = win.lexer
		if not lexer then return end

		-- TODO: improve heuristic for initial style
		local viewport = win.viewport.bytes
		if not viewport then return end
		local horizon_max = win.horizon or 2^15
		local horizon     = viewport.start < horizon_max and viewport.start or horizon_max
		local view_start  = viewport.start
		local lex_start   = viewport.start - horizon
		viewport.start    = lex_start
		local data        = win.file:content(viewport)
		local style_ids   = vis.ui.style_ids
		local tokens      = lexer:lex(data, 1)
		local token_end   = lex_start + (tokens[#tokens] or 1) - 1

		for i = #tokens - 1, 1, -2 do
			local token_start = lex_start + (tokens[i-1] or 1) - 1
			if token_end < view_start then
				break
			end
			local name = tokens[i]
			local style_id = style_ids[name]
			if style_id ~= nil then
				win:style(style_id, token_start, token_end)
			end
			token_end = token_start - 1
		end
	end)
end


-- WIN_STATUS runs on each keypress and it also runs before the window opens
-- So while strange, you're better off setting options for the window in here
vis.events.subscribe(vis.events.WIN_STATUS, function(win)
	if win.file.size > 2^25 or win.selection.col > 2^16 then
		win.large = true
	else
		win.large = false
	end
end)


local ft = vis.ftdetect


-- FILE_OPEN runs before WIN_OPEN
vis.events.subscribe(vis.events.FILE_OPEN, function(
	file -- FILE -- file.name: string|nil, file.path: string|nil
)
	local path = file.path
	local name = file.name
	if not name and path then name = path:match'[^/]+$' end
	file.basename = name
	-- ft.Detect depends on file.basename
	file.syntax = path==nil and 'text' or ft.Detect(file)
end)


vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	-- vis:command"info"
	local syntax = ft.Actions(win.file.syntax, win)
	win:set_syntax(syntax)
end)


-- :o -> :w file.ext -- doesn't update file or syntax, nor file.name
vis.events.subscribe(vis.events.FILE_SAVE_POST, function(file, path)
	-- file.name: string|nil; file.path: string
	-- You cannot set file.name from lua
	local name = file.name
	if name==nil then
		if not file.basename then
			file.basename = name or path:match'[^/]+$'
		end
		local win = vis.win if not win then return true end
		local syntax = file.syntax if syntax~='text' then return true end
		file.syntax = ft.Detect(file)
		if syntax~=file.syntax then
			syntax = ft.Actions(file.syntax, win)
			win:set_syntax(syntax)
		end
	end
end)


--[[ BUG
:o -> :w p.lua -- esc fails, goes into permanent insert mode
it doesn't go into the statusbar
vis.events.subscribe(vis.events.FILE_SAVE_POST, function(file, path)
	if file.name==nil and file.syntax=='text' then
		vis:message"hey"
	end
end)
--]]

