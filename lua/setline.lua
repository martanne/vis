local win = vis.win
setline(win, 1, 'setline 1')
setline(win, 3, {'setline 2', 'setline 3'})
win.cursor:to(6, 0)
setline(win, '.', 'setline 4')
setline(win, '$', 'setline 5')
vis:command('w! setline.out')
