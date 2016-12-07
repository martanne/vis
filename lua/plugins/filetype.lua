vis.ftdetect = {}

vis.ftdetect.ignoresuffixes = {
	"~", ".orig", ".bak", ".old", ".new"
}

vis.ftdetect.filetypes = {
	actionscript = {
		ext = { ".as", ".asc" },
	},
	ada = {
		ext = { ".adb", ".ads" },
	},
	ansi_c = {
		ext = { ".c", ".C", ".h" },
	},
	antlr = {
		ext = { ".g", ".g4" },
	},
	apdl = {
		ext = { ".ans", ".inp", ".mac" },
	},
	apl = {
		ext = { ".apl" }
	},
	applescript = {
		ext = { ".applescript" },
	},
	asm = {
		ext = { ".asm", ".ASM", ".s", ".S" },
	},
	asp = {
		ext = { ".asa", ".asp", ".hta" },
	},
	autoit = {
		ext = { ".au3", ".a3x" },
	},
	awk = {
		ext = { ".awk" },
	},
	bash = {
		ext = { ".bash", ".csh", ".sh", ".zsh" },
		mime = { "text/x-shellscript" },
	},
	batch = {
		ext = { ".bat", ".cmd" },
	},
	bibtex = {
		ext = { ".bib" },
	},
	boo = {
		ext = { ".boo" },
	},
	caml = {
		ext = { ".caml", ".ml", ".mli", ".mll", ".mly" },
	},
	chuck = {
		ext = { ".ck" },
	},
	cmake = {
		ext = { ".cmake", ".cmake.in", ".ctest", ".ctest.in" },
	},
	coffeescript = {
		ext = { ".coffee" },
	},
	cpp = {
		ext = { ".cpp", ".cxx", ".c++", ".cc", ".hh", ".hpp", ".hxx", ".h++" },
	},
	crystal = {
		ext = { ".cr" },
	},
	csharp = {
		ext = { ".cs" },
	},
	css = {
		ext = { ".css" },
	},
	cuda = {
		ext = { ".cu", ".cuh" },
	},
	dart = {
		ext = { ".dart" },
	},
	desktop = {
		ext = { ".desktop" },
	},
	diff = {
		ext = { ".diff", ".patch" },
	},
	dmd = {
		ext = { ".d", ".di" },
	},
	dockerfile = {
		ext = { "Dockerfile" },
	},
	dot = {
		ext = { ".dot" },
	},
	dsv = {
		ext = { "group", "gshadow", "passwd", "shadow" },
	},
	eiffel = {
		ext = { ".e", ".eif" },
	},
	elixir = {
		ext = { ".ex", ".exs" },
	},
	erlang = {
		ext = { ".erl", ".hrl" },
	},
	faust = {
		ext = { ".dsp" },
	},
	fish = {
		ext = { ".fish" },
	},
	forth = {
		ext = { ".forth", ".frt", ".fs" },
	},
	fortran = {
		ext = { ".f", ".for", ".ftn", ".fpp", ".f77", ".f90", ".f95", ".f03", ".f08" },
	},
	fsharp = {
		ext = { ".fs" },
	},
	fstab = {
		ext = { "fstab" },
	},
	gap = {
		ext = { ".g", ".gd", ".gi", ".gap" },
	},
	gettext = {
		ext = { ".po", ".pot" },
	},
	gherkin = {
		ext = { ".feature" },
	},
	glsl = {
		ext = { ".glslf", ".glslv" },
	},
	gnuplot = {
		ext = { ".dem", ".plt" },
	},
	go = {
		ext = { ".go" },
	},
	groovy = {
		ext = { ".groovy", ".gvy" },
	},
	gtkrc = {
		ext = { ".gtkrc" },
	},
	haskell = {
		ext = { ".hs" },
	},
	html = {
		ext = { ".htm", ".html", ".shtm", ".shtml", ".xhtml" },
	},
	icon = {
		ext = { ".icn" },
	},
	idl = {
		ext = { ".idl", ".odl" },
	},
	inform = {
		ext = { ".inf", ".ni" },
	},
	ini = {
		ext = { ".cfg", ".cnf", ".inf", ".ini", ".reg" },
	},
	io_lang = {
		ext = { ".io" },
	},
	java = {
		ext = { ".bsh", ".java" },
	},
	javascript = {
		ext = { ".js", ".jsfl" },
	},
	json = {
		ext = { ".json" },
	},
	jsp = {
		ext = { ".jsp" },
	},
	latex = {
		ext = { ".bbl", ".dtx", ".ins", ".ltx", ".tex", ".sty" },
	},
	ledger = {
		ext = { ".ledger", ".journal" },
	},
	less = {
		ext = { ".less" },
	},
	lilypond = {
		ext = { ".lily", ".ly" },
	},
	lisp = {
		ext = { ".cl", ".el", ".lisp", ".lsp" },
	},
	litcoffee = {
		ext = { ".litcoffee" },
	},
	lua = {
		ext = { ".lua" },
	},
	makefile = {
		ext = { ".iface", ".mak", ".mk", "GNUmakefile", "makefile", "Makefile" },
	},
	man = {
		ext = { ".1", ".2", ".3", ".4", ".5", ".6", ".7", ".8", ".9", ".1x", ".2x", ".3x", ".4x", ".5x", ".6x", ".7x", ".8x", ".9x" },
	},
	markdown = {
		ext = { ".md", ".markdown" },
	},
	moonscript = {
		ext = { ".moon" },
	},
	nemerle = {
		ext = { ".n" },
	},
	networkd = {
		ext = { ".link", ".network", ".netdev" },
	},
	nim = {
		ext = { ".nim" },
	},
	nsis = {
		ext = { ".nsh", ".nsi", ".nsis" },
	},
	objective_c = {
		ext = { ".m", ".mm", ".objc" },
	},
	pascal = {
		ext = { ".dpk", ".dpr", ".p", ".pas" },
	},
	perl = {
		ext = { ".al", ".perl", ".pl", ".pm", ".pod" },
	},
	php = {
		ext = { ".inc", ".php", ".php3", ".php4", ".phtml" },
	},
	pico8 = {
		ext = { ".p8" },
	},
	pike = {
		ext = { ".pike", ".pmod" },
	},
	pkgbuild = {
		ext = { "PKGBUILD" },
	},
	powershell = {
		ext = { ".ps1" },
	},
	prolog = {
		ext = { ".prolog" },
	},
	props = {
		ext = { ".props", ".properties" },
	},
	protobuf = {
		ext = { ".proto" },
	},
	ps = {
		ext = { ".eps", ".ps" },
	},
	pure = {
		ext = { ".pure" },
	},
	python = {
		ext = { ".sc", ".py", ".pyw" },
	},
	rebol = {
		ext = { ".r", ".reb" },
	},
	rest = {
		ext = { ".rst" },
	},
	rexx = {
		ext = { ".orx", ".rex" },
	},
	rhtml = {
		ext = { ".erb", ".rhtml" },
	},
	rstats = {
		ext = { ".R", ".Rout", ".Rhistory", ".Rt", "Rout.save", "Rout.fail" },
	},
	ruby = {
		ext = { ".Rakefile", ".rake", ".rb", ".rbw" },
	},
	rust = {
		ext = { ".rs" },
	},
	sass = {
		ext = { ".sass", ".scss" },
	},
	scala = {
		ext = { ".scala" },
	},
	scheme = {
		ext = { ".sch", ".scm" },
	},
	smalltalk = {
		ext = { ".changes", ".st", ".sources" },
	},
	sml = {
		ext = { ".sml", ".fun", ".sig" },
	},
	snobol4 = {
		ext = { ".sno", ".SNO" },
	},
	sql= {
		ext = { ".ddl", ".sql" },
	},
	systemd = {
		ext = { ".automount", ".device", ".mount", ".path", ".scope", ".service", ".slice", ".socket", ".swap", ".target", ".timer" },
	},
	taskpaper = {
		ext = { ".taskpaper" },
	},
	tcl = {
		ext = { ".tcl", ".tk" },
	},
	texinfo = {
		ext = { ".texi" },
	},
	toml = {
		ext = { ".toml" },
	},
	vala = {
		ext = { ".vala" }
	},
	vb = {
		ext = { ".asa", ".bas", ".cls", ".ctl", ".dob", ".dsm", ".dsr", ".frm", ".pag", ".vb", ".vba", ".vbs" },
	},
	vcard = {
		ext = { ".vcf", ".vcard" },
	},
	verilog = {
		ext = { ".v", ".ver" },
	},
	vhdl = {
		ext = { ".vh", ".vhd", ".vhdl" },
	},
	wsf = {
		ext = { ".wsf" },
	},
	xml = {
		ext = { ".dtd", ".svg", ".xml", ".xsd", ".xsl", ".xslt", ".xul" },
	},
	xtend = {
		ext = {".xtend" },
	},
	yaml = {
		ext = { ".yaml" },
	},
}

