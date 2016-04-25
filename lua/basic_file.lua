local win = vis.win
local results = {}
results[1] = win.file.name == 'basic_file.in'
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
vis:command('w! basic_file.out')
