-- Copyright 2006-2022 Mitchell. See LICENSE.
-- AutoIt LPeg lexer.
-- Contributed by Jeff Stone.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('autoit')

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match({
  'False', 'True', 'And', 'Or', 'Not', 'ContinueCase', 'ContinueLoop', 'Default', 'Dim', 'Global',
  'Local', 'Const', 'Do', 'Until', 'Enum', 'Exit', 'ExitLoop', 'For', 'To', 'Step', 'Next', 'In',
  'Func', 'Return', 'EndFunc', 'If', 'Then', 'ElseIf', 'Else', 'EndIf', 'Null', 'ReDim', 'Select',
  'Case', 'EndSelect', 'Static', 'Switch', 'EndSwitch', 'Volatile', 'While', 'WEnd', 'With',
  'EndWith'
}, true)))

-- Functions.
lex:add_rule('function', token(lexer.FUNCTION, word_match({
  'Abs', 'ACos', 'AdlibRegister', 'AdlibUnRegister', 'Asc', 'AscW', 'ASin', 'Assign', 'ATan',
  'AutoItSetOption', 'AutoItWinGetTitle', 'AutoItWinSetTitle', 'Beep', 'Binary', 'BinaryLen',
  'BinaryMid', 'BinaryToString', 'BitAND', 'BitNOT', 'BitOR', 'BitRotate', 'BitShift', 'BitXOR',
  'BlockInput', 'Break', 'Call', 'CDTray', 'Ceiling', 'Chr', 'ChrW', 'ClipGet', 'ClipPut',
  'ConsoleRead', 'ConsoleWrite', 'ConsoleWriteError', 'ControlClick', 'ControlCommand',
  'ControlDisable', 'ControlEnable', 'ControlFocus', 'ControlGetFocus', 'ControlGetHandle',
  'ControlGetPos', 'ControlGetText', 'ControlHide', 'ControlListView', 'ControlMove', 'ControlSend',
  'ControlSetText', 'ControlShow', 'ControlTreeView', 'Cos', 'Dec', 'DirCopy', 'DirCreate',
  'DirGetSize', 'DirMove', 'DirRemove', 'DllCall', 'DllCallAddress', 'DllCallbackFree',
  'DllCallbackGetPtr', 'DllCallbackRegister', 'DllClose', 'DllOpen', 'DllStructCreate',
  'DllStructGetData', 'DllStructGetPtr', 'DllStructGetSize', 'DllStructSetData', 'DriveGetDrive',
  'DriveGetFileSystem', 'DriveGetLabel', 'DriveGetSerial', 'DriveGetType', 'DriveMapAdd',
  'DriveMapDel', 'DriveMapGet', 'DriveSetLabel', 'DriveSpaceFree', 'DriveSpaceTotal', 'DriveStatus',
  'EnvGet', 'EnvSet', 'EnvUpdate', 'Eval', 'Execute', 'Exp', 'FileChangeDir', 'FileClose',
  'FileCopy', 'FileCreateNTFSLink', 'FileCreateShortcut', 'FileDelete', 'FileExists',
  'FileFindFirstFile', 'FileFindNextFile', 'FileFlush', 'FileGetAttrib', 'FileGetEncoding',
  'FileGetLongName', 'FileGetPos', 'FileGetShortcut', 'FileGetShortName', 'FileGetSize',
  'FileGetTime', 'FileGetVersion', 'FileInstall', 'FileMove', 'FileOpen', 'FileOpenDialog',
  'FileRead', 'FileReadLine', 'FileReadToArray', 'FileRecycle', 'FileRecycleEmpty',
  'FileSaveDialog', 'FileSelectFolder', 'FileSetAttrib', 'FileSetEnd', 'FileSetPos', 'FileSetTime',
  'FileWrite', 'FileWriteLine', 'Floor', 'FtpSetProxy', 'FuncName', 'GUICreate', 'GUICtrlCreateAvi',
  'GUICtrlCreateButton', 'GUICtrlCreateCheckbox', 'GUICtrlCreateCombo', 'GUICtrlCreateContextMenu',
  'GUICtrlCreateDate', 'GUICtrlCreateDummy', 'GUICtrlCreateEdit', 'GUICtrlCreateGraphic',
  'GUICtrlCreateGroup', 'GUICtrlCreateIcon', 'GUICtrlCreateInput', 'GUICtrlCreateLabel',
  'GUICtrlCreateList', 'GUICtrlCreateListView', 'GUICtrlCreateListViewItem', 'GUICtrlCreateMenu',
  'GUICtrlCreateMenuItem', 'GUICtrlCreateMonthCal', 'GUICtrlCreateObj', 'GUICtrlCreatePic',
  'GUICtrlCreateProgress', 'GUICtrlCreateRadio', 'GUICtrlCreateSlider', 'GUICtrlCreateTab',
  'GUICtrlCreateTabItem', 'GUICtrlCreateTreeView', 'GUICtrlCreateTreeViewItem',
  'GUICtrlCreateUpdown', 'GUICtrlDelete', 'GUICtrlGetHandle', 'GUICtrlGetState', 'GUICtrlRead',
  'GUICtrlRecvMsg', 'GUICtrlRegisterListViewSort', 'GUICtrlSendMsg', 'GUICtrlSendToDummy',
  'GUICtrlSetBkColor', 'GUICtrlSetColor', 'GUICtrlSetCursor', 'GUICtrlSetData',
  'GUICtrlSetDefBkColor', 'GUICtrlSetDefColor', 'GUICtrlSetFont', 'GUICtrlSetGraphic',
  'GUICtrlSetImage', 'GUICtrlSetLimit', 'GUICtrlSetOnEvent', 'GUICtrlSetPos', 'GUICtrlSetResizing',
  'GUICtrlSetState', 'GUICtrlSetStyle', 'GUICtrlSetTip', 'GUIDelete', 'GUIGetCursorInfo',
  'GUIGetMsg', 'GUIGetStyle', 'GUIRegisterMsg', 'GUISetAccelerators', 'GUISetBkColor',
  'GUISetCoord', 'GUISetCursor', 'GUISetFont', 'GUISetHelp', 'GUISetIcon', 'GUISetOnEvent',
  'GUISetState', 'GUISetStyle', 'GUIStartGroup', 'GUISwitch', 'Hex', 'HotKeySet', 'HttpSetProxy',
  'HttpSetUserAgent', 'HWnd', 'InetClose', 'InetGet', 'InetGetInfo', 'InetGetSize', 'InetRead',
  'IniDelete', 'IniRead', 'IniReadSection', 'IniReadSectionNames', 'IniRenameSection', 'IniWrite',
  'IniWriteSection', 'InputBox', 'Int', 'IsAdmin', 'IsArray', 'IsBinary', 'IsBool', 'IsDeclared',
  'IsDllStruct', 'IsFloat', 'IsFunc', 'IsHWnd', 'IsInt', 'IsKeyword', 'IsNumber', 'IsObj', 'IsPtr',
  'IsString', 'Log', 'MemGetStats', 'Mod', 'MouseClick', 'MouseClickDrag', 'MouseDown',
  'MouseGetCursor', 'MouseGetPos', 'MouseMove', 'MouseUp', 'MouseWheel', 'MsgBox', 'Number',
  'ObjCreate', 'ObjCreateInterface', 'ObjEvent', 'ObjGet', 'ObjName', 'OnAutoItExitRegister',
  'OnAutoItExitUnRegister', 'Ping', 'PixelChecksum', 'PixelGetColor', 'PixelSearch', 'ProcessClose',
  'ProcessExists', 'ProcessGetStats', 'ProcessList', 'ProcessSetPriority', 'ProcessWait',
  'ProcessWaitClose', 'ProgressOff', 'ProgressOn', 'ProgressSet', 'Ptr', 'Random', 'RegDelete',
  'RegEnumKey', 'RegEnumVal', 'RegRead', 'RegWrite', 'Round', 'Run', 'RunAs', 'RunAsWait',
  'RunWait', 'Send', 'SendKeepActive', 'SetError', 'SetExtended', 'ShellExecute',
  'ShellExecuteWait', 'Shutdown', 'Sin', 'Sleep', 'SoundPlay', 'SoundSetWaveVolume',
  'SplashImageOn', 'SplashOff', 'SplashTextOn', 'Sqrt', 'SRandom', 'StatusbarGetText', 'StderrRead',
  'StdinWrite', 'StdioClose', 'StdoutRead', 'String', 'StringAddCR', 'StringCompare',
  'StringFormat', 'StringFromASCIIArray', 'StringInStr', 'StringIsAlNum', 'StringIsAlpha',
  'StringIsASCII', 'StringIsDigit', 'StringIsFloat', 'StringIsInt', 'StringIsLower',
  'StringIsSpace', 'StringIsUpper', 'StringIsXDigit', 'StringLeft', 'StringLen', 'StringLower',
  'StringMid', 'StringRegExp', 'StringRegExpReplace', 'StringReplace', 'StringReverse',
  'StringRight', 'StringSplit', 'StringStripCR', 'StringStripWS', 'StringToASCIIArray',
  'StringToBinary', 'StringTrimLeft', 'StringTrimRight', 'StringUpper', 'Tan', 'TCPAccept',
  'TCPCloseSocket', 'TCPConnect', 'TCPListen', 'TCPNameToIP', 'TCPRecv', 'TCPSend', 'TCPShutdown',
  'TCPStartup', 'TimerDiff', 'TimerInit', 'ToolTip', 'TrayCreateItem', 'TrayCreateMenu',
  'TrayGetMsg', 'TrayItemDelete', 'TrayItemGetHandle', 'TrayItemGetState', 'TrayItemGetText',
  'TrayItemSetOnEvent', 'TrayItemSetState', 'TrayItemSetText', 'TraySetClick', 'TraySetIcon',
  'TraySetOnEvent', 'TraySetPauseIcon', 'TraySetState', 'TraySetToolTip', 'TrayTip', 'UBound',
  'UDPBind', 'UDPCloseSocket', 'UDPOpen', 'UDPRecv', 'UDPSend', 'UDPShutdown', 'UDPStartup',
  'VarGetType', 'WinActivate', 'WinActive', 'WinClose', 'WinExists', 'WinFlash', 'WinGetCaretPos',
  'WinGetClassList', 'WinGetClientSize', 'WinGetHandle', 'WinGetPos', 'WinGetProcess',
  'WinGetState', 'WinGetText', 'WinGetTitle', 'WinKill', 'WinList', 'WinMenuSelectItem',
  'WinMinimizeAll', 'WinMinimizeAllUndo', 'WinMove', 'WinSetOnTop', 'WinSetState', 'WinSetTitle',
  'WinSetTrans', 'WinWait', 'WinWaitActive', 'WinWaitClose', 'WinWaitNotActive'
}, true)))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol(';')
local block_comment = lexer.range('#comments-start', '#comments-end') + lexer.range('#cs', '#ce')
lex:add_rule('comment', token(lexer.COMMENT, line_comment + block_comment))

-- Preprocessor.
lex:add_rule('preprocessor', token(lexer.PREPROCESSOR, '#' * word_match({
  'include-once', 'include', 'pragma', 'forceref', 'RequireAdmin', 'NoTrayIcon',
  'OnAutoItStartRegister'
}, true)))

-- Strings.
local dq_str = lexer.range('"', true, false)
local sq_str = lexer.range("'", true, false)
local inc = lexer.range('<', '>', true, false, true)
lex:add_rule('string', token(lexer.STRING, dq_str + sq_str + inc))

-- Macros.
lex:add_rule('macro', token('macro', '@' * (lexer.alnum + '_')^1))
lex:add_style('macro', lexer.styles.preprocessor)

-- Variables.
lex:add_rule('variable', token(lexer.VARIABLE, '$' * (lexer.alnum + '_')^1))

-- Numbers.
lex:add_rule('number', token(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, S('+-^*/&<>=?:()[]')))

return lex
