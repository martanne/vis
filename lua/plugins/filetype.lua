
local M = {}
vis.ftdetect = M

-- Remove all suffixes to get the real extension/filename
vis.ftdetect.ignoresuffixes = {
	"~+$", "%.orig$", "%.bak$", "%.old$", "%.new$"
}

--[[
The filetypes table is the master table, its supposed to be lexer = { [items] = {} }
but lexer doesn't necessarily need to be a lexer,
it could hold options or settings that are applied on :set syntax [syntax]
almost all items are arrays of lua pattern strings
Running hundreds of string.find on each file open is slow, so if you must add to an item
use the lookup tables like extensions and try to point to the filetypes table

[syntax|lexer] = {
	alt_name = "" -- lexer name
	name = { lua pattern string list }
	cmd = { vis:command() list of strings }
	utility = {} list of patterns for the hashbang utility (no paths)
	hashbang = {} list of patterns for the complete hashbang
	datap = {} patterns for the first 256 characters of the file}
}
--]]
local filetypes = {
	cmake = {
		name = { "%.cmake.in$", "%.ctest.in$" }
	},
	crontab = {
		cmd = { "set savemethod inplace" },
		name = { "^crontab.*$" }
	},
	["git-commit"] = {
		alt_name = "diff",
		cmd = { "set colorcolumn 72" },
	},
	lua = {
		utility = { "^lua%-?5?%d?$", "^lua%-?5%.%d$" }
	},
	python = {
		utility = { "^python%d?" }
	},
}
-- backwards compatibility
	for syntax,T in pairs(filetypes) do
		T.ext = {}
		T.name = {}
		T.mime  = {}
		T.utility = {}
		T.hashbang = {}
		T.cmd = {}
	end
	vis.ftdetect.filetypes = filetypes
	-- In order to be semi backwards compatible, we create a table on each index access
	-- This should be removed... eventually.
	setmetatable(filetypes, {
		__index = function (t,k)
			local T = {
				ext = {},
				name = {},
				utility = {},
				mime = {},
				hashbang = {},
				cmd = {}
			}
			rawset(t,k,T)
			return T
		end
	})

local data_patterns = {
	["^execve%("] = 'strace',
	["^#.* by RouterOS"] = 'routeros',
}
M.data_patterns = data_patterns

-- complete filename, case sensitive
local filenames = {
	APKBUILD = 'bash',
	[".login"] = 'bash',
	profile = 'bash',
	['.profile'] = 'bash',
	mkshrc = 'bash',
	['.mkshrc'] = 'bash',
	['bash.bashrc'] = 'bash',
	['bash.bash.logout'] = 'bash',
	['.bash_profile'] = 'bash',
	['.bashrc'] = 'bash',
	['.bash_logout'] = 'bash',
	['.shinit'] = 'bash',
	['.xprofile'] = 'bash',
	xsin = 'xs',
	xsrc = 'xs',
	Vagrantfile = 'ruby',
	Dockerfile = 'dockerfile',
	group = 'dsv',
	gshadow = 'dsv',
	passwd = 'dsv',
	shadow = 'dsv',
	fstab = 'fstab',
	COMMIT_EDITMSG = 'git-commit',
	["git%-rebase%-todo"] = 'git-rebase',
	Jenkinsfile = 'groovy',
	GNUmakefile = 'makefile',
	makefile = 'makefile',
	Makefile = 'makefile',
	['meson.build'] = 'meson',
	['meson.options'] = 'meson',
	['meson_options.txt'] = 'meson',
	PKGBUILD = 'pkgbuild',
}
M.filenames = filenames

