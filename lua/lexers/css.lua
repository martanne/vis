-- Copyright 2006-2022 Mitchell. See LICENSE.
-- CSS LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('css')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Properties.
lex:add_rule('property', token('property', word_match{
  -- CSS 1.
  'color', 'background-color', 'background-image', 'background-repeat', 'background-attachment',
  'background-position', 'background', 'font-family', 'font-style', 'font-variant', 'font-weight',
  'font-size', 'font', 'word-spacing', 'letter-spacing', 'text-decoration', 'vertical-align',
  'text-transform', 'text-align', 'text-indent', 'line-height', 'margin-top', 'margin-right',
  'margin-bottom', 'margin-left', 'margin', 'padding-top', 'padding-right', 'padding-bottom',
  'padding-left', 'padding', 'border-top-width', 'border-right-width', 'border-bottom-width',
  'border-left-width', 'border-width', 'border-top', 'border-right', 'border-bottom', 'border-left',
  'border', 'border-color', 'border-style', 'width', 'height', 'float', 'clear', 'display',
  'white-space', 'list-style-type', 'list-style-image', 'list-style-position', 'list-style',
  -- CSS 2.
  'border-top-color', 'border-right-color', 'border-bottom-color', 'border-left-color',
  'border-color', 'border-top-style', 'border-right-style', 'border-bottom-style',
  'border-left-style', 'border-style', 'top', 'right', 'bottom', 'left', 'position', 'z-index',
  'direction', 'unicode-bidi', 'min-width', 'max-width', 'min-height', 'max-height', 'overflow',
  'clip', 'visibility', 'content', 'quotes', 'counter-reset', 'counter-increment', 'marker-offset',
  'size', 'marks', 'page-break-before', 'page-break-after', 'page-break-inside', 'page', 'orphans',
  'widows', 'font-stretch', 'font-size-adjust', 'unicode-range', 'units-per-em', 'src', 'panose-1',
  'stemv', 'stemh', 'slope', 'cap-height', 'x-height', 'ascent', 'descent', 'widths', 'bbox',
  'definition-src', 'baseline', 'centerline', 'mathline', 'topline', 'text-shadow', 'caption-side',
  'table-layout', 'border-collapse', 'border-spacing', 'empty-cells', 'speak-header', 'cursor',
  'outline', 'outline-width', 'outline-style', 'outline-color', 'volume', 'speak', 'pause-before',
  'pause-after', 'pause', 'cue-before', 'cue-after', 'cue', 'play-during', 'azimuth', 'elevation',
  'speech-rate', 'voice-family', 'pitch', 'pitch-range', 'stress', 'richness', 'speak-punctuation',
  'speak-numeral',
  -- CSS 3.
  'flex', 'flex-basis', 'flex-direction', 'flex-flow', 'flex-grow', 'flex-shrink', 'flex-wrap',
  'align-content', 'align-items', 'align-self', 'justify-content', 'order', 'border-radius',
  'transition', 'transform', 'box-shadow', 'filter', 'opacity', 'resize', 'word-break', 'word-wrap',
  'box-sizing', 'animation', 'text-overflow'
}))
lex:add_style('property', lexer.styles.keyword)

