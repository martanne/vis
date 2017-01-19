-- Copyright 2006-2017 Mitchell mitchell.att.foicica.com. See LICENSE.
-- AutoIt LPeg lexer.
-- Contributed by Jeff Stone.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'autoit'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local line_comment = ';' * l.nonnewline_esc^0
local block_comment1 = '#comments-start' * (l.any - '#comments-end')^0 *
                       P('#comments-end')^-1
local block_comment2 = '#cs' * (l.any - '#ce')^0 * P('#ce')^-1
local comment = token(l.COMMENT, line_comment + block_comment1 + block_comment2)

-- Keywords.
local kw = token(l.KEYWORD, word_match({
  'False', 'True', 'And', 'Or', 'Not', 'ContinueCase', 'ContinueLoop',
  'Default', 'Dim', 'Global', 'Local', 'Const', 'Do', 'Until', 'Enum', 'Exit',
  'ExitLoop', 'For', 'To', 'Step', 'Next', 'In', 'Func', 'Return', 'EndFunc',
  'If', 'Then', 'ElseIf', 'Else', 'EndIf', 'Null', 'ReDim', 'Select', 'Case',
  'EndSelect', 'Static', 'Switch', 'EndSwitch', 'Volatile', 'While', 'WEnd',
  'With', 'EndWith'
}, nil, true))

