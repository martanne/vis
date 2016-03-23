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

	local before, after = pos - 4096, pos + 4096;
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

local set_filetype = function(win)
	local files = {
		[".1|.2|.3|.4|.5|.6|.7|.8|.9|.1x|.2x|.3x|.4x|.5x|.6x|.7x|.8x|.9x"] = "man",
		[".au3|.a3x"] = "autoit",
		[".as|.asc"] = "actionscript",
		[".adb|.ads"] = "ada",
		[".g|.g4"] = "antlr",
		[".ans|.inp|.mac"] = "apdl",
		[".apl"] = "apl",
		[".applescript"] = "applescript",
		[".asm|.ASM|.s|.S"] = "asm",
		[".asa|.asp|.hta"] = "asp",
		[".awk"] = "awk",
		[".bat|.cmd"] = "batch",
		[".bib"] = "bibtex",
		[".boo"] = "boo",
		[".cs"] = "csharp",
		[".c|.cc|.C"] = "ansi_c",
		[".cpp|.cxx|.c++|.h|.hh|.hpp|.hxx|.h++"] = "cpp",
		[".ck"] = "chuck",
		[".cmake|.cmake.in|.ctest|.ctest.in"] = "cmake",
		[".coffee"] = "coffeescript",
		[".css"] = "css",
		[".cu|.cuh"] = "cuda",
		[".d|.di"] = "dmd",
		[".dart"] = "dart",
		[".desktop"] = "desktop",
		[".diff|.patch"] = "diff",
		["Dockerfile"] = "dockerfile",
		[".dot"] = "dot",
		[".e|.eif"] = "eiffel",
		[".ex|.exs"] = "elixir",
		[".erl|.hrl"] = "erlang",
		[".dsp"] = "faust",
		[".feature"] = "gherkin",
		[".fs"] = "fsharp",
		[".fish"] = "fish",
		[".forth|.frt|.fs"] = "forth",
		[".f|.for|.ftn|.fpp|.f77|.f90|.f95|.f03|.f08"] = "fortran",
		[".g|.gd|.gi|.gap"] = "gap",
		[".po|.pot"] = "gettext",
		[".glslf|.glslv"] = "glsl",
		[".dem|.plt"] = "gnuplot",
		[".go"] = "go",
		[".groovy|.gvy"] = "groovy",
		[".gtkrc"] = "gtkrc",
		[".hs"] = "haskell",
		[".htm|.html|.shtm|.shtml|.xhtml"] = "html",
		[".icn"] = "icon",
		[".idl|.odl"] = "idl",
		[".inf|.ni"] = "inform",
		[".cfg|.cnf|.inf|.ini|.reg"] = "ini",
		[".io"] = "io_lang",
		[".bsh|.java"] = "java",
		[".js|.jsfl"] = "javascript",
		[".json"] = "json",
		[".jsp"] = "jsp",
		[".bbl|.dtx|.ins|.ltx|.tex|.sty"] = "latex",
		[".less"] = "less",
		[".lily|.ly"] = "lilypond",
		[".ledger|.journal"] = "ledger",
		[".cl|.el|.lisp|.lsp"] = "lisp",
		[".litcoffee"] = "litcoffee",
		[".lua"] = "lua",
		["GNUmakefile|.iface|.mak|.mk|makefile|Makefile"] = "makefile",
		[".md"] = "markdown",
		[".moon"] = "moonscript",
		[".n"] = "nemerle",
		[".nim"] = "nim",
		[".nsh|.nsi|.nsis"] = "nsis",
		[".m|.mm|.objc"] = "objective_c",
		[".caml|.ml|.mli|.mll|.mly"] = "caml",
		[".dpk|.dpr|.p|.pas"] = "pascal",
		[".al|.perl|.pl|.pm|.pod"] = "perl",
		[".inc|.php|.php3|.php4|.phtml"] = "php",
		[".p8"] = "pico8",
		[".pike|.pmod"] = "pike",
		["PKGBUILD"] = "pkgbuild",
		[".ps1"] = "powershell",
		[".eps|.ps"] = "ps",
		[".prolog"] = "prolog",
		[".props|.properties"] = "props",
		[".pure"] = "pure",
		[".sc|.py|.pyw"] = "python",
		[".R|.Rout|.Rhistory|.Rt|Rout.save|Rout.fail"] = "rstats",
		[".r|.reb"] = "rebol",
		[".rst"] = "rest",
		[".orx|.rex"] = "rexx",
		[".erb|.rhtml"] = "rhtml",
		[".Rakefile|.rake|.rb|.rbw"] = "ruby",
		[".rs"] = "rust",
		[".sass|.scss"] = "sass",
		[".scala"] = "scala",
		[".sch|.scm"] = "scheme",
		[".sno|.SNO"] = "snobol4",
		[".bash|.bashrc|.bash_profile|.configure|.csh|.sh|.zsh"] = "bash",
		[".changes|.st|.sources"] = "smalltalk",
		[".ddl|.sql"] = "sql",
		[".tcl|.tk"] = "tcl",
		[".texi"] = "texinfo",
		[".toml"] = "toml",
		[".vala"] = "vala",
		[".vcf|.vcard"] = "vcard",
		[".v|.ver"] = "verilog",
		[".vh|.vhd|.vhdl"] = "vhdl",
		[".asa|.bas|.cls|.ctl|.dob|.dsm|.dsr|.frm|.pag|.vb|.vba|.vbs"] = "vb",
		[".wsf"] = "wsf",
		[".dtd|.svg|.xml|.xsd|.xsl|.xslt|.xul"] = "xml",
		[".xtend"] = "xtend",
		[".yaml"] = "yaml",
	}

	local filename = win.file.name

	if filename ~= nil then
		-- filename = string.lower(filename)
		for patterns, lang in pairs(files) do
			for pattern in string.gmatch(patterns, '[^|]+') do
				if #filename >= #pattern then
					local s, e = string.find(filename, pattern, -#pattern, true)
					if s ~= e and e == #filename then
						win.syntax = lang
						return;
					end
				end
			end
		end
	end

	win.syntax = nil
end

vis.events.win_open = function(win)
	set_filetype(win)

	-- eg Turn on line numbering
	-- vis:command('set number')
end