-- Values.
lex:add_rule('value', token('value', word_match{
  -- CSS 1.
  'auto', 'none', 'normal', 'italic', 'oblique', 'small-caps', 'bold', 'bolder', 'lighter',
  'xx-small', 'x-small', 'small', 'medium', 'large', 'x-large', 'xx-large', 'larger', 'smaller',
  'transparent', 'repeat', 'repeat-x', 'repeat-y', 'no-repeat', 'scroll', 'fixed', 'top', 'bottom',
  'left', 'center', 'right', 'justify', 'both', 'underline', 'overline', 'line-through', 'blink',
  'baseline', 'sub', 'super', 'text-top', 'middle', 'text-bottom', 'capitalize', 'uppercase',
  'lowercase', 'thin', 'medium', 'thick', 'dotted', 'dashed', 'solid', 'double', 'groove', 'ridge',
  'inset', 'outset', 'block', 'inline', 'list-item', 'pre', 'no-wrap', 'inside', 'outside', 'disc',
  'circle', 'square', 'decimal', 'lower-roman', 'upper-roman', 'lower-alpha', 'upper-alpha', 'aqua',
  'black', 'blue', 'fuchsia', 'gray', 'green', 'lime', 'maroon', 'navy', 'olive', 'purple', 'red',
  'silver', 'teal', 'white', 'yellow',
  -- CSS 2.
  'inherit', 'run-in', 'compact', 'marker', 'table', 'inline-table', 'table-row-group',
  'table-header-group', 'table-footer-group', 'table-row', 'table-column-group', 'table-column',
  'table-cell', 'table-caption', 'static', 'relative', 'absolute', 'fixed', 'ltr', 'rtl', 'embed',
  'bidi-override', 'visible', 'hidden', 'scroll', 'collapse', 'open-quote', 'close-quote',
  'no-open-quote', 'no-close-quote', 'decimal-leading-zero', 'lower-greek', 'lower-latin',
  'upper-latin', 'hebrew', 'armenian', 'georgian', 'cjk-ideographic', 'hiragana', 'katakana',
  'hiragana-iroha', 'katakana-iroha', 'landscape', 'portrait', 'crop', 'cross', 'always', 'avoid',
  'wider', 'narrower', 'ultra-condensed', 'extra-condensed', 'condensed', 'semi-condensed',
  'semi-expanded', 'expanded', 'extra-expanded', 'ultra-expanded', 'caption', 'icon', 'menu',
  'message-box', 'small-caption', 'status-bar', 'separate', 'show', 'hide', 'once', 'crosshair',
  'default', 'pointer', 'move', 'text', 'wait', 'help', 'e-resize', 'ne-resize', 'nw-resize',
  'n-resize', 'se-resize', 'sw-resize', 's-resize', 'w-resize', 'ActiveBorder', 'ActiveCaption',
  'AppWorkspace', 'Background', 'ButtonFace', 'ButtonHighlight', 'ButtonShadow',
  'InactiveCaptionText', 'ButtonText', 'CaptionText', 'GrayText', 'Highlight', 'HighlightText',
  'InactiveBorder', 'InactiveCaption', 'InfoBackground', 'InfoText', 'Menu', 'MenuText',
  'Scrollbar', 'ThreeDDarkShadow', 'ThreeDFace', 'ThreeDHighlight', 'ThreeDLightShadow',
  'ThreeDShadow', 'Window', 'WindowFrame', 'WindowText', 'silent', 'x-soft', 'soft', 'medium',
  'loud', 'x-loud', 'spell-out', 'mix', 'left-side', 'far-left', 'center-left', 'center-right',
  'far-right', 'right-side', 'behind', 'leftwards', 'rightwards', 'below', 'level', 'above',
  'higher', 'lower', 'x-slow', 'slow', 'medium', 'fast', 'x-fast', 'faster', 'slower', 'male',
  'female', 'child', 'x-low', 'low', 'high', 'x-high', 'code', 'digits', 'continous',
  -- CSS 3.
  'flex', 'row', 'column', 'ellipsis', 'inline-block'
}))
lex:add_style('value', lexer.styles.constant)

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match{
  'attr', 'blackness', 'blend', 'blenda', 'blur', 'brightness', 'calc', 'circle', 'color-mod',
  'contrast', 'counter', 'cubic-bezier', 'device-cmyk', 'drop-shadow', 'ellipse', 'gray',
  'grayscale', 'hsl', 'hsla', 'hue', 'hue-rotate', 'hwb', 'image', 'inset', 'invert', 'lightness',
  'linear-gradient', 'matrix', 'matrix3d', 'opacity', 'perspective', 'polygon', 'radial-gradient',
  'rect', 'repeating-linear-gradient', 'repeating-radial-gradient', 'rgb', 'rgba', 'rotate',
  'rotate3d', 'rotateX', 'rotateY', 'rotateZ', 'saturate', 'saturation', 'scale', 'scale3d',
  'scaleX', 'scaleY', 'scaleZ', 'sepia', 'shade', 'skewX', 'skewY', 'steps', 'tint', 'toggle',
  'translate', 'translate3d', 'translateX', 'translateY', 'translateZ', 'url', 'whiteness', 'var'
}))