vis.filetype_detect = function(win)
	local name = win.file.name
	-- remove ignored suffixes from filename
	local sanitizedfn = name
	if sanitizedfn ~= nil then
		sanitizedfn = sanitizedfn:gsub('^.*/', '')
		repeat
			local changed = false
			for _, pattern in pairs(vis.ftdetect.ignoresuffixes) do
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

	-- detect filetype by filename ending with a configured extension
	if sanitizedfn ~= nil then
		for lang, ft in pairs(vis.ftdetect.filetypes) do
			for _, pattern in pairs(ft.ext or {}) do
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

	-- run file(1) to determine mime type
	if name ~= nil then
		local file = io.popen(string.format("file -bL --mime-type -- '%s'", name:gsub("'", "'\\''")))
		if file then
			local mime = file:read('*all')
			if mime then
				mime = mime:gsub('%s*$', '')
			end
			if mime and #mime > 0 then
				for lang, ft in pairs(vis.ftdetect.filetypes) do
					for _, ft_mime in pairs(ft.mime or {}) do
						if mime == ft_mime then
							win.syntax = lang
							return
						end
					end
				end
			end
		end
	end

	-- pass first few bytes of file to custom file type detector functions
	local file = win.file
	local data = file:content(0, 256)
	if data and #data > 0 then
		for lang, ft in pairs(vis.ftdetect.filetypes) do
			if type(ft.detect) == 'function' and ft.detect(file, data) then
				win.syntax = lang
				return
			end
		end
	end

	win.syntax = nil
end

