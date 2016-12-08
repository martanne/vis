-- load standard vis module, providing parts of the Lua API
require('vis')
require('plugins/filetype')
require('plugins/textobject-lexer')

vis.events.subscribe(vis.events.START, function()
	-- Your global configuration options e.g.
	-- vis:command('map! normal j gj')
end)

vis.events.subscribe(vis.events.WIN_OPEN, function(win)
	-- Your per window configuration options e.g.
	-- vis:command('set number')
end)
