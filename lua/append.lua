local win = vis.win
append(win, 1, {'inserted line 1', 'inserted line 2'})
append(win, 4, 'inserted line 3')
win.cursor:to(7, 0)
append(win, '.', 'inserted line 4')
append(win, '$', 'inserted line 5')
vis:command('w! append.out')