local extensions = {
	["1"] = "troff",
	["1p"] = "man",
	["1x"] = "troff",
	["2"] = "troff",
	["2p"] = "man",
	["2x"] = "troff",
	["3"] = "troff",
	["3p"] = "man",
	["3x"] = "troff",
	["4"] = "troff",
	["4p"] = "man",
	["4x"] = "troff",
	["5"] = "troff",
	["5p"] = "man",
	["5x"] = "troff",
	["6"] = "troff",
	["6p"] = "man",
	["6x"] = "troff",
	["7"] = "troff",
	["7p"] = "man",
	["7x"] = "troff",
	["8"] = "troff",
	["8p"] = "man",
	["8x"] = "troff",
	["9"] = "troff",
	["9p"] = "man",
	["9x"] = "troff",
	ASM = "asm",
	C = "c",
	["CMakeLists.txt"] = "cmake",
	Dockerfile = "dockerfile",
	GNUmakefile = "makefile",
	Makefile = "makefile",
	PKGBUILD = "pkgbuild",
	R = "r",
	Rakefile = "ruby",
	Rhistory = "r",
	Rout = "r",
	["Rout.fail"] = "r",
	["Rout.save"] = "r",
	Rt = "r",
	S = "asm",
	SNO = "snobol4",
	a3x = "autoit",
	adb = "ada",
	adoc = "asciidoc",
	ads = "ada",
	ahk = "autohotkey",
	al = "perl",
	ans = "apdl",
	apl = "apl",
	applescript = "applescript",
	as = "actionscript",
	asa = "asp",
	asc = "actionscript",
	asm = "asm",
	asp = "asp",
	au3 = "autoit",
	automount = "systemd",
	awk = "awk",
	bas = "vb",
	bash = "bash",
	bashrc = "bash",
	bat = "batch",
	bbl = "latex",
	bib = "bibtex",
	boo = "boo",
	bsh = "java",
	c = "c",
	["c++"] = "cpp",
	caml = "caml",
	cc = "cpp",
	cfg = "ini",
	changes = "smalltalk",
	cjs = "javascript",
	ck = "chuck",
	cl = "lisp",
	clj = "clojure",
	cljc = "clojure",
	cljs = "clojure",
	cls = "vb",
	cmake = "cmake",
	["cmake.in"] = "cmake",
	cmd = "batch",
	cnf = "ini",
	coffee = "coffeescript",
	conf = "ini",
	configure = "bash",
	container = "systemd",
	cpp = "cpp",
	cr = "crystal",
	cs = "csharp",
	csh = "bash",
	css = "css",
	ctest = "cmake",
	["ctest.in"] = "cmake",
	ctl = "vb",
	cu = "cuda",
	cuh = "cuda",
	cxx = "cpp",
	d = "d",
	dart = "dart",
	ddl = "sql",
	def = "modula2",
	dem = "gnuplot",
	desktop = "desktop",
	device = "systemd",
	di = "d",
	diff = "diff",
	dob = "vb",
	dot = "dot",
	dpk = "pascal",
	dpr = "pascal",
	dsm = "vb",
	dsp = "faust",
	dsr = "vb",
	dtd = "xml",
	dtx = "latex",
	e = "eiffel",
	ebuild = "bash",
	edn = "clojure",
	eif = "eiffel",
	el = "lisp",
	elm = "elm",
	eml = "mail",
	eps = "ps",
	erb = "rhtml",
	erl = "erlang",
	es = "rc",
	ex = "elixir",
	exs = "elixir",
	f = "fortran",
	f03 = "fortran",
	f08 = "fortran",
	f77 = "fortran",
	f90 = "fortran",
	f95 = "fortran",
	factor = "factor",
	fan = "fantom",
	feature = "gherkin",
	fish = "fish",
	fnl = "fennel",
	["for"] = "fortran",
	forth = "forth",
	fpp = "fortran",
	frm = "vb",
	frt = "forth",
	fs = "fsharp",
--	fs = "forth", -- DUPLICATE
	fstab = "fstab",
	fth = "forth",
	ftn = "fortran",
	fun = "sml",
	g = "antlr",
	g4 = "antlr",
	gap = "gap",
	gd = "gap",
	gi = "gap",
	gleam = "gleam",
	glif = "xml",
	glsl = "glsl",
	glslf = "glsl",
	glslv = "glsl",
	gmi = "gemini",
	go = "go",
	groovy = "groovy",
	gtkrc = "gtkrc",
	gvy = "groovy",
	h = "c",
	["h++"] = "cpp",
	ha = "hare",
	hh = "cpp",
	hpp = "cpp",
	hrl = "erlang",
	hs = "haskell",
	hta = "asp",
	htm = "html",
	html = "html",
	hxx = "cpp",
	i3 = "modula3",
	i = 'c',
	icn = "icon",
	idl = "idl",
	iface = "makefile",
	ig = "modula3",
	ily = "lilypond",
	inc = "php",
	inf = "ini",
	ini = "ini",
	inp = "apdl",
	ins = "latex",
	io = "io_lang",
	java = "java",
	jl = "julia",
	journal = "ledger",
	jq = "jq",
	js = "javascript",
	jsfl = "javascript",
	json = "json",
	jsp = "jsp",
	jsx = "javascript",
	ksh = "bash",
	ledger = "ledger",
	less = "less",
	lgt = "logtalk",
	lily = "lilypond",
	link = "networkd",
	lisp = "lisp",
	litcoffee = "litcoffee",
	lsp = "lisp",
	ltx = "latex",
	lua = "lua",
	ly = "lilypond",
	m = "objective_c",
	m3 = "modula3",
	mac = "apdl",
	mak = "makefile",
	makefile = "makefile",
	markdown = "markdown",
	md = "markdown",
	me = "man",
	meson = "meson",
	["meson.build"] = "meson",
	mg = "modula3",
	mjs = "javascript",
	mk = "makefile",
	mksh = "bash",
	ml = "caml",
	mli = "caml",
	mll = "caml",
	mly = "caml",
	mm = "objective_c",
	mod = "modula2",
	mom = "man",
	moon = "moonscript",
	mount = "systemd",
	ms = "man",
	myr = "myrddin",
	n = "nemerle",
	netdev = "networkd",
	network = "networkd",
	ni = "inform",
	nim = "nim",
	nix = "nix",
	nsh = "nsis",
	nsi = "nsis",
	nsis = "nsis",
	objc = "objective_c",
	obs = "objeck",
	odl = "idl",
	org = "org",
	orx = "rexx",
	p = "pascal",
	p8 = "pico8",
	pag = "vb",
	pas = "pascal",
	patch = "diff",
	path = "systemd",
	perl = "perl",
	php = "php",
	php3 = "php",
	php4 = "php",
	phtml = "php",
	pike = "pike",
	pl = "perl",
	plist = "xml",
	plt = "gnuplot",
	pm = "perl",
	pmod = "pike",
	po = "gettext",
	pod = "perl",
	pony = "pony",
	pot = "gettext",
	pro = "prolog",
	prolog = "prolog",
	properties = "props",
	props = "props",
	proto = "protobuf",
	ps = "ps",
	ps1 = "powershell",
	psm1 = "powershell",
	pure = "pure",
	py = "python",
	pyi = "python",
	pyw = "python",
	pyx = "python",
	r = "rebol",
	rake = "ruby",
	rb = "ruby",
	rbw = "ruby",
	rc = "rc",
	re = "reason",
	reb = "rebol",
	reg = "ini",
	rej = "diff",
	rex = "rexx",
	rhtml = "rhtml",
	rkt = "scheme",
	rs = "rust",
	rsc = "routeros",
	rst = "rest",
	s = "asm",
	sass = "sass",
	sc = "python",
	scala = "scala",
	sch = "scheme",
	scm = "scheme",
	scope = "systemd",
	scss = "sass",
	service = "systemd",
	sh = "bash",
	shtm = "html",
	shtml = "html",
	sig = "sml",
	sld = "scheme",
	slice = "systemd",
	sls = "scheme",
	sml = "sml",
	sno = "snobol4",
	socket = "systemd",
	sources = "smalltalk",
	spec = "rpmspec",
	spin = "spin",
	sql = "sql",
	ss = "scheme",
	st = "smalltalk",
	sty = "latex",
	sv = "verilog",
	svg = "xml",
	swap = "systemd",
	t2t = "txt2tags",
	target = "systemd",
	taskpaper = "taskpaper",
	tcl = "tcl",
	tex = "latex",
	texi = "texinfo",
	timer = "systemd",
	tk = "tcl",
	tmac = "man",
	toml = "toml",
	ts = "typescript",
	tsx = "typescript",
	txt = "text",
	typ = "typst",
	typst = "typst",
	usfm = "usfm",
	v = "verilog",
	vala = "vala",
	vb = "vb",
	vba = "vb",
	vbs = "vb",
	vcard = "vcard",
	vcf = "vcard",
	ver = "verilog",
	vh = "vhdl",
	vhd = "vhdl",
	vhdl = "vhdl",
	vue = "html",
	wiki = "mediawiki",
	wsf = "wsf",
	xhtm = "html",
	xhtml = "html",
	xml = "xml",
	xs = "xs",
	xsd = "xml",
	xsin = "xs",
	xsl = "xml",
	xslt = "xml",
	xsrc = "xs",
	xtend = "xtend",
	xul = "xml",
	yaml = "yaml",
	yml = "yaml",
	zig = "zig",
	zsh = "bash"
}
M.extensions = extensions

