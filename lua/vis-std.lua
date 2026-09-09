-- standard vis event handlers

vis.events.subscribe(vis.events.INIT, function()
	if os.getenv("TERM_PROGRAM") == "Apple_Terminal" then
		vis:command("set change256colors false")
	end

	-- NOTE: keep standard vis robust against missing theme files
	local theme = "default"
	if pcall(require, "themes/"..theme) then
		vis:command("set theme " .. theme)
	end
end)

vis:option_register("theme", "string", function(name)
	if name ~= nil then
		local theme = 'themes/'..name
		package.loaded[theme] = nil
		require(theme)
	end

	local lexers = vis.lexers
	lexers.lexers = {}

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
end, "Color theme to use, filename without extension")

vis:option_register("syntax", "string", function(name)
	if not vis.win then return false end
	if not vis.win:set_syntax(name) then
		vis:info(string.format("Unknown syntax definition: `%s'", name))
		return false
	end
	return true
end, "Syntax highlighting lexer to use")

local function binsearch_token_idx(tokens, pos)
	local i0 = 2
	local i1 = #tokens
	if pos < tokens[i0]-1 then return i0 end
	if pos > tokens[i1]-1 then return i1 end
	local i repeat
		i = (i0 + i1) / 2
		i = i + i % 2
		if pos < tokens[i]-1 then i1 = i else i0 = i end
	until (i - i0 <= 2 and pos < tokens[i]-1) or i0 == i1
	return i
end

-- NOTE: skip_same_tokens() is only needed for suboptimal lexers that generate long sequences
-- of single character tokens with the same style (like the current markup lexer).
-- Otherwise, this would suffice:
--   return idx - skip_count
local function skip_same_tokens(tokens, idx, skip_count)
	for _ = 2, skip_count, 2 do
		local style = tokens[idx - 1]
		local j = 2
		while idx - j > 0 and style == tokens[idx - j - 1] do
			j = j + 2
		end
		idx = idx - j
	end
	return idx
end

local function find_token_at(tokens, pos)
	local min_token_cache_entries = 4
	if #tokens <= min_token_cache_entries then return 0 end
	local token_cache_size = (tokens[#tokens] or 2) - 1
	if pos == token_cache_size then return #tokens - min_token_cache_entries end
	local idx = binsearch_token_idx(tokens, pos)
	if idx <= min_token_cache_entries then return 0 end
	return skip_same_tokens(tokens, idx, min_token_cache_entries)
end

local function lex_range(win, start, finish)
	if not win.syntax or not vis.lexers.load then return {} end
	local lexer = vis.lexers.load(win.syntax, nil, true)
	if not lexer then return {} end
	if win.token_cache == nil then return {} end

	if finish < start then return win.token_cache end
	local prev_idx = find_token_at(win.token_cache, start)
	local prev_pos = 0
	if prev_idx > 0 then prev_pos = win.token_cache[prev_idx] - 1 end

	for i = #win.token_cache, prev_idx+1, -1 do win.token_cache[i] = nil end
	local data = win.file:content(prev_pos, finish - prev_pos + 1)
	local tokens = lexer:lex(data, 1)
	for i, v in ipairs(tokens) do
		if i % 2 == 0 then v = v + prev_pos end
		table.insert(win.token_cache, v)
	end

	return win.token_cache
end

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	win.token_cache = {}
end)

vis.events.subscribe(vis.events.FILE_MODIFIED, function(file, _op, pos, _len)
	for win in vis:windows() do
		if win.file ~= file then break end
		win.token_cache = lex_range(win, pos, win.viewport.bytes.finish)
	end
end)

vis.events.subscribe(vis.events.WIN_HIGHLIGHT, function(win)
	if win.token_cache == nil then win.token_cache = {} end
	local style_ids = vis.ui.style_ids

	--- token_cache_last_pos points to the byte position right after the last lexed token
	local token_cache_last_pos = (win.token_cache[#win.token_cache] or 2) - 1
	if next(win.token_cache) == nil or token_cache_last_pos < win.viewport.bytes.finish then
		win.token_cache = lex_range(win, token_cache_last_pos, win.viewport.bytes.finish)
		if next(win.token_cache) == nil then return end
	end

	local idx_start = binsearch_token_idx(win.token_cache, win.viewport.bytes.start)
	local idx_end   = binsearch_token_idx(win.token_cache, win.viewport.bytes.finish)
	for i = idx_start, idx_end, 2 do
		local name = win.token_cache[i-1]
		local style = style_ids[name]
		if style ~= nil then
			if idx_start == idx_end then
				-- if we have a big token that is larger than the viewport, e.g. a very
				-- large comment, we can limit setting the cell styles to the viewport
				win:style(style, win.viewport.bytes.start, win.viewport.bytes.finish)
			else
				local token_start = (win.token_cache[i-2] or 1) - 1
				-- win.token_cache points one byte beyond the token and is 1-indexed
				local token_end  = win.token_cache[i] - 2
				win:style(style, token_start, token_end)
			end
		end
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

	if not win.large then
		local col = selection.col
		table.insert(right_parts, selection.line..', '..col)
		if size > 33554432 or col > 65536 then
			win.large = true
		end
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
