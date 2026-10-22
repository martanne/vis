-- % and %% must fail in an unnamed buffer instead of running
-- the command with an empty file name, i.e. the buffer content
-- has to stay untouched
local win = vis.win
vis:command('new')
vis.win.file:insert(0, 'scratch\n')
vis:command(',| echo "%"')
local unchanged = vis.win.file:content(0, vis.win.file.size) == 'scratch\n'
vis:command('q!')
vis.win = win
win.file:insert(win.file.size, string.format('unnamed unchanged: %s\n', tostring(unchanged)))
