local win = vis.win
vis:command_register("foo", function(argv, force, win, cursor, range)
	append(win, '$', 'args: ' .. #argv)
	for i,arg in ipairs(argv) do
		append(win, '$', '\t' .. i .. ' ' .. arg)
	end
	append(win, '$', 'bang: ' .. (force and "yes" or "no"))
	append(win, '$', 'name: ' .. win.file.name)
	append(win, '$', 'pos: ' .. cursor.pos)
	append(win, '$', 'range: ' .. (range ~= nil and
				('['..range.start..', '..range.finish..']')
				or "invalid range"))
	append(win, '$', '')
	return true;
end)

vis:command('foo')
vis:command('foo!')
-- TODO Providing a range does not work
vis:command('2,4foo!')
vis:command('%foo')
vis:command('foo one')
vis:command('foo one two')
-- TODO Is there a limit on number of arguments?
vis:command('foo one two three four five six seven eight nine ten')

vis:command('w! basic_command_register.out')
