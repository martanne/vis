
-- vis can work without lexing and lpeg, so these are optional, but HIGHLY recommended
if pcall(require, 'lpeg') then
	-- lexer.lua expects _G.lpeg or it requires (and errors)
	_G.lpeg = require'lpeg'
	if not pcall(require, 'lexer') then
		vis:message('WARNING: could not find lexer module')
	end
else
	vis:message('WARNING: could not find lpeg module')
end

vis.Noop = function()end

-- everything depends on base
require"vis.base"

vis.ftdetect = require"vis.filetype" -- <base

require"vis.options" -- <base, <filetype -- :set
require"vis.plugins" -- mostly self contained, but also depends on base
require"vis.events" -- <base

vis.statusbar = require"vis.statusbar" -- <base, filetype
