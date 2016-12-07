-- Inform LPeg lexer for Scintillua.
-- JMS 2010-04-25.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'inform'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, '!' * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local string = token(l.STRING, sq_str + dq_str)

-- Numbers.
local inform_hex = '$' * l.xdigit^1
local inform_bin = '$$' * S('01')^1
local number = token(l.NUMBER, l.integer + inform_hex + inform_bin)

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  'Abbreviate', 'Array', 'Attribute', 'Class', 'Constant', 'Default', 'End',
  'Endif', 'Extend', 'Global', 'Ifdef', 'Iffalse', 'Ifndef', 'Ifnot', 'Iftrue',
  'Import', 'Include', 'Link', 'Lowstring', 'Message', 'Object', 'Property',
  'Release', 'Replace', 'Serial', 'StartDaemon', 'Statusline', 'StopDaemon',
  'Switches', 'Verb', 'absent', 'action', 'actor', 'add_to_scope', 'address',
  'additive', 'after', 'and', 'animate', 'article', 'articles', 'before',
  'bold', 'box', 'break', 'cant_go', 'capacity', 'char', 'class', 'child',
  'children', 'clothing', 'concealed', 'container', 'continue', 'creature',
  'daemon', 'deadflag', 'default', 'describe', 'description', 'do', 'door',
  'door_dir', 'door_to', 'd_to', 'd_obj', 'e_to', 'e_obj', 'each_turn',
  'edible', 'else', 'enterable', 'false', 'female', 'first', 'font', 'for',
  'found_in', 'general', 'give', 'grammar', 'has', 'hasnt', 'held', 'if', 'in',
  'in_to', 'in_obj', 'initial', 'inside_description', 'invent', 'jump', 'last',
  'life', 'light', 'list_together', 'location', 'lockable', 'locked', 'male',
  'move', 'moved', 'multi', 'multiexcept', 'multiheld', 'multiinside', 'n_to',
  'n_obj', 'ne_to', 'ne_obj', 'nw_to', 'nw_obj', 'name', 'neuter', 'new_line',
  'nothing', 'notin', 'noun', 'number', 'objectloop', 'ofclass', 'off', 'on',
  'only', 'open', 'openable', 'or', 'orders', 'out_to', 'out_obj', 'parent',
  'parse_name', 'player', 'plural', 'pluralname', 'print', 'print_ret',
  'private', 'proper', 'provides', 'random', 'react_after', 'react_before',
  'remove', 'replace', 'return', 'reverse', 'rfalse','roman', 'rtrue', 's_to',
  's_obj', 'se_to', 'se_obj', 'sw_to', 'sw_obj', 'scenery', 'scope', 'score',
  'scored', 'second', 'self', 'short_name', 'short_name_indef', 'sibling',
  'spaces', 'static', 'string', 'style', 'supporter', 'switch', 'switchable',
  'talkable', 'thedark', 'time_left', 'time_out', 'to', 'topic', 'transparent',
  'true', 'underline', 'u_to', 'u_obj', 'visited', 'w_to', 'w_obj',
  'when_closed', 'when_off', 'when_on', 'when_open', 'while', 'with',
  'with_key', 'workflag', 'worn'
})

-- Library actions.
local action = token('action', word_match{
  'Answer', 'Ask', 'AskFor', 'Attack', 'Blow', 'Burn', 'Buy', 'Climb', 'Close',
  'Consult', 'Cut', 'Dig', 'Disrobe', 'Drink', 'Drop', 'Eat', 'Empty', 'EmptyT',
  'Enter', 'Examine', 'Exit', 'Fill', 'FullScore', 'GetOff', 'Give', 'Go',
  'GoIn', 'Insert', 'Inv', 'InvTall', 'InvWide', 'Jump', 'JumpOver', 'Kiss',
  'LetGo', 'Listen', 'LMode1', 'LMode2', 'LMode3', 'Lock', 'Look', 'LookUnder',
  'Mild', 'No', 'NotifyOff', 'NotifyOn', 'Objects', 'Open', 'Order', 'Places',
  'Pray', 'Pronouns', 'Pull', 'Push', 'PushDir', 'PutOn', 'Quit', 'Receive',
  'Remove', 'Restart', 'Restore', 'Rub', 'Save', 'Score', 'ScriptOff',
  'ScriptOn', 'Search', 'Set', 'SetTo', 'Show', 'Sing', 'Sleep', 'Smell',
  'Sorry', 'Squeeze', 'Strong', 'Swim', 'Swing', 'SwitchOff', 'SwitchOn',
  'Take', 'Taste', 'Tell', 'Think', 'ThrowAt', 'ThrownAt', 'Tie', 'Touch',
  'Transfer', 'Turn', 'Unlock', 'VagueGo', 'Verify', 'Version', 'Wake',
  'WakeOther', 'Wait', 'Wave', 'WaveHands', 'Wear', 'Yes'
})

-- Identifiers.
local identifier = token(l.IDENTIFIER, l.word)

-- Operators.
local operator = token(l.OPERATOR, S('@~=+-*/%^#=<>;:,.{}[]()&|?'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'string', string},
  {'number', number},
  {'keyword', keyword},
  {'action', action},
  {'identifier', identifier},
  {'operator', operator},
}

_styles = {
  {'action', l.STYLE_VARIABLE}
}

return M