-- Functions.
local func = token(l.FUNCTION, word_match({
  'Abs', 'ACos', 'AdlibRegister', 'AdlibUnRegister', 'Asc', 'AscW', 'ASin',
  'Assign', 'ATan', 'AutoItSetOption', 'AutoItWinGetTitle', 'AutoItWinSetTitle',
  'Beep', 'Binary', 'BinaryLen', 'BinaryMid', 'BinaryToString', 'BitAND',
  'BitNOT', 'BitOR', 'BitRotate', 'BitShift', 'BitXOR', 'BlockInput', 'Break',
  'Call', 'CDTray', 'Ceiling', 'Chr', 'ChrW', 'ClipGet', 'ClipPut',
  'ConsoleRead', 'ConsoleWrite', 'ConsoleWriteError', 'ControlClick',
  'ControlCommand', 'ControlDisable', 'ControlEnable', 'ControlFocus',
  'ControlGetFocus', 'ControlGetHandle', 'ControlGetPos', 'ControlGetText',
  'ControlHide', 'ControlListView', 'ControlMove', 'ControlSend',
  'ControlSetText', 'ControlShow', 'ControlTreeView', 'Cos', 'Dec', 'DirCopy',
  'DirCreate', 'DirGetSize', 'DirMove', 'DirRemove', 'DllCall',
  'DllCallAddress', 'DllCallbackFree', 'DllCallbackGetPtr',
  'DllCallbackRegister', 'DllClose', 'DllOpen', 'DllStructCreate',
  'DllStructGetData', 'DllStructGetPtr', 'DllStructGetSize', 'DllStructSetData',
  'DriveGetDrive', 'DriveGetFileSystem', 'DriveGetLabel', 'DriveGetSerial',
  'DriveGetType', 'DriveMapAdd', 'DriveMapDel', 'DriveMapGet', 'DriveSetLabel',
  'DriveSpaceFree', 'DriveSpaceTotal', 'DriveStatus', 'EnvGet', 'EnvSet',
  'EnvUpdate', 'Eval', 'Execute', 'Exp', 'FileChangeDir', 'FileClose',
  'FileCopy', 'FileCreateNTFSLink', 'FileCreateShortcut', 'FileDelete',
  'FileExists', 'FileFindFirstFile', 'FileFindNextFile', 'FileFlush',
  'FileGetAttrib', 'FileGetEncoding', 'FileGetLongName', 'FileGetPos',
  'FileGetShortcut', 'FileGetShortName', 'FileGetSize', 'FileGetTime',
  'FileGetVersion', 'FileInstall', 'FileMove', 'FileOpen', 'FileOpenDialog',
  'FileRead', 'FileReadLine', 'FileReadToArray', 'FileRecycle',
  'FileRecycleEmpty', 'FileSaveDialog', 'FileSelectFolder', 'FileSetAttrib',
  'FileSetEnd', 'FileSetPos', 'FileSetTime', 'FileWrite', 'FileWriteLine',
  'Floor', 'FtpSetProxy', 'FuncName', 'GUICreate', 'GUICtrlCreateAvi',
  'GUICtrlCreateButton', 'GUICtrlCreateCheckbox', 'GUICtrlCreateCombo',
  'GUICtrlCreateContextMenu', 'GUICtrlCreateDate', 'GUICtrlCreateDummy',
  'GUICtrlCreateEdit', 'GUICtrlCreateGraphic', 'GUICtrlCreateGroup',
  'GUICtrlCreateIcon', 'GUICtrlCreateInput', 'GUICtrlCreateLabel',
  'GUICtrlCreateList', 'GUICtrlCreateListView', 'GUICtrlCreateListViewItem',
  'GUICtrlCreateMenu', 'GUICtrlCreateMenuItem', 'GUICtrlCreateMonthCal',
  'GUICtrlCreateObj', 'GUICtrlCreatePic', 'GUICtrlCreateProgress',
  'GUICtrlCreateRadio', 'GUICtrlCreateSlider', 'GUICtrlCreateTab',
  'GUICtrlCreateTabItem', 'GUICtrlCreateTreeView', 'GUICtrlCreateTreeViewItem',
  'GUICtrlCreateUpdown', 'GUICtrlDelete', 'GUICtrlGetHandle', 'GUICtrlGetState',
  'GUICtrlRead', 'GUICtrlRecvMsg', 'GUICtrlRegisterListViewSort',
  'GUICtrlSendMsg', 'GUICtrlSendToDummy', 'GUICtrlSetBkColor',
  'GUICtrlSetColor', 'GUICtrlSetCursor', 'GUICtrlSetData',
  'GUICtrlSetDefBkColor', 'GUICtrlSetDefColor', 'GUICtrlSetFont',
  'GUICtrlSetGraphic', 'GUICtrlSetImage', 'GUICtrlSetLimit',
  'GUICtrlSetOnEvent', 'GUICtrlSetPos', 'GUICtrlSetResizing', 'GUICtrlSetState',
  'GUICtrlSetStyle', 'GUICtrlSetTip', 'GUIDelete', 'GUIGetCursorInfo',
  'GUIGetMsg', 'GUIGetStyle', 'GUIRegisterMsg', 'GUISetAccelerators',
  'GUISetBkColor', 'GUISetCoord', 'GUISetCursor', 'GUISetFont', 'GUISetHelp',
  'GUISetIcon', 'GUISetOnEvent', 'GUISetState', 'GUISetStyle', 'GUIStartGroup',
  'GUISwitch', 'Hex', 'HotKeySet', 'HttpSetProxy', 'HttpSetUserAgent', 'HWnd',
  'InetClose', 'InetGet', 'InetGetInfo', 'InetGetSize', 'InetRead', 'IniDelete',
  'IniRead', 'IniReadSection', 'IniReadSectionNames', 'IniRenameSection',
  'IniWrite', 'IniWriteSection', 'InputBox', 'Int', 'IsAdmin', 'IsArray',
  'IsBinary', 'IsBool', 'IsDeclared', 'IsDllStruct', 'IsFloat', 'IsFunc',
  'IsHWnd', 'IsInt', 'IsKeyword', 'IsNumber', 'IsObj', 'IsPtr', 'IsString',
  'Log', 'MemGetStats', 'Mod', 'MouseClick', 'MouseClickDrag', 'MouseDown',
  'MouseGetCursor', 'MouseGetPos', 'MouseMove', 'MouseUp', 'MouseWheel',
  'MsgBox', 'Number', 'ObjCreate', 'ObjCreateInterface', 'ObjEvent', 'ObjGet',
  'ObjName', 'OnAutoItExitRegister', 'OnAutoItExitUnRegister', 'Ping',
  'PixelChecksum', 'PixelGetColor', 'PixelSearch', 'ProcessClose',
  'ProcessExists', 'ProcessGetStats', 'ProcessList', 'ProcessSetPriority',
  'ProcessWait', 'ProcessWaitClose', 'ProgressOff', 'ProgressOn', 'ProgressSet',
  'Ptr', 'Random', 'RegDelete', 'RegEnumKey', 'RegEnumVal', 'RegRead',
  'RegWrite', 'Round', 'Run', 'RunAs', 'RunAsWait', 'RunWait', 'Send',
  'SendKeepActive', 'SetError', 'SetExtended', 'ShellExecute',
  'ShellExecuteWait', 'Shutdown', 'Sin', 'Sleep', 'SoundPlay',
  'SoundSetWaveVolume', 'SplashImageOn', 'SplashOff', 'SplashTextOn', 'Sqrt',
  'SRandom', 'StatusbarGetText', 'StderrRead', 'StdinWrite', 'StdioClose',
  'StdoutRead', 'String', 'StringAddCR', 'StringCompare', 'StringFormat',
  'StringFromASCIIArray', 'StringInStr', 'StringIsAlNum', 'StringIsAlpha',
  'StringIsASCII', 'StringIsDigit', 'StringIsFloat', 'StringIsInt',
  'StringIsLower', 'StringIsSpace', 'StringIsUpper', 'StringIsXDigit',
  'StringLeft', 'StringLen', 'StringLower', 'StringMid', 'StringRegExp',
  'StringRegExpReplace', 'StringReplace', 'StringReverse', 'StringRight',
  'StringSplit', 'StringStripCR', 'StringStripWS', 'StringToASCIIArray',
  'StringToBinary', 'StringTrimLeft', 'StringTrimRight', 'StringUpper', 'Tan',
  'TCPAccept', 'TCPCloseSocket', 'TCPConnect', 'TCPListen', 'TCPNameToIP',
  'TCPRecv', 'TCPSend', 'TCPShutdown, UDPShutdown', 'TCPStartup, UDPStartup',
  'TimerDiff', 'TimerInit', 'ToolTip', 'TrayCreateItem', 'TrayCreateMenu',
  'TrayGetMsg', 'TrayItemDelete', 'TrayItemGetHandle', 'TrayItemGetState',
  'TrayItemGetText', 'TrayItemSetOnEvent', 'TrayItemSetState',
  'TrayItemSetText', 'TraySetClick', 'TraySetIcon', 'TraySetOnEvent',
  'TraySetPauseIcon', 'TraySetState', 'TraySetToolTip', 'TrayTip', 'UBound',
  'UDPBind', 'UDPCloseSocket', 'UDPOpen', 'UDPRecv', 'UDPSend', 'VarGetType',
  'WinActivate', 'WinActive', 'WinClose', 'WinExists', 'WinFlash',
  'WinGetCaretPos', 'WinGetClassList', 'WinGetClientSize', 'WinGetHandle',
  'WinGetPos', 'WinGetProcess', 'WinGetState', 'WinGetText', 'WinGetTitle',
  'WinKill', 'WinList', 'WinMenuSelectItem', 'WinMinimizeAll',
  'WinMinimizeAllUndo', 'WinMove', 'WinSetOnTop', 'WinSetState', 'WinSetTitle',
  'WinSetTrans', 'WinWait', 'WinWaitActive', 'WinWaitClose', 'WinWaitNotActive'
}, nil, true))

