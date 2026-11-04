
------------------- vis options

vis.theme = 'default'
vis:option_register("theme", "string", function (name)
	local theme_path = name and ('vis.themes.' .. name)
	if theme_path and package.searchpath(theme_path, package.path) then
		package.loaded[theme_path] = nil
		require(theme_path)
		vis.theme = name
	else
		vis:message(('Bad theme: "%s"'):format(name))
		return false
	end

	local lexers = vis.lexers
	if not lexers.load then return false end
	if not lexers.property then lexers.load("text") end
	local colors = lexers.colors
	local default_colors = {
		"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"
	}
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
		win:set_syntax(win.syntax) -- WIN_OPEN sets win.syntax
	end
	return true
end, "Color theme to use, filename without extension")


vis:option_register("horizon", "number", function(horizon)
	if not vis.win then return false end
	vis.win.horizon = horizon
	return true
end, "Number of bytes to consider for syntax highlighting")



local ft = vis.ftdetect

-- Sets syntax and performs default settings
vis:option_register("syntax", "string", function(syntax)
	local win = vis.win
	if not win then return false end
	if syntax=='off' then
		win.lastlexer = win.lexer
		win.lexer = nil
		return true
	elseif syntax=='on' then
		win.lexer = win.lexer or win.lastlexer or win:set_syntax(win.syntax)
		return true
	end
	syntax = ft.Actions(syntax, win)
	win.file.syntax = syntax
	if win:set_syntax(syntax, true) then return true end
	vis:info(string.format("Unknown syntax definition: `%s'", syntax))
end, "Changes window syntax")


vis:option_register("lexer", "string", function(lexer)
	local win = vis.win
	if not win then return false end
	if win:set_syntax(lexer, true) then return true end
	vis:info(string.format("Unknown lexer definition: `%s'", lexer))
end, "Changes window lexer")



