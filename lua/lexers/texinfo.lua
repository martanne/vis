-- Copyright 2014-2022 stef@ailleurs.land. See LICENSE.
-- Plain Texinfo version 5.2 LPeg lexer
-- Freely inspired from Mitchell work and valuable help from him too !

-- Directives are processed (more or less) in the Reference Card Texinfo order Reference Card
-- page for each directive group is in comment for reference

--[[
Note: Improving Fold Points use with Texinfo

At the very beginning of your Texinfo file, it could be wised to insert theses alias :

@alias startchapter = comment
@alias endchapter = comment

Then use this to begin each chapter :

@endchapter --------------------------------------------------------------------
@chapter CHAPTER TITLE
@startchapter ------------------------------------------------------------------

With the use of Scintilla's `SCI_FOLDALL(SC_FOLDACTION_TOGGLE)` or Textadept's
`buffer:fold_all(buffer.FOLDACTION_TOGGLE)`, you have then a nice chapter folding, useful with
large documents.
]]

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('texinfo')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Directives.
local directives_base = word_match({
  'end',
  -- Custom keywords for chapter folding
  'startchapter', 'endchapter',
  -- List and tables (page 2, column 2)
  'itemize', 'enumerate',
  -- Beginning a Texinfo document (page 1, column 1)
  'titlepage', 'copying',
  -- Block environments (page 2, column 1)
  'cartouche',
  -- Block environments > Displays using fixed-width fonts (page 2, column 2)
  'example', 'smallexample',
  -- List and tables (page 2, column 2)
  'multitable',
  -- Floating Displays (page 2, column 3)
  'float', 'listoffloats', 'caption', 'shortcaption', 'image',
  -- Floating Displays > Footnotes (page 2, column 3)
  'footnote', 'footnotestyle',
  -- Conditionally (in)visible text > Output formats (page 3, column 3)
  'ifdocbook', 'ifhtml', 'ifinfo', 'ifplaintext', 'iftex', 'ifxml', 'ifnotdocbook', 'ifnothtml',
  'ifnotplaintext', 'ifnottex', 'ifnotxml', 'ifnotinfo', 'inlinefmt', 'inlinefmtifelse',
  -- Conditionally (in)visible text > Raw formatter text (page 4, column 1)
  'docbook', 'html', 'tex', 'xml', 'inlineraw',
  -- Conditionally (in)visible text > Documents variables (page 4, column 1)
  'set', 'clear', 'value', 'ifset', 'ifclear', 'inlineifset', 'inlineifclear',
  -- Conditionally (in)visible text > Testing for commands (page 4, column 1)
  'ifcommanddefined', 'ifcommandnotdefined', 'end',
  -- Defining new Texinfo commands (page 4, column 1)
  'alias', 'macro', 'unmacro', 'definfounclose',
  -- File inclusion (page 4, column 1)
  'include', 'verbatiminclude',
  -- Formatting and headers footers for TeX (page 4, column 1)
  'allowcodebreaks', 'finalout', 'fonttextsize',
  -- Formatting and headers footers for TeX > paper size (page 4, column 2)
  'smallbook', 'afourpaper', 'afivepaper', 'afourlatex', 'afourwide', 'pagesizes',
  -- Formatting and headers footers for TeX > Page headers and footers (page 4, column 2)
  -- not implemented
  -- Document preferences (page 4, column 2)
  -- not implemented
  -- Ending a Texinfo document (page 4, column 2)
  'bye'
}, true)
lex:add_rule('directive', token('directives', ('@end' * lexer.space^1 + '@') * directives_base))
lex:add_style('directives', lexer.styles['function'])

-- Chapters.
local chapters_base = word_match({
  -- Chapter structuring (page 1, column 2)
  'lowersections', 'raisesections', 'part',
  -- Chapter structuring > Numbered, included in contents (page 1, column 2)
  'chapter', 'centerchap',
  -- Chapter structuring > Context-dependent, included in contents (page 1, column 2)
  'section', 'subsection', 'subsubsection',
  -- Chapter structuring > Unumbered, included in contents (page 1, column 2)
  'unnumbered', 'unnumberedsec', 'unnumberedsubsec', 'unnumberedsubsection', 'unnumberedsubsubsec',
  'unnumberedsubsubsection',
  -- Chapter structuring > Letter and numbered, included in contents (page 1, column 2)
  'appendix', 'appendixsec', 'appendixsection', 'appendixsubsec', 'appendixsubsection',
  'appendixsubsubsec', 'appendixsubsubsection',
  -- Chapter structuring > Unumbered, not included in contents, no new page (page 1, column 3)
  'chapheading', 'majorheading', 'heading', 'subheading', 'subsubheading'
}, true)
lex:add_rule('chapter', token('chapters', ('@end' * lexer.space^1 + '@') * chapters_base))
lex:add_style('chapters', lexer.styles.class)

