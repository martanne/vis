vis.ftdetect = {}

vis.ftdetect.ignoresuffixes = {
	"~+$", "%.orig$", "%.bak$", "%.old$", "%.new$"
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
		hashbang = { "^/usr/bin/[mng]awk%s+%-f" },
		utility = { "^[mgn]?awk$", "^goawk$" },
		ext = { "%.awk$" },
	},
	bash = {
		utility = { "^[db]ash$", "^sh$","^t?csh$","^zsh$" },
		ext = { "%.bash$", "%.csh$", "%.sh$", "%.zsh$" ,"^APKBUILD$", "%.ebuild$", "^.bashrc$", "^.bash_profile$" },
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
		ext = { "%.diff$", "%.patch$", "%.rej$", "^COMMIT_EDITMSG$" },
		cmd = { "set colorcolumn 72" },
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
		utility = { "^fish$" },
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
--  Gemini lexer is truly not supported anymore, if anybody cares about it
--  uncomment this field
--	gemini = {
--		ext = { "%.gmi" },
--		mime = { "text/gemini" },
--	},
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
		ext = { "%.groovy$", "%.gvy$", "^Jenkinsfile$" },
	},
	gtkrc = {
		ext = { "%.gtkrc$" },
	},
	hare = {
		ext = { "%.ha$" }
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
		ext = { "%.cjs$", "%.js$", "%.jsfl$", "%.mjs$" },
	},
	jq = {
		ext = { "%.jq$" },
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
		ext = { "%.ily$", "%.ly$" },
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
		utility = {"^lua%-?5?%d?$", "^lua%-?5%.%d$" },
		ext = { "%.lua$" },
		mime = { "text/x-lua" },
	},
	makefile = {
		hashbang = {"^#!/usr/bin/make"},
		utility = {"^make$"},
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
	meson = {
		ext = { "^meson%.build$" },
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
		ext = { "^PKGBUILD$", "%.PKGBUILD$" },
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
		utility = { "^python%d?" },
		ext = { "%.sc$", "%.py$", "%.pyw$" },
		mime = { "text/x-python", "text/x-script.python" },
	},
	reason = {
		ext = { "%.re$" },
	},
	rc = {
		utility = {"^rc$"},
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
	routeros = {
		ext = { "%.rsc" },
		detect = function(_, data)
			return data:match("^#.* by RouterOS")
		end
	},
	rpmspec = {
		ext = { "%.spec$" },
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
		ext = { "%.rkt$", "%.sch$", "%.scm$", "%.sld$", "%.sls$", "%.ss$" },
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
		detect = function(_, data)
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
		utility = {"^tclsh$", "^jimsh$" },
		ext = { "%.tcl$", "%.tk$" },
	},
	texinfo = {
		ext = { "%.texi$" },
	},
	text = {
		ext = { "%.txt$" },
		-- Do *not* list mime "text/plain" here, it is covered below,
		-- see 'try text lexer as a last resort'
	},
	toml = {
		ext = { "%.toml$" },
	},
	typescript = {
		ext = { "%.ts$" },
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
		ext = { "%.v$", "%.ver$", "%.sv$" },
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
		ext = {
			"%.dtd$", "%.glif$", "%.plist$", "%.svg$", "%.xml$",
			"%.xsd$", "%.xsl$", "%.xslt$", "%.xul$"
		},
	},
	xtend = {
		ext = {"%.xtend$" },
	},
	yaml = {
		ext = { "%.yaml$", "%.yml$" },
		mime = { "text/x-yaml" },
	},
	zig = {
		ext = { "%.zig$" },
	},
}

vis.events.subscribe(vis.events.WIN_OPEN, function(win)

	local set_filetype = function(syntax, filetype)
		for _, cmd in pairs(filetype.cmd or {}) do
			vis:command(cmd)
		end
		local path = vis.lexers.property['lexer.lpeg.home']:gsub(';', '/?.lua;') .. '/?.lua'
		local lexpath = package.searchpath('lexers/'..syntax, path)
		if lexpath ~= nil then
			win:set_syntax(syntax)
		else
			win:set_syntax(nil)
		end
	end

	local path = win.file.name -- filepath
	local mime

	if path and #path > 0 then
		local name = path:match("[^/]+$") -- filename
		if name then
			local unchanged
			while #name > 0 and name ~= unchanged do
				unchanged = name
				for _, pattern in ipairs(vis.ftdetect.ignoresuffixes) do
					name = name:gsub(pattern, "")
				end
			end
		end

		if name and #name > 0 then
			-- detect filetype by filename ending with a configured extension
			for lang, ft in pairs(vis.ftdetect.filetypes) do
				for _, pattern in pairs(ft.ext or {}) do
					if name:match(pattern) then
						set_filetype(lang, ft)
						return
					end
				end
			end
		end

		-- run file(1) to determine mime type
		local file = io.popen(string.format("file -bL --mime-type -- '%s'", path:gsub("'", "'\\''")))
		if file then
			mime = file:read('*all')
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

--[[ hashbang check
	hashbangs only have command <SPACE> argument
		if /env, find utility in args
			discard first arg if /-[^S]*S/; and all subsequent /=/
			NOTE: this means you can't have a command with /^-|=/
	return first field, which should be the utility.
	NOTE: long-options unsupported
--]]
		local fullhb, utility = data:match"^#![ \t]*(/+[^/\n]+[^\n]*)"
		if fullhb then
			local i, field = 1, {}
			for m in fullhb:gmatch"%g+" do field[i],i = m,i+1 end
			-- NOTE: executables should not have a space (or =, see below)
			if field[1]:match"/env$" then
				table.remove(field,1)
				-- it is assumed that the first argument are short options, with -S inside
				if string.match(field[1] or "", "^%-[^S-]*S") then -- -S found
					table.remove(field,1)
					-- skip all name=value
					while string.match(field[1] or "","=") do
						table.remove(field,1)
					end
					-- (hopefully) whatever is left in field[1] should be the utility or nil
				end
			end
			utility = string.match(field[1] or "", "[^/]+$") -- remove filepath
		end

		local function searcher(tbl, subject)
			for i, pattern in ipairs(tbl or {}) do
				if string.match(subject, pattern) then
					return true
				end
			end
			return false
		end

		if utility or fullhb then
			for lang, ft in pairs(vis.ftdetect.filetypes) do
				if
					utility and searcher(ft.utility, utility)
					or
					fullhb and searcher(ft.hashbang, fullhb)
				then
					set_filetype(lang, ft)
					return
				end
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

