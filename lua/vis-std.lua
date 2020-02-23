-- standard vis event handlers

vis.events.subscribe(vis.events.INIT, function()
	if os.getenv("TERM_PROGRAM") == "Apple_Terminal" then
		vis:command("set change-256colors false");
	end
	vis:command("set theme ".. (vis.ui.colors <= 16 and "default-16" or "default-256"))
end)

vis:option_register("theme", "string", function(name)
	if name ~= nil then
		local theme = 'themes/'..name
		package.loaded[theme] = nil
		require(theme)
	end

	vis.lexers.lexers = {}

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

vis:option_register("horizon", "number", function(horizon)
	if not vis.win then return false end
	vis.win.horizon = horizon
	return true
end, "Number of bytes to consider for syntax highlighting")

vis:option_register("redrawtime", "string", function(redrawtime)
	if not vis.win then return false end
	local value = tonumber(redrawtime)
	if not value or value <= 0 then
		vis:info("A positive real number expected")
		return false
	end
	vis.win.redrawtime = value
	return true
end, "Seconds to wait for syntax highlighting before aborting it")

local function populate(horizons, view_start, tokens, horizon_min)
	for i = 2, #tokens, 2 do
		if tokens[i] >= view_start then break end
		if tokens[i] - horizons[#horizons] > horizon_min then
			table.insert(horizons, tokens[i] - 1)
		end
	end
end

local function purge(list, base)
	for i = #list, 1, -1 do
		if list[i] > base then
			table.remove(list)
		else
			break
		end
	end
end

local function make_highlighter()
	local horizons = {0}
	local last_view_start = 0
	local last_view_len = 0
	local redrawtime_exceeded
	return function(win)
		if not win.syntax or not vis.lexers.load then return end
		local lexer = vis.lexers.load(win.syntax, nil, true)
		if not lexer then return end
		local viewport = win.viewport
		if not viewport then return end
		local redrawtime_max = win.redrawtime or 1.0
		local horizon_max = win.horizon or 32768
		local horizon = viewport.start < horizon_max and viewport.start or horizon_max
		local view_start = viewport.start
		local horizon_min = last_view_len  -- keeps horizon candidates one screenful apart. could be some constant as well
		local leap = view_start - last_view_start
		if leap <= last_view_len then
			purge(horizons, view_start)
		end
		local lex_start = redrawtime_exceeded and viewport.start - horizon or horizons[#horizons]
		local data = win.file:content({start = lex_start, finish = viewport.finish})
		local tokens, timedout = lexer:lex(data, 1, redrawtime_max)
		if not redrawtime_exceeded and timedout then
			redrawtime_exceeded = timedout
			tokens = lexer:lex(data, 1, redrawtime_max, viewport.start - horizon - lex_start)
		elseif leap > last_view_len and not redrawtime_exceeded then
			populate(horizons, view_start, tokens, horizon_min)
		end
		local token_end = lex_start + (tokens[#tokens] or 1) - 1
		local token_styles = lexer._TOKENSTYLES
		for i = #tokens - 1, 1, -2 do
			local token_start = lex_start + (tokens[i-1] or 1) - 1
			if token_end < view_start then
				if leap > 0                                       -- we are scrolling forward
					and view_start - lex_start > horizon_min  -- keep horizons not too close to each other
					and token_start > lex_start               -- don't store duplicate values
					and not redrawtime_exceeded then
					table.insert(horizons, token_start)
				end
				break
			end
			local name = tokens[i]
			local style = token_styles[name]
			if style ~= nil then
				win:style(style, token_start, token_end)
			end
			token_end = token_start - 1
		end
		last_view_start = view_start
		last_view_len = viewport.finish - view_start
	end
end

local highlight_func = {}

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	highlight_func[win] = make_highlighter()
end)

vis.events.subscribe(vis.events.WIN_CLOSE, function(win)
	highlight_func[win] = nil
end)

vis.events.subscribe(vis.events.WIN_HIGHLIGHT, function(win)
	if highlight_func[win] then
		highlight_func[win](win)
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