-- Colors.
local xdigit = lexer.xdigit
lex:add_rule('color', token('color', word_match{
  'aliceblue', 'antiquewhite', 'aqua', 'aquamarine', 'azure', 'beige', 'bisque', 'black',
  'blanchedalmond', 'blue', 'blueviolet', 'brown', 'burlywood', 'cadetblue', 'chartreuse',
  'chocolate', 'coral', 'cornflowerblue', 'cornsilk', 'crimson', 'cyan', 'darkblue', 'darkcyan',
  'darkgoldenrod', 'darkgray', 'darkgreen', 'darkgrey', 'darkkhaki', 'darkmagenta',
  'darkolivegreen', 'darkorange', 'darkorchid', 'darkred', 'darksalmon', 'darkseagreen',
  'darkslateblue', 'darkslategray', 'darkslategrey', 'darkturquoise', 'darkviolet', 'deeppink',
  'deepskyblue', 'dimgray', 'dimgrey', 'dodgerblue', 'firebrick', 'floralwhite', 'forestgreen',
  'fuchsia', 'gainsboro', 'ghostwhite', 'gold', 'goldenrod', 'gray', 'green', 'greenyellow', 'grey',
  'honeydew', 'hotpink', 'indianred', 'indigo', 'ivory', 'khaki', 'lavender', 'lavenderblush',
  'lawngreen', 'lemonchiffon', 'lightblue', 'lightcoral', 'lightcyan', 'lightgoldenrodyellow',
  'lightgray', 'lightgreen', 'lightgrey', 'lightpink', 'lightsalmon', 'lightseagreen',
  'lightskyblue', 'lightslategray', 'lightslategrey', 'lightsteelblue', 'lightyellow', 'lime',
  'limegreen', 'linen', 'magenta', 'maroon', 'mediumaquamarine', 'mediumblue', 'mediumorchid',
  'mediumpurple', 'mediumseagreen', 'mediumslateblue', 'mediumspringgreen', 'mediumturquoise',
  'mediumvioletred', 'midnightblue', 'mintcream', 'mistyrose', 'moccasin', 'navajowhite', 'navy',
  'oldlace', 'olive', 'olivedrab', 'orange', 'orangered', 'orchid', 'palegoldenrod', 'palegreen',
  'paleturquoise', 'palevioletred', 'papayawhip', 'peachpuff', 'peru', 'pink', 'plum', 'powderblue',
  'purple', 'rebeccapurple', 'red', 'rosybrown', 'royalblue', 'saddlebrown', 'salmon', 'sandybrown',
  'seagreen', 'seashell', 'sienna', 'silver', 'skyblue', 'slateblue', 'slategray', 'slategrey',
  'snow', 'springgreen', 'steelblue', 'tan', 'teal', 'thistle', 'tomato', 'transparent',
  'turquoise', 'violet', 'wheat', 'white', 'whitesmoke', 'yellow', 'yellowgreen'
} + '#' * xdigit * xdigit * xdigit * (xdigit * xdigit * xdigit)^-1))
lex:add_style('color', lexer.styles.number)

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.alpha * (lexer.alnum + S('_-'))^0))

-- Pseudo classes and pseudo elements.
lex:add_rule('pseudoclass', ':' * token('pseudoclass', word_match{
  'active', 'checked', 'disabled', 'empty', 'enabled', 'first-child', 'first-of-type', 'focus',
  'hover', 'in-range', 'invalid', 'lang', 'last-child', 'last-of-type', 'link', 'not', 'nth-child',
  'nth-last-child', 'nth-last-of-type', 'nth-of-type', 'only-of-type', 'only-child', 'optional',
  'out-of-range', 'read-only', 'read-write', 'required', 'root', 'target', 'valid', 'visited'
}))
lex:add_style('pseudoclass', lexer.styles.constant)
lex:add_rule('pseudoelement', '::' *
  token('pseudoelement', word_match('after before first-letter first-line selection')))
lex:add_style('pseudoelement', lexer.styles.constant)

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.range('/*', '*/')))

-- Numbers.
local unit = token('unit', word_match(
  'ch cm deg dpcm dpi dppx em ex grad Hz in kHz mm ms pc pt px q rad rem s turn vh vmax vmin vw'))
lex:add_style('unit', lexer.styles.number)
lex:add_rule('number', token(lexer.NUMBER, lexer.dec_num) * unit^-1)

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('~!#*>+=|.,:;()[]{}')))

-- At rule.
lex:add_rule('at_rule', token('at_rule', '@' *
  word_match('charset font-face media page import namespace keyframes')))
lex:add_style('at_rule', lexer.styles.preprocessor)

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

return lex
