-- Copyright 2010-2022 Jeff Stone. See LICENSE.
-- Inform LPeg lexer for Scintillua.
-- JMS 2010-04-25.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('inform')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  'Abbreviate', 'Array', 'Attribute', 'Class', 'Constant', 'Default', 'End', 'Endif', 'Extend',
  'Global', 'Ifdef', 'Iffalse', 'Ifndef', 'Ifnot', 'Iftrue', 'Import', 'Include', 'Link',
  'Lowstring', 'Message', 'Object', 'Property', 'Release', 'Replace', 'Serial', 'StartDaemon',
  'Statusline', 'StopDaemon', 'Switches', 'Verb', --
  'absent', 'action', 'actor', 'add_to_scope', 'address', 'additive', 'after', 'and', 'animate',
  'article', 'articles', 'before', 'bold', 'box', 'break', 'cant_go', 'capacity', 'char', 'class',
  'child', 'children', 'clothing', 'concealed', 'container', 'continue', 'creature', 'daemon',
  'deadflag', 'default', 'describe', 'description', 'do', 'door', 'door_dir', 'door_to', 'd_to',
  'd_obj', 'e_to', 'e_obj', 'each_turn', 'edible', 'else', 'enterable', 'false', 'female', 'first',
  'font', 'for', 'found_in', 'general', 'give', 'grammar', 'has', 'hasnt', 'held', 'if', 'in',
  'in_to', 'in_obj', 'initial', 'inside_description', 'invent', 'jump', 'last', 'life', 'light',
  'list_together', 'location', 'lockable', 'locked', 'male', 'move', 'moved', 'multi',
  'multiexcept', 'multiheld', 'multiinside', 'n_to', 'n_obj', 'ne_to', 'ne_obj', 'nw_to', 'nw_obj',
  'name', 'neuter', 'new_line', 'nothing', 'notin', 'noun', 'number', 'objectloop', 'ofclass',
  'off', 'on', 'only', 'open', 'openable', 'or', 'orders', 'out_to', 'out_obj', 'parent',
  'parse_name', 'player', 'plural', 'pluralname', 'print', 'print_ret', 'private', 'proper',
  'provides', 'random', 'react_after', 'react_before', 'remove', 'replace', 'return', 'reverse',
  'rfalseroman', 'rtrue', 's_to', 's_obj', 'se_to', 'se_obj', 'sw_to', 'sw_obj', 'scenery', 'scope',
  'score', 'scored', 'second', 'self', 'short_name', 'short_name_indef', 'sibling', 'spaces',
  'static', 'string', 'style', 'supporter', 'switch', 'switchable', 'talkable', 'thedark',
  'time_left', 'time_out', 'to', 'topic', 'transparent', 'true', 'underline', 'u_to', 'u_obj',
  'visited', 'w_to', 'w_obj', 'when_closed', 'when_off', 'when_on', 'when_open', 'while', 'with',
  'with_key', 'workflag', 'worn'
}))

-- Library actions.
lex:add_rule('action', token('action', word_match{
  'Answer', 'Ask', 'AskFor', 'Attack', 'Blow', 'Burn', 'Buy', 'Climb', 'Close', 'Consult', 'Cut',
  'Dig', 'Disrobe', 'Drink', 'Drop', 'Eat', 'Empty', 'EmptyT', 'Enter', 'Examine', 'Exit', 'Fill',
  'FullScore', 'GetOff', 'Give', 'Go', 'GoIn', 'Insert', 'Inv', 'InvTall', 'InvWide', 'Jump',
  'JumpOver', 'Kiss', 'LetGo', 'Listen', 'LMode1', 'LMode2', 'LMode3', 'Lock', 'Look', 'LookUnder',
  'Mild', 'No', 'NotifyOff', 'NotifyOn', 'Objects', 'Open', 'Order', 'Places', 'Pray', 'Pronouns',
  'Pull', 'Push', 'PushDir', 'PutOn', 'Quit', 'Receive', 'Remove', 'Restart', 'Restore', 'Rub',
  'Save', 'Score', 'ScriptOff', 'ScriptOn', 'Search', 'Set', 'SetTo', 'Show', 'Sing', 'Sleep',
  'Smell', 'Sorry', 'Squeeze', 'Strong', 'Swim', 'Swing', 'SwitchOff', 'SwitchOn', 'Take', 'Taste',
  'Tell', 'Think', 'ThrowAt', 'ThrownAt', 'Tie', 'Touch', 'Transfer', 'Turn', 'Unlock', 'VagueGo',
  'Verify', 'Version', 'Wait', 'Wake', 'WakeOther', 'Wave', 'WaveHands', 'Wear', 'Yes'
}))
lex:add_style('action', lexer.styles.variable)

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.to_eol('!')))

-- Numbers.
local inform_hex = '$' * lexer.xdigit^1
local inform_bin = '$$' * S('01')^1
lex:add_rule('number', token(lexer.NUMBER, lexer.integer + inform_hex + inform_bin))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('@~=+-*/%^#=<>;:,.{}[]()&|?')))

return lex