-- Common keywords.
local keyword_base = word_match({
  'end',
  -- Beginning a Texinfo document (page 1, column 1)
  'setfilename', 'settitle', 'insertcopying',
  -- Beginning a Texinfo document > Internationlization (page 1, column 1)
  'documentencoding', 'documentlanguage', 'frenchspacing',
  -- Beginning a Texinfo document > Info directory specification and HTML document description
  -- (page 1, column 1)
  'dircategory', 'direntry', 'documentdescription',
  -- Beginning a Texinfo document > Titre pages (page 1, column 1)
  'shorttitlepage', 'center', 'titlefont', 'title', 'subtitle', 'author',
  -- Beginning a Texinfo document > Tables of contents (page 1, column 2)
  'shortcontents', 'summarycontents', 'contents', 'setcontentsaftertitlepage',
  'setshortcontentsaftertitlepage',
  -- Nodes (page 1, column 2)
  'node', 'top', 'anchor', 'novalidate',
  -- Menus (page 1, column 2)
  'menu', 'detailmenu',
  -- Cross references > Within the Info system (page 1, column 3)
  'xref', 'pxref', 'ref', 'inforef', 'xrefautomaticsectiontitle',
  -- Cross references > Outside of info (page 1, column 3)
  'url', 'cite',
  -- Marking text > Markup for regular text (page 1, column 3)
  'var', 'dfn', 'acronym', 'abbr',
  -- Marking text > Markup for litteral text (page 1, column 3)
  'code', 'file', 'command', 'env', 'option', 'kbd', 'key', 'email', 'indicateurl', 'samp', 'verb',
  -- Marking text > GUI sequences (page 2, column 1)
  'clicksequence', 'click', 'clickstyle', 'arrow',
  -- Marking text > Math (page 2, column 1)
  'math', 'minus', 'geq', 'leq',
  -- Marking text > Explicit font selection (page 2, column 1)
  'sc', 'r', 'i', 'slanted', 'b', 'sansserif', 't',
  -- Block environments (page 2, column 1)
  'noindent', 'indent', 'exdent',
  -- Block environments > Normally filled displays using regular text fonts (page 2, column 1)
  'quotation', 'smallquotation', 'indentedblock', 'smallindentedblock', 'raggedright',
  -- Block environments > Line-for-line displays using regular test fonts (page 2, column 2)
  'format', 'smallformat', 'display', 'smalldisplay', 'flushleft', 'flushright',
  -- Block environments > Displays using fixed-width fonts (page 2, column 2)
  'lisp', 'smalllisp', 'verbatim',
  -- List and tables (page 2, column 2)
  'table', 'ftable', 'vtable', 'tab', 'item', 'itemx', 'headitem', 'headitemfont', 'asis',
  -- Indices (page 2, column 3)
  'cindex', 'findex', 'vindex', 'kindex', 'pindex', 'tindex', 'defcodeindex', 'syncodeindex',
  'synindex', 'printindex',
  -- Insertions within a paragraph > Characters special to Texinfo (page 2, column 3)
  '@', '{', '}', 'backslashcar', 'comma', 'hashcar', ':', '.', '?', '!', 'dmn',
  -- Insertions within a paragraph > Accents (page 3, column 1)
  -- not implemented
  -- Insertions within a paragraph > Non-English characters (page 3, column 1)
  -- not implemented
  -- Insertions within a paragraph > Other text characters an logos (page 3, column 1)
  'bullet', 'dots', 'enddots', 'euro', 'pounds', 'textdegree', 'copyright', 'registeredsymbol',
  'TeX', 'LaTeX', 'today', 'guillemetleft', 'guillementright', 'guillemotleft', 'guillemotright',
  -- Insertions within a paragraph > Glyphs for code examples (page 3, column 2)
  'equiv', 'error', 'expansion', 'point', 'print', 'result',
  -- Making and preventing breaks (page 3, column 2)
  '*', '/', '-', 'hyphenation', 'tie', 'w', 'refill',
  -- Vertical space (page 3, column 2)
  'sp', 'page', 'need', 'group', 'vskip'
  -- Definition commands (page 3, column 2)
  -- not implemented
}, true)
lex:add_rule('keyword', token(lexer.KEYWORD, ('@end' * lexer.space^1 + '@') * keyword_base))

-- Italics
local nested_braces = lexer.range('{', '}', false, false, true)
lex:add_rule('emph', token('emph', '@emph' * nested_braces))
lex:add_style('emph', lexer.styles.string .. {italics = true})

-- Bold
lex:add_rule('strong', token('strong', '@strong' * nested_braces))
lex:add_style('strong', lexer.styles.string .. {bold = true})

-- Identifiers
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
lex:add_rule('string', token(lexer.STRING, nested_braces))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Comments.
local line_comment = lexer.to_eol('@c', true)
-- local line_comment_long = lexer.to_eol('@comment', true)
local block_comment = lexer.range('@ignore', '@end ignore')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Fold points.
lex:add_fold_point('directives', '@titlepage', '@end titlepage')
lex:add_fold_point('directives', '@copying', '@end copying')
lex:add_fold_point('directives', '@ifset', '@end ifset')
lex:add_fold_point('directives', '@tex', '@end tex')
lex:add_fold_point('directives', '@itemize', '@end itemize')
lex:add_fold_point('directives', '@enumerate', '@end enumerate')
lex:add_fold_point('directives', '@multitable', '@end multitable')
lex:add_fold_point('directives', '@example', '@end example')
lex:add_fold_point('directives', '@smallexample', '@end smallexample')
lex:add_fold_point('directives', '@cartouche', '@end cartouche')
lex:add_fold_point('directives', '@startchapter', '@end startchapter')

return lex
