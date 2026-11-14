
local M = {}
vis.ftdetect = M
-- This specific module has a style guide
-- 1. Everything is local (luacheck should detect typos)
-- 2. identifiers that start with uppercase and are words must be functions
-- 2a. everything else is data

--[[
	filetype.lua identifies the opened file's filetype and sets syntax highlighting and etc

	If you want to perform per filetype settings, require this file and
		vis.ftdetect[filetype][setting]
	filetype usually is a lexer or a syntax see the filetype table for options

	Usually all tables here (apart from the filetypes table) values point to a lexer or
	filetype[syntax]

	Example:

	vis.ftdetect.extensions.sh = "bash"
	vis.ftdetect.filenames.makefile = "makefile"
	vis.ftdetect.mimes["plain"] = "text"
	vis.ftdetect.utilities.python3 = "python" -- for shebang
	vis.ftdetect.data_patterns["^#!%s*.-[/ ]awk"] = "awk"

	If you do not want an exact match, you can use the filetypes table below
	See FINDERS
		table.insert(vis.ftdetect.cmake.name, "%.cmake%.in")

	Actions can be performed via the filetype table as well, assuming extensions is sh then
		table.insert( vis.ftdetect.filetypes.bash.cmd, "set shell /bin/bash")
	would set the shell to /bin/bash when the filetype is bash, see ACTIONS
--]]

