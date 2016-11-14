-- Vis Lua plugin API standard library

local ok, msg = pcall(function()
	vis.lexers = {}
	vis.lexers = require('lexer')
end)

if not ok then
	vis:info('WARNING: could not load lexer module, is lpeg installed?')
end

vis.events = {}

vis.motion_new = function(vis, key, motion)
	local id = vis:motion_register(motion)
	if id < 0 then
		return false
	end
	local binding = function()
		vis:motion(id)
	end
	vis:map(vis.MODE_NORMAL, key, binding)
	vis:map(vis.MODE_VISUAL, key, binding)
	vis:map(vis.MODE_OPERATOR_PENDING, key, binding)
	return true
end

vis.textobject_new = function(vis, key, textobject)
	local id = vis:textobject_register(textobject)
	if id < 0 then
		return false
	end
	local binding = function()
		vis:textobject(id)
	end
	vis:map(vis.MODE_VISUAL, key, binding)
	vis:map(vis.MODE_OPERATOR_PENDING, key, binding)
	return true
end

vis:textobject_new("ii", function(win, pos)

	if win.syntax == nil then
		return pos, pos
	end

	local before, after = pos - 4096, pos + 4096
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

vis.ftdetect = {}

vis.ftdetect.ignoresuffixes = "~|.orig|.bak|.old|.new|.dpkg-dist|.dpkg-old|.dpkg-new|.dpkg-bak|.pacsave|.pacnew"

vis.ftdetect.filetypes = {
	actionscript = { ext = ".as|.asc" },
	ada          = { ext = ".adb|.ads" },
	ansi_c       = { ext = ".c|.C|.h" },
	antlr        = { ext = ".g|.g4" },
	apdl         = { ext = ".ans|.inp|.mac" },
	apl          = { ext = ".apl" },
	applescript  = { ext = ".applescript" },
	asm          = { ext = ".asm|.ASM|.s|.S" },
	asp          = { ext = ".asa|.asp|.hta" },
	autoit       = { ext = ".au3|.a3x" },
	awk          = { ext = ".awk", app = "awk|nawk|mawk|gawk" },
	bash         = { ext = ".bash|.csh|.sh|.zsh", name = "%.bashrc|%.bash_profile|%.configure", app = "bash|csh|sh|zsh|ash|dash|tcsh" },
	batch        = { ext = ".bat|.cmd" },
	bibtex       = { ext = ".bib" },
	boo          = { ext = ".boo" },
	caml         = { ext = ".caml|.ml|.mli|.mll|.mly" },
	chuck        = { ext = ".ck" },
	cmake        = { ext = ".cmake|.cmake.in|.ctest|.ctest.in" },
	coffeescript = { ext = ".coffee", app = "coffee" },
	cpp          = { ext = ".cpp|.cxx|.c++|.cc|.hh|.hpp|.hxx|.h++" },
	crystal      = { ext = ".cr" },
	csharp       = { ext = ".cs" },
	css          = { ext = ".css" },
	cuda         = { ext = ".cu|.cuh" },
	dart         = { ext = ".dart", app = "dart" },
	desktop      = { ext = ".desktop" },
	diff         = { ext = ".diff|.patch" },
	dmd          = { ext = ".d|.di", app = "rdmd" },
	dockerfile   = { name = "Dockerfile" },
	dot          = { ext = ".dot" },
	dsv          = { name = "group|gshadow|passwd|shadow" },
	eiffel       = { ext = ".e|.eif" },
	elixir       = { ext = ".ex|.exs" },
	erlang       = { ext = ".erl|.hrl", app = "escript" },
	faust        = { ext = ".dsp" },
	fish         = { ext = ".fish", app = "fish" },
	forth        = { ext = ".forth|.frt|.fs", app = "gforth" },
	fortran      = { ext = ".f|.for|.ftn|.fpp|.f77|.f90|.f95|.f03|.f08" },
	fsharp       = { ext = ".fs" },
	fstab        = { name = "fstab" },
	gap          = { ext = ".g|.gd|.gi|.gap" },
	gettext      = { ext = ".po|.pot" },
	gherkin      = { ext = ".feature" },
	glsl         = { ext = ".glslf|.glslv" },
	gnuplot      = { ext = ".dem|.plt" },
	go           = { ext = ".go" },
	groovy       = { ext = ".groovy|.gvy", app = "groovy" },
	gtkrc        = { name = "%.?gtkrc.*" },
	haskell      = { ext = ".hs", app = "ghci" },
	html         = { ext = ".htm|.html|.shtm|.shtml|.xhtml" },
	icon         = { ext = ".icn" },
	idl          = { ext = ".idl|.odl" },
	inform       = { ext = ".inf|.ni" },
	ini          = { ext = ".cfg|.cnf|.inf|.ini|.reg" },
	io_lang      = { ext = ".io" },
	java         = { ext = ".bsh|.java" },
	javascript   = { ext = ".js|.jsfl", app = "jsc|node|rhino" },
	json         = { ext = ".json" },
	jsp          = { ext = ".jsp" },
	latex        = { ext = ".bbl|.dtx|.ins|.ltx|.tex|.sty" },
	ledger       = { ext = ".ledger|.journal" },
	less         = { ext = ".less" },
	lilypond     = { ext = ".lily|.ly" },
	lisp         = { ext = ".cl|.el|.lisp|.lsp", app = "sbcl|clisp" },
	litcoffee    = { ext = ".litcoffee" },
	lua          = { ext = ".lua", app = "lua" },
	makefile     = { ext = ".iface|.mak|.mk", name = "GNUmakefile|makefile|Makefile" },
	man          = { ext = ".1|.2|.3|.4|.5|.6|.7|.8|.9|.1x|.2x|.3x|.4x|.5x|.6x|.7x|.8x|.9x" },
	markdown     = { ext = ".md|.markdown" },
	moonscript   = { ext = ".moon" },
	nemerle      = { ext = ".n" },
	networkd     = { ext = ".link|.network|.netdev" },
	nim          = { ext = ".nim", app = "nim" },
	nsis         = { ext = ".nsh|.nsi|.nsis" },
	objective_c  = { ext = ".m|.mm|.objc" },
	pascal       = { ext = ".dpk|.dpr|.p|.pas" },
	perl         = { ext = ".al|.perl|.pl|.pm|.pod", app = "perl[%d.%a-]*|rakudo" },
	php          = { ext = ".inc|.php|.php3|.php4|.phtml", app = "php[%d]*" },
	pico8        = { ext = ".p8" },
	pike         = { ext = ".pike|.pmod" },
	pkgbuild     = { name = "PKGBUILD" },
	powershell   = { ext = ".ps1" },
	prolog       = { ext = ".prolog" },
	props        = { ext = ".props|.properties" },
	protobuf     = { ext = ".proto" },
	ps           = { ext = ".eps|.ps" },
	pure         = { ext = ".pure" },
	python       = { ext = ".sc|.py|.pyw", app = "python[%d.]*" },
	rebol        = { ext = ".r|.reb" },
	rest         = { ext = ".rst" },
	rexx         = { ext = ".orx|.rex" },
	rhtml        = { ext = ".erb|.rhtml" },
	rstats       = { ext = ".R|.Rout|.Rhistory|.Rt|Rout.save|Rout.fail" },
	ruby         = { ext = ".Rakefile|.rake|.rb|.rbw", app = "ruby" },
	rust         = { ext = ".rs" },
	sass         = { ext = ".sass|.scss" },
	scala        = { ext = ".scala" },
	scheme       = { ext = ".sch|.scm" },
	smalltalk    = { ext = ".changes|.st|.sources" },
	snobol4      = { ext = ".sno|.SNO" },
	sql          = { ext = ".ddl|.sql" },
	systemd      = { ext = ".automount|.device|.mount|.path|.scope|.service|.slice|.socket|.swap|.target|.timer" },
	taskpaper    = { ext = ".taskpaper" },
	tcl          = { ext = ".tcl|.tk" },
	texinfo      = { ext = ".texi" },
	toml         = { ext = ".toml" },
	vala         = { ext = ".vala" },
	vb           = { ext = ".asa|.bas|.cls|.ctl|.dob|.dsm|.dsr|.frm|.pag|.vb|.vba|.vbs" },
	vcard        = { ext = ".vcf|.vcard" },
	verilog      = { ext = ".v|.ver" },
	vhdl         = { ext = ".vh|.vhd|.vhdl" },
	wsf          = { ext = ".wsf" },
	xml          = { ext = ".dtd|.svg|.xml|.xsd|.xsl|.xslt|.xul" },
	xtend        = { ext = ".xtend" },
	yaml         = { ext = ".yaml" },
}

-- array of filetype detecting functions (win, filename, shebang, app) -> string
vis.ftdetect.customdetectors = {
	function(file, data)
		if data:sub(1, 5) == '<?xml' then
			return 'xml'
		end
	end,
}

vis.filetype_detect = function(win)
	-- remove ignored suffixes from filename
	local sanitizedfn = win.file.name
	if sanitizedfn ~= nil then
		sanitizedfn = sanitizedfn:gsub('^.*/', '')
		repeat
			local changed = false
			for pattern in vis.ftdetect.ignoresuffixes:gmatch('[^|]+') do
				if #sanitizedfn >= #pattern then
					local s, e = sanitizedfn:find(pattern, -#pattern, true)
					if e == #sanitizedfn then
						sanitizedfn = sanitizedfn:sub(1, #sanitizedfn - #pattern)
						changed = true
					end
				end
			end
		until not changed
	end

	local data = win.file:content(0, 256);

	-- find out via shebang which application would run our file
	local shebang
	local app
	if data:sub(1, 2) == '#!' then
		shebang = data:gsub('^#!%s*', ''):gsub('\n.*$', '')
		app = shebang:gsub('^/usr/bin/env%s*', ''):gsub('%s.*$', ''):gsub('^.*/', '')
		if #app == 0 then
			app = nil
		end
	end

	-- call custom detectors if any
	for _, func in pairs(vis.ftdetect.customdetectors) do
		local fres = func(win.file, data, sanitizedfn, shebang, app)
		if fres ~= nil then
			win.syntax = fres
			return
		end
	end

	-- detect filetype by application mentioned in the shebang
	if app ~= nil then
		for lang, ft in pairs(vis.ftdetect.filetypes) do
			if ft.app ~= nil then
				for pattern in ft.app:gmatch('[^|]+') do
					if app:match('^'..pattern..'$') ~= nil then
						win.syntax = lang
						return
					end
				end
			end
		end
	end

	-- detect filetype by filename
	if sanitizedfn ~= nil then
		-- try filetypes recognizable by exact filename or filename pattern rather than extension
		for lang, ft in pairs(vis.ftdetect.filetypes) do
			if ft.name ~= nil then
				for pattern in ft.name:gmatch('[^|]+') do
					if sanitizedfn:match('^'..pattern..'$') ~= nil then
						win.syntax = lang
						return
					end
				end
			end
		end
		-- then try to determine filetype by filename extention
		for lang, ft in pairs(vis.ftdetect.filetypes) do
			if ft.ext ~= nil then
				for pattern in ft.ext:gmatch('[^|]+') do
					if #sanitizedfn >= #pattern then
						local s, e = sanitizedfn:find(pattern, -#pattern, true)
						if e == #sanitizedfn then
							win.syntax = lang
							return
						end
					end
				end
			end
		end
	end

	win.syntax = nil
end

vis.events.theme_change = function(name)
	if name ~= nil then
		local theme = 'themes/'..name
		package.loaded[theme] = nil
		require(theme)
	end

	if vis.lexers ~= nil then
		vis.lexers.lexers = {}
	end

	for win in vis:windows() do
		win.syntax = win.syntax;
	end
end

vis.events.win_syntax = function(win, name)
	local lexers = vis.lexers
	if not lexers.load then
		return false
	end

	win:style_define(win.STYLE_DEFAULT, lexers.STYLE_DEFAULT)
	win:style_define(win.STYLE_CURSOR, lexers.STYLE_CURSOR)
	win:style_define(win.STYLE_CURSOR_PRIMARY, lexers.STYLE_CURSOR_PRIMARY)
	win:style_define(win.STYLE_CURSOR_LINE, lexers.STYLE_CURSOR_LINE)
	win:style_define(win.STYLE_SELECTION, lexers.STYLE_SELECTION)
	win:style_define(win.STYLE_LINENUMBER, lexers.STYLE_LINENUMBER)
	win:style_define(win.STYLE_COLOR_COLUMN, lexers.STYLE_COLOR_COLUMN)

	if name == nil then
		return true
	end

	local lexer = lexers.load(name)
	if not lexer then
		return false
	end

	for token_name, id in pairs(lexer._TOKENSTYLES) do
		local style = lexers['STYLE_'..string.upper(token_name)] or lexer._EXTRASTYLES[token_name]
		win:style_define(id, style)
	end

	return true
end

vis.events.win_highlight = function(win, horizon_max)
	if win.syntax == nil or vis.lexers == nil then
		return
	end
	local lexer = vis.lexers.load(win.syntax)
	if lexer == nil then
		return
	end

	-- TODO: improve heuristic for initial style
	local viewport = win.viewport
	if not viewport then
		return
	end
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
end

local modes = {
	[vis.MODE_NORMAL] = '',
	[vis.MODE_OPERATOR_PENDING] = '',
	[vis.MODE_VISUAL] = 'VISUAL',
	[vis.MODE_VISUAL_LINE] = 'VISUAL-LINE',
	[vis.MODE_INSERT] = 'INSERT',
	[vis.MODE_REPLACE] = 'REPLACE',
}

vis.events.win_status = function(win)
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
end

vis.events.theme_change((not vis.ui or vis.ui.colors <= 16) and "default-16" or "default-256")
