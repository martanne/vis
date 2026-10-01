-- Non-LPeg fallback for lexers/lexer.lua, providing only the file type detection part
-- of the lexer API. Which doesn't require Lpeg.
--
-- To resync after a Scintillua resync, regenerate the body between the
-- BEGIN/END markers with:
--
--	sed -n '/^--- Map of file extensions, without/,/^-- The following are utility functions/p' \
--		lua/lexers/lexer.lua | sed '$d'
--
-- Do not hand-edit between the markers.

local M = {}

-- BEGIN verbatim copy of lua/lexers/lexer.lua (detect tables + M.detect)
--- Map of file extensions, without the '.' prefix, to their associated lexer names.
-- @usage lexer.detect_extensions.luadoc = 'lua'
M.detect_extensions = {}

--- Map of file extensions (without the '.' prefix) to strip from filenames during detection to
-- `true`.
-- @usage lexer.ignore_extensions.backup = true
M.ignore_extensions = {orig = true, back = true, old = true, new = true}

--- Map of first-line patterns to their associated lexer names.
-- These are Lua string patterns, not LPeg patterns.
-- @usage lexer.detect_patterns['^#!.+/zsh'] = 'bash'
M.detect_patterns = {}

--- List of filename parts to strip from filenames during detection.
-- Filename parts are expressed as Lua patterns.
-- @usage table.insert(lexer.ignore_patterns, '%.%d+$') -- ignore digit extensions
M.ignore_patterns = {'~+$'}

