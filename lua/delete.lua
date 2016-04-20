local win = vis.win
delete(win, 2, 2)
delete(win, 3)
-- delete(win, '%')
win.cursor:to(5, 0)
delete(win, '.')
delete(win, '$')
vis:command('w! delete.out')
