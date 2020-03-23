vis.ftdetect = {}

vis.ftdetect.ignoresuffixes = {
	"~$", "%.orig$", "%.bak$", "%.old$", "%.new$"
}

vis.ftdetect.filetypes = {
	actionscript = {
		ext = { "%.as$", "%.asc$" },
	},
	ada = {
		ext = { "%.adb$", "%.ads$" },
	},
	ansi_c = {
		ext = { "%.c$", "%.C$", "%.h$" },
		mime = { "text/x-c" },
	},
	antlr = {
		ext = { "%.g$", "%.g4$" },
	},
	apdl = {
		ext = { "%.ans$", "%.inp$", "%.mac$" },
	},
	apl = {
		ext = { "%.apl$" }
	},
	applescript = {
		ext = { "%.applescript$" },
	},
	asm = {
		ext = { "%.asm$", "%.ASM$", "%.s$", "%.S$" },
	},
	asp = {
		ext = { "%.asa$", "%.asp$", "%.hta$" },
	},
	autoit = {
		ext = { "%.au3$", "%.a3x$" },
	},
	awk = {
		ext = { "%.awk$" },
	},
	bash = {
		ext = { "%.bash$", "%.csh$", "%.sh$", "%.zsh$" },
		mime = { "text/x-shellscript", "application/x-shellscript" },
	},
	batch = {
		ext = { "%.bat$", "%.cmd$" },
	},
	bibtex = {
		ext = { "%.bib$" },
	},
	boo = {
		ext = { "%.boo$" },
	},
	caml = {
		ext = { "%.caml$", "%.ml$", "%.mli$", "%.mll$", "%.mly$" },
	},
	chuck = {
		ext = { "%.ck$" },
	},
	clojure = {
		ext = { "%.clj$", "%.cljc$",  "%.cljs$", "%.edn$" }
	},
	cmake = {
		ext = { "%.cmake$", "%.cmake.in$", "%.ctest$", "%.ctest.in$" },
	},
	coffeescript = {
		ext = { "%.coffee$" },
		mime = { "text/x-coffee" },
	},
	cpp = {
		ext = { "%.cpp$", "%.cxx$", "%.c++$", "%.cc$", "%.hh$", "%.hpp$", "%.hxx$", "%.h++$" },
		mime = { "text/x-c++" },
	},
	crontab = {
		ext = { "^crontab.*$" },
		cmd = { "set savemethod inplace" },
	},
	crystal = {
		ext = { "%.cr$" },
	},
	csharp = {
		ext = { "%.cs$" },
	},
	css = {
		ext = { "%.css$" },
		mime = { "text/x-css" },
	},
	cuda = {
		ext = { "%.cu$", "%.cuh$" },
	},
	dart = {
		ext = { "%.dart$" },
	},
	desktop = {
		ext = { "%.desktop$" },
	},
	diff = {
		ext = { "%.diff$", "%.patch$", "%.rej$" },
	},
	dmd = {
		ext = { "%.d$", "%.di$" },
	},
	dockerfile = {
		ext = { "^Dockerfile$", "%.Dockerfile$" },
	},
	dot = {
		ext = { "%.dot$" },
	},
	dsv = {
		ext = { "^group$", "^gshadow$", "^passwd$", "^shadow$" },
	},
	eiffel = {
		ext = { "%.e$", "%.eif$" },
	},
	elixir = {
		ext = { "%.ex$", "%.exs$" },
	},
	elm = {
		ext = { "%.elm$" },
	},
	erlang = {
		ext = { "%.erl$", "%.hrl$" },
	},
	fantom = {
		ext = { "%.fan$" },
	},
	faust = {
		ext = { "%.dsp$" },
	},
	fennel = {
		ext = { "%.fnl$" },
	},
	fish = {
		ext = { "%.fish$" },
	},
	forth = {
		ext = { "%.forth$", "%.frt$", "%.fs$", "%.fth$" },
	},
	fortran = {
		ext = { "%.f$", "%.for$", "%.ftn$", "%.fpp$", "%.f77$", "%.f90$", "%.f95$", "%.f03$", "%.f08$" },
	},
	fsharp = {
		ext = { "%.fs$" },
	},
	fstab = {
		ext = { "fstab" },
	},
	gap = {
		ext = { "%.g$", "%.gd$", "%.gi$", "%.gap$" },
	},
	gettext = {
		ext = { "%.po$", "%.pot$" },
	},
	gherkin = {
		ext = { "%.feature$" },
	},
	['git-commit'] = {
		ext = { "^COMMIT_EDITMSG$" },
		cmd = { "set colorcolumn 72" },
	},
	['git-rebase'] = {
		ext = { "git%-rebase%-todo" },
	},
	glsl = {
		ext = { "%.glslf$", "%.glslv$" },
	},
	gnuplot = {
		ext = { "%.dem$", "%.plt$" },
	},
	go = {
		ext = { "%.go$" },
	},
	groovy = {
		ext = { "%.groovy$", "%.gvy$" },
	},
	gtkrc = {
		ext = { "%.gtkrc$" },
	},
	haskell = {
		ext = { "%.hs$" },
		mime = { "text/x-haskell" },
	},
	html = {
		ext = { "%.htm$", "%.html$", "%.shtm$", "%.shtml$", "%.xhtml$" },
		mime = { "text/x-html" },
	},
	icon = {
		ext = { "%.icn$" },
	},
	idl = {
		ext = { "%.idl$", "%.odl$" },
	},
	inform = {
		ext = { "%.inf$", "%.ni$" },
	},
	ini = {
		ext = { "%.cfg$", "%.cnf$", "%.conf$", "%.inf$", "%.ini$", "%.reg$" },
	},
	io_lang = {
		ext = { "%.io$" },
	},
	java = {
		ext = { "%.bsh$", "%.java$" },
	},
	javascript = {
		ext = { "%.js$", "%.jsfl$" },
	},
	json = {
		ext = { "%.json$" },
		mime = { "text/x-json" },
	},
	jsp = {
		ext = { "%.jsp$" },
	},
	julia = {
		ext = { "%.jl$" },
	},
	latex = {
		ext = { "%.bbl$", "%.cls$", "%.dtx$", "%.ins$", "%.ltx$", "%.tex$", "%.sty$" },
		mime = { "text/x-tex" },
	},
	ledger = {
		ext = { "%.ledger$", "%.journal$" },
	},
	less = {
		ext = { "%.less$" },
	},
	lilypond = {
		ext = { "%.lily$", "%.ly$" },
	},
	lisp = {
		ext = { "%.cl$", "%.el$", "%.lisp$", "%.lsp$" },
		mime = { "text/x-lisp" },
	},
	litcoffee = {
		ext = { "%.litcoffee$" },
	},
	logtalk = {
		ext = { "%.lgt$" },
	},
	lua = {
		ext = { "%.lua$" },
		mime = { "text/x-lua" },
	},
	makefile = {
		ext = { "%.iface$", "%.mak$", "%.mk$", "GNUmakefile", "makefile", "Makefile" },
		mime = { "text/x-makefile" },
	},
	man = {
		ext = {
			"%.1$", "%.2$", "%.3$", "%.4$", "%.5$", "%.6$", "%.7$",
			"%.8$", "%.9$", "%.1x$", "%.2x$", "%.3x$", "%.4x$",
			"%.5x$", "%.6x$", "%.7x$", "%.8x$", "%.9x$"
		},
	},
	markdown = {
		ext = { "%.md$", "%.markdown$" },
		mime = { "text/x-markdown" },
	},
	moonscript = {
		ext = { "%.moon$" },
		mime = { "text/x-moon" },
	},
	myrddin = {
		ext = { "%.myr$" },
	},
	nemerle = {
		ext = { "%.n$" },
	},
	networkd = {
		ext = { "%.link$", "%.network$", "%.netdev$" },
	},
	nim = {
		ext = { "%.nim$" },
	},
	nsis = {
		ext = { "%.nsh$", "%.nsi$", "%.nsis$" },
	},
	objective_c = {
		ext = { "%.m$", "%.mm$", "%.objc$" },
		mime = { "text/x-objc" },
	},
	pascal = {
		ext = { "%.dpk$", "%.dpr$", "%.p$", "%.pas$" },
	},
	perl = {
		ext = { "%.al$", "%.perl$", "%.pl$", "%.pm$", "%.pod$" },
		mime = { "text/x-perl" },
	},
	php = {
		ext = { "%.inc$", "%.php$", "%.php3$", "%.php4$", "%.phtml$" },
	},
	pico8 = {
		ext = { "%.p8$" },
	},
	pike = {
		ext = { "%.pike$", "%.pmod$" },
	},
	pkgbuild = {
		ext = { "^PKGBUILD$" },
	},
	pony = {
		ext = { "%.pony$" },
	},
	powershell = {
		ext = { "%.ps1$" },
	},
	prolog = {
		ext = { "%.pl$", "%.pro$", "%.prolog$" },
	},
	props = {
		ext = { "%.props$", "%.properties$" },
	},
	protobuf = {
		ext = { "%.proto$" },
	},
	ps = {
		ext = { "%.eps$", "%.ps$" },
	},
	pure = {
		ext = { "%.pure$" },
	},
	python = {
		ext = { "%.sc$", "%.py$", "%.pyw$" },
		mime = { "text/x-python" },
	},
	reason = {
		ext = { "%.re$" },
	},
	rc = {
		ext = { "%.rc$", "%.es$" },
	},
	rebol = {
		ext = { "%.r$", "%.reb$" },
	},
	rest = {
		ext = { "%.rst$" },
	},
	rexx = {
		ext = { "%.orx$", "%.rex$" },
	},
	rhtml = {
		ext = { "%.erb$", "%.rhtml$" },
	},
	rstats = {
		ext = { "%.R$", "%.Rout$", "%.Rhistory$", "%.Rt$", "Rout.save", "Rout.fail" },
	},
	ruby = {
		ext = { "%.Rakefile$", "%.rake$", "%.rb$", "%.rbw$", "^Vagrantfile$" },
		mime = { "text/x-ruby" },
	},
	rust = {
		ext = { "%.rs$" },
		mime = { "text/x-rust" },
	},
	sass = {
		ext = { "%.sass$", "%.scss$" },
		mime = { "text/x-sass", "text/x-scss" },
	},
	scala = {
		ext = { "%.scala$" },
		mime = { "text/x-scala" },
	},
	scheme = {
		ext = { "%.sch$", "%.scm$", "%.sld$", "%.sls$", "%.ss$" },
	},
	smalltalk = {
		ext = { "%.changes$", "%.st$", "%.sources$" },
	},
	sml = {
		ext = { "%.sml$", "%.fun$", "%.sig$" },
	},
	snobol4 = {
		ext = { "%.sno$", "%.SNO$" },
	},
	spin = {
		ext = { "%.spin$" }
	},
	sql= {
		ext = { "%.ddl$", "%.sql$" },
	},
	strace = {
		detect = function(file, data)
			return data:match("^execve%(")
		end
	},
	systemd = {
		ext = {
			"%.automount$", "%.device$", "%.mount$", "%.path$",
			"%.scope$", "%.service$", "%.slice$", "%.socket$",
			"%.swap$", "%.target$", "%.timer$"
		},
	},
	taskpaper = {
		ext = { "%.taskpaper$" },
	},
	tcl = {
		ext = { "%.tcl$", "%.tk$" },
	},
	texinfo = {
		ext = { "%.texi$" },
	},
	text = {
		ext = { "%.txt$" },
		mime = { "text/plain" },
	},
	toml = {
		ext = { "%.toml$" },
	},
	vala = {
		ext = { "%.vala$" }
	},
	vb = {
		ext = {
			"%.asa$", "%.bas$", "%.ctl$", "%.dob$",
			"%.dsm$", "%.dsr$", "%.frm$", "%.pag$", "%.vb$",
			"%.vba$", "%.vbs$"
		},
	},
	vcard = {
		ext = { "%.vcf$", "%.vcard$" },
	},
	verilog = {
		ext = { "%.v$", "%.ver$" },
	},
	vhdl = {
		ext = { "%.vh$", "%.vhd$", "%.vhdl$" },
	},
	wsf = {
		ext = { "%.wsf$" },
	},
	xs = {
		ext = { "%.xs$", "^%.xsin$", "^%.xsrc$" },
	},
	xml = {
		ext = { "%.dtd$", "%.plist$", "%.svg$", "%.xml$", "%.xsd$", "%.xsl$", "%.xslt$", "%.xul$" },
	},
	xtend = {
		ext = {"%.xtend$" },
	},
	yaml = {
		ext = { "%.yaml$", "%.yml$" },
		mime = { "text/x-yaml" },
	},
}

