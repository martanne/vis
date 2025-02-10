-- Copyright 2022-2025 Mitchell. See LICENSE.
-- AutoHotkey LPeg lexer.
-- Contributed by Snoopy.

local lexer = lexer
local P, S, B = lpeg.P, lpeg.S, lpeg.B

local lex = lexer.new(...)

-- Keywords.
lex:add_rule('keyword', lex:tag(lexer.KEYWORD, lex:word_match(lexer.KEYWORD, true)))

-- Variables.
lex:add_rule('variable',
	lex:tag(lexer.VARIABLE_BUILTIN, 'A_' * lex:word_match(lexer.VARIABLE_BUILTIN, true)))

-- Constants.
lex:add_rule('constant', lex:tag(lexer.CONSTANT_BUILTIN, S('fF') * lexer.digit * (lexer.digit)^-1 +
	lex:word_match(lexer.CONSTANT_BUILTIN, true)))

-- Functions.
local builtin_func = -B('.') *
	lex:tag(lexer.FUNCTION_BUILTIN, lex:word_match(lexer.FUNCTION_BUILTIN, true))
local func = lex:tag(lexer.FUNCTION, lexer.word)
local method = B('.') * lex:tag(lexer.FUNCTION_METHOD, lexer.word)
lex:add_rule('function', (builtin_func + method + func) * #(lexer.space^0 * '('))

-- Identifiers.
lex:add_rule('identifier', lex:tag(lexer.IDENTIFIER, lexer.word))

-- Comments.
local line_comment = lexer.to_eol(';')
local block_comment = lexer.range('/*', '*/')
lex:add_rule('comment', lex:tag(lexer.COMMENT, line_comment + block_comment))

-- Preprocessor.
lex:add_rule('preprocessor',
	lex:tag(lexer.PREPROCESSOR, '#' * lex:word_match(lexer.PREPROCESSOR, true)))

-- Strings.
local dq_str = lexer.range('"', true, false)
local sq_str = lexer.range("'", true, false)
lex:add_rule('string', lex:tag(lexer.STRING, dq_str + sq_str))

-- Numbers.
lex:add_rule('number', lex:tag(lexer.NUMBER, lexer.number))

-- Operators.
lex:add_rule('operator', lex:tag(lexer.OPERATOR, S('~+-^*/&<>=?:()[]{}')))

lex:set_word_list(lexer.KEYWORD, {
	'as', 'and', 'class', 'contains', 'extends', 'false', 'in', 'is', 'IsSet', 'not', 'or', 'super',
	'true', 'unset', 'Break', 'Catch', 'Continue', 'Else', 'Finally', 'For', 'Global', 'Goto', 'If',
	'Local', 'Loop', 'Return', 'Static', 'Throw', 'Try', 'Until', 'While'
})

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	'Abs', 'AutoTrim', 'Asc', 'ASin', 'ACos', 'ATan', 'BlockInput', 'Ceil', 'Chr', 'Click',
	'ClipWait', 'ComObjActive', 'ComObjArray', 'ComObjConnect', 'ComObjCreate', 'ComObject',
	'ComObjEnwrap', 'ComObjUnwrap', 'ComObjError', 'ComObjFlags', 'ComObjGet', 'ComObjMissing',
	'ComObjParameter', 'ComObjQuery', 'ComObjType', 'ComObjValue', 'Control', 'ControlClick',
	'ControlFocus', 'ControlGet', 'ControlGetFocus', 'ControlGetPos', 'ControlGetText', 'ControlMove',
	'ControlSend', 'ControlSendRaw', 'ControlSetText', 'CoordMode', 'Cos', 'Critical',
	'DetectHiddenText', 'DetectHiddenWindows', 'DllCall', 'Drive', 'DriveGet', 'DriveSpaceFree',
	'Edit', 'Else', 'EnvAdd', 'EnvDiv', 'EnvGet', 'EnvMult', 'EnvSet', 'EnvSub', 'EnvUpdate',
	'Exception', 'Exit', 'ExitApp', 'Exp', 'FileAppend', 'FileCopy', 'FileCopyDir', 'FileCreateDir',
	'FileCreateShortcut', 'FileDelete', 'FileEncoding', 'FileExist', 'FileInstall', 'FileGetAttrib',
	'FileGetShortcut', 'FileGetSize', 'FileGetTime', 'FileGetVersion', 'FileMove', 'FileMoveDir',
	'FileOpen', 'FileRead', 'FileReadLine', 'FileRecycle', 'FileRecycleEmpty', 'FileRemoveDir',
	'FileSelectFile', 'FileSelectFolder', 'FileSetAttrib', 'FileSetTime', 'Floor', 'Format',
	'FormatTime', 'Func', 'GetKeyName', 'GetKeyVK', 'GetKeySC', 'GetKeyState', 'GetKeyState', 'Gosub',
	'GroupActivate', 'GroupAdd', 'GroupClose', 'GroupDeactivate', 'Gui', 'GuiControl',
	'GuiControlGet', 'Hotkey', 'Hotstring', 'IfEqual', 'IfNotEqual', 'IfExist', 'IfNotExist',
	'IfGreater', 'IfGreaterOrEqual', 'IfInString', 'IfNotInString', 'IfLess', 'IfLessOrEqual',
	'IfMsgBox', 'IfWinActive', 'IfWinNotActive', 'IfWinExist', 'IfWinNotExist', 'IL_Create', 'IL_Add',
	'IL_Destroy', 'ImageSearch', 'IniDelete', 'IniRead', 'IniWrite', 'Input', 'InputBox', 'InputHook',
	'InStr', 'IsByRef', 'IsFunc', 'IsLabel', 'IsObject', 'IsSet', 'KeyHistory', 'KeyWait',
	'ListHotkeys', 'ListLines', 'ListVars', 'LoadPicture', 'Log', 'Ln', 'LV_Add', 'LV_Delete',
	'LV_DeleteCol', 'LV_GetCount', 'LV_GetNext', 'LV_GetText', 'LV_Insert', 'LV_InsertCol',
	'LV_Modify', 'LV_ModifyCol', 'LV_SetImageList', 'Max', 'Menu', 'MenuGetHandle', 'MenuGetName',
	'Min', 'Mod', 'MouseClick', 'MouseClickDrag', 'MouseGetPos', 'MouseMove', 'MsgBox', 'NumGet',
	'NumPut', 'ObjAddRef', 'ObjRelease', 'ObjBindMethod', 'ObjClone', 'ObjCount', 'ObjDelete',
	'ObjGetAddress', 'ObjGetCapacity', 'ObjHasKey', 'ObjInsert', 'ObjInsertAt', 'ObjLength',
	'ObjMaxIndex', 'ObjMinIndex', 'ObjNewEnum', 'ObjPop', 'ObjPush', 'ObjRemove', 'ObjRemoveAt',
	'ObjSetCapacity', 'ObjGetBase', 'ObjRawGet', 'ObjRawSet', 'ObjSetBase', 'OnClipboardChange',
	'OnError', 'OnExit', 'OnExit', 'OnMessage', 'Ord', 'OutputDebug', 'Pause', 'PixelGetColor',
	'PixelSearch', 'PostMessage', 'Process', 'Progress', 'Random', 'RegExMatch', 'RegExReplace',
	'RegDelete', 'RegRead', 'RegWrite', 'RegisterCallback', 'Reload', 'Round', 'Run', 'RunAs',
	'RunWait', 'SB_SetIcon', 'SB_SetParts', 'SB_SetText', 'Send', 'SendRaw', 'SendInput', 'SendPlay',
	'SendEvent', 'SendLevel', 'SendMessage', 'SendMode', 'SetBatchLines', 'SetCapsLockState',
	'SetControlDelay', 'SetDefaultMouseSpeed', 'SetEnv', 'SetFormat', 'SetKeyDelay', 'SetMouseDelay',
	'SetNumLockState', 'SetScrollLockState', 'SetRegView', 'SetStoreCapsLockMode', 'SetTimer',
	'SetTitleMatchMode', 'SetWinDelay', 'SetWorkingDir', 'Shutdown', 'Sin', 'Sleep', 'Sort',
	'SoundBeep', 'SoundGet', 'SoundGetWaveVolume', 'SoundPlay', 'SoundSet', 'SoundSetWaveVolume',
	'SplashImage', 'SplashTextOn', 'SplashTextOff', 'SplitPath', 'Sqrt', 'StatusBarGetText',
	'StatusBarWait', 'StrGet', 'StringCaseSense', 'StringGetPos', 'StringLeft', 'StringLen',
	'StringLower', 'StringMid', 'StringReplace', 'StringRight', 'StringSplit', 'StringTrimLeft',
	'StringTrimRight', 'StringUpper', 'StrLen', 'StrPut', 'StrReplace', 'StrSplit', 'SubStr',
	'Suspend', 'Switch', 'SysGet', 'Tan', 'Thread', 'ToolTip', 'Transform', 'TrayTip', 'Trim',
	'LTrim', 'RTrim', 'TV_Add', 'TV_Delete', 'TV_Get', 'TV_GetChild', 'TV_GetCount', 'TV_GetNext',
	'TV_GetParent', 'TV_GetPrev', 'TV_GetSelection', 'TV_GetText', 'TV_Modify', 'TV_SetImageList',
	'UrlDownloadToFile', 'VarSetCapacity', 'WinActivate', 'WinActivateBottom', 'WinActive',
	'WinClose', 'WinExist', 'WinGetActiveStats', 'WinGetActiveTitle', 'WinGetClass', 'WinGet',
	'WinGetPos', 'WinGetText', 'WinGetTitle', 'WinHide', 'WinKill', 'WinMaximize',
	'WinMenuSelectItem', 'WinMinimize', 'WinMinimizeAll', 'WinMinimizeAllUndo', 'WinMove',
	'WinRestore', 'WinSet', 'WinSetTitle', 'WinShow', 'WinWait', 'WinWaitActive', 'WinWaitNotActive',
	'WinWaitClose'
})

