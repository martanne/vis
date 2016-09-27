-- load standard vis module, providing parts of the Lua API
require('vis')

vis.events.start = function()
	-- Your global configuration options e.g.
	-- vis:command('map! normal j gj')
end

vis.events.win_open = function(win)
	-- enable syntax highlighting for known file types
	vis.filetype_detect(win)

	-- Your per window configuration options e.g.
	-- vis:command('set number')
end
