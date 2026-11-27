-- standard vis event handlers

-- vis:info doesn't stack, so you won't see all the errors pre INIT, just latest
-- vis:message should be used in the compilation stage
-- for commands and options, vis:warn should be used
vis.warn = vis.message

vis.events.subscribe(vis.events.INIT, function()
	if os.getenv("TERM_PROGRAM") == "Apple_Terminal" then
		vis:command("set change256colors false")
	end

	vis:command("set theme default")
end)

local active_theme
vis:option_register("theme", "string", function(name)
	local theme_path = 'themes/' .. name
	package.loaded[theme_path] = nil
	if not pcall(require, theme_path) then
		vis:warn( ('Theme "%s" not found'):format(name) )
		return false
	end

	-- set the active theme
	active_theme = name

	local lexers = vis.lexers
	lexers.lexers = {} -- clears lexers cache

	if not lexers.load then return false end
	if not lexers.property then lexers.load("text") end
	local colors = lexers.colors
	local default_colors = { "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white" }
	for _, c in ipairs(default_colors) do
		if not colors[c] or colors[c] == '' then
			colors[c] = c
		end
	end

	local ui = vis.ui
	ui:style_define(ui.style_ids.DEFAULT,           lexers.STYLE_DEFAULT           or '')
	ui:style_define(ui.style_ids.CURSOR,            lexers.STYLE_CURSOR            or '')
	ui:style_define(ui.style_ids.CURSOR_PRIMARY,    lexers.STYLE_CURSOR_PRIMARY    or '')
	ui:style_define(ui.style_ids.CURSOR_LINE,       lexers.STYLE_CURSOR_LINE       or '')
	ui:style_define(ui.style_ids.SELECTION,         lexers.STYLE_SELECTION         or '')
	ui:style_define(ui.style_ids.LINENUMBER,        lexers.STYLE_LINENUMBER        or '')
	ui:style_define(ui.style_ids.LINENUMBER_CURSOR, lexers.STYLE_LINENUMBER_CURSOR or '')
	ui:style_define(ui.style_ids.COLOR_COLUMN,      lexers.STYLE_COLOR_COLUMN      or '')
	ui:style_define(ui.style_ids.STATUS,            lexers.STYLE_STATUS            or '')
	ui:style_define(ui.style_ids.STATUS_FOCUSED,    lexers.STYLE_STATUS_FOCUSED    or '')
	ui:style_define(ui.style_ids.SEPARATOR,         lexers.STYLE_SEPARATOR         or '')
	ui:style_define(ui.style_ids.INFO,              lexers.STYLE_INFO              or '')
	ui:style_define(ui.style_ids.EOF,               lexers.STYLE_EOF               or '')
	ui:style_define(ui.style_ids.WHITESPACE,        lexers.STYLE_WHITESPACE        or '')

	for win in vis:windows() do
		win:set_syntax(win.syntax)
	end
	return true
end, function()
	return active_theme
end, "Color theme to use, filename without extension")

vis:option_register("syntax", "string", function(name)
	if not vis.win then return false end
	if not vis.win:set_syntax(name) then
		vis:warn(string.format("Unknown syntax definition: `%s'", name))
		return false
	end
	return true
end, "Syntax highlighting lexer to use")

vis:option_register("horizon", "number", function(horizon)
	if not vis.win then return false end
	vis.win.horizon = horizon
	return true
end, "Number of bytes to consider for syntax highlighting")

vis.events.subscribe(vis.events.WIN_HIGHLIGHT, function(win)
	if not win.syntax or not vis.lexers.load then return end
	local lexer = vis.lexers.load(win.syntax, nil, true)
	if not lexer then return end

	-- TODO: improve heuristic for initial style
	local viewport = win.viewport.bytes
	if not viewport then return end
	local horizon_max = win.horizon or 32768
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
	local selection = win.selection

	local mode = modes[vis.mode]
	if mode ~= '' and vis.win == win then
		table.insert(left_parts, mode)
	end

	table.insert(left_parts, (file.name or '[No Name]') ..
		(file.modified and ' [+]' or '') .. (vis.recording and ' @' or ''))

	local count = vis.count
	local keys = vis.input_queue
	if keys ~= '' then
		table.insert(right_parts, keys)
	elseif count then
		table.insert(right_parts, count)
	end

	if #win.selections > 1 then
		table.insert(right_parts, selection.number..'/'..#win.selections)
	end

	local size = file.size
	local pos = selection.pos
	if not pos then pos = 0 end
	table.insert(right_parts, (size == 0 and "0" or math.ceil(pos/size*100)).."%")

	if not win:large() then
		local col = selection.col
		table.insert(right_parts, selection.line..', '..col)
	end

	local left = ' ' .. table.concat(left_parts, " » ") .. ' '
	local right = ' ' .. table.concat(right_parts, " « ") .. ' '
	win:status(left, right);
end)

-- default plugins

require('plugins/filetype')
require('plugins/textobject-lexer')
require('plugins/digraph')
require('plugins/number-inc-dec')
require('plugins/complete-word')
require('plugins/complete-filename')
