-- Copyright 2006-2022 Mitchell. See LICENSE.
-- APDL LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('apdl', {case_insensitive_fold_points = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  '*abbr', '*abb', '*afun', '*afu', '*ask', '*cfclos', '*cfc', '*cfopen', '*cfo', '*cfwrite',
  '*cfw', '*create', '*cre', '*cycle', '*cyc', '*del', '*dim', '*do', '*elseif', '*else', '*enddo',
  '*endif', '*end', '*eval', '*eva', '*exit', '*exi', '*get', '*go', '*if', '*list', '*lis',
  '*mfouri', '*mfo', '*mfun', '*mfu', '*mooney', '*moo', '*moper', '*mop', '*msg', '*repeat',
  '*rep', '*set', '*status', '*sta', '*tread', '*tre', '*ulib', '*uli', '*use', '*vabs', '*vab',
  '*vcol', '*vco', '*vcum', '*vcu', '*vedit', '*ved', '*vfact', '*vfa', '*vfill', '*vfi', '*vfun',
  '*vfu', '*vget', '*vge', '*vitrp', '*vit', '*vlen', '*vle', '*vmask', '*vma', '*voper', '*vop',
  '*vplot', '*vpl', '*vput', '*vpu', '*vread', '*vre', '*vscfun', '*vsc', '*vstat', '*vst',
  '*vwrite', '*vwr', --
  '/anfile', '/anf', '/angle', '/ang', '/annot', '/ann', '/anum', '/anu', '/assign', '/ass',
  '/auto', '/aut', '/aux15', '/aux2', '/aux', '/axlab', '/axl', '/batch', '/bat', '/clabel', '/cla',
  '/clear', '/cle', '/clog', '/clo', '/cmap', '/cma', '/color', '/col', '/com', '/config',
  '/contour', '/con', '/copy', '/cop', '/cplane', '/cpl', '/ctype', '/cty', '/cval', '/cva',
  '/delete', '/del', '/devdisp', '/device', '/dev', '/dist', '/dis', '/dscale', '/dsc', '/dv3d',
  '/dv3', '/edge', '/edg', '/efacet', '/efa', '/eof', '/erase', '/era', '/eshape', '/esh', '/exit',
  '/exi', '/expand', '/exp', '/facet', '/fac', '/fdele', '/fde', '/filname', '/fil', '/focus',
  '/foc', '/format', '/for', '/ftype', '/fty', '/gcmd', '/gcm', '/gcolumn', '/gco', '/gfile',
  '/gfi', '/gformat', '/gfo', '/gline', '/gli', '/gmarker', '/gma', '/golist', '/gol', '/gopr',
  '/gop', '/go', '/graphics', '/gra', '/gresume', '/gre', '/grid', '/gri', '/gropt', '/gro',
  '/grtyp', '/grt', '/gsave', '/gsa', '/gst', '/gthk', '/gth', '/gtype', '/gty', '/header', '/hea',
  '/input', '/inp', '/larc', '/lar', '/light', '/lig', '/line', '/lin', '/lspec', '/lsp',
  '/lsymbol', '/lsy', '/menu', '/men', '/mplib', '/mpl', '/mrep', '/mre', '/mstart', '/mst',
  '/nerr', '/ner', '/noerase', '/noe', '/nolist', '/nol', '/nopr', '/nop', '/normal', '/nor',
  '/number', '/num', '/opt', '/output', '/out', '/page', '/pag', '/pbc', '/pbf', '/pcircle', '/pci',
  '/pcopy', '/pco', '/plopts', '/plo', '/pmacro', '/pma', '/pmeth', '/pme', '/pmore', '/pmo',
  '/pnum', '/pnu', '/polygon', '/pol', '/post26', '/post1', '/pos', '/prep7', '/pre', '/psearch',
  '/pse', '/psf', '/pspec', '/psp', '/pstatus', '/pst', '/psymb', '/psy', '/pwedge', '/pwe',
  '/quit', '/qui', '/ratio', '/rat', '/rename', '/ren', '/replot', '/rep', '/reset', '/res', '/rgb',
  '/runst', '/run', '/seclib', '/sec', '/seg', '/shade', '/sha', '/showdisp', '/show', '/sho',
  '/shrink', '/shr', '/solu', '/sol', '/sscale', '/ssc', '/status', '/sta', '/stitle', '/sti',
  '/syp', '/sys', '/title', '/tit', '/tlabel', '/tla', '/triad', '/tri', '/trlcy', '/trl', '/tspec',
  '/tsp', '/type', '/typ', '/ucmd', '/ucm', '/uis', '/ui', '/units', '/uni', '/user', '/use',
  '/vcone', '/vco', '/view', '/vie', '/vscale', '/vsc', '/vup', '/wait', '/wai', '/window', '/win',
  '/xrange', '/xra', '/yrange', '/yra', '/zoom', '/zoo'
}, true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, lexer.range("'", true, false)))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, lexer.range('%', true, false)))

-- Labels.
lex:add_rule('label', token(lexer.LABEL, lexer.starts_line(':') * lexer.word))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('!')))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-*/$=,;()')))

-- Fold points.
lex:add_fold_point(lexer.KEYWORD, '*if', '*endif')
lex:add_fold_point(lexer.KEYWORD, '*do', '*enddo')
lex:add_fold_point(lexer.KEYWORD, '*dowhile', '*enddo')
lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('!'))

return lex