lex:set_word_list(lexer.PREPROCESSOR, {
	'ClipboardTimeout', 'CommentFlag', 'Delimiter', 'DerefChar', 'ErrorStdOut', 'EscapeChar',
	'HotkeyInterval', 'HotkeyModifierTimeout', 'Hotstring', 'If', 'IfTimeout', 'IfWinActive',
	'IfWinNotActive', 'IfWinExist', 'IfWinNotExist', 'Include', 'IncludeAgain', 'InputLevel',
	'InstallKeybdHook', 'InstallMouseHook', 'KeyHistory', 'LTrim', 'MaxHotkeysPerInterval', 'MaxMem',
	'MaxThreads', 'MaxThreadsBuffer', 'MaxThreadsPerHotkey', 'MenuMaskKey', 'NoEnv', 'NoTrayIcon',
	'Persistent', 'Requires', 'SingleInstance', 'UseHook', 'Warn', 'WinActivateForce'
})

lex:set_word_list(lexer.CONSTANT_BUILTIN, {
	'LButton', 'RButton', 'MButton', 'Advanced Buttons', 'XButton1', 'XButton2', 'Wheel', 'WheelDown',
	'WheelUp', 'WheelLeft', 'WheelRight', 'CapsLock', 'Space', 'Tab', 'Enter', 'Return', 'Esc',
	'Escape', 'BS', 'Backspace', 'ScrollLock', 'Del', 'Delete', 'Ins', 'Insert', 'Home', 'End',
	'PgUp', 'PgDn', 'Up', 'Down', 'Left', 'Right', 'Numpad0', 'NumpadIns', 'Numpad1', 'NumpadEnd',
	'Numpad2', 'NumpadDown', 'Numpad3', 'NumpadPgDn', 'Numpad4', 'NumpadLeft', 'Numpad5',
	'NumpadClear', 'Numpad6', 'NumpadRight', 'Numpad7', 'NumpadHome', 'Numpad8', 'NumpadUp',
	'Numpad9', 'NumpadPgUp', 'NumpadDot', 'NumpadDel', 'NumLock', 'NumpadDiv', 'NumpadMult',
	'NumpadAdd', 'NumpadSub', 'NumpadEnter', 'LWin', 'RWin', 'Ctrl', 'Control', 'Alt', 'Shift',
	'LCtrl', 'LControl', 'RCtrl', 'RControl', 'LShift', 'RShift', 'LAlt', 'RAlt', 'Browser_Back',
	'Browser_Forward', 'Browser_Refresh', 'Browser_Stop', 'Browser_Search', 'Browser_Favorites',
	'Browser_Home', 'Volume_Mute', 'Volume_Down', 'Volume_Up', 'Media_Next', 'Media_Prev',
	'Media_Stop', 'Media_Play_Pause', 'Launch_Mail', 'Launch_Media', 'Launch_App1', 'Launch_App2',
	'AppsKey', 'PrintScreen', 'CtrlBreak', 'Pause', 'Break', 'Help'
})

