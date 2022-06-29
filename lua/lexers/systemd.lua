-- Copyright 2016-2022 Christian Hesse. See LICENSE.
-- systemd unit file LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('systemd', {lex_by_line = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- Boolean values.
  'true', 'false', 'on', 'off', 'yes', 'no',
  -- Service types.
  'forking', 'simple', 'oneshot', 'dbus', 'notify', 'idle',
  -- Special system units.
  'basic.target', 'ctrl-alt-del.target', 'cryptsetup.target', 'dbus.service', 'dbus.socket',
  'default.target', 'display-manager.service', 'emergency.target', 'exit.target', 'final.target',
  'getty.target', 'graphical.target', 'hibernate.target', 'hybrid-sleep.target', 'halt.target',
  'initrd-fs.target', 'kbrequest.target', 'kexec.target', 'local-fs.target', 'multi-user.target',
  'network-online.target', 'paths.target', 'poweroff.target', 'reboot.target', 'remote-fs.target',
  'rescue.target', 'initrd-root-fs.target', 'runlevel2.target', 'runlevel3.target',
  'runlevel4.target', 'runlevel5.target', 'shutdown.target', 'sigpwr.target', 'sleep.target',
  'slices.target', 'sockets.target', 'suspend.target', 'swap.target', 'sysinit.target',
  'syslog.socket', 'system-update.target', 'timers.target', 'umount.target',
  -- Special system units for devices.
  'bluetooth.target', 'printer.target', 'smartcard.target', 'sound.target',
  -- Special passive system units.
  'cryptsetup-pre.target', 'local-fs-pre.target', 'network.target', 'network-pre.target',
  'nss-lookup.target', 'nss-user-lookup.target', 'remote-fs-pre.target', 'rpcbind.target',
  'time-sync.target',
  -- Specail slice units.
  '-.slice', 'system.slice', 'user.slice', 'machine.slice',
  -- Environment variables.
  'PATH', 'LANG', 'USER', 'LOGNAME', 'HOME', 'SHELL', 'XDG_RUNTIME_DIR', 'XDG_SESSION_ID',
  'XDG_SEAT', 'XDG_VTNR', 'MAINPID', 'MANAGERPID', 'LISTEN_FDS', 'LISTEN_PID', 'LISTEN_FDNAMES',
  'NOTIFY_SOCKET', 'WATCHDOG_PID', 'WATCHDOG_USEC', 'TERM'
}))