-- Dictionary of filetypes, Should match opened filename on WIN_OPEN
-- Slow and error prone due to its iterative nature, use the hashtables
-- You can still use it for per filetype settings via filetypes[syntax][cmd|lexer|actions]
-- scintilluas lexer.lua also has a detect function which is also used
local filetypes = { -- dict --
--[[
	[filetype: string]: dict = {
		filetype should be lexer, if not, set .lexer as subkey

	ACTIONS: These run if a finder succeeds
		lexer: string -- Should be in lexers/$alt_name
		alt_name: string -- same as lexer, BACKWARDS COMPAT
		cmd: ipairs = { "command" } -- run list of commands (via vis:command(command))
		actions: ipairs = { function(win, filetype) }

	FINDERS/Searchers:
		name: ipairs = { "luapattern" } -- patterns matching filename
			USE M.filenames for absolute match
		ext: ipairs = { "luapattern" } -- same as name (BACKWARDS COMPAT)
			USE M.extensions for absolute match
		utility: ipairs = { "luapattern" } for hashbang utility (no path)
			USE M.utilities if absolute_match
		hashbang: ipairs = { "luapattern" } Contains path+args (no #!%s+)
			USE M.data_patterns or M.utilities
		mime: ipairs = { "exact match" } -- NOTE: This will be removed
			USE M.mimes
		detect: function (file, data)
	}
--]]
	cmake = {
		name = { "%.cmake.in$", "%.ctest.in$" }
	},
	crontab = {
		cmd = { "set savemethod inplace" },
		name = { "^crontab.*$" },
		lexer = 'text'
	},
	["git-commit"] = {
		lexer = "diff",
		cmd = { "set colorcolumn 72" },
	},
	lua = {
		utility = { "^lua%-?5?%d?$", "^lua%-?5%.%d$" }
	},
	python = {
		utility = { "^python%d?" }
	},
	r = {
		name = { "%.Rout%.fail$", "%.Rout%.save$" }
	}
}
	for syntax,T in pairs(filetypes) do
		T.ext = T.ext or {}
		T.name = T.name or {}
		T.utility = T.utility or {}
		T.hashbang = T.hashbang or {}
		T.cmd = T.cmd or {}
		T.actions = T.actions or {}
	end

	setmetatable(filetypes, {
		__index = function (t,k)
			local T = {
				ext = {},
				name = {},
				utility = {},
				hashbang = {},
				cmd = {},
				actions = {}
			}
			rawset(t,k,T)
			return T
		end
	})

-- From here on, values MUST MATCH THE FILETYPE TABLE ABOVE
-- This is because the filetype table contains settings
-- when a lexer name changes, update the filetype table to use the correct name

-- This table matches filenames AND extensions
-- This is done because L.detect also mixes it
-- Single extensions only (no dots "."!)
-- filename is lowered, if its lowercase, it will match
local fnames = {
	-- extensions
	fs = "fsharp",
	--	fs = "forth", -- DUPLICATE
	["1p"] = "man",
	["2p"] = "man",
	["3p"] = "man",
	["4p"] = "man",
	["5p"] = "man",
	["6p"] = "man",
	["7p"] = "man",
	["8p"] = "man",
	["9p"] = "man",
	adoc = "asciidoc",
	ash = "bash",
	cjs = "javascript",
	conf = "ini",
	container = "systemd",
	def = "modula2",
	ebuild = "bash",
	eml = "mail",
	es = "rc",
	fth = "forth",
	glif = "xml",
	glsl = "glsl",
	h = "c",
	i = "c",
	i3 = "modula3",
	ig = "modula3",
	ily = "lilypond",
	jsx = "javascript",
	m3 = "modula3",
	me = "man",
	meson = "meson",
	mg = "modula3",
	mjs = "javascript",
	mk = "makefile",
	mod = "modula2",
	mom = "man",
	ms = "man",
	plist = "xml",
	pro = "prolog",
	psm1 = "powershell",
	pyi = "python",
	pyx = "python",
	rc = "rc",
	rej = "diff",
	rkt = "scheme",
	sld = "scheme",
	sls = "scheme",
	ss = "scheme",
	sv = "verilog",
	tmac = "man",
	tsx = "typescript",
	txt = "text",
	typ = "typst",
	typst = "typst",
	usfm = "usfm",
	wiki = "mediawiki",
	xhtm = "html",
	yash = "bash",

	-- filenames
	[".bash_logout"] = "bash",
	[".bash_profile"] = "bash",
	[".bashrc"] = "bash",
	[".login"] = "bash",
	[".mkshrc"] = "bash",
	[".profile"] = "bash",
	[".sh.profile"] = "bash",
	[".shinit"] = "bash",
	[".xprofile"] = "bash",
	[".yash_profile"] = "bash",
	[".yashrc"] = "bash",
	APKBUILD = "bash",
	COMMIT_EDITMSG = "git-commit",
	Jenkinsfile = "groovy",
	Vagrantfile = "ruby",
	["bash.bash.logout"] = "bash",
	["bash.bashrc"] = "bash",
	["git%-rebase%-todo"] = "git-rebase",
	group = "dsv",
	gshadow = "dsv",
	["meson.options"] = "meson",
	["meson_options.txt"] = "meson",
	mkshrc = "bash",
	passwd = "dsv",
	profile = "bash",
	shadow = "dsv"
}

-- Lookup table for mime [mime] = "lexer"
-- "text/" is prepended, you can omit it
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


local utilities = {
	awk = 'awk', mawk = 'awk', nawk = 'awk', gawk = 'awk', goawk = 'awk',

	sh = 'bash', ash = 'bash', dash = 'bash',
	bash = 'bash',
	ksh = 'bash', mksh = 'bash',
	csh = 'bash', tcsh = 'bash',
	zsh = 'bash',

	make = 'make',

	python = 'python', python2 = 'python', python3 = 'python',

	rc = 'rc', es = 'rc',

	tclsh = 'tcl', jimsh = 'tcl',

	lua = 'lua',

	octave = 'matlab',
	perl = 'perl',
	php = 'php',
	ruby = 'ruby',
}


local data_patterns = {
	["^execve%("] = 'strace',
	["^#.* by RouterOS"] = 'routeros'
}


-- Removes all suffixes to get the real extension/filename
local ignoresuffixes = {
	-- list is luapatterns matching filename --
	"~+$",

	-- dict is extensions --
	-- Used by gsub, MUST BE ""
	orig = "", bak = "", old = "", new = ""
}


-- lexer contains detecting functionality
local L = package.loaded.lexer if not L then
	-- Need lexer.lua's detecting function, and lexer.lua expects _G.lpeg
	-- this creates a fake _G.lpeg and requires it forcefully
	local pattern = {} -- fake stub
	local RPF -- Returns pattern function
		= function() return pattern end
	pattern = setmetatable(pattern, {
		-- Operator Overloading
		__add = RPF , __sub = RPF , __mul = RPF , __div = RPF
		, __mod = RPF , __pow = RPF , __unm = RPF , __idiv = RPF
		, __band = RPF , __bor = RPF , __bxor = RPF , __bnot = RPF
		, __shl = RPF , __shr = RPF , __concat = RPF , __len = RPF
		, __eq = RPF , __lt = RPF , __le = RPF

		-- __index is not used, if something fails, then this hack should be removed
	})
	local fakepeg = {
		P=RPF, R=RPF, S=RPF, V=RPF, B=RPF, C=RPF
		, setmaxstack = function()end -- basically a Noop
	}
	local lexerenv = {lpeg = fakepeg} for k,v in pairs(_G) do lexerenv[k] = v end
	local chunk = loadfile(package.searchpath('lexer',  package.path), 't', lexerenv)
	local b; b, L = pcall(chunk)
	if not b then
		vis:message"filetype.lua: lexer.lua Initial fallback failed"
		L = require'plugins.filetype-fallback-lexer' -- last working fallback lexer
	end
	L.detect_patterns = data_patterns
	L.detect_extensions = fnames
end

---------------------------------- Database section ends


-- string.find(table, pattern)
local function TStringFind(tbl, subject) --> boolean
	for _, pattern in ipairs(tbl or {}) do
		if subject:find(pattern) then
			return true
		end
	end
	return false
end

--[[ Hashbang Utility Extractor
	#! is UTILITY <SPACE> ARG
	If UTILITY matches /env, its in ARG

	FreeBSD/GNU has -S flag which splits ARG by spaces and has other options
		if -S is found, all options are discarded until it finds utility
		options are identified as /^-|=/
		NOTE: This means a utility cannot match this pattern
--]]
local function GetHashBang(data)
	local hb, pathname, args = data:match"^#![ \t]*((/%S+)[\t ]*([^\n]*))\n"
	local utility = pathname and pathname:match"[^/]+$"
	if utility=="env" then
		local a = args:match"^%-[^S-]*S%s*(.+)" -- #!env -Ssh or -S sh
		if a then
			args = a
				:gsub("%-[Cu] %S+", "")
				:gsub("[^%s=]+=[^%s=]+","")
		end
		utility = args:match"%S+"
	end
	return hb, utility
end


-- utility -> datap -> filename -> extension -> L.detect() -> mime.
-- mime is supposed to be the most correct, but it execs file
-- Which is slow and semi non-portable, so its left for last
local function Detect(file)
	local R -- string: syntax

	-- pass first few bytes of file to custom file type detector functions
	local data = file:content(0, 256)

	local line
	if data and #data > 0 then
		line = file.lines[1]
		local fullhb, utility = GetHashBang(data)
		if fullhb then
			if utility then
				R = utilities[utility]
					-- for utilities like python3.n, just match python3
					or utilities[utility:match"^%w+"]
				if R then return R end
			end
			for lang, ft in pairs(filetypes) do
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
			for lang, ft in pairs(filetypes) do
				if type(ft.detect) == 'function' and ft.detect(file, data)
				then return lang end
			end
		end
	end

	local path = file.path or file.name
	local name = file.basename or (path or ''):match("[^/]+$")

	if name then
		local unchanged
		local suffixes = ignoresuffixes
		while #name > 0 and name ~= unchanged do
			unchanged = name
			name = name:gsub('%.([^.]+)$', suffixes) -- extensions
			for _, pattern in ipairs(suffixes) do
				name = name:gsub(pattern, "")
			end
		end
	end

	if name or line then
		R = L.detect(name, line or "")
			or name and L.detect(name:lower(), "")
		if R then return R end
	end

	if name and #name > 0 then
		-- detect filetype by filename pattern
		for lang, ft in pairs(filetypes) do
			if ft.name and TStringFind(ft.name, name)
				or ft.ext and TStringFind(ft.ext, name) -- REMOVE
			then return lang end
		end


		-- run file(1) to determine mime type
		local fileh = io.popen(
			string.format(
				"file -bL --mime-type -- '%s'", path:gsub("'", "'\\''")
			)
		)
		if fileh then
			local mime = fileh:read('*l')
			fileh:close()
			if mime then
				R = mimes[mime] or mimes["text/" .. mime]
				if R then return R end
			end
		end
	end

	return 'text'
end


-- perform actions saved in filetype[]
local Action = function(win, syntax) --> string lexer_name
	local lexer_name = syntax or win.file.syntax
	local filetype = rawget(filetypes, lexer_name) -- avoid backwards compatibility mt
	if filetype then
		for _, Action in ipairs(filetype.actions or {}) do
			Action(win, filetype)
		end
		for _, cmd in ipairs(filetype.cmd or {}) do
			vis:command(cmd)
		end
		-- Override
		lexer_name = filetype.lexer
			or filetype.alt_name -- backwards compat -- REMOVE ME
			or lexer_name
	end
	return lexer_name
end


----------------------------- Events

-- FILE_OPEN runs before WIN_OPEN
vis.events.subscribe(vis.events.FILE_OPEN, function(file)
	file.basename = (file.name or file.path or ''):match("[^/]+$")
	file.syntax = Detect(file) -- ALWAYS points to a detected syntax, not lexer_name
end)


vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	local syntax = Action(win)
	if not win:set_syntax(syntax) then
		vis:warn( ("Lexer '%s' not found"):format(syntax) )
	end
end)

vis.events.subscribe(vis.events.FILE_SAVE_POST, function(file,path)
	-- Perform actions on newly written files. :new -> :w file.ext
	--
	-- Sets .basename
	-- :set syntax to detected new file/path

	-- FILE_OPEN and WIN_OPEN above events already triggered
	if file.name==nil -- is unassignable from lua
		and path -- exists and is not stdout
		and file.basename==nil -- :new, so FILE_OPEN couldn't set basename
			-- since .name and .path were nil
	then
		file.basename = path:match("[^/]+$") -- only run once because basename is set
		local syntax = file.syntax
		file.syntax = Detect(file)
		local win = vis.win if not win then return true end
		if syntax~=file.syntax then
			syntax = Action(win)
			win:set_syntax(syntax)
		end
	end

	return true
end)



----------------------- Export

M.filetypes = filetypes
M.filenames = fnames
M.extensions = fnames
M.utilities = utilities
M.mimes = mimes
M.data_patterns = data_patterns
M.ignoresuffixes = ignoresuffixes
M.ft_action = Action
M.detect = Detect


return M
