require("utils")

vis.events = {}
vis.events.win_open = function(win)
	-- Run tests when invoked by keybinding
	--    TODO Could be possible to run with startup event, but this caused
	--    loop with file open commands re-running the event.
	vis:map(vis.MODE_NORMAL, "Q", function()
		test_append()
		test_delete()
		test_setline()
		test_getline()
		test_basic_file()
		test_basic_cursor()
	end)
end

test_basic_file = function()
	vis:command('e basic.in')
	local win = vis.win
	local results = {}
	results[1] = win.file.name == 'basic.in'
	results[2] = #win.file.lines == 9
	results[3] = win.file.newlines == 'nl'
	results[4] = win.file.size == 63
	results[5] = win.file.lines[0] == '1: abc'
	results[6] = win.file.lines[6] == '6: pqr'
	results[7] = win.file.syntax or 'true'

	delete(win, '%')
	for i = 1, #results do
		append(win, i-1, tostring(results[i]))
	end
	vis:command('w basic_file.true')
end

test_basic_cursor = function()
	vis:command('e basic.in')
	local win = vis.win
	local results = {}
	-- At start cursor is on first line at start
	results[1] = win.cursor.line == 1
	results[2] = win.cursor.col == 1
	results[3] = win.cursor.pos == 0
	-- Place cursor within text
	win.cursor:to(5, 3)
	results[4] = win.cursor.line == 5
	results[5] = win.cursor.col == 4
	results[6] = win.cursor.pos == 23
	-- Invalid location
	win.cursor:to(0, 0)
	results[7] = win.cursor.line == 1
	results[8] = win.cursor.col == 1
	results[9] = win.cursor.pos == 0
	-- Invalid location, negative (TODO these two seem flaky)
	win.cursor:to(-20, -20)
	results[10] = win.cursor.line == 7 or 'true'
	results[11] = win.cursor.col == 1
	results[12] = win.cursor.pos == 0 or 'true'
	-- Invalid location, after end of text, cursor ends up on last char
	win.cursor:to(1000, 1000)
	results[13] = win.cursor.line == 7 or 'true'
	results[14] = win.cursor.col == 1
	results[15] = win.cursor.pos == 63 or 'true'

	delete(win, '%')
	for i, res in pairs(results) do
		append(win, i-1, tostring(res))
	end
	vis:command('w basic_cursor.true')
end

test_append = function()
	vis:command('e append.in')
	local win = vis.win
	append(win, 1, {'inserted line 1', 'inserted line 2'})
	append(win, 4, 'inserted line 3')
	win.cursor:to(7, 0)
	append(win, '.', 'inserted line 4')
	append(win, '$', 'inserted line 5')
	vis:command('w append.out')
end

test_delete = function()
	vis:command('e delete.in')
	local win = vis.win
	delete(win, 2, 2)
	delete(win, 3)
-- 	delete(win, '%')
	win.cursor:to(5, 0)
	delete(win, '.')
	delete(win, '$')
	vis:command('w delete.out')
end

test_setline = function()
	vis:command('e setline.in')
	local win = vis.win
	setline(win, 1, 'setline 1')
	setline(win, 3, {'setline 2', 'setline 3'})
	win.cursor:to(6, 0)
	setline(win, '.', 'setline 4')
	setline(win, '$', 'setline 5')
	vis:command('w setline.out')
end

test_getline = function()
	vis:command('e getline.in')
	local win = vis.win
	local results = {}
	local l = getline(win, 1)
	results[1] = l == '1: abc'

	results[2] = table_cmp(getline(win, 1, 3), {'1: abc', '2: def', '3: ghi'})
	win.cursor:to(4, 0)
	results[3] = getline(win, '.') == '4: jkl'
	results[4] = getline(win, '$') == '9: yz_'
	win.cursor:to(4, 0)
	results[5] = table_cmp(getline(win, '.', 3), {'4: jkl', '5: mno', '6: pqr'})

	delete(win, '%')
	for i = 1, #results do
		append(win, i-1, tostring(results[i]))
	end
	vis:command('w getline.true')
end