local L = require"lexers.lexer"
L.detect_extensions = extensions
L.detect_patterns = data_patterns

-- Lookup table for mime [mime] = "lexer"
-- "text/" is prepended so you can omit it
local mimes = {
	gemini = "gemini",
	xml = "xml",
	["application/x-shellscript"] = "bash",
	["x-c"] = "c",
	["x-c++"] = "cpp",
	["x-coffee"] = "coffeescript",
	["x-css"] = "css",
	["x-haskell"] = "haskell",
	["x-html"] = "html",
	["x-json"] = "json",
	["x-lisp"] = "lisp",
	["x-lua"] = "lua",
	["x-makefile"] = "makefile",
	["x-markdown"] = "markdown",
	["x-moon"] = "moonscript",
	["x-objc"] = "objective_c",
	["x-perl"] = "perl",
	["x-python"] = "python",
	["x-ruby"] = "ruby",
	["x-rust"] = "rust",
	["x-sass"] = "sass",
	["x-scala"] = "scala",
	["x-script.python"] = "python",
	["x-scss"] = "sass",
	["x-shellscript"] = "bash",
	["x-tex"] = "latex",
	["x-yaml"] = "yaml",
}
M.mimes = mimes

local utilities = {
	awk = 'awk',
	mawk = 'awk',
	nawk = 'awk',
	gawk = 'awk',
	goawk = 'awk',

	sh = 'bash',
	bash = 'bash',
	ash = 'bash',
	ksh = 'bash',
	mksh = 'bash',
	csh = 'bash',
	tcsh = 'bash',
	zsh = 'bash',
	dash = 'bash',

	make = 'make',

	python = 'python',
	python2 = 'python',
	python3 = 'python',

	rc = 'rc',
	es = 'rc',

	tclsh = 'tcl',
	jimsh = 'tcl',

	lua = 'lua',

	octave = 'matlab',
	perl = 'perl',
	php = 'php',
	ruby = 'ruby',
}
M.utils = utilities