-- Preprocessor.
local preproc = token(l.PREPROCESSOR, '#' * word_match({
  'include-once', 'include', 'pragma', 'forceref', 'RequireAdmin', 'NoTrayIcon',
  'OnAutoItStartRegister'
}, '-', true))

-- Strings.
local dq_str = l.delimited_range('"', true, true)
local sq_str = l.delimited_range("'", true, true)
local inc = l.delimited_range('<>', true, true, true)
local str = token(l.STRING, dq_str + sq_str + inc)

-- Macros.
local macro = token('macro', '@' * (l.alnum + '_')^1)

-- Variables.
local var = token(l.VARIABLE, '$' * (l.alnum + '_')^1)

-- Identifiers.
local ident = token(l.IDENTIFIER, (l.alnum + '_')^1)

-- Numbers.
local nbr = token(l.NUMBER, l.float + l.integer)

-- Operators.
local oper = token(l.OPERATOR, S('+-^*/&<>=?:()[]'))

M._rules = {
  {'whitespace', ws},
  {'comment', comment},
  {'keyword', kw},
  {'function', func},
  {'preproc', preproc},
  {'string', str},
  {'macro', macro},
  {'variable', var},
  {'number', nbr},
  {'identifier', ident},
  {'operator', oper}
}

M._tokenstyles = {
  macro = l.STYLE_PREPROCESSOR
}

return M
