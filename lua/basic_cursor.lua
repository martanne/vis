local win = vis.win
local results = {}
-- At start cursor is on first line at start
results[1] = win.cursor.line == 1
results[2] = win.cursor.col == 1
results[3] = win.cursor.pos == 0
-- Place cursor within text
win.cursor:to(5, 3)
results[4] = win.cursor.line == 5
results[5] = win.cursor.col == 3
results[6] = win.cursor.pos == 30
win.cursor:to(8, 1)
results[7] = win.cursor.line == 8
results[8] = win.cursor.col == 1
results[9] = win.cursor.pos == 49
-- Invalid location
win.cursor:to(0, 0)
results[10] = win.cursor.line == 1
results[11] = win.cursor.col == 1
results[12] = win.cursor.pos == 0
-- Invalid location, negative
local ok, msg = pcall(function()
	win.cursor:to(-20, -20)
end)
results[13] = not ok
results[14] = win.cursor.line == 1
results[15] = win.cursor.col == 1
results[16] = win.cursor.pos == 0
-- Invalid location, after end of text, cursor ends up on last char
win.cursor:to(1000, 1000)
results[17] = win.cursor.line == 9 or true
results[18] = win.cursor.col == 1
results[19] = win.cursor.pos == 63 or true

delete(win, '%')
for i, res in pairs(results) do
	append(win, i-1, tostring(res))
end
vis:command('w! basic_cursor.out')
