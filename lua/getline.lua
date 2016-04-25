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
vis:command('w getline.out')
