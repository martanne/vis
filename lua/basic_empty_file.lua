-- Tests vis.win.file.* for a file that doesn't exist
local win = vis.win
local results = {}
results[1] = win.file.name == 'basic_empty_file.in'
results[2] = win.file.newlines == 'nl'
results[3] = win.file.size == 0
results[4] = #win.file.lines == 0
results[5] = win.file.lines[0] == ''
results[6] = win.file.syntax or 'true'

delete(win, '%')
for i = 1, #results do
	append(win, i-1, tostring(results[i]))
end
vis:command('w! basic_empty_file.out')