--- Returns the name of the lexer often associated a particular filename and/or file content.
-- @param[opt] filename String filename to inspect. The default value is read from the
--   "lexer.scintillua.filename" property.
-- @param[optchain] line String first content line, such as a shebang line. The default value
--   is read from the "lexer.scintillua.line" property.
-- @return string lexer name to pass to `lexer.load()`, or `nil` if none was detected
function M.detect(filename, line)
	if not filename then filename = M.property and M.property['lexer.scintillua.filename'] or '' end
	if not line then line = M.property and M.property['lexer.scintillua.line'] or '' end

	-- Locally scoped in order to avoid persistence in memory.
	local extensions = {
		as = 'actionscript', asc = 'actionscript', --
		adb = 'ada', ads = 'ada', --
		g = 'antlr', g4 = 'antlr', --
		ans = 'apdl', inp = 'apdl', mac = 'apdl', --
		apl = 'apl', --
		applescript = 'applescript', --
		ino = 'arduino', --
		asm = 'asm', ASM = 'asm', s = 'asm', S = 'asm', --
		asa = 'asp', asp = 'asp', hta = 'asp', --
		ahk = 'autohotkey', --
		au3 = 'autoit', a3x = 'autoit', --
		awk = 'awk', --
		bat = 'batch', cmd = 'batch', --
		bib = 'bibtex', --
		boo = 'boo', --
		cs = 'csharp', --
		c = 'c', C = 'c', cc = 'cpp', cpp = 'cpp', cxx = 'cpp', ['c++'] = 'cpp', h = 'cpp', H = 'cpp',
		hh = 'cpp', hpp = 'cpp', hxx = 'cpp', ['h++'] = 'cpp', --
		ck = 'chuck', --
		clj = 'clojure', cljs = 'clojure', cljc = 'clojure', edn = 'clojure', --
		['CMakeLists.txt'] = 'cmake', cmake = 'cmake', ['cmake.in'] = 'cmake', ctest = 'cmake',
		['ctest.in'] = 'cmake', --
		coffee = 'coffeescript', --
		cr = 'crystal', --
		css = 'css', --
		cu = 'cuda', cuh = 'cuda', --
		d = 'd', di = 'd', --
		dart = 'dart', --
		desktop = 'desktop', --
		diff = 'diff', patch = 'diff', rej = 'diff', --
		Dockerfile = 'dockerfile', --
		dot = 'dot', --
		e = 'eiffel', eif = 'eiffel', --
		ex = 'elixir', exs = 'elixir', --
		elm = 'elm', --
		erl = 'erlang', hrl = 'erlang', --
		fs = 'fsharp', --
		factor = 'factor', --
		fan = 'fantom', --
		dsp = 'faust', --
		fnl = 'fennel', --
		fish = 'fish', --
		forth = 'forth', frt = 'forth', --
		f = 'fortran', ['for'] = 'fortran', ftn = 'fortran', fpp = 'fortran', f77 = 'fortran',
		f90 = 'fortran', f95 = 'fortran', f03 = 'fortran', f08 = 'fortran', --
		fstab = 'fstab', --
		gd = 'gap', gi = 'gap', gap = 'gap', --
		gmi = 'gemini', --
		po = 'gettext', pot = 'gettext', --
		feature = 'gherkin', --
		gleam = 'gleam', --
		glslf = 'glsl', glslv = 'glsl', --
		dem = 'gnuplot', plt = 'gnuplot', --
		go = 'go', --
		groovy = 'groovy', gvy = 'groovy', --
		gtkrc = 'gtkrc', --
		ha = 'hare', --
		hs = 'haskell', --
		htm = 'html', html = 'html', shtm = 'html', shtml = 'html', xhtml = 'html', vue = 'html', --
		icn = 'icon', --
		idl = 'idl', odl = 'idl', --
		ni = 'inform', --
		cfg = 'ini', cnf = 'ini', inf = 'ini', ini = 'ini', reg = 'ini', --
		io = 'io_lang', --
		janet = 'janet', --
		bsh = 'java', java = 'java', --
		cjs = 'javascript', js = 'javascript', jsfl = 'javascript', mjs = 'javascript', --
		jq = 'jq', --
		json = 'json', --
		jsp = 'jsp', --
		jl = 'julia', --
		bbl = 'latex', dtx = 'latex', ins = 'latex', ltx = 'latex', tex = 'latex', sty = 'latex', --
		ledger = 'ledger', journal = 'ledger', --
		less = 'less', --
		lily = 'lilypond', ly = 'lilypond', --
		cl = 'lisp', el = 'lisp', lisp = 'lisp', lsp = 'lisp', --
		litcoffee = 'litcoffee', --
		lgt = 'logtalk', --
		lua = 'lua', --
		GNUmakefile = 'makefile', iface = 'makefile', mak = 'makefile', makefile = 'makefile',
		Makefile = 'makefile', mk = 'makefile', --
		md = 'markdown', markdown = 'markdown', --
		['meson.build'] = 'meson', --
		moon = 'moonscript', --
		myr = 'myrddin', --
		n = 'nemerle', --
		link = 'networkd', network = 'networkd', netdev = 'networkd', --
		nim = 'nim', --
		nix = 'nix', --
		nsh = 'nsis', nsi = 'nsis', nsis = 'nsis', --
		obs = 'objeck', --
		m = 'objective_c', mm = 'objective_c', objc = 'objective_c', --
		caml = 'caml', ml = 'caml', mli = 'caml', mll = 'caml', mly = 'caml', --
		org = 'org', --
		dpk = 'pascal', dpr = 'pascal', p = 'pascal', pas = 'pascal', pp = 'pascal', --
		al = 'perl', perl = 'perl', pl = 'perl', PL = 'perl', pm = 'perl', pod = 'perl', --
		inc = 'php', php = 'php', php3 = 'php', php4 = 'php', phtml = 'php', --
		p8 = 'pico8', --
		pike = 'pike', pmod = 'pike', --
		PKGBUILD = 'pkgbuild', --
		pony = 'pony', --
		eps = 'ps', ps = 'ps', --
		ps1 = 'powershell', psm1 = 'powershell', --
		prolog = 'prolog', --
		props = 'props', properties = 'props', --
		proto = 'protobuf', --
		pure = 'pure', --
		sc = 'python', py = 'python', pyi = 'python', pyw = 'python', --
		R = 'r', Rout = 'r', Rhistory = 'r', Rt = 'r', ['Rout.save'] = 'r', ['Rout.fail'] = 'r', --
		re = 'reason', --
		r = 'rebol', reb = 'rebol', --
		rst = 'rest', --
		orx = 'rexx', rex = 'rexx', --
		erb = 'rhtml', rhtml = 'rhtml', --
		rsc = 'routeros', --
		spec = 'rpmspec', --
		Rakefile = 'ruby', rake = 'ruby', rb = 'ruby', rbw = 'ruby', --
		rs = 'rust', --
		sass = 'sass', scss = 'sass', --
		scala = 'scala', --
		sch = 'scheme', scm = 'scheme', --
		bash = 'bash', bashrc = 'bash', bash_profile = 'bash', configure = 'bash', csh = 'bash',
		ksh = 'bash', mksh = 'bash', sh = 'bash', zsh = 'bash', --
		changes = 'smalltalk', st = 'smalltalk', sources = 'smalltalk', --
		sml = 'sml', fun = 'sml', sig = 'sml', --
		sno = 'snobol4', SNO = 'snobol4', --
		spin = 'spin', --
		ddl = 'sql', sql = 'sql', --
		swift = 'swift', --
		automount = 'systemd', device = 'systemd', mount = 'systemd', path = 'systemd',
		scope = 'systemd', service = 'systemd', slice = 'systemd', socket = 'systemd', swap = 'systemd',
		target = 'systemd', timer = 'systemd', --
		taskpaper = 'taskpaper', --
		tcl = 'tcl', tk = 'tcl', --
		texi = 'texinfo', --
		['todo.txt'] = 'todotxt', ['Todo.txt'] = 'todotxt', ['done.txt'] = 'todotxt',
		['Done.txt'] = 'todotxt', --
		toml = 'toml', --
		['1'] = 'troff', ['2'] = 'troff', ['3'] = 'troff', ['4'] = 'troff', ['5'] = 'troff',
		['6'] = 'troff', ['7'] = 'troff', ['8'] = 'troff', ['9'] = 'troff', ['1x'] = 'troff',
		['2x'] = 'troff', ['3x'] = 'troff', ['4x'] = 'troff', ['5x'] = 'troff', ['6x'] = 'troff',
		['7x'] = 'troff', ['8x'] = 'troff', ['9x'] = 'troff', --
		t2t = 'txt2tags', --
		ts = 'typescript', --
		vala = 'vala', --
		vcf = 'vcard', vcard = 'vcard', --
		v = 'verilog', ver = 'verilog', --
		vh = 'vhdl', vhd = 'vhdl', vhdl = 'vhdl', --
		bas = 'vb', cls = 'vb', ctl = 'vb', dob = 'vb', dsm = 'vb', dsr = 'vb', frm = 'vb', pag = 'vb',
		vb = 'vb', vba = 'vb', vbs = 'vb', --
		wsf = 'wsf', --
		dtd = 'xml', svg = 'xml', xml = 'xml', xsd = 'xml', xsl = 'xml', xslt = 'xml', xul = 'xml', --
		xs = 'xs', xsin = 'xs', xsrc = 'xs', --
		xtend = 'xtend', --
		yaml = 'yaml', yml = 'yaml', --
		zig = 'zig'
	}
	local patterns = {
		['^#!.+[/ ][gm]?awk'] = 'awk', ['^#!.+[/ ]lua'] = 'lua', ['^#!.+[/ ]octave'] = 'matlab',
		['^#!.+[/ ]perl'] = 'perl', ['^#!.+[/ ]php'] = 'php', ['^#!.+[/ ]python'] = 'python',
		['^#!.+[/ ]ruby'] = 'ruby', ['^#!.+[/ ]bash'] = 'bash', ['^#!.+/m?ksh'] = 'bash',
		['^#!.+/sh'] = 'bash', ['^%s*class%s+%S+%s*<%s*ApplicationController'] = 'rails',
		['^%s*class%s+%S+%s*<%s*ActionController::Base'] = 'rails',
		['^%s*class%s+%S+%s*<%s*ActiveRecord::Base'] = 'rails',
		['^%s*class%s+%S+%s*<%s*ActiveRecord::Migration'] = 'rails', ['^%s*<%?xml%s'] = 'xml',
		['^#cloud%-config'] = 'yaml'
	}

	for patt, name in pairs(M.detect_patterns) do if line:find(patt) then return name end end
	for patt, name in pairs(patterns) do if line:find(patt) then return name end end
	local name, ext = filename:match('[^/\\]+$'), filename:match('[^.]*$')
	local detected = M.detect_extensions[name] or extensions[name] or M.detect_extensions[ext] or
		extensions[ext]
	if detected then return detected end

	-- Strip ignored filename parts and extensions, and try again.
	-- Do not do this first for the sake of performance; this should be a fallback option.
	for _, patt in ipairs(M.ignore_patterns) do filename = filename:gsub(patt, '') end
	ext = filename:match('[^.]*$')
	while M.ignore_extensions[ext] do filename, ext = filename:match('^(.-%.?([^.]*))%.[^.]+$') end
	return M.detect_extensions[ext] or extensions[ext]
end

-- END verbatim copy of lua/lexers/lexer.lua (detect tables + M.detect)

return M