-- string.find(table, pattern)
local function TStringFind(tbl, subject)
	for _, pattern in ipairs(tbl or {}) do
		if subject:find(pattern) then
			return true
		end
	end
	return false
end

--[[ hashbang check
	hashbangs only have PATH <SPACE> ARG
		if PATH matches /env, utility should be in args
			POSIX does not have -S, but BSDs/Linux seem to
			All do seem to have name=value
			So far, this does the rudimentary job of parsing env args
			for most cases, but its a bandaid
			discard first arg if /-[^S]*S/; and all subsequent /=/
			NOTE: this means you can't have a command with /^-|=/
	return first field, which should be the utility.
	NOTE: long-options (GNUisms) unsupported
--]]
local function GetHashBang(data)
	local fullhb, utility = data:match"^#![ \t]*(/+[^/\n]+[^\n]*)"
	if fullhb then
		local i, field = 1, {}
		for m in fullhb:gmatch"%g+" do field[i],i = m,i+1 end
		-- NOTE: executables should not have a space (or =, see below)
		if field[1]:match"/env$" then
			table.remove(field,1)
			-- it is assumed that the first argument are short options, with -S inside
			if string.find(field[1] or "", "^%-[^S-]*S") then -- -S found
				table.remove(field,1)
				-- skip all name=value
				while string.find(field[1] or "","=") do
					table.remove(field,1)
				end
				-- (hopefully) whatever is left in field[1] should be the utility or nil
			end
		end
		utility = string.match(field[1] or "", "[^/]+$") -- remove filepath
	end
	return fullhb, utility