vis.events.subscribe(vis.events.WIN_OPEN, function(win)

	local set_filetype = function(syntax, filetype)
		for _, cmd in pairs(filetype.cmd or {}) do
			vis:command(cmd)
		end
		win:set_syntax(syntax)
	end

	local name = win.file.name
	-- remove ignored suffixes from filename
	local sanitizedfn = name
	if sanitizedfn ~= nil then
		sanitizedfn = sanitizedfn:gsub('^.*/', '')
		repeat
			local changed = false
			for _, pattern in pairs(vis.ftdetect.ignoresuffixes) do
				local start = sanitizedfn:find(pattern)
				if start then
					sanitizedfn = sanitizedfn:sub(1, start-1)
					changed = true
				end
			end
		until not changed
	end

	-- detect filetype by filename ending with a configured extension
	if sanitizedfn ~= nil then
		for lang, ft in pairs(vis.ftdetect.filetypes) do
			for _, pattern in pairs(ft.ext or {}) do
				if sanitizedfn:match(pattern) then
					set_filetype(lang, ft)
					return
				end
			end
		end
	end

	-- run file(1) to determine mime type
	if name ~= nil then
		local file = io.popen(string.format("file -bL --mime-type -- '%s'", name:gsub("'", "'\\''")))
		if file then
			local mime = file:read('*all')
			file:close()
			if mime then
				mime = mime:gsub('%s*$', '')
			end
			if mime and #mime > 0 then
				for lang, ft in pairs(vis.ftdetect.filetypes) do
					for _, ft_mime in pairs(ft.mime or {}) do
						if mime == ft_mime then
							set_filetype(lang, ft)
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
				set_filetype(lang, ft)
				return
			end
		end
	end

	-- try text lexer as a last resort
	if (mime or 'text/plain'):match('^text/.+$') then
		set_filetype('text', vis.ftdetect.filetypes.text)
		return
	end

	win:set_syntax(nil)
end)