lex:set_word_list(lexer.VARIABLE_BUILTIN, {
	'Space', 'Tab', 'Args', 'WorkingDir', 'InitialWorkingDir', 'ScriptDir', 'ScriptName',
	'ScriptFullPath', 'ScriptHwnd', 'LineNumber', 'LineFile', 'ThisFunc', 'ThisLabel', 'AhkVersion',
	'AhkPath', 'IsUnicode', 'IsCompiled', 'ExitReason', 'Year', 'MM', 'DD', 'MMMM', 'MMM', 'DDDD',
	'DDD', 'WDay', 'YDay', 'YWeek', 'Hour', 'Min', 'Sec', 'MSec', 'Now', 'NowUTC', 'TickCount',
	'IsSuspended', 'IsPaused', 'IsCritical', 'BatchLines', 'ListLines', 'TitleMatchMode',
	'TitleMatchModeSpeed', 'DetectHiddenWindows', 'DetectHiddenText', 'AutoTrim', 'StringCaseSense',
	'FileEncoding', 'FormatInteger', 'FormatFloat', 'SendMode', 'SendLevel', 'StoreCapsLockMode',
	'KeyDelay', 'KeyDuration', 'KeyDelayPlay', 'KeyDurationPlay', 'WinDelay', 'ControlDelay',
	'MouseDelay', 'MouseDelayPlay', 'DefaultMouseSpeed', 'CoordModeToolTip', 'CoordModePixel',
	'CoordModeMouse', 'CoordModeCaret', 'CoordModeMenu', 'RegView', 'IconHidden', 'IconTip',
	'IconFile', 'IconNumber', 'TimeIdle', 'TimeIdlePhysical', 'TimeIdleKeyboard', 'TimeIdleMouse',
	'DefaultGui', 'DefaultListView', 'DefaultTreeView', 'Gui', 'GuiControl', 'GuiWidth', 'GuiHeight',
	'GuiX', 'GuiY', 'GuiEvent', 'GuiControlEvent', 'EventInfo', 'ThisMenuItem', 'ThisMenu',
	'ThisMenuItemPos', 'ThisHotkey', 'PriorHotkey', 'PriorKey', 'TimeSinceThisHotkey',
	'TimeSincePriorHotkey', 'EndChar', 'ComSpec', 'Temp', 'OSType', 'OSVersion', 'Is64bitOS',
	'PtrSize', 'Language', 'ComputerName', 'UserName', 'WinDir', 'ProgramFiles', 'AppData',
	'AppDataCommon', 'Desktop', 'DesktopCommon', 'StartMenu', 'StartMenuCommon', 'Programs',
	'ProgramsCommon', 'Startup', 'StartupCommon', 'MyDocuments', 'IsAdmin', 'ScreenWidth',
	'ScreenHeight', 'ScreenDPI', 'Cursor', 'CaretX', 'CaretY', 'Clipboard', 'LastError', 'Index',
	'LoopFileName', 'LoopRegName', 'LoopReadLine', 'LoopField'
})

lexer.property['scintillua.comment'] = ';'

-- Fold points.
lex:add_fold_point(lexer.OPERATOR, '{', '}')
lex:add_fold_point(lexer.COMMENT, '/*', '*/')

return lex
