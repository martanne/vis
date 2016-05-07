-- Copyright 2016 Christian Hesse
-- systemd networkd file LPeg lexer.

local l = require('lexer')
local token, word_match = l.token, l.word_match
local P, R, S = lpeg.P, lpeg.R, lpeg.S

local M = {_NAME = 'networkd'}

-- Whitespace.
local ws = token(l.WHITESPACE, l.space^1)

-- Comments.
local comment = token(l.COMMENT, l.starts_line(S(';#')) * l.nonnewline^0)

-- Strings.
local sq_str = l.delimited_range("'")
local dq_str = l.delimited_range('"')
local section_word = word_match{
  'Address',
  'Link',
  'Match',
  'Network',
  'Route',
  'DHCP',
  'DHCPServer',
  'Bridge',
  'BridgeFDB',
  'NetDev',
  'VLAN',
  'MACVLAN',
  'MACVTAP',
  'IPVLAN',
  'VXLAN',
  'Tunnel',
  'Peer',
  'Tun',
  'Tap',
  'Bond'
}
local string = token(l.STRING, sq_str + dq_str + '[' * section_word * ']')

-- Numbers.
local dec = l.digit^1 * ('_' * l.digit^1)^0
local oct_num = '0' * S('01234567_')^1
local integer = S('+-')^-1 * (l.hex_num + oct_num + dec)
local number = token(l.NUMBER, (l.float + integer))

-- Keywords.
local keyword = token(l.KEYWORD, word_match{
  -- boolean values
  'true',
  'false',
  'on',
  'off',
  'yes',
  'no',
})

-- Options.
local option_word = word_match{
  -- match section options
  'MACAddress',
  'OriginalName',
  'Path',
  'Driver',
  'Type',
  'Host',
  'Name',
  'Virtualization',
  'KernelCommandLine',
  'Architecture',

  -- link section options
  'Description',
  'Alias',
  'MACAddressPolicy',
  'MACAddress',
  'NamePolicy',
  'Name',
  'MTUBytes',
  'BitsPerSecond',
  'Duplex',
  'WakeOnLan',

  -- network section options
  'Description',
  'DHCP',
  'DHCPServer',
  'LinkLocalAddressing',
  'IPv4LLRoute',
  'IPv6Token',
  'LLMNR',
  'MulticastDNS',
  'DNSSEC',
  'DNSSECNegativeTrustAnchors',
  'LLDP',
  'BindCarrier',
  'Address',
  'Gateway',
  'DNS',
  'Domains',
  'NTP',
  'IPForward',
  'IPMasquerade',
  'IPv6PrivacyExtensions',
  'IPv6AcceptRouterAdvertisements',
  'IPv6DuplicateAddressDetection',
  'IPv6HopLimit',
  'Bridge',
  'Bond',
  'VLAN',
  'MACVLAN',
  'VXLAN',
  'Tunnel',

  -- address section options
  'Address',
  'Peer',
  'Broadcast',
  'Label',

  -- route section options
  'Gateway',
  'Destination',
  'Source',
  'Metric',
  'Scope',
  'PreferredSource',

  -- dhcp section options
  'UseDNS',
  'UseNTP',
  'UseMTU',
  'SendHostname',
  'UseHostname',
  'Hostname',
  'UseDomains',
  'UseRoutes',
  'UseTimezone',
  'CriticalConnection',
  'ClientIdentifier',
  'VendorClassIdentifier',
  'RequestBroadcast',
  'RouteMetric',

  -- dhcpserver section options
  'PoolOffset',
  'PoolSize',
  'DefaultLeaseTimeSec',
  'MaxLeaseTimeSec',
  'EmitDNS',
  'DNS',
  'EmitNTP',
  'NTP',
  'EmitTimezone',
  'Timezone',

  -- bridge section options
  'UnicastFlood',
  'HairPin',
  'UseBPDU',
  'FastLeave',
  'AllowPortToBeRoot',
  'Cost',

  -- bridgefdb section options
  'MACAddress',
  'VLANId',

  -- netdev section options
  'Description',
  'Name',
  'Kind',
  'MTUBytes',
  'MACAddress',

  -- bridge (netdev) section options
  'HelloTimeSec',
  'MaxAgeSec',
  'ForwardDelaySec',

  -- vlan section options
  'Id',

  -- macvlan, macvtap and ipvlan section options
  'Mode',

  -- vxlan section options
  'Id',
  'Group',
  'TOS',
  'TTL',
  'MacLearning',
  'FDBAgeingSec',
  'MaximumFDBEntries',
  'ARPProxy',
  'L2MissNotification',
  'L3MissNotification',
  'RouteShortCircuit',
  'UDPCheckSum',
  'UDP6ZeroChecksumTx',
  'UDP6ZeroCheckSumRx',
  'GroupPolicyExtension',
  'DestinationPort',
  'PortRange',

  -- tunnel section options
  'Local',
  'Remote',
  'TOS',
  'TTL',
  'DiscoverPathMTU',
  'IPv6FlowLabel',
  'CopyDSCP',
  'EncapsulationLimit',
  'Mode',

  -- peer section options
  'Name',
  'MACAddress',

  -- tun and tap section options
  'OneQueue',
  'MultiQueue',
  'PacketInfo',
  'VNetHeader',
  'User',
  'Group',

  -- bond section options
  'Mode',
  'TransmitHashPolicy',
  'LACPTransmitRate',
  'MIIMonitorSec',
  'UpDelaySec',
  'DownDelaySec',
  'LearnPacketIntervalSec',
  'AdSelect',
  'FailOverMACPolicy',
  'ARPValidate',
  'ARPIntervalSec',
  'ARPIPTargets',
  'ARPAllTargets',
  'PrimaryReselectPolicy',
  'ResendIGMP',
  'PacketsPerSlave',
  'GratuitousARP',
  'AllSlavesActive',
  'MinLinks',
}
local preproc = token(l.PREPROCESSOR, option_word)

-- Identifiers.
local word = (l.alpha + '_') * (l.alnum + S('_.'))^0
local identifier = token(l.IDENTIFIER, word)

-- Operators.
local operator = token(l.OPERATOR, '=')

M._rules = {
  {'whitespace', ws},
  {'keyword', keyword},
  {'string', string},
  {'preproc', preproc},
  {'identifier', identifier},
  {'comment', comment},
  {'number', number},
  {'operator', operator},
}

M._LEXBYLINE = true

return M
