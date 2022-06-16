-- Copyright 2006-2022 Mitchell. See LICENSE.

local M = {}
lpeg = require'lpeg'

--[=[ This comment is for LuaDoc.
---
-- Lexes Scintilla documents and source code with Lua and LPeg.
--
-- ### Writing Lua Lexers
--
-- Lexers highlight the syntax of source code. Scintilla (the editing component behind
-- [Textadept][] and [SciTE][]) traditionally uses static, compiled C++ lexers which are
-- notoriously difficult to create and/or extend. On the other hand, Lua makes it easy to to
-- rapidly create new lexers, extend existing ones, and embed lexers within one another. Lua
-- lexers tend to be more readable than C++ lexers too.
--
-- Lexers are Parsing Expression Grammars, or PEGs, composed with the Lua [LPeg library][]. The
-- following table comes from the LPeg documentation and summarizes all you need to know about
-- constructing basic LPeg patterns. This module provides convenience functions for creating
-- and working with other more advanced patterns and concepts.
--
-- Operator | Description
-- -|-
-- `lpeg.P(string)` | Matches `string` literally.
-- `lpeg.P(`_`n`_`)` | Matches exactly _`n`_ number of characters.
-- `lpeg.S(string)` | Matches any character in set `string`.
-- `lpeg.R("`_`xy`_`")`| Matches any character between range `x` and `y`.
-- `patt^`_`n`_ | Matches at least _`n`_ repetitions of `patt`.
-- `patt^-`_`n`_ | Matches at most _`n`_ repetitions of `patt`.
-- `patt1 * patt2` | Matches `patt1` followed by `patt2`.
-- `patt1 + patt2` | Matches `patt1` or `patt2` (ordered choice).
-- `patt1 - patt2` | Matches `patt1` if `patt2` does not also match.
-- `-patt` | Equivalent to `("" - patt)`.
-- `#patt` | Matches `patt` but consumes no input.
--
-- The first part of this document deals with rapidly constructing a simple lexer. The next part
-- deals with more advanced techniques, such as custom coloring and embedding lexers within one
-- another. Following that is a discussion about code folding, or being able to tell Scintilla
-- which code blocks are "foldable" (temporarily hideable from view). After that are instructions
-- on how to use Lua lexers with the aforementioned Textadept and SciTE editors. Finally there
-- are comments on lexer performance and limitations.
--
-- [LPeg library]: http://www.inf.puc-rio.br/~roberto/lpeg/lpeg.html
-- [Textadept]: https://orbitalquark.github.io/textadept
-- [SciTE]: https://scintilla.org/SciTE.html
--
-- ### Lexer Basics
--
-- The *lexers/* directory contains all lexers, including your new one. Before attempting to
-- write one from scratch though, first determine if your programming language is similar to
-- any of the 100+ languages supported. If so, you may be able to copy and modify that lexer,
-- saving some time and effort. The filename of your lexer should be the name of your programming
-- language in lower case followed by a *.lua* extension. For example, a new Lua lexer has the
-- name *lua.lua*.
--
-- Note: Try to refrain from using one-character language names like "c", "d", or "r". For
-- example, Scintillua uses "ansi_c", "dmd", and "rstats", respectively.
--
-- #### New Lexer Template
--
-- There is a *lexers/template.txt* file that contains a simple template for a new lexer. Feel
-- free to use it, replacing the '?'s with the name of your lexer. Consider this snippet from
-- the template:
--
--     -- ? LPeg lexer.
--
--     local lexer = require('lexer')
--     local token, word_match = lexer.token, lexer.word_match
--     local P, S = lpeg.P, lpeg.S
--
--     local lex = lexer.new('?')
--
--     -- Whitespace.
--     local ws = token(lexer.WHITESPACE, lexer.space^1)
--     lex:add_rule('whitespace', ws)
--
--     [...]
--
--     return lex
--
-- The first 3 lines of code simply define often used convenience variables. The fourth and
-- last lines [define](#lexer.new) and return the lexer object Scintilla uses; they are very
-- important and must be part of every lexer. The fifth line defines something called a "token",
-- an essential building block of lexers. You will learn about tokens shortly. The sixth line
-- defines a lexer grammar rule, which you will learn about later, as well as token styles. (Be
-- aware that it is common practice to combine these two lines for short rules.)  Note, however,
-- the `local` prefix in front of variables, which is needed so-as not to affect Lua's global
-- environment. All in all, this is a minimal, working lexer that you can build on.
--
-- #### Tokens
--
-- Take a moment to think about your programming language's structure. What kind of key
-- elements does it have? In the template shown earlier, one predefined element all languages
-- have is whitespace. Your language probably also has elements like comments, strings, and
-- keywords. Lexers refer to these elements as "tokens". Tokens are the fundamental "building
-- blocks" of lexers. Lexers break down source code into tokens for coloring, which results
-- in the syntax highlighting familiar to you. It is up to you how specific your lexer is
-- when it comes to tokens. Perhaps only distinguishing between keywords and identifiers is
-- necessary, or maybe recognizing constants and built-in functions, methods, or libraries is
-- desirable. The Lua lexer, for example, defines 11 tokens: whitespace, keywords, built-in
-- functions, constants, built-in libraries, identifiers, strings, comments, numbers, labels,
-- and operators. Even though constants, built-in functions, and built-in libraries are subsets
-- of identifiers, Lua programmers find it helpful for the lexer to distinguish between them
-- all. It is perfectly acceptable to just recognize keywords and identifiers.
--
-- In a lexer, tokens consist of a token name and an LPeg pattern that matches a sequence of
-- characters recognized as an instance of that token. Create tokens using the [`lexer.token()`]()
-- function. Let us examine the "whitespace" token defined in the template shown earlier:
--
--     local ws = token(lexer.WHITESPACE, lexer.space^1)
--
-- At first glance, the first argument does not appear to be a string name and the second
-- argument does not appear to be an LPeg pattern. Perhaps you expected something like:
--
--     local ws = token('whitespace', S('\t\v\f\n\r ')^1)
--
-- The `lexer` module actually provides a convenient list of common token names and common LPeg
-- patterns for you to use. Token names include [`lexer.DEFAULT`](), [`lexer.WHITESPACE`](),
-- [`lexer.COMMENT`](), [`lexer.STRING`](), [`lexer.NUMBER`](), [`lexer.KEYWORD`](),
-- [`lexer.IDENTIFIER`](), [`lexer.OPERATOR`](), [`lexer.ERROR`](), [`lexer.PREPROCESSOR`](),
-- [`lexer.CONSTANT`](), [`lexer.VARIABLE`](), [`lexer.FUNCTION`](), [`lexer.CLASS`](),
-- [`lexer.TYPE`](), [`lexer.LABEL`](), [`lexer.REGEX`](), and [`lexer.EMBEDDED`](). Patterns
-- include [`lexer.any`](), [`lexer.alpha`](), [`lexer.digit`](), [`lexer.alnum`](),
-- [`lexer.lower`](), [`lexer.upper`](), [`lexer.xdigit`](), [`lexer.graph`](), [`lexer.print`](),
-- [`lexer.punct`](), [`lexer.space`](), [`lexer.newline`](), [`lexer.nonnewline`](),
-- [`lexer.dec_num`](), [`lexer.hex_num`](), [`lexer.oct_num`](), [`lexer.integer`](),
-- [`lexer.float`](), [`lexer.number`](), and [`lexer.word`](). You may use your own token names
-- if none of the above fit your language, but an advantage to using predefined token names is
-- that your lexer's tokens will inherit the universal syntax highlighting color theme used by
-- your text editor.
--
-- ##### Example Tokens
--
-- So, how might you define other tokens like keywords, comments, and strings?  Here are some
-- examples.
--
-- **Keywords**
--
-- Instead of matching _n_ keywords with _n_ `P('keyword_`_`n`_`')` ordered choices, use another
-- convenience function: [`lexer.word_match()`](). It is much easier and more efficient to
-- write word matches like:
--
--     local keyword = token(lexer.KEYWORD, lexer.word_match{
--       'keyword_1', 'keyword_2', ..., 'keyword_n'
--     })
--
--     local case_insensitive_keyword = token(lexer.KEYWORD, lexer.word_match({
--       'KEYWORD_1', 'keyword_2', ..., 'KEYword_n'
--     }, true))
--
--     local hyphened_keyword = token(lexer.KEYWORD, lexer.word_match{
--       'keyword-1', 'keyword-2', ..., 'keyword-n'
--     })
--
-- For short keyword lists, you can use a single string of words. For example:
--
--     local keyword = token(lexer.KEYWORD, lexer.word_match('key_1 key_2 ... key_n'))
--
-- **Comments**
--
-- Line-style comments with a prefix character(s) are easy to express with LPeg:
--
--     local shell_comment = token(lexer.COMMENT, lexer.to_eol('#'))
--     local c_line_comment = token(lexer.COMMENT, lexer.to_eol('//', true))
--
-- The comments above start with a '#' or "//" and go to the end of the line. The second comment
-- recognizes the next line also as a comment if the current line ends with a '\' escape character.
--
-- C-style "block" comments with a start and end delimiter are also easy to express:
--
--     local c_comment = token(lexer.COMMENT, lexer.range('/*', '*/'))
--
-- This comment starts with a "/\*" sequence and contains anything up to and including an ending
-- "\*/" sequence. The ending "\*/" is optional so the lexer can recognize unfinished comments
-- as comments and highlight them properly.
--
-- **Strings**
--
-- Most programming languages allow escape sequences in strings such that a sequence like
-- "\\&quot;" in a double-quoted string indicates that the '&quot;' is not the end of the
-- string. [`lexer.range()`]() handles escapes inherently.
--
--     local dq_str = lexer.range('"')
--     local sq_str = lexer.range("'")
--     local string = token(lexer.STRING, dq_str + sq_str)
--
-- In this case, the lexer treats '\' as an escape character in a string sequence.
--
-- **Numbers**
--
-- Most programming languages have the same format for integer and float tokens, so it might
-- be as simple as using a predefined LPeg pattern:
--
--     local number = token(lexer.NUMBER, lexer.number)
--
-- However, some languages allow postfix characters on integers.
--
--     local integer = P('-')^-1 * (lexer.dec_num * S('lL')^-1)
--     local number = token(lexer.NUMBER, lexer.float + lexer.hex_num + integer)
--
-- Your language may need other tweaks, but it is up to you how fine-grained you want your
-- highlighting to be. After all, you are not writing a compiler or interpreter!
--
-- #### Rules
--
-- Programming languages have grammars, which specify valid token structure. For example,
-- comments usually cannot appear within a string. Grammars consist of rules, which are simply
-- combinations of tokens. Recall from the lexer template the [`lexer.add_rule()`]() call,
-- which adds a rule to the lexer's grammar:
--
--     lex:add_rule('whitespace', ws)
--
-- Each rule has an associated name, but rule names are completely arbitrary and serve only to
-- identify and distinguish between different rules. Rule order is important: if text does not
-- match the first rule added to the grammar, the lexer tries to match the second rule added, and
-- so on. Right now this lexer simply matches whitespace tokens under a rule named "whitespace".
--
-- To illustrate the importance of rule order, here is an example of a simplified Lua lexer:
--
--     lex:add_rule('whitespace', token(lexer.WHITESPACE, ...))
--     lex:add_rule('keyword', token(lexer.KEYWORD, ...))
--     lex:add_rule('identifier', token(lexer.IDENTIFIER, ...))
--     lex:add_rule('string', token(lexer.STRING, ...))
--     lex:add_rule('comment', token(lexer.COMMENT, ...))
--     lex:add_rule('number', token(lexer.NUMBER, ...))
--     lex:add_rule('label', token(lexer.LABEL, ...))
--     lex:add_rule('operator', token(lexer.OPERATOR, ...))
--
-- Note how identifiers come after keywords. In Lua, as with most programming languages,
-- the characters allowed in keywords and identifiers are in the same set (alphanumerics
-- plus underscores). If the lexer added the "identifier" rule before the "keyword" rule,
-- all keywords would match identifiers and thus incorrectly highlight as identifiers instead
-- of keywords. The same idea applies to function, constant, etc. tokens that you may want to
-- distinguish between: their rules should come before identifiers.
--
-- So what about text that does not match any rules? For example in Lua, the '!'  character is
-- meaningless outside a string or comment. Normally the lexer skips over such text. If instead
-- you want to highlight these "syntax errors", add an additional end rule:
--
--     lex:add_rule('whitespace', ws)
--     ...
--     lex:add_rule('error', token(lexer.ERROR, lexer.any))
--
-- This identifies and highlights any character not matched by an existing rule as a `lexer.ERROR`
-- token.
--
-- Even though the rules defined in the examples above contain a single token, rules may
-- consist of multiple tokens. For example, a rule for an HTML tag could consist of a tag token
-- followed by an arbitrary number of attribute tokens, allowing the lexer to highlight all
-- tokens separately. That rule might look something like this:
--
--     lex:add_rule('tag', tag_start * (ws * attributes)^0 * tag_end^-1)
--
-- Note however that lexers with complex rules like these are more prone to lose track of their
-- state, especially if they span multiple lines.
--
-- #### Summary
--
-- Lexers primarily consist of tokens and grammar rules. At your disposal are a number of
-- convenience patterns and functions for rapidly creating a lexer. If you choose to use
-- predefined token names for your tokens, you do not have to define how the lexer highlights
-- them. The tokens will inherit the default syntax highlighting color theme your editor uses.
--
-- ### Advanced Techniques
--
-- #### Styles and Styling
--
-- The most basic form of syntax highlighting is assigning different colors to different
-- tokens. Instead of highlighting with just colors, Scintilla allows for more rich highlighting,
-- or "styling", with different fonts, font sizes, font attributes, and foreground and background
-- colors, just to name a few. The unit of this rich highlighting is called a "style". Styles
-- are simply Lua tables of properties. By default, lexers associate predefined token names like
-- `lexer.WHITESPACE`, `lexer.COMMENT`, `lexer.STRING`, etc. with particular styles as part
-- of a universal color theme. These predefined styles are contained in [`lexer.styles`](),
-- and you may define your own styles. See that table's documentation for more information. As
-- with token names, LPeg patterns, and styles, there is a set of predefined color names,
-- but they vary depending on the current color theme in use. Therefore, it is generally not
-- a good idea to manually define colors within styles in your lexer since they might not fit
-- into a user's chosen color theme. Try to refrain from even using predefined colors in a
-- style because that color may be theme-specific. Instead, the best practice is to either use
-- predefined styles or derive new color-agnostic styles from predefined ones. For example, Lua
-- "longstring" tokens use the existing `lexer.styles.string` style instead of defining a new one.
--
-- ##### Example Styles
--
-- Defining styles is pretty straightforward. An empty style that inherits the default theme
-- settings is simply an empty table:
--
--     local style_nothing = {}
--
-- A similar style but with a bold font face looks like this:
--
--     local style_bold = {bold = true}
--
-- You can derive new styles from predefined ones without having to rewrite them. This operation
-- leaves the old style unchanged. For example, if you had a "static variable" token whose
-- style you wanted to base off of `lexer.styles.variable`, it would probably look like:
--
--     local style_static_var = lexer.styles.variable .. {italics = true}
--
-- The color theme files in the *lexers/themes/* folder give more examples of style definitions.
--
-- #### Token Styles
--
-- Lexers use the [`lexer.add_style()`]() function to assign styles to particular tokens. Recall
-- the token definition and from the lexer template:
--
--     local ws = token(lexer.WHITESPACE, lexer.space^1)
--     lex:add_rule('whitespace', ws)
--
-- Why is a style not assigned to the `lexer.WHITESPACE` token? As mentioned earlier, lexers
-- automatically associate tokens that use predefined token names with a particular style. Only
-- tokens with custom token names need manual style associations. As an example, consider a
-- custom whitespace token:
--
--     local ws = token('custom_whitespace', lexer.space^1)
--
-- Assigning a style to this token looks like:
--
--     lex:add_style('custom_whitespace', lexer.styles.whitespace)
--
-- Do not confuse token names with rule names. They are completely different entities. In the
-- example above, the lexer associates the "custom_whitespace" token with the existing style
-- for `lexer.WHITESPACE` tokens. If instead you prefer to color the background of whitespace
-- a shade of grey, it might look like:
--
--     lex:add_style('custom_whitespace', lexer.styles.whitespace .. {back = lexer.colors.grey})
--
-- Remember to refrain from assigning specific colors in styles, but in this case, all user
-- color themes probably define `colors.grey`.
--
-- #### Line Lexers
--
-- By default, lexers match the arbitrary chunks of text passed to them by Scintilla. These
-- chunks may be a full document, only the visible part of a document, or even just portions
-- of lines. Some lexers need to match whole lines. For example, a lexer for the output of a
-- file "diff" needs to know if the line started with a '+' or '-' and then style the entire
-- line accordingly. To indicate that your lexer matches by line, create the lexer with an
-- extra parameter:
--
--     local lex = lexer.new('?', {lex_by_line = true})
--
-- Now the input text for the lexer is a single line at a time. Keep in mind that line lexers
-- do not have the ability to look ahead at subsequent lines.
--
-- #### Embedded Lexers
--
-- Lexers embed within one another very easily, requiring minimal effort. In the following
-- sections, the lexer being embedded is called the "child" lexer and the lexer a child is
-- being embedded in is called the "parent". For example, consider an HTML lexer and a CSS
-- lexer. Either lexer stands alone for styling their respective HTML and CSS files. However, CSS
-- can be embedded inside HTML. In this specific case, the CSS lexer is the "child" lexer with
-- the HTML lexer being the "parent". Now consider an HTML lexer and a PHP lexer. This sounds
-- a lot like the case with CSS, but there is a subtle difference: PHP _embeds itself into_
-- HTML while CSS is _embedded in_ HTML. This fundamental difference results in two types of
-- embedded lexers: a parent lexer that embeds other child lexers in it (like HTML embedding CSS),
-- and a child lexer that embeds itself into a parent lexer (like PHP embedding itself in HTML).
--
-- ##### Parent Lexer
--
-- Before embedding a child lexer into a parent lexer, the parent lexer needs to load the child
-- lexer. This is done with the [`lexer.load()`]() function. For example, loading the CSS lexer
-- within the HTML lexer looks like:
--
--     local css = lexer.load('css')
--
-- The next part of the embedding process is telling the parent lexer when to switch over
-- to the child lexer and when to switch back. The lexer refers to these indications as the
-- "start rule" and "end rule", respectively, and are just LPeg patterns. Continuing with the
-- HTML/CSS example, the transition from HTML to CSS is when the lexer encounters a "style"
-- tag with a "type" attribute whose value is "text/css":
--
--     local css_tag = P('<style') * P(function(input, index)
--       if input:find('^[^>]+type="text/css"', index) then return index end
--     end)
--
-- This pattern looks for the beginning of a "style" tag and searches its attribute list for
-- the text "`type="text/css"`". (In this simplified example, the Lua pattern does not consider
-- whitespace between the '=' nor does it consider that using single quotes is valid.) If there
-- is a match, the functional pattern returns a value instead of `nil`. In this case, the value
-- returned does not matter because we ultimately want to style the "style" tag as an HTML tag,
-- so the actual start rule looks like this:
--
--     local css_start_rule = #css_tag * tag
--
-- Now that the parent knows when to switch to the child, it needs to know when to switch
-- back. In the case of HTML/CSS, the switch back occurs when the lexer encounters an ending
-- "style" tag, though the lexer should still style the tag as an HTML tag:
--
--     local css_end_rule = #P('</style>') * tag
--
-- Once the parent loads the child lexer and defines the child's start and end rules, it embeds
-- the child with the [`lexer.embed()`]() function:
--
--     lex:embed(css, css_start_rule, css_end_rule)
--
-- ##### Child Lexer
--
-- The process for instructing a child lexer to embed itself into a parent is very similar to
-- embedding a child into a parent: first, load the parent lexer into the child lexer with the
-- [`lexer.load()`]() function and then create start and end rules for the child lexer. However,
-- in this case, call [`lexer.embed()`]() with switched arguments. For example, in the PHP lexer:
--
--     local html = lexer.load('html')
--     local php_start_rule = token('php_tag', '<?php ')
--     local php_end_rule = token('php_tag', '?>')
--     lex:add_style('php_tag', lexer.styles.embedded)
--     html:embed(lex, php_start_rule, php_end_rule)
--
-- #### Lexers with Complex State
--
-- A vast majority of lexers are not stateful and can operate on any chunk of text in a
-- document. However, there may be rare cases where a lexer does need to keep track of some
-- sort of persistent state. Rather than using `lpeg.P` function patterns that set state
-- variables, it is recommended to make use of Scintilla's built-in, per-line state integers via
-- [`lexer.line_state`](). It was designed to accommodate up to 32 bit flags for tracking state.
-- [`lexer.line_from_position()`]() will return the line for any position given to an `lpeg.P`
-- function pattern. (Any positions derived from that position argument will also work.)
--
-- Writing stateful lexers is beyond the scope of this document.
--
-- ### Code Folding
--
-- When reading source code, it is occasionally helpful to temporarily hide blocks of code like
-- functions, classes, comments, etc. This is the concept of "folding". In the Textadept and
-- SciTE editors for example, little indicators in the editor margins appear next to code that
-- can be folded at places called "fold points". When the user clicks an indicator, the editor
-- hides the code associated with the indicator until the user clicks the indicator again. The
-- lexer specifies these fold points and what code exactly to fold.
--
-- The fold points for most languages occur on keywords or character sequences. Examples of
-- fold keywords are "if" and "end" in Lua and examples of fold character sequences are '{',
-- '}', "/\*", and "\*/" in C for code block and comment delimiters, respectively. However,
-- these fold points cannot occur just anywhere. For example, lexers should not recognize fold
-- keywords that appear within strings or comments. The [`lexer.add_fold_point()`]() function
-- allows you to conveniently define fold points with such granularity. For example, consider C:
--
--     lex:add_fold_point(lexer.OPERATOR, '{', '}')
--     lex:add_fold_point(lexer.COMMENT, '/*', '*/')
--
-- The first assignment states that any '{' or '}' that the lexer recognized as an `lexer.OPERATOR`
-- token is a fold point. Likewise, the second assignment states that any "/\*" or "\*/" that
-- the lexer recognizes as part of a `lexer.COMMENT` token is a fold point. The lexer does
-- not consider any occurrences of these characters outside their defined tokens (such as in
-- a string) as fold points. How do you specify fold keywords? Here is an example for Lua:
--
--     lex:add_fold_point(lexer.KEYWORD, 'if', 'end')
--     lex:add_fold_point(lexer.KEYWORD, 'do', 'end')
--     lex:add_fold_point(lexer.KEYWORD, 'function', 'end')
--     lex:add_fold_point(lexer.KEYWORD, 'repeat', 'until')
--
-- If your lexer has case-insensitive keywords as fold points, simply add a
-- `case_insensitive_fold_points = true` option to [`lexer.new()`](), and specify keywords in
-- lower case.
--
-- If your lexer needs to do some additional processing in order to determine if a token is
-- a fold point, pass a function that returns an integer to `lex:add_fold_point()`. Returning
-- `1` indicates the token is a beginning fold point and returning `-1` indicates the token is
-- an ending fold point. Returning `0` indicates the token is not a fold point. For example:
--
--     local function fold_strange_token(text, pos, line, s, symbol)
--       if ... then
--         return 1 -- beginning fold point
--       elseif ... then
--         return -1 -- ending fold point
--       end
--       return 0
--     end
--
--     lex:add_fold_point('strange_token', '|', fold_strange_token)
--
-- Any time the lexer encounters a '|' that is a "strange_token", it calls the `fold_strange_token`
-- function to determine if '|' is a fold point. The lexer calls these functions with the
-- following arguments: the text to identify fold points in, the beginning position of the
-- current line in the text to fold, the current line's text, the position in the current line
-- the fold point text starts at, and the fold point text itself.
--
-- #### Fold by Indentation
--
-- Some languages have significant whitespace and/or no delimiters that indicate fold points. If
-- your lexer falls into this category and you would like to mark fold points based on changes
-- in indentation, create the lexer with a `fold_by_indentation = true` option:
--
--     local lex = lexer.new('?', {fold_by_indentation = true})
--
-- ### Using Lexers
--
-- **Textadept**
--
-- Put your lexer in your *~/.textadept/lexers/* directory so you do not overwrite it when
-- upgrading Textadept. Also, lexers in this directory override default lexers. Thus, Textadept
-- loads a user *lua* lexer instead of the default *lua* lexer. This is convenient for tweaking
-- a default lexer to your liking. Then add a [file type](#textadept.file_types) for your lexer
-- if necessary.
--
-- **SciTE**
--
-- Create a *.properties* file for your lexer and `import` it in either your *SciTEUser.properties*
-- or *SciTEGlobal.properties*. The contents of the *.properties* file should contain:
--
--     file.patterns.[lexer_name]=[file_patterns]
--     lexer.$(file.patterns.[lexer_name])=[lexer_name]
--
-- where `[lexer_name]` is the name of your lexer (minus the *.lua* extension) and
-- `[file_patterns]` is a set of file extensions to use your lexer for.
--
-- Please note that Lua lexers ignore any styling information in *.properties* files. Your
-- theme file in the *lexers/themes/* directory contains styling information.
--
-- ### Migrating Legacy Lexers
--
-- Legacy lexers are of the form:
--
--     local l = require('lexer')
--     local token, word_match = l.token, l.word_match
--     local P, R, S = lpeg.P, lpeg.R, lpeg.S
--
--     local M = {_NAME = '?'}
--
--     [... token and pattern definitions ...]
--
--     M._rules = {
--       {'rule', pattern},
--       [...]
--     }
--
--     M._tokenstyles = {
--       'token' = 'style',
--       [...]
--     }
--
--     M._foldsymbols = {
--       _patterns = {...},
--       ['token'] = {['start'] = 1, ['end'] = -1},
--       [...]
--     }
--
--     return M
--
-- While Scintillua will handle such legacy lexers just fine without any changes, it is
-- recommended that you migrate yours. The migration process is fairly straightforward:
--
-- 1. Replace all instances of `l` with `lexer`, as it's better practice and results in less
--    confusion.
-- 2. Replace `local M = {_NAME = '?'}` with `local lex = lexer.new('?')`, where `?` is the
--    name of your legacy lexer. At the end of the lexer, change `return M` to `return lex`.
-- 3. Instead of defining rules towards the end of your lexer, define your rules as you define
--    your tokens and patterns using [`lex:add_rule()`](#lexer.add_rule).
-- 4. Similarly, any custom token names should have their styles immediately defined using
--    [`lex:add_style()`](#lexer.add_style).
-- 5. Optionally convert any table arguments passed to [`lexer.word_match()`]() to a
--    space-separated string of words.
-- 6. Replace any calls to `lexer.embed(M, child, ...)` and `lexer.embed(parent, M, ...)` with
--    [`lex:embed`](#lexer.embed)`(child, ...)` and `parent:embed(lex, ...)`, respectively.
-- 7. Define fold points with simple calls to [`lex:add_fold_point()`](#lexer.add_fold_point). No
--    need to mess with Lua patterns anymore.
-- 8. Any legacy lexer options such as `M._FOLDBYINDENTATION`, `M._LEXBYLINE`, `M._lexer`,
--    etc. should be added as table options to [`lexer.new()`]().
-- 9. Any external lexer rule fetching and/or modifications via `lexer._RULES` should be changed
--    to use [`lexer.get_rule()`]() and [`lexer.modify_rule()`]().
--
-- As an example, consider the following sample legacy lexer:
--
--     local l = require('lexer')
--     local token, word_match = l.token, l.word_match
--     local P, R, S = lpeg.P, lpeg.R, lpeg.S
--
--     local M = {_NAME = 'legacy'}
--
--     local ws = token(l.WHITESPACE, l.space^1)
--     local comment = token(l.COMMENT, '#' * l.nonnewline^0)
--     local string = token(l.STRING, l.delimited_range('"'))
--     local number = token(l.NUMBER, l.float + l.integer)
--     local keyword = token(l.KEYWORD, word_match{'foo', 'bar', 'baz'})
--     local custom = token('custom', P('quux'))
--     local identifier = token(l.IDENTIFIER, l.word)
--     local operator = token(l.OPERATOR, S('+-*/%^=<>,.()[]{}'))
--
--     M._rules = {
--       {'whitespace', ws},
--       {'keyword', keyword},
--       {'custom', custom},
--       {'identifier', identifier},
--       {'string', string},
--       {'comment', comment},
--       {'number', number},
--       {'operator', operator}
--     }
--
--     M._tokenstyles = {
--       'custom' = l.STYLE_KEYWORD .. ',bold'
--     }
--
--     M._foldsymbols = {
--       _patterns = {'[{}]'},
--       [l.OPERATOR] = {['{'] = 1, ['}'] = -1}
--     }
--
--     return M
--
-- Following the migration steps would yield:
--
--     local lexer = require('lexer')
--     local token, word_match = lexer.token, lexer.word_match
--     local P, S = lpeg.P, lpeg.S
--
--     local lex = lexer.new('legacy')
--
--     lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))
--     lex:add_rule('keyword', token(lexer.KEYWORD, word_match('foo bar baz')))
--     lex:add_rule('custom', token('custom', 'quux'))
--     lex:add_style('custom', lexer.styles.keyword .. {bold = true})
--     lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))
--     lex:add_rule('string', token(lexer.STRING, lexer.range('"')))
--     lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('#')))
--     lex:add_rule('number', token(lexer.NUMBER, lexer.number))
--     lex:add_rule('operator', token(lexer.OPERATOR, S('+-*/%^=<>,.()[]{}')))
--
--     lex:add_fold_point(lexer.OPERATOR, '{', '}')
--
--     return lex
--
-- ### Considerations
--
-- #### Performance
--
-- There might be some slight overhead when initializing a lexer, but loading a file from disk
-- into Scintilla is usually more expensive. On modern computer systems, I see no difference in
-- speed between Lua lexers and Scintilla's C++ ones. Optimize lexers for speed by re-arranging
-- `lexer.add_rule()` calls so that the most common rules match first. Do keep in mind that
-- order matters for similar rules.
--
-- In some cases, folding may be far more expensive than lexing, particularly in lexers with a
-- lot of potential fold points. If your lexer is exhibiting signs of slowness, try disabling
-- folding in your text editor first. If that speeds things up, you can try reducing the number
-- of fold points you added, overriding `lexer.fold()` with your own implementation, or simply
-- eliminating folding support from your lexer.
--
-- #### Limitations
--
-- Embedded preprocessor languages like PHP cannot completely embed in their parent languages
-- in that the parent's tokens do not support start and end rules. This mostly goes unnoticed,
-- but code like
--
--     <div id="<?php echo $id; ?>">
--
-- will not style correctly.
--
-- #### Troubleshooting
--
-- Errors in lexers can be tricky to debug. Lexers print Lua errors to `io.stderr` and `_G.print()`
-- statements to `io.stdout`. Running your editor from a terminal is the easiest way to see
-- errors as they occur.
--
-- #### Risks
--
-- Poorly written lexers have the ability to crash Scintilla (and thus its containing application),
-- so unsaved data might be lost. However, I have only observed these crashes in early lexer
-- development, when syntax errors or pattern errors are present. Once the lexer actually starts
-- styling text (either correctly or incorrectly, it does not matter), I have not observed
-- any crashes.
--
-- #### Acknowledgements
--
-- Thanks to Peter Odding for his [lexer post][] on the Lua mailing list that provided inspiration,
-- and thanks to Roberto Ierusalimschy for LPeg.
--
-- [lexer post]: http://lua-users.org/lists/lua-l/2007-04/msg00116.html
-- @field DEFAULT (string)
--   The token name for default tokens.
-- @field WHITESPACE (string)
--   The token name for whitespace tokens.
-- @field COMMENT (string)
--   The token name for comment tokens.
-- @field STRING (string)
--   The token name for string tokens.
-- @field NUMBER (string)
--   The token name for number tokens.
-- @field KEYWORD (string)
--   The token name for keyword tokens.
-- @field IDENTIFIER (string)
--   The token name for identifier tokens.
-- @field OPERATOR (string)
--   The token name for operator tokens.
-- @field ERROR (string)
--   The token name for error tokens.
-- @field PREPROCESSOR (string)
--   The token name for preprocessor tokens.
-- @field CONSTANT (string)
--   The token name for constant tokens.
-- @field VARIABLE (string)
--   The token name for variable tokens.
-- @field FUNCTION (string)
--   The token name for function tokens.
-- @field CLASS (string)
--   The token name for class tokens.
-- @field TYPE (string)
--   The token name for type tokens.
-- @field LABEL (string)
--   The token name for label tokens.
-- @field REGEX (string)
--   The token name for regex tokens.
-- @field any (pattern)
--   A pattern that matches any single character.
-- @field ascii (pattern)
--   A pattern that matches any ASCII character (codes 0 to 127).
-- @field extend (pattern)
--   A pattern that matches any ASCII extended character (codes 0 to 255).
-- @field alpha (pattern)
--   A pattern that matches any alphabetic character ('A'-'Z', 'a'-'z').
-- @field digit (pattern)
--   A pattern that matches any digit ('0'-'9').
-- @field alnum (pattern)
--   A pattern that matches any alphanumeric character ('A'-'Z', 'a'-'z', '0'-'9').
-- @field lower (pattern)
--   A pattern that matches any lower case character ('a'-'z').
-- @field upper (pattern)
--   A pattern that matches any upper case character ('A'-'Z').
-- @field xdigit (pattern)
--   A pattern that matches any hexadecimal digit ('0'-'9', 'A'-'F', 'a'-'f').
-- @field cntrl (pattern)
--   A pattern that matches any control character (ASCII codes 0 to 31).
-- @field graph (pattern)
--   A pattern that matches any graphical character ('!' to '~').
-- @field print (pattern)
--   A pattern that matches any printable character (' ' to '~').
-- @field punct (pattern)
--   A pattern that matches any punctuation character ('!' to '/', ':' to '@', '[' to ''',
--   '{' to '~').
-- @field space (pattern)
--   A pattern that matches any whitespace character ('\t', '\v', '\f', '\n', '\r', space).
-- @field newline (pattern)
--   A pattern that matches a sequence of end of line characters.
-- @field nonnewline (pattern)
--   A pattern that matches any single, non-newline character.
-- @field dec_num (pattern)
--   A pattern that matches a decimal number.
-- @field hex_num (pattern)
--   A pattern that matches a hexadecimal number.
-- @field oct_num (pattern)
--   A pattern that matches an octal number.
-- @field integer (pattern)
--   A pattern that matches either a decimal, hexadecimal, or octal number.
-- @field float (pattern)
--   A pattern that matches a floating point number.
-- @field number (pattern)
--   A pattern that matches a typical number, either a floating point, decimal, hexadecimal,
--   or octal number.
-- @field word (pattern)
--   A pattern that matches a typical word. Words begin with a letter or underscore and consist
--   of alphanumeric and underscore characters.
-- @field FOLD_BASE (number)
--   The initial (root) fold level.
-- @field FOLD_BLANK (number)
--   Flag indicating that the line is blank.
-- @field FOLD_HEADER (number)
--   Flag indicating the line is fold point.
-- @field fold_level (table, Read-only)
--   Table of fold level bit-masks for line numbers starting from 1.
--   Fold level masks are composed of an integer level combined with any of the following bits:
--
--   * `lexer.FOLD_BASE`
--     The initial fold level.
--   * `lexer.FOLD_BLANK`
--     The line is blank.
--   * `lexer.FOLD_HEADER`
--     The line is a header, or fold point.
-- @field indent_amount (table, Read-only)
--   Table of indentation amounts in character columns, for line numbers starting from 1.
-- @field line_state (table)
--   Table of integer line states for line numbers starting from 1.
--   Line states can be used by lexers for keeping track of persistent states.
-- @field property (table)
--   Map of key-value string pairs.
-- @field property_expanded (table, Read-only)
--   Map of key-value string pairs with `$()` and `%()` variable replacement performed in values.
-- @field property_int (table, Read-only)
--   Map of key-value pairs with values interpreted as numbers, or `0` if not found.
-- @field style_at (table, Read-only)
--   Table of style names at positions in the buffer starting from 1.
-- @field folding (boolean)
--   Whether or not folding is enabled for the lexers that support it.
--   This option is disabled by default.
--   This is an alias for `lexer.property['fold'] = '1|0'`.
-- @field fold_on_zero_sum_lines (boolean)
--   Whether or not to mark as a fold point lines that contain both an ending and starting fold
--   point. For example, `} else {` would be marked as a fold point.
--   This option is disabled by default. This is an alias for
--   `lexer.property['fold.on.zero.sum.lines'] = '1|0'`.
-- @field fold_compact (boolean)
--   Whether or not blank lines after an ending fold point are included in that
--   fold.
--   This option is disabled by default.
--   This is an alias for `lexer.property['fold.compact'] = '1|0'`.
-- @field fold_by_indentation (boolean)
--   Whether or not to fold based on indentation level if a lexer does not have
--   a folder.
--   Some lexers automatically enable this option. It is disabled by default.
--   This is an alias for `lexer.property['fold.by.indentation'] = '1|0'`.
-- @field fold_line_groups (boolean)
--   Whether or not to fold multiple, consecutive line groups (such as line comments and import
--   statements) and only show the top line.
--   This option is disabled by default.
--   This is an alias for `lexer.property['fold.line.groups'] = '1|0'`.
module('lexer')]=]

local lpeg = require('lpeg')
local lpeg_P, lpeg_R, lpeg_S, lpeg_V = lpeg.P, lpeg.R, lpeg.S, lpeg.V
local lpeg_Ct, lpeg_Cc, lpeg_Cp = lpeg.Ct, lpeg.Cc, lpeg.Cp
local lpeg_Cmt, lpeg_C = lpeg.Cmt, lpeg.C
local lpeg_match = lpeg.match

---
-- Map of color name strings to color values in `0xBBGGRR` or `"#RRGGBB"` format.
-- Note: for applications running within a terminal emulator, only 16 color values are recognized,
-- regardless of how many colors a user's terminal actually supports. (A terminal emulator's
-- settings determines how to actually display these recognized color values, which may end up
-- being mapped to a completely different color set.) In order to use the light variant of a
-- color, some terminals require a style's `bold` attribute must be set along with that normal
-- color. Recognized color values are black (0x000000), red (0x000080), green (0x008000), yellow
-- (0x008080), blue (0x800000), magenta (0x800080), cyan (0x808000), white (0xC0C0C0), light black
-- (0x404040), light red (0x0000FF), light green (0x00FF00), light yellow (0x00FFFF), light blue
-- (0xFF0000), light magenta (0xFF00FF), light cyan (0xFFFF00), and light white (0xFFFFFF).
-- @name colors
-- @class table
M.colors = setmetatable({}, {
  __index = function(_, name)
    local color = M.property['color.' .. name]
    return tonumber(color) or color
  end, __newindex = function(_, name, color) M.property['color.' .. name] = color end
})

-- A style object that distills into a property string that can be read by the LPeg lexer.
local style_obj = {}
style_obj.__index = style_obj

-- Create a style object from a style name, property table, or legacy style string.
function style_obj.new(name_or_props)
  local prop_string = tostring(name_or_props)
  if type(name_or_props) == 'string' and name_or_props:find('^[%w_]+$') then
    prop_string = string.format('$(style.%s)', name_or_props)
  elseif type(name_or_props) == 'table' then
    local settings = {}
    for k, v in pairs(name_or_props) do
      settings[#settings + 1] = type(v) ~= 'boolean' and string.format('%s:%s', k, v) or
        string.format('%s%s', v and '' or 'not', k)
    end
    prop_string = table.concat(settings, ',')
  end
  return setmetatable({prop_string = prop_string}, style_obj)
end

-- Returns a new style based on this one with the properties defined in the given table or
-- legacy style string.
function style_obj.__concat(self, props)
  if type(props) == 'table' then props = tostring(style_obj.new(props)) end
  return setmetatable({prop_string = string.format('%s,%s', self.prop_string, props)}, style_obj)
end

-- Returns this style object as property string for use with the LPeg lexer.
function style_obj.__tostring(self) return self.prop_string end

---
-- Map of style names to style definition tables.
--
-- Style names consist of the following default names as well as the token names defined by lexers.
--
-- * `default`: The default style all others are based on.
-- * `line_number`: The line number margin style.
-- * `control_char`: The style of control character blocks.
-- * `indent_guide`: The style of indentation guides.
-- * `call_tip`: The style of call tip text. Only the `font`, `size`, `fore`, and `back` style
--   definition fields are supported.
-- * `fold_display_text`: The style of text displayed next to folded lines.
-- * `class`, `comment`, `constant`, `embedded`, `error`, `function`, `identifier`, `keyword`,
--   `label`, `number`, `operator`, `preprocessor`, `regex`, `string`, `type`, `variable`,
--   `whitespace`: Some token names used by lexers. Some lexers may define more token names,
--   so this list is not exhaustive.
-- * *`lang`*`_whitespace`: A special style for whitespace tokens in lexer name *lang*. It
--   inherits from `whitespace`, and is used in place of it for all lexers.
--
-- Style definition tables may contain the following fields:
--
-- * `font`: String font name.
-- * `size`: Integer font size.
-- * `bold`: Whether or not the font face is bold. The default value is `false`.
-- * `weight`: Integer weight or boldness of a font, between 1 and 999.
-- * `italics`: Whether or not the font face is italic. The default value is `false`.
-- * `underlined`: Whether or not the font face is underlined. The default value is `false`.
-- * `fore`: Font face foreground color in `0xBBGGRR` or `"#RRGGBB"` format.
-- * `back`: Font face background color in `0xBBGGRR` or `"#RRGGBB"` format.
-- * `eolfilled`: Whether or not the background color extends to the end of the line. The
--   default value is `false`.
-- * `case`: Font case: `'u'` for upper, `'l'` for lower, and `'m'` for normal, mixed case. The
--   default value is `'m'`.
-- * `visible`: Whether or not the text is visible. The default value is `true`.
-- * `changeable`: Whether the text is changeable instead of read-only. The default value is
--   `true`.
-- @class table
-- @name styles
M.styles = setmetatable({}, {
  __index = function(_, name) return style_obj.new(name) end, __newindex = function(_, name, style)
    if getmetatable(style) ~= style_obj then style = style_obj.new(style) end
    M.property['style.' .. name] = tostring(style)
  end
})

-- Default styles.
local default = {
  'nothing', 'whitespace', 'comment', 'string', 'number', 'keyword', 'identifier', 'operator',
  'error', 'preprocessor', 'constant', 'variable', 'function', 'class', 'type', 'label', 'regex',
  'embedded'
}
for _, name in ipairs(default) do
  M[name:upper()] = name
  M['STYLE_' .. name:upper()] = style_obj.new(name) -- backward compatibility
end
-- Predefined styles.
local predefined = {
  'default', 'line_number', 'brace_light', 'brace_bad', 'control_char', 'indent_guide', 'call_tip',
  'fold_display_text'
}
for _, name in ipairs(predefined) do
  M[name:upper()] = name
  M['STYLE_' .. name:upper()] = style_obj.new(name) -- backward compatibility
end

---
-- Adds pattern *rule* identified by string *id* to the ordered list of rules for lexer *lexer*.
-- @param lexer The lexer to add the given rule to.
-- @param id The id associated with this rule. It does not have to be the same as the name
--   passed to `token()`.
-- @param rule The LPeg pattern of the rule.
-- @see modify_rule
-- @name add_rule
function M.add_rule(lexer, id, rule)
  if lexer._lexer then lexer = lexer._lexer end -- proxy; get true parent
  if not lexer._RULES then
    lexer._RULES = {}
    -- Contains an ordered list (by numerical index) of rule names. This is used in conjunction
    -- with lexer._RULES for building _TOKENRULE.
    lexer._RULEORDER = {}
  end
  lexer._RULES[id] = rule
  lexer._RULEORDER[#lexer._RULEORDER + 1] = id
  lexer:build_grammar()
end

---
-- Replaces in lexer *lexer* the existing rule identified by string *id* with pattern *rule*.
-- @param lexer The lexer to modify.
-- @param id The id associated with this rule.
-- @param rule The LPeg pattern of the rule.
-- @name modify_rule
function M.modify_rule(lexer, id, rule)
  if lexer._lexer then lexer = lexer._lexer end -- proxy; get true parent
  lexer._RULES[id] = rule
  lexer:build_grammar()
end

---
-- Returns the rule identified by string *id*.
-- @param lexer The lexer to fetch a rule from.
-- @param id The id of the rule to fetch.
-- @return pattern
-- @name get_rule
function M.get_rule(lexer, id)
  if lexer._lexer then lexer = lexer._lexer end -- proxy; get true parent
  return lexer._RULES[id]
end

---
-- Associates string *token_name* in lexer *lexer* with style table *style*.
-- *style* may have the following fields:
--
-- * `font`: String font name.
-- * `size`: Integer font size.
-- * `bold`: Whether or not the font face is bold. The default value is `false`.
-- * `weight`: Integer weight or boldness of a font, between 1 and 999.
-- * `italics`: Whether or not the font face is italic. The default value is `false`.
-- * `underlined`: Whether or not the font face is underlined. The default value is `false`.
-- * `fore`: Font face foreground color in `0xBBGGRR` or `"#RRGGBB"` format.
-- * `back`: Font face background color in `0xBBGGRR` or `"#RRGGBB"` format.
-- * `eolfilled`: Whether or not the background color extends to the end of the line. The
--   default value is `false`.
-- * `case`: Font case, `'u'` for upper, `'l'` for lower, and `'m'` for normal, mixed case. The
--   default value is `'m'`.
-- * `visible`: Whether or not the text is visible. The default value is `true`.
-- * `changeable`: Whether the text is changeable instead of read-only. The default value is
--   `true`.
--
-- Field values may also contain "$(property.name)" expansions for properties defined in Scintilla,
-- theme files, etc.
-- @param lexer The lexer to add a style to.
-- @param token_name The name of the token to associated with the style.
-- @param style A style string for Scintilla.
-- @usage lex:add_style('longstring', lexer.styles.string)
-- @usage lex:add_style('deprecated_func', lexer.styles['function'] .. {italics = true}
-- @usage lex:add_style('visible_ws', lexer.styles.whitespace .. {back = lexer.colors.grey}
-- @name add_style
function M.add_style(lexer, token_name, style)
  local num_styles = lexer._numstyles
  if num_styles == 33 then num_styles = num_styles + 8 end -- skip predefined
  if num_styles >= 256 then print('Too many styles defined (256 MAX)') end
  lexer._TOKENSTYLES[token_name], lexer._numstyles = num_styles, num_styles + 1
  if type(style) == 'table' and not getmetatable(style) then style = style_obj.new(style) end
  lexer._EXTRASTYLES[token_name] = tostring(style)
  -- If the lexer is a proxy or a child that embedded itself, copy this style to the parent lexer.
  if lexer._lexer then lexer._lexer:add_style(token_name, style) end
end

---
-- Adds to lexer *lexer* a fold point whose beginning and end tokens are string *token_name*
-- tokens with string content *start_symbol* and *end_symbol*, respectively.
-- In the event that *start_symbol* may or may not be a fold point depending on context, and that
-- additional processing is required, *end_symbol* may be a function that ultimately returns
-- `1` (indicating a beginning fold point), `-1` (indicating an ending fold point), or `0`
-- (indicating no fold point). That function is passed the following arguments:
--
--   * `text`: The text being processed for fold points.
--   * `pos`: The position in *text* of the beginning of the line currently being processed.
--   * `line`: The text of the line currently being processed.
--   * `s`: The position of *start_symbol* in *line*.
--   * `symbol`: *start_symbol* itself.
-- @param lexer The lexer to add a fold point to.
-- @param token_name The token name of text that indicates a fold point.
-- @param start_symbol The text that indicates the beginning of a fold point.
-- @param end_symbol Either the text that indicates the end of a fold point, or a function that
--   returns whether or not *start_symbol* is a beginning fold point (1), an ending fold point
--   (-1), or not a fold point at all (0).
-- @usage lex:add_fold_point(lexer.OPERATOR, '{', '}')
-- @usage lex:add_fold_point(lexer.KEYWORD, 'if', 'end')
-- @usage lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('#'))
-- @usage lex:add_fold_point('custom', function(text, pos, line, s, symbol) ... end)
-- @name add_fold_point
function M.add_fold_point(lexer, token_name, start_symbol, end_symbol)
  if not lexer._FOLDPOINTS then lexer._FOLDPOINTS = {_SYMBOLS = {}} end
  local symbols = lexer._FOLDPOINTS._SYMBOLS
  if not lexer._FOLDPOINTS[token_name] then lexer._FOLDPOINTS[token_name] = {} end
  if lexer._CASEINSENSITIVEFOLDPOINTS then
    start_symbol = start_symbol:lower()
    if type(end_symbol) == 'string' then end_symbol = end_symbol:lower() end
  end
  if type(end_symbol) == 'string' then
    if not symbols[end_symbol] then symbols[#symbols + 1], symbols[end_symbol] = end_symbol, true end
    lexer._FOLDPOINTS[token_name][start_symbol] = 1
    lexer._FOLDPOINTS[token_name][end_symbol] = -1
  else
    lexer._FOLDPOINTS[token_name][start_symbol] = end_symbol -- function or int
  end
  if not symbols[start_symbol] then
    symbols[#symbols + 1], symbols[start_symbol] = start_symbol, true
  end
  -- If the lexer is a proxy or a child that embedded itself, copy this fold point to the
  -- parent lexer.
  if lexer._lexer then lexer._lexer:add_fold_point(token_name, start_symbol, end_symbol) end
end

-- (Re)constructs `lexer._TOKENRULE`.
local function join_tokens(lexer)
  local patterns, order = lexer._RULES, lexer._RULEORDER
  local token_rule = patterns[order[1]]
  for i = 2, #order do token_rule = token_rule + patterns[order[i]] end
  lexer._TOKENRULE = token_rule + M.token(M.DEFAULT, M.any)
  return lexer._TOKENRULE
end

-- Metatable for Scintillua grammars.
-- These grammars are just tables ultimately passed to `lpeg.P()`.
local grammar_mt = {
  __index = {
    -- Adds lexer *lexer* and any of its embedded lexers to this grammar.
    -- @param lexer The lexer to add.
    add_lexer = function(self, lexer)
      local lexer_name = lexer._PARENTNAME or lexer._NAME
      local token_rule = lexer:join_tokens()
      for _, child in ipairs(lexer._CHILDREN) do
        if child._CHILDREN then self:add_lexer(child) end
        local rules = child._EMBEDDEDRULES[lexer_name]
        local rules_token_rule = self['__' .. child._NAME] or rules.token_rule
        self[child._NAME] = (-rules.end_rule * rules_token_rule)^0 * rules.end_rule^-1 *
          lpeg_V(lexer_name)
        local embedded_child = '_' .. child._NAME
        self[embedded_child] = rules.start_rule * (-rules.end_rule * rules_token_rule)^0 *
          rules.end_rule^-1
        token_rule = lpeg_V(embedded_child) + token_rule
      end
      self['__' .. lexer_name] = token_rule -- can contain embedded lexer rules
      self[lexer_name] = token_rule^0
    end
  }
}

-- (Re)constructs `lexer._GRAMMAR`.
-- @param initial_rule The name of the rule to start lexing with. The default value is
--   `lexer._NAME`. Multilang lexers use this to start with a child rule if necessary.
local function build_grammar(lexer, initial_rule)
  if not lexer._RULES then return end
  if lexer._CHILDREN then
    if not initial_rule then initial_rule = lexer._NAME end
    local grammar = setmetatable({initial_rule}, grammar_mt)
    grammar:add_lexer(lexer)
    lexer._INITIALRULE = initial_rule
    lexer._GRAMMAR = lpeg_Ct(lpeg_P(grammar))
  else
    lexer._GRAMMAR = lpeg_Ct(lexer:join_tokens()^0)
  end
end

---
-- Embeds child lexer *child* in parent lexer *lexer* using patterns *start_rule* and *end_rule*,
-- which signal the beginning and end of the embedded lexer, respectively.
-- @param lexer The parent lexer.
-- @param child The child lexer.
-- @param start_rule The pattern that signals the beginning of the embedded lexer.
-- @param end_rule The pattern that signals the end of the embedded lexer.
-- @usage html:embed(css, css_start_rule, css_end_rule)
-- @usage html:embed(lex, php_start_rule, php_end_rule) -- from php lexer
-- @name embed
function M.embed(lexer, child, start_rule, end_rule)
  if lexer._lexer then lexer = lexer._lexer end -- proxy; get true parent
  -- Add child rules.
  if not child._EMBEDDEDRULES then child._EMBEDDEDRULES = {} end
  if not child._RULES then error('Cannot embed lexer with no rules') end
  child._EMBEDDEDRULES[lexer._NAME] = {
    start_rule = start_rule, token_rule = child:join_tokens(), end_rule = end_rule
  }
  if not lexer._CHILDREN then lexer._CHILDREN = {} end
  local children = lexer._CHILDREN
  children[#children + 1] = child
  -- Add child styles.
  for token, style in pairs(child._EXTRASTYLES) do lexer:add_style(token, style) end
  -- Add child fold symbols.
  if child._FOLDPOINTS then
    for token_name, symbols in pairs(child._FOLDPOINTS) do
      if token_name ~= '_SYMBOLS' then
        for symbol, v in pairs(symbols) do lexer:add_fold_point(token_name, symbol, v) end
      end
    end
  end
  lexer:build_grammar()
  child._lexer = lexer -- use parent's tokens if child is embedding itself
end

---
-- Lexes a chunk of text *text* (that has an initial style number of *init_style*) using lexer
-- *lexer*, returning a table of token names and positions.
-- @param lexer The lexer to lex text with.
-- @param text The text in the buffer to lex.
-- @param init_style The current style. Multiple-language lexers use this to determine which
--   language to start lexing in.
-- @return table of token names and positions.
-- @name lex
function M.lex(lexer, text, init_style)
  if not lexer._GRAMMAR then return {M.DEFAULT, #text + 1} end
  if not lexer._LEXBYLINE then
    -- For multilang lexers, build a new grammar whose initial_rule is the current language.
    if lexer._CHILDREN then
      for style, style_num in pairs(lexer._TOKENSTYLES) do
        if style_num == init_style then
          local lexer_name = style:match('^(.+)_whitespace') or lexer._PARENTNAME or lexer._NAME
          if lexer._INITIALRULE ~= lexer_name then lexer:build_grammar(lexer_name) end
          break
        end
      end
    end
    return lpeg_match(lexer._GRAMMAR, text)
  else
    local function append(tokens, line_tokens, offset)
      for i = 1, #line_tokens, 2 do
        tokens[#tokens + 1] = line_tokens[i]
        tokens[#tokens + 1] = line_tokens[i + 1] + offset
      end
    end
    local tokens = {}
    local offset = 0
    local grammar = lexer._GRAMMAR
    for line in text:gmatch('[^\r\n]*\r?\n?') do
      local line_tokens = lpeg_match(grammar, line)
      if line_tokens then append(tokens, line_tokens, offset) end
      offset = offset + #line
      -- Use the default style to the end of the line if none was specified.
      if tokens[#tokens] ~= offset then
        tokens[#tokens + 1], tokens[#tokens + 2] = 'default', offset + 1
      end
    end
    return tokens
  end
end

---
-- Determines fold points in a chunk of text *text* using lexer *lexer*, returning a table of
-- fold levels associated with line numbers.
-- *text* starts at position *start_pos* on line number *start_line* with a beginning fold
-- level of *start_level* in the buffer.
-- @param lexer The lexer to fold text with.
-- @param text The text in the buffer to fold.
-- @param start_pos The position in the buffer *text* starts at, counting from 1.
-- @param start_line The line number *text* starts on, counting from 1.
-- @param start_level The fold level *text* starts on.
-- @return table of fold levels associated with line numbers.
-- @name fold
function M.fold(lexer, text, start_pos, start_line, start_level)
  local folds = {}
  if text == '' then return folds end
  local fold = M.property_int['fold'] > 0
  local FOLD_BASE = M.FOLD_BASE
  local FOLD_HEADER, FOLD_BLANK = M.FOLD_HEADER, M.FOLD_BLANK
  if fold and lexer._FOLDPOINTS then
    local lines = {}
    for p, l in (text .. '\n'):gmatch('()(.-)\r?\n') do lines[#lines + 1] = {p, l} end
    local fold_zero_sum_lines = M.property_int['fold.on.zero.sum.lines'] > 0
    local fold_compact = M.property_int['fold.compact'] > 0
    local fold_points = lexer._FOLDPOINTS
    local fold_point_symbols = fold_points._SYMBOLS
    local style_at, fold_level = M.style_at, M.fold_level
    local line_num, prev_level = start_line, start_level
    local current_level = prev_level
    for _, captures in ipairs(lines) do
      local pos, line = captures[1], captures[2]
      if line ~= '' then
        if lexer._CASEINSENSITIVEFOLDPOINTS then line = line:lower() end
        local ranges = {}
        local function is_valid_range(s, e)
          if not s or not e then return false end
          for i = 1, #ranges - 1, 2 do
            local range_s, range_e = ranges[i], ranges[i + 1]
            if s >= range_s and s <= range_e or e >= range_s and e <= range_e then
              return false
            end
          end
          ranges[#ranges + 1] = s
          ranges[#ranges + 1] = e
          return true
        end
        local level_decreased = false
        for _, symbol in ipairs(fold_point_symbols) do
          local word = not symbol:find('[^%w_]')
          local s, e = line:find(symbol, 1, true)
          while is_valid_range(s, e) do
            -- if not word or line:find('^%f[%w_]' .. symbol .. '%f[^%w_]', s) then
            local word_before = s > 1 and line:find('^[%w_]', s - 1)
            local word_after = line:find('^[%w_]', e + 1)
            if not word or not (word_before or word_after) then
              local symbols = fold_points[style_at[start_pos + pos - 1 + s - 1]]
              local level = symbols and symbols[symbol]
              if type(level) == 'function' then
                level = level(text, pos, line, s, symbol)
              end
              if type(level) == 'number' then
                current_level = current_level + level
                if level < 0 and current_level < prev_level then
                  -- Potential zero-sum line. If the level were to go back up on the same line,
                  -- the line may be marked as a fold header.
                  level_decreased = true
                end
              end
            end
            s, e = line:find(symbol, s + 1, true)
          end
        end
        folds[line_num] = prev_level
        if current_level > prev_level then
          folds[line_num] = prev_level + FOLD_HEADER
        elseif level_decreased and current_level == prev_level and fold_zero_sum_lines then
          if line_num > start_line then
            folds[line_num] = prev_level - 1 + FOLD_HEADER
          else
            -- Typing within a zero-sum line.
            local level = fold_level[line_num] - 1
            if level > FOLD_HEADER then level = level - FOLD_HEADER end
            if level > FOLD_BLANK then level = level - FOLD_BLANK end
            folds[line_num] = level + FOLD_HEADER
            current_level = current_level + 1
          end
        end
        if current_level < FOLD_BASE then current_level = FOLD_BASE end
        prev_level = current_level
      else
        folds[line_num] = prev_level + (fold_compact and FOLD_BLANK or 0)
      end
      line_num = line_num + 1
    end
  elseif fold and (lexer._FOLDBYINDENTATION or M.property_int['fold.by.indentation'] > 0) then
    -- Indentation based folding.
    -- Calculate indentation per line.
    local indentation = {}
    for indent, line in (text .. '\n'):gmatch('([\t ]*)([^\r\n]*)\r?\n') do
      indentation[#indentation + 1] = line ~= '' and #indent
    end
    -- Find the first non-blank line before start_line. If the current line is indented, make
    -- that previous line a header and update the levels of any blank lines inbetween. If the
    -- current line is blank, match the level of the previous non-blank line.
    local current_level = start_level
    for i = start_line, 1, -1 do
      local level = M.fold_level[i]
      if level >= FOLD_HEADER then level = level - FOLD_HEADER end
      if level < FOLD_BLANK then
        local indent = M.indent_amount[i]
        if indentation[1] and indentation[1] > indent then
          folds[i] = FOLD_BASE + indent + FOLD_HEADER
          for j = i + 1, start_line - 1 do folds[j] = start_level + FOLD_BLANK end
        elseif not indentation[1] then
          current_level = FOLD_BASE + indent
        end
        break
      end
    end
    -- Iterate over lines, setting fold numbers and fold flags.
    for i = 1, #indentation do
      if indentation[i] then
        current_level = FOLD_BASE + indentation[i]
        folds[start_line + i - 1] = current_level
        for j = i + 1, #indentation do
          if indentation[j] then
            if FOLD_BASE + indentation[j] > current_level then
              folds[start_line + i - 1] = current_level + FOLD_HEADER
              current_level = FOLD_BASE + indentation[j] -- for any blanks below
            end
            break
          end
        end
      else
        folds[start_line + i - 1] = current_level + FOLD_BLANK
      end
    end
  else
    -- No folding, reset fold levels if necessary.
    local current_line = start_line
    for _ in text:gmatch('\r?\n') do
      folds[current_line] = start_level
      current_line = current_line + 1
    end
  end
  return folds
end

---
-- Creates a returns a new lexer with the given name.
-- @param name The lexer's name.
-- @param opts Table of lexer options. Options currently supported:
--   * `lex_by_line`: Whether or not the lexer only processes whole lines of text (instead of
--     arbitrary chunks of text) at a time. Line lexers cannot look ahead to subsequent lines.
--     The default value is `false`.
--   * `fold_by_indentation`: Whether or not the lexer does not define any fold points and that
--     fold points should be calculated based on changes in line indentation. The default value
--     is `false`.
--   * `case_insensitive_fold_points`: Whether or not fold points added via
--     `lexer.add_fold_point()` ignore case. The default value is `false`.
--   * `inherit`: Lexer to inherit from. The default value is `nil`.
-- @usage lexer.new('rhtml', {inherit = lexer.load('html')})
-- @name new
function M.new(name, opts)
  local lexer = {
    _NAME = assert(name, 'lexer name expected'), _LEXBYLINE = opts and opts['lex_by_line'],
    _FOLDBYINDENTATION = opts and opts['fold_by_indentation'],
    _CASEINSENSITIVEFOLDPOINTS = opts and opts['case_insensitive_fold_points'],
    _lexer = opts and opts['inherit']
  }

  -- Create the initial maps for token names to style numbers and styles.
  local token_styles = {}
  for i = 1, #default do token_styles[default[i]] = i end
  for i = 1, #predefined do token_styles[predefined[i]] = i + 32 end
  lexer._TOKENSTYLES, lexer._numstyles = token_styles, #default + 1
  lexer._EXTRASTYLES = {}

  return setmetatable(lexer, {
    __index = {
      add_rule = M.add_rule, modify_rule = M.modify_rule, get_rule = M.get_rule,
      add_style = M.add_style, add_fold_point = M.add_fold_point, join_tokens = join_tokens,
      build_grammar = build_grammar, embed = M.embed, lex = M.lex, fold = M.fold
    }
  })
end

-- Legacy support for older lexers.
-- Processes the `lex._rules`, `lex._tokenstyles`, and `lex._foldsymbols` tables. Since legacy
-- lexers may be processed up to twice, ensure their default styles and rules are not processed
-- more than once.
local function process_legacy_lexer(lexer)
  local function warn(msg) --[[io.stderr:write(msg, "\n")]]end
  if not lexer._LEGACY then
    lexer._LEGACY = true
    warn("lexers as tables are deprecated; use 'lexer.new()'")
    local token_styles = {}
    for i = 1, #default do token_styles[default[i]] = i end
    for i = 1, #predefined do token_styles[predefined[i]] = i + 32 end
    lexer._TOKENSTYLES, lexer._numstyles = token_styles, #default + 1
    lexer._EXTRASTYLES = {}
    setmetatable(lexer, getmetatable(M.new('')))
    if lexer._rules then
      warn("lexer '_rules' table is deprecated; use 'add_rule()'")
      for _, rule in ipairs(lexer._rules) do lexer:add_rule(rule[1], rule[2]) end
    end
  end
  if lexer._tokenstyles then
    warn("lexer '_tokenstyles' table is deprecated; use 'add_style()'")
    for token, style in pairs(lexer._tokenstyles) do
      -- If this legacy lexer is being processed a second time, only add styles added since
      -- the first processing.
      if not lexer._TOKENSTYLES[token] then lexer:add_style(token, style) end
    end
  end
  if lexer._foldsymbols then
    warn("lexer '_foldsymbols' table is deprecated; use 'add_fold_point()'")
    for token_name, symbols in pairs(lexer._foldsymbols) do
      if type(symbols) == 'table' and token_name ~= '_patterns' then
        for symbol, v in pairs(symbols) do lexer:add_fold_point(token_name, symbol, v) end
      end
    end
    if lexer._foldsymbols._case_insensitive then lexer._CASEINSENSITIVEFOLDPOINTS = true end
  elseif lexer._fold then
    lexer.fold = function(self, ...) return lexer._fold(...) end
  end
end

local lexers = {} -- cache of loaded lexers
---
-- Initializes or loads and returns the lexer of string name *name*.
-- Scintilla calls this function in order to load a lexer. Parent lexers also call this function
-- in order to load child lexers and vice-versa. The user calls this function in order to load
-- a lexer when using Scintillua as a Lua library.
-- @param name The name of the lexing language.
-- @param alt_name The alternate name of the lexing language. This is useful for embedding the
--   same child lexer with multiple sets of start and end tokens.
-- @param cache Flag indicating whether or not to load lexers from the cache. This should only
--   be `true` when initially loading a lexer (e.g. not from within another lexer for embedding
--   purposes). The default value is `false`.
-- @return lexer object
-- @name load

-- finds the vis installed lexer
-- TODO: include ~/.config/vis or ~/.vis
local LEXERPATH1 = package.searchpath("lexers/null", package.path):gsub("/null%.lua$","")

function M.load(name, alt_name, cache)
  if cache and lexers[alt_name or name] then return lexers[alt_name or name] end

  -- When using Scintillua as a stand-alone module, the `property`, `property_int`, and
  -- `property_expanded` tables do not exist (they are not useful). Create them in order prevent
  -- errors from occurring.
  if not M.property then
    M.property = setmetatable( {['lexer.lpeg.home'] = LEXERPATH1 }, {
      __index = function() return '' end,
      __newindex = function(t, k, v) rawset(t, k, tostring(v)) end
    })
    M.property_int = setmetatable({}, {
      __index = function(t, k) return tonumber(M.property[k]) or 0 end,
      __newindex = function() error('read-only property') end
    })
    M.property_expanded = setmetatable({}, {
      __index = function(t, key)
        return M.property[key]:gsub('[$%%](%b())', function(key) return t[key:sub(2, -2)] end)
      end, __newindex = function() error('read-only property') end
    })
  end

  -- Load the language lexer with its rules, styles, etc.
  -- However, replace the default `WHITESPACE` style name with a unique whitespace style name
  -- (and then automatically add it afterwards), since embedded lexing relies on these unique
  -- whitespace style names. Note that loading embedded lexers changes `WHITESPACE` again,
  -- so when adding it later, do not reference the potentially incorrect value.
  M.WHITESPACE = (alt_name or name) .. '_whitespace'
  local lexer = require("lexers/" .. name)
  -- local lexer = dofile(assert( package.searchpath(name, LEXERPATH2) ))
  assert(lexer, string.format("'%s.lua' did not return a lexer", name))
  if alt_name then lexer._NAME = alt_name end
  if not getmetatable(lexer) or lexer._LEGACY then
    -- A legacy lexer may need to be processed a second time in order to pick up any `_tokenstyles`
    -- or `_foldsymbols` added after `lexer.embed_lexer()`.
    process_legacy_lexer(lexer)
    if lexer._lexer and lexer._lexer._LEGACY then
      process_legacy_lexer(lexer._lexer) -- mainly for `_foldsymbols` edits
    end
  end
  lexer:add_style((alt_name or name) .. '_whitespace', M.styles.whitespace)

  -- If the lexer is a proxy or a child that embedded itself, set the parent to be the main
  -- lexer. Keep a reference to the old parent name since embedded child rules reference and
  -- use that name.
  if lexer._lexer then
    lexer = lexer._lexer
    lexer._PARENTNAME, lexer._NAME = lexer._NAME, alt_name or name
  end

  if cache then lexers[alt_name or name] = lexer end
  return lexer
end

-- The following are utility functions lexers will have access to.

-- Common patterns.
M.any = lpeg_P(1)
M.alpha = lpeg_R('AZ', 'az')
M.digit = lpeg_R('09')
M.alnum = lpeg_R('AZ', 'az', '09')
M.lower = lpeg_R('az')
M.upper = lpeg_R('AZ')
M.xdigit = lpeg_R('09', 'AF', 'af')
M.graph = lpeg_R('!~')
M.punct = lpeg_R('!/', ':@', '[\'', '{~')
M.space = lpeg_S('\t\v\f\n\r ')

M.newline = lpeg_P('\r')^-1 * '\n'
M.nonnewline = 1 - M.newline

M.dec_num = M.digit^1
M.hex_num = '0' * lpeg_S('xX') * M.xdigit^1
M.oct_num = '0' * lpeg_R('07')^1
M.integer = lpeg_S('+-')^-1 * (M.hex_num + M.oct_num + M.dec_num)
M.float = lpeg_S('+-')^-1 *
  ((M.digit^0 * '.' * M.digit^1 + M.digit^1 * '.' * M.digit^0 * -lpeg_P('.')) *
    (lpeg_S('eE') * lpeg_S('+-')^-1 * M.digit^1)^-1 +
    (M.digit^1 * lpeg_S('eE') * lpeg_S('+-')^-1 * M.digit^1))
M.number = M.float + M.integer

M.word = (M.alpha + '_') * (M.alnum + '_')^0

-- Deprecated.
M.nonnewline_esc = 1 - (M.newline + '\\') + '\\' * M.any
M.ascii = lpeg_R('\000\127')
M.extend = lpeg_R('\000\255')
M.cntrl = lpeg_R('\000\031')
M.print = lpeg_R(' ~')

---
-- Creates and returns a token pattern with token name *name* and pattern *patt*.
-- If *name* is not a predefined token name, its style must be defined via `lexer.add_style()`.
-- @param name The name of token. If this name is not a predefined token name, then a style
--   needs to be assiciated with it via `lexer.add_style()`.
-- @param patt The LPeg pattern associated with the token.
-- @return pattern
-- @usage local ws = token(lexer.WHITESPACE, lexer.space^1)
-- @usage local annotation = token('annotation', '@' * lexer.word)
-- @name token
function M.token(name, patt)
  return lpeg_Cc(name) * patt * lpeg_Cp()
end

---
-- Creates and returns a pattern that matches from string or pattern *prefix* until the end of
-- the line.
-- *escape* indicates whether the end of the line can be escaped with a '\' character.
-- @param prefix String or pattern prefix to start matching at.
-- @param escape Optional flag indicating whether or not newlines can be escaped by a '\'
--  character. The default value is `false`.
-- @return pattern
-- @usage local line_comment = lexer.to_eol('//')
-- @usage local line_comment = lexer.to_eol(S('#;'))
-- @name to_eol
function M.to_eol(prefix, escape)
  return prefix * (not escape and M.nonnewline or M.nonnewline_esc)^0
end

---
-- Creates and returns a pattern that matches a range of text bounded by strings or patterns *s*
-- and *e*.
-- This is a convenience function for matching more complicated ranges like strings with escape
-- characters, balanced parentheses, and block comments (nested or not). *e* is optional and
-- defaults to *s*. *single_line* indicates whether or not the range must be on a single line;
-- *escapes* indicates whether or not to allow '\' as an escape character; and *balanced*
-- indicates whether or not to handle balanced ranges like parentheses, and requires *s* and *e*
-- to be different.
-- @param s String or pattern start of a range.
-- @param e Optional string or pattern end of a range. The default value is *s*.
-- @param single_line Optional flag indicating whether or not the range must be on a single
--   line. The default value is `false`.
-- @param escapes Optional flag indicating whether or not the range end may be escaped by a '\'
--   character. The default value is `false` unless *s* and *e* are identical, single-character
--   strings. In that case, the default value is `true`.
-- @param balanced Optional flag indicating whether or not to match a balanced range, like the
--   "%b" Lua pattern. This flag only applies if *s* and *e* are different.
-- @return pattern
-- @usage local dq_str_escapes = lexer.range('"')
-- @usage local dq_str_noescapes = lexer.range('"', false, false)
-- @usage local unbalanced_parens = lexer.range('(', ')')
-- @usage local balanced_parens = lexer.range('(', ')', false, false, true)
-- @name range
function M.range(s, e, single_line, escapes, balanced)
  if type(e) ~= 'string' and type(e) ~= 'userdata' then
    e, single_line, escapes, balanced = s, e, single_line, escapes
  end
  local any = M.any - e
  if single_line then any = any - '\n' end
  if balanced then any = any - s end
  if escapes == nil then
    -- Only allow escapes by default for ranges with identical, single-character string delimiters.
    escapes = type(s) == 'string' and #s == 1 and s == e
  end
  if escapes then any = any - '\\' + '\\' * M.any end
  if balanced and s ~= e then
    return lpeg_P{s * (any + lpeg_V(1))^0 * lpeg_P(e)^-1}
  else
    return s * any^0 * lpeg_P(e)^-1
  end
end

-- Deprecated function. Use `lexer.range()` instead.
-- Creates and returns a pattern that matches a range of text bounded by *chars* characters.
-- This is a convenience function for matching more complicated delimited ranges like strings
-- with escape characters and balanced parentheses. *single_line* indicates whether or not the
-- range must be on a single line, *no_escape* indicates whether or not to ignore '\' as an
-- escape character, and *balanced* indicates whether or not to handle balanced ranges like
-- parentheses and requires *chars* to be composed of two characters.
-- @param chars The character(s) that bound the matched range.
-- @param single_line Optional flag indicating whether or not the range must be on a single line.
-- @param no_escape Optional flag indicating whether or not the range end character may be
--   escaped by a '\\' character.
-- @param balanced Optional flag indicating whether or not to match a balanced range, like the
--   "%b" Lua pattern. This flag only applies if *chars* consists of two different characters
--   (e.g. "()").
-- @return pattern
-- @usage local dq_str_escapes = lexer.delimited_range('"')
-- @usage local dq_str_noescapes = lexer.delimited_range('"', false, true)
-- @usage local unbalanced_parens = lexer.delimited_range('()')
-- @usage local balanced_parens = lexer.delimited_range('()', false, false, true)
-- @see range
-- @name delimited_range
function M.delimited_range(chars, single_line, no_escape, balanced)
  print("lexer.delimited_range() is deprecated, use lexer.range()")
  local s = chars:sub(1, 1)
  local e = #chars == 2 and chars:sub(2, 2) or s
  local range
  local b = balanced and s or ''
  local n = single_line and '\n' or ''
  if no_escape then
    local invalid = lpeg_S(e .. n .. b)
    range = M.any - invalid
  else
    local invalid = lpeg_S(e .. n .. b) + '\\'
    range = M.any - invalid + '\\' * M.any
  end
  if balanced and s ~= e then
    return lpeg_P{s * (range + lpeg_V(1))^0 * e}
  else
    return s * range^0 * lpeg_P(e)^-1
  end
end

---
-- Creates and returns a pattern that matches pattern *patt* only at the beginning of a line.
-- @param patt The LPeg pattern to match on the beginning of a line.
-- @return pattern
-- @usage local preproc = token(lexer.PREPROCESSOR, lexer.starts_line(lexer.to_eol('#')))
-- @name starts_line
function M.starts_line(patt)
  return lpeg_Cmt(lpeg_C(patt), function(input, index, match, ...)
    local pos = index - #match
    if pos == 1 then return index, ... end
    local char = input:sub(pos - 1, pos - 1)
    if char == '\n' or char == '\r' or char == '\f' then return index, ... end
  end)
end

---
-- Creates and returns a pattern that verifies the first non-whitespace character behind the
-- current match position is in string set *s*.
-- @param s String character set like one passed to `lpeg.S()`.
-- @return pattern
-- @usage local regex = lexer.last_char_includes('+-*!%^&|=,([{') * lexer.range('/')
-- @name last_char_includes
function M.last_char_includes(s)
  s = string.format('[%s]', s:gsub('[-%%%[]', '%%%1'))
  return lpeg_P(function(input, index)
    if index == 1 then return index end
    local i = index
    while input:sub(i - 1, i - 1):match('[ \t\r\n\f]') do i = i - 1 end
    if input:sub(i - 1, i - 1):match(s) then return index end
  end)
end

-- Deprecated function. Use `lexer.range()` instead.
-- Returns a pattern that matches a balanced range of text that starts with string *start_chars*
-- and ends with string *end_chars*.
-- With single-character delimiters, this function is identical to `delimited_range(start_chars ..
-- end_chars, false, true, true)`.
-- @param start_chars The string starting a nested sequence.
-- @param end_chars The string ending a nested sequence.
-- @return pattern
-- @usage local nested_comment = lexer.nested_pair('/*', '*/')
-- @see range
-- @name nested_pair
function M.nested_pair(start_chars, end_chars)
  print("lexer.nested_pair() is deprecated, use lexer.range()")
  local s, e = start_chars, lpeg_P(end_chars)^-1
  return lpeg_P{s * (M.any - s - end_chars + lpeg_V(1))^0 * e}
end

---
-- Creates and returns a pattern that matches any single word in list or string *words*.
-- *case_insensitive* indicates whether or not to ignore case when matching words.
-- This is a convenience function for simplifying a set of ordered choice word patterns.
-- @param word_list A list of words or a string list of words separated by spaces.
-- @param case_insensitive Optional boolean flag indicating whether or not the word match is
--   case-insensitive. The default value is `false`.
-- @param word_chars Unused legacy parameter.
-- @return pattern
-- @usage local keyword = token(lexer.KEYWORD, word_match{'foo', 'bar', 'baz'})
-- @usage local keyword = token(lexer.KEYWORD, word_match({'foo-bar', 'foo-baz', 'bar-foo',
--   'bar-baz', 'baz-foo', 'baz-bar'}, true))
-- @usage local keyword = token(lexer.KEYWORD, word_match('foo bar baz'))
-- @name word_match
function M.word_match(word_list, case_insensitive, word_chars)
  if type(case_insensitive) == 'string' or type(word_chars) == 'boolean' then
    -- Legacy `word_match(word_list, word_chars, case_insensitive)` form.
    word_chars, case_insensitive = case_insensitive, word_chars
  elseif type(word_list) == 'string' then
    local words = word_list -- space-separated list of words
    word_list = {}
    for word in words:gsub('%-%-[^\n]+', ''):gmatch('%S+') do word_list[#word_list + 1] = word end
  end
  if not word_chars then word_chars = '' end
  for _, word in ipairs(word_list) do
    word_list[case_insensitive and word:lower() or word] = true
    for char in word:gmatch('[^%w_%s]') do
      if not word_chars:find(char, 1, true) then word_chars = word_chars .. char end
    end
  end
  local chars = M.alnum + '_'
  if word_chars ~= '' then chars = chars + lpeg_S(word_chars) end
  return lpeg_Cmt(chars^1, function(input, index, word)
    if case_insensitive then word = word:lower() end
    return word_list[word] and index or nil
  end)
end

-- Deprecated legacy function. Use `parent:embed()` instead.
-- Embeds child lexer *child* in parent lexer *parent* using patterns *start_rule* and *end_rule*,
-- which signal the beginning and end of the embedded lexer, respectively.
-- @param parent The parent lexer.
-- @param child The child lexer.
-- @param start_rule The pattern that signals the beginning of the embedded lexer.
-- @param end_rule The pattern that signals the end of the embedded lexer.
-- @usage lexer.embed_lexer(M, css, css_start_rule, css_end_rule)
-- @usage lexer.embed_lexer(html, M, php_start_rule, php_end_rule)
-- @usage lexer.embed_lexer(html, ruby, ruby_start_rule, ruby_end_rule)
-- @see embed
-- @name embed_lexer
function M.embed_lexer(parent, child, start_rule, end_rule)
  if not getmetatable(parent) then process_legacy_lexer(parent) end
  if not getmetatable(child) then process_legacy_lexer(child) end
  parent:embed(child, start_rule, end_rule)
end

-- Determines if the previous line is a comment.
-- This is used for determining if the current comment line is a fold point.
-- @param prefix The prefix string defining a comment.
-- @param text The text passed to a fold function.
-- @param pos The pos passed to a fold function.
-- @param line The line passed to a fold function.
-- @param s The s passed to a fold function.
local function prev_line_is_comment(prefix, text, pos, line, s)
  local start = line:find('%S')
  if start < s and not line:find(prefix, start, true) then return false end
  local p = pos - 1
  if text:sub(p, p) == '\n' then
    p = p - 1
    if text:sub(p, p) == '\r' then p = p - 1 end
    if text:sub(p, p) ~= '\n' then
      while p > 1 and text:sub(p - 1, p - 1) ~= '\n' do p = p - 1 end
      while text:sub(p, p):find('^[\t ]$') do p = p + 1 end
      return text:sub(p, p + #prefix - 1) == prefix
    end
  end
  return false
end

-- Determines if the next line is a comment.
-- This is used for determining if the current comment line is a fold point.
-- @param prefix The prefix string defining a comment.
-- @param text The text passed to a fold function.
-- @param pos The pos passed to a fold function.
-- @param line The line passed to a fold function.
-- @param s The s passed to a fold function.
local function next_line_is_comment(prefix, text, pos, line, s)
  local p = text:find('\n', pos + s)
  if p then
    p = p + 1
    while text:sub(p, p):find('^[\t ]$') do p = p + 1 end
    return text:sub(p, p + #prefix - 1) == prefix
  end
  return false
end

---
-- Returns for `lexer.add_fold_point()` the parameters needed to fold consecutive lines that
-- start with string *prefix*.
-- @param prefix The prefix string (e.g. a line comment).
-- @usage lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('--'))
-- @usage lex:add_fold_point(lexer.COMMENT, lexer.fold_consecutive_lines('//'))
-- @usage lex:add_fold_point(lexer.KEYWORD, lexer.fold_consecutive_lines('import'))
-- @name fold_consecutive_lines
function M.fold_consecutive_lines(prefix)
  local property_int = M.property_int
  return prefix, function(text, pos, line, s)
    if property_int['fold.line.groups'] == 0 then return 0 end
    if s > 1 and line:match('^%s*()') < s then return 0 end
    local prev_line_comment = prev_line_is_comment(prefix, text, pos, line, s)
    local next_line_comment = next_line_is_comment(prefix, text, pos, line, s)
    if not prev_line_comment and next_line_comment then return 1 end
    if prev_line_comment and not next_line_comment then return -1 end
    return 0
  end
end

-- Deprecated legacy function. Use `lexer.fold_consecutive_lines()` instead.
-- Returns a fold function (to be passed to `lexer.add_fold_point()`) that folds consecutive
-- line comments that start with string *prefix*.
-- @param prefix The prefix string defining a line comment.
-- @usage lex:add_fold_point(lexer.COMMENT, '--', lexer.fold_line_comments('--'))
-- @usage lex:add_fold_point(lexer.COMMENT, '//', lexer.fold_line_comments('//'))
-- @name fold_line_comments
function M.fold_line_comments(prefix)
  print('lexer.fold_line_comments() is deprecated, use lexer.fold_consecutive_lines()')
  return select(2, M.fold_consecutive_lines(prefix))
end

--[[ The functions and fields below were defined in C.

---
-- Returns the line number (starting from 1) of the line that contains position *pos*, which
-- starts from 1.
-- @param pos The position to get the line number of.
-- @return number
local function line_from_position(pos) end
]]

return M
