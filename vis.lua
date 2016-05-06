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

vis.filetypes =	{
	[".1|.2|.3|.4|.5|.6|.7|.8|.9|.1x|.2x|.3x|.4x|.5x|.6x|.7x|.8x|.9x"] = "man",
	[".adb|.ads"] = "ada",
	[".al|.perl|.pl|.pm|.pod"] = "perl",
	[".ans|.inp|.mac"] = "apdl",
	[".apl"] = "apl",
	[".applescript"] = "applescript",
	[".asa|.asp|.hta"] = "asp",
	[".asa|.bas|.cls|.ctl|.dob|.dsm|.dsr|.frm|.pag|.vb|.vba|.vbs"] = "vb",
	[".as|.asc"] = "actionscript",
	[".asm|.ASM|.s|.S"] = "asm",
	[".au3|.a3x"] = "autoit",
	[".awk"] = "awk",
	[".bash|.bashrc|.bash_profile|.configure|.csh|.sh|.zsh"] = "bash",
	[".bat|.cmd"] = "batch",
	[".bbl|.dtx|.ins|.ltx|.tex|.sty"] = "latex",
	[".bib"] = "bibtex",
	[".boo"] = "boo",
	[".bsh|.java"] = "java",
	[".caml|.ml|.mli|.mll|.mly"] = "caml",
	[".c|.cc|.C|.h"] = "ansi_c",
	[".cfg|.cnf|.inf|.ini|.reg"] = "ini",
	[".changes|.st|.sources"] = "smalltalk",
	[".ck"] = "chuck",
	[".cl|.el|.lisp|.lsp"] = "lisp",
	[".cmake|.cmake.in|.ctest|.ctest.in"] = "cmake",
	[".coffee"] = "coffeescript",
	[".cpp|.cxx|.c++|.hh|.hpp|.hxx|.h++"] = "cpp",
	[".cs"] = "csharp",
	[".css"] = "css",
	[".cu|.cuh"] = "cuda",
	[".dart"] = "dart",
	[".d|.di"] = "dmd",
	[".ddl|.sql"] = "sql",
	[".dem|.plt"] = "gnuplot",
	[".desktop"] = "desktop",
	[".diff|.patch"] = "diff",
	["Dockerfile"] = "dockerfile",
	[".dot"] = "dot",
	[".dpk|.dpr|.p|.pas"] = "pascal",
	[".dsp"] = "faust",
	[".dtd|.svg|.xml|.xsd|.xsl|.xslt|.xul"] = "xml",
	[".e|.eif"] = "eiffel",
	[".eps|.ps"] = "ps",
	[".erb|.rhtml"] = "rhtml",
	[".erl|.hrl"] = "erlang",
	[".ex|.exs"] = "elixir",
	[".feature"] = "gherkin",
	[".f|.for|.ftn|.fpp|.f77|.f90|.f95|.f03|.f08"] = "fortran",
	[".fish"] = "fish",
	[".forth|.frt|.fs"] = "forth",
	[".fs"] = "fsharp",
	[".g|.g4"] = "antlr",
	[".g|.gd|.gi|.gap"] = "gap",
	[".glslf|.glslv"] = "glsl",
	["GNUmakefile|.iface|.mak|.mk|makefile|Makefile"] = "makefile",
	[".go"] = "go",
	[".groovy|.gvy"] = "groovy",
	[".gtkrc"] = "gtkrc",
	[".hs"] = "haskell",
	[".htm|.html|.shtm|.shtml|.xhtml"] = "html",
	[".icn"] = "icon",
	[".idl|.odl"] = "idl",
	[".inc|.php|.php3|.php4|.phtml"] = "php",
	[".inf|.ni"] = "inform",
	[".io"] = "io_lang",
	[".js|.jsfl"] = "javascript",
	[".json"] = "json",
	[".jsp"] = "jsp",
	[".ledger|.journal"] = "ledger",
	[".less"] = "less",
	[".lily|.ly"] = "lilypond",
	[".litcoffee"] = "litcoffee",
	[".lua"] = "lua",
	[".md|.markdown"] = "markdown",
	[".m|.mm|.objc"] = "objective_c",
	[".moon"] = "moonscript",
	[".nim"] = "nim",
	[".n"] = "nemerle",
	[".nsh|.nsi|.nsis"] = "nsis",
	[".orx|.rex"] = "rexx",
	[".p8"] = "pico8",
	[".pike|.pmod"] = "pike",
	["PKGBUILD"] = "pkgbuild",
	[".po|.pot"] = "gettext",
	[".prolog"] = "prolog",
	[".props|.properties"] = "props",
	[".ps1"] = "powershell",
	[".pure"] = "pure",
	[".Rakefile|.rake|.rb|.rbw"] = "ruby",
	[".r|.reb"] = "rebol",
	[".R|.Rout|.Rhistory|.Rt|Rout.save|Rout.fail"] = "rstats",
	[".rs"] = "rust",
	[".rst"] = "rest",
	[".sass|.scss"] = "sass",
	[".scala"] = "scala",
	[".sch|.scm"] = "scheme",
	[".sc|.py|.pyw"] = "python",
	[".sno|.SNO"] = "snobol4",
	[".tcl|.tk"] = "tcl",
	[".texi"] = "texinfo",
	[".toml"] = "toml",
	[".vala"] = "vala",
	[".vcf|.vcard"] = "vcard",
	[".vh|.vhd|.vhdl"] = "vhdl",
	[".v|.ver"] = "verilog",
	[".wsf"] = "wsf",
	[".xtend"] = "xtend",
	[".yaml"] = "yaml",
}

vis.filetype_detect = function(win)
	local filename = win.file.name

	if filename ~= nil then
		-- filename = string.lower(filename)
		for patterns, lang in pairs(vis.filetypes) do
			for pattern in string.gmatch(patterns, '[^|]+') do
				if #filename >= #pattern then
					local s, e = string.find(filename, pattern, -#pattern, true)
					if s ~= e and e == #filename then
						win.syntax = lang
						return
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

