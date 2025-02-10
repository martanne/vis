-- Copyright 2024-2025 Samuel Marquis. See LICENSE.
-- Nix LPeg lexer.

local lexer = lexer
local P, S = lpeg.P, lpeg.S

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD)))

-- Functions.
lex:add_rule('function', lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, lex:word_match(lexer.CONSTANT_BUILTIN)))

-- Strings.
local str = lexer.range('"', true)
local ml_str = lexer.range("''", false)
lex:add_rule('string', lex:tag(lexer.STRING, str + ml_str))

-- Paths.
local path_char = lexer.alnum + S('_-.+')
local path_seg = ('/' * path_char^1)
local path = P('~')^-1 * path_char^0 * path_seg^1 * P('/')^-1
lex:add_rule('path', lex:tag(lexer.LINK, path))

-- URIs.
local uri_char = lexer.alnum + S("%/?:@&=+$,-_.!~*'")
local uri = lexer.alpha * (lexer.alnum + S('+-.'))^0 * ':' * uri_char^1
lex:add_rule('uri', lex:tag(lexer.LINK, uri))

-- Angle-bracket paths.
local spath = '<' * path_char^1 * path_seg^0 * '>'
lex:add_rule('spath', lex:tag(lexer.LINK, spath))

-- Identifiers.
local id = (lexer.alpha + '_') * (lexer.alnum + S('_-'))^0
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, id))

-- Comments.
local line_comment = lexer.to_eol('#', true)
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
local l_ops = P('&&') + '||' + '->' + '//' + '++'
local s_ops = S('?+-.*/!<>=,;:()[]{}')
lex:add_rule('operator', lex:tag(lexer.OPERATOR, l_ops + s_ops))

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '(', ')')
lex:add_fold_point(lexer.OPERATOR, '[', ']')
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

-- Word lists.
lex:set_word_list(lexer.KEYWORD, {
	'if', 'then', 'else', 'assert', 'with', 'let', 'in', 'rec', 'inherit', 'or', '...'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'builtins', 'true', 'false', 'null', --
	'builtins.currentSystem', 'builtins.currentTime', 'builtins.langVersion', 'builtins.nixPath',
	'builtins.nixVersion', 'builtins.storeDir'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'derivation', 'import', 'abort', 'throw', --
	'builtins.add', 'builtins.all', 'builtins.any', 'builtins.attrNames', 'builtins.attrValues',
	'builtins.baseNameOf', 'builtins.bitAnd', 'builtins.bitOr', 'builtins.bitXor', 'builtins.break',
	'builtins.catAttrs', 'builtins.ceil', 'builtins.compareVersions', 'builtins.concatLists',
	'builtins.concatMap', 'builtins.concatStringsSep', 'builtins.deepSeq', 'builtins.dirOf',
	'builtins.div', 'builtins.elem', 'builtins.elemAt', 'builtins.fetchClosure', 'builtins.fetchGit',
	'builtins.fetchTarball', 'builtins.fetchurl', 'builtins.filter', 'builtins.filterSource',
	'builtins.findFile', 'builtins.flakeRefToString', 'builtins.floor', "builtins.foldl'",
	'builtins.fromJSON', 'builtins.fromTOML', 'builtins.functionArgs', 'builtins.genList',
	'builtins.genericClosure', 'builtins.getAttr', 'builtins.getContext', 'builtins.getEnv',
	'builtins.getFlake', 'builtins.groupBy', 'builtins.hasAttr', 'builtins.hasContext',
	'builtins.hashFile', 'builtins.hashString', 'builtins.head', 'builtins.import',
	'builtins.intersectAttrs', 'builtins.isAttrs', 'builtins.isBool', 'builtins.isFloat',
	'builtins.isFunction', 'builtins.isInt', 'builtins.isList', 'builtins.isNull', 'builtins.isPath',
	'builtins.isString', 'builtins.length', 'builtins.lessThan', 'builtins.listToAttrs',
	'builtins.map', 'builtins.mapAttrs', 'builtins.match', 'builtins.mul', 'builtins.outputOf',
	'builtins.parseDrvName', 'builtins.parseFlakeRef', 'builtins.partition', 'builtins.path',
	'builtins.pathExists', 'builtins.placeholder', 'builtins.readDir', 'builtins.readFile',
	'builtins.readFileType', 'builtins.removeAttrs', 'builtins.replaceStrings', 'builtins.seq',
	'builtins.sort', 'builtins.split', 'builtins.splitVersion', 'builtins.storePath',
	'builtins.stringLength', 'builtins.sub', 'builtins.substring', 'builtins.tail', 'builtins.throw',
	'builtins.toFile', 'builtins.toJSON', 'builtins.toPath', 'builtins.toString', 'builtins.toXML',
	'builtins.trace', 'builtins.traceVerbose', 'builtins.tryEval', 'builtins.typeOf',
	'builtins.zipAttrsWith'
})

lexer.property['scintillua.comment'] = '#'

return lex
