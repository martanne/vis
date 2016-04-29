-- load standard vis module, providing parts of the Lua API
require('vis')

vis.events.win_open = function(win)
	-- enable syntax highlighting for known file types
	vis.filetype_detect(win)

	-- Your local configuration options e.g.
	-- vis:command('set number')
	-- vis:command('map! normal j gj')
end
