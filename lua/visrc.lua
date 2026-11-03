
-- load standard vis module, providing parts of the Lua API
require"vis"

--[=[
-- Examples

-- You can quickly set or change how a file is identified to its lexer

-- In priority order
vis.ftdetect.extensions.sh = "bash"
vis.ftdetect.filenames.makefile = "makefile"
vis.ftdetect.mimes.plain = "text" -- the mime text/plain is tied to "text" lexer
vis.ftdetect.utilities.python3 = "python" -- #!/bin/python3 is a python file
vis.ftdetect.data_patterns["^#!%s*.-[/ ]awk"] = "awk" -- ["regex"]

-- look at the lexers/ directory for the list of lexers

-- You can also configure the statusbar
table.insert(vis.statusbar.right, 1, function() return vis.theme end)
-- There's also a vis.statusbar.left, See statusbar.lua for more

-- see filetype.lua for a more thorough explanation on how each vis.ftdetect[key] works
--]=]

vis.events.subscribe(vis.events.INIT, function()
	-- Your global configuration options
end)

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	-- Your per window configuration options e.g.
	-- vis:command('set number')
end)