-- Options.
lex:add_rule('option', token(lexer.PREPROCESSOR, word_match{
  -- Unit section.
  'Description', 'Documentation', 'Requires', 'Requisite', 'Wants', 'BindsTo', 'PartOf',
  'Conflicts', 'Before', 'After', 'OnFailure', 'PropagatesReloadTo', 'ReloadPropagatedFrom',
  'JoinsNamespaceOf', 'RequiresMountsFor', 'OnFailureJobMode', 'IgnoreOnIsolate',
  'StopWhenUnneeded', 'RefuseManualStart', 'RefuseManualStop', 'AllowIsolate',
  'DefaultDependencies', 'JobTimeoutSec', 'JobTimeoutAction', 'JobTimeoutRebootArgument',
  'StartLimitInterval', 'StartLimitBurst', 'StartLimitAction', 'RebootArgument',
  'ConditionArchitecture', 'ConditionVirtualization', 'ConditionHost', 'ConditionKernelCommandLine',
  'ConditionSecurity', 'ConditionCapability', 'ConditionACPower', 'ConditionNeedsUpdate',
  'ConditionFirstBoot', 'ConditionPathExists', 'ConditionPathExistsGlob',
  'ConditionPathIsDirectory', 'ConditionPathIsSymbolicLink', 'ConditionPathIsMountPoint',
  'ConditionPathIsReadWrite', 'ConditionDirectoryNotEmpty', 'ConditionFileNotEmpty',
  'ConditionFileIsExecutable', 'AssertArchitecture', 'AssertVirtualization', 'AssertHost',
  'AssertKernelCommandLine', 'AssertSecurity', 'AssertCapability', 'AssertACPower',
  'AssertNeedsUpdate', 'AssertFirstBoot', 'AssertPathExists', 'AssertPathExistsGlob',
  'AssertPathIsDirectory', 'AssertPathIsSymbolicLink', 'AssertPathIsMountPoint',
  'AssertPathIsReadWrite', 'AssertDirectoryNotEmpty', 'AssertFileNotEmpty',
  'AssertFileIsExecutable', 'SourcePath',
  -- Install section.
  'Alias', 'WantedBy', 'RequiredBy', 'Also', 'DefaultInstance',
  -- Service section.
  'Type', 'RemainAfterExit', 'GuessMainPID', 'PIDFile', 'BusName', 'BusPolicy', 'ExecStart',
  'ExecStartPre', 'ExecStartPost', 'ExecReload', 'ExecStop', 'ExecStopPost', 'RestartSec',
  'TimeoutStartSec', 'TimeoutStopSec', 'TimeoutSec', 'RuntimeMaxSec', 'WatchdogSec', 'Restart',
  'SuccessExitStatus', 'RestartPreventExitStatus', 'RestartForceExitStatus', 'PermissionsStartOnly',
  'RootDirectoryStartOnly', 'NonBlocking', 'NotifyAccess', 'Sockets', 'FailureAction',
  'FileDescriptorStoreMax', 'USBFunctionDescriptors', 'USBFunctionStrings',
  -- Socket section.
  'ListenStream', 'ListenDatagram', 'ListenSequentialPacket', 'ListenFIFO', 'ListenSpecial',
  'ListenNetlink', 'ListenMessageQueue', 'ListenUSBFunction', 'SocketProtocol', 'BindIPv6Only',
  'Backlog', 'BindToDevice', 'SocketUser', 'SocketGroup', 'SocketMode', 'DirectoryMode', 'Accept',
  'Writable', 'MaxConnections', 'KeepAlive', 'KeepAliveTimeSec', 'KeepAliveIntervalSec',
  'KeepAliveProbes', 'NoDelay', 'Priority', 'DeferAcceptSec', 'ReceiveBuffer', 'SendBuffer',
  'IPTOS', 'IPTTL', 'Mark', 'ReusePort', 'SmackLabel', 'SmackLabelIPIn', 'SmackLabelIPOut',
  'SELinuxContextFromNet', 'PipeSize', 'MessageQueueMaxMessages', 'MessageQueueMessageSize',
  'FreeBind', 'Transparent', 'Broadcast', 'PassCredentials', 'PassSecurity', 'TCPCongestion',
  'ExecStartPre', 'ExecStartPost', 'ExecStopPre', 'ExecStopPost', 'TimeoutSec', 'Service',
  'RemoveOnStop', 'Symlinks', 'FileDescriptorName',
  -- Mount section.
  'What', 'Where', 'Type', 'Options', 'SloppyOptions', 'DirectoryMode', 'TimeoutSec',
  -- Path section.
  'PathExists', 'PathExistsGlob', 'PathChanged', 'PathModified', 'DirectoryNotEmpty', 'Unit',
  'MakeDirectory', 'DirectoryMode',
  -- Timer section.
  'OnActiveSec', 'OnBootSec', 'OnStartupSec', 'OnUnitActiveSec', 'OnUnitInactiveSec', 'OnCalendar',
  'AccuracySec', 'RandomizedDelaySec', 'Unit', 'Persistent', 'WakeSystem', 'RemainAfterElapse',
  -- Exec section.
  'WorkingDirectory', 'RootDirectory', 'User', 'Group', 'SupplementaryGroups', 'Nice',
  'OOMScoreAdjust', 'IOSchedulingClass', 'IOSchedulingPriority', 'CPUSchedulingPolicy',
  'CPUSchedulingPriority', 'CPUSchedulingResetOnFork', 'CPUAffinity', 'UMask', 'Environment',
  'EnvironmentFile', 'PassEnvironment', 'StandardInput', 'StandardOutput', 'StandardError',
  'TTYPath', 'TTYReset', 'TTYVHangup', 'TTYVTDisallocate', 'SyslogIdentifier', 'SyslogFacility',
  'SyslogLevel', 'SyslogLevelPrefix', 'TimerSlackNSec', 'LimitCPU', 'LimitFSIZE', 'LimitDATA',
  'LimitSTACK', 'LimitCORE', 'LimitRSS', 'LimitNOFILE', 'LimitAS', 'LimitNPROC', 'LimitMEMLOCK',
  'LimitLOCKS', 'LimitSIGPENDING', 'LimitMSGQUEUE', 'LimitNICE', 'LimitRTPRIO', 'LimitRTTIME',
  'PAMName', 'CapabilityBoundingSet', 'AmbientCapabilities', 'SecureBits', 'Capabilities',
  'ReadWriteDirectories', 'ReadOnlyDirectories', 'InaccessibleDirectories', 'PrivateTmp',
  'PrivateDevices', 'PrivateNetwork', 'ProtectSystem', 'ProtectHome', 'MountFlags',
  'UtmpIdentifier', 'UtmpMode', 'SELinuxContext', 'AppArmorProfile', 'SmackProcessLabel',
  'IgnoreSIGPIPE', 'NoNewPrivileges', 'SystemCallFilter', 'SystemCallErrorNumber',
  'SystemCallArchitectures', 'RestrictAddressFamilies', 'Personality', 'RuntimeDirectory',
  'RuntimeDirectoryMode'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + '_') * (lexer.alnum + S('_.'))^0))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Sections.
lex:add_rule('section', token(lexer.LABEL, '[' *
  word_match('Automount BusName Install Mount Path Service Service Socket Timer Unit') * ']'))

-- Comments.
lex:add_rule('comment', token(lexer.COMMENT, lexer.starts_line(lexer.to_eol(S(';#')))))

-- Numbers.
local dec = lexer.digit^1 * ('_' * lexer.digit^1)^0
local oct_num = '0' * S('01234567_')^1
local integer = S('+-')^-1 * (lexer.hex_num + oct_num + dec)
lex:add_rule('number', token(lexer.NUMBER, lexer.float + integer))

-- Operators.
lex:add_rule('operator', token(lexer.OPERATOR, '='))

return lex