end

-- Returns syntax filetype
-- Correctness order:
-- utility -> datap -> detect -> filename -> extension -> mime
-- Some extensions are duplicated, so they are low priority.
-- Technically in terms of quickness it should be
-- extension -> filename -> utility -> hashbang -> mime

-- Mimes is supposed to be the most correct, but its a fork to file
-- Which is slow, and semi non-portable
local function Detect(win)
	local file = win.file

	-- pass first few bytes of file to custom file type detector functions
	local data = file:content(0, 256)
	if data and #data > 0 then
		local fullhb, utility = GetHashBang(data)
		if fullhb then
			if utility and utilities[utility] then
				return utilities[utility]
			end
			for lang, ft in pairs(M.filetypes) do
				if
					utility and TStringFind(ft.utility, utility)
					or TStringFind(ft.hashbang, fullhb)
					-- Same as below but saves us a loop
					or (ft.detect and ft.detect(file, data))
				then
					return lang
				end
			end
		else
			for patt, syntax in pairs(data_patterns) do
				if data:find(patt) then return syntax end
			end
			for lang, ft in pairs(M.filetypes) do
				if
					TStringFind(ft.datap, data)
					or type(ft.detect) == 'function' and ft.detect(file, data)
				then
					return lang
				end
			end
		end
	end

	local mime
	local path = file.name -- filepath
	if path and path~="" then
		local name = path and path:match("[^/]+$") -- filename
		if name then
			local unchanged
			while #name > 0 and name ~= unchanged do
				unchanged = name
				for _, pattern in ipairs(M.ignoresuffixes) do
					name = name:gsub(pattern, "")
				end
			end
		end

		if name and #name > 0 then
			if filenames[name] then return filenames[name] end

			local l = L.detect(name, data and data:match"^[^\n]+" or "")
			if l then return l end

			-- detect filetype by filename ending with a configured extension
			local ext = name:match"%.([^%.]+)$"
			if ext then
				local lexer = extensions[ext] or extensions[ext:lower()]
				if lexer then return lexer end
			end

			-- detect filetype by filename pattern
			for lang, ft in pairs(M.filetypes) do
				for _, pattern in ipairs(ft.name or {}) do
					if name:find(pattern) then return lang end
				end
				for _, pattern in ipairs(ft.ext or {}) do
					if name:find(pattern) then return lang end
				end
			end
		end

		-- run file(1) to determine mime type
		local fileh = io.popen(
			string.format(
				"file -bL --mime-type -- '%s'", path:gsub("'", "'\\''")
			)
		)
		mime = fileh:read('*l')
		fileh:close()
		if mime then
			local lexer = mimes[mime] or mimes["text/" .. mime]
			if lexer then return lexer end
		end
	end

	-- try text lexer as a last resort
	if (mime or 'text/plain'):match('^text/.+$') then
		return 'text'
	end

	return nil
end

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	local syntax = Detect(win)
	if syntax then
		-- in order to avoid the backwards compatibility metatable, rawget is used
		local filetype = rawget(filetypes, syntax)
		if filetype then
			for _, cmd in ipairs(filetype.cmd or {}) do
				vis:command(cmd)
			end
			syntax = filetype.alt_name or syntax
		end
		if package.searchpath("lexers." .. syntax, package.path) then
			win:set_syntax(syntax)
			return
		else
			vis:info(
				string.format(
					"Lexer '%s' not found", syntax
				)
			)
		end
	end
	win:set_syntax(nil)
	return nil
end)
