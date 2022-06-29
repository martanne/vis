-- Copyright 2016-2022 Christian Hesse. See LICENSE.
-- systemd networkd file LPeg lexer.

local lexer = require('lexer')
local token, word_match = lexer.token, lexer.word_match
local P, S = lpeg.P, lpeg.S

local lex = lexer.new('networkd', {lex_by_line = true})

-- Whitespace.
lex:add_rule('whitespace', token(lexer.WHITESPACE, lexer.space^1))

-- Keywords.
lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
  -- Boolean values.
  'true', 'false', 'on', 'off', 'yes', 'no'
}))

-- Options.
lex:add_rule('option', token(lexer.PREPROCESSOR, word_match{
  -- Match section.
  'MACAddress', 'OriginalName', 'Path', 'Driver', 'Type', 'Host', 'Name', 'Virtualization',
  'KernelCommandLine', 'Architecture',
  -- Link section.
  'Description', 'Alias', 'MACAddressPolicy', 'MACAddress', 'NamePolicy', 'Name', 'MTUBytes',
  'BitsPerSecond', 'Duplex', 'WakeOnLan',
  -- Network section.
  'Description', 'DHCP', 'DHCPServer', 'LinkLocalAddressing', 'IPv4LLRoute', 'IPv6Token', 'LLMNR',
  'MulticastDNS', 'DNSSEC', 'DNSSECNegativeTrustAnchors', 'LLDP', 'BindCarrier', 'Address',
  'Gateway', 'DNS', 'Domains', 'NTP', 'IPForward', 'IPMasquerade', 'IPv6PrivacyExtensions',
  'IPv6AcceptRouterAdvertisements', 'IPv6DuplicateAddressDetection', 'IPv6HopLimit', 'Bridge',
  'Bond', 'VLAN', 'MACVLAN', 'VXLAN', 'Tunnel',
  -- Address section.
  'Address', 'Peer', 'Broadcast', 'Label',
  -- Route section.
  'Gateway', 'Destination', 'Source', 'Metric', 'Scope', 'PreferredSource',
  -- DHCP section.
  'UseDNS', 'UseNTP', 'UseMTU', 'SendHostname', 'UseHostname', 'Hostname', 'UseDomains',
  'UseRoutes', 'UseTimezone', 'CriticalConnection', 'ClientIdentifier', 'VendorClassIdentifier',
  'RequestBroadcast', 'RouteMetric',
  -- DHCPServer section.
  'PoolOffset', 'PoolSize', 'DefaultLeaseTimeSec', 'MaxLeaseTimeSec', 'EmitDNS', 'DNS', 'EmitNTP',
  'NTP', 'EmitTimezone', 'Timezone',
  -- Bridge section.
  'UnicastFlood', 'HairPin', 'UseBPDU', 'FastLeave', 'AllowPortToBeRoot', 'Cost',
  -- BridgeFDP section.
  'MACAddress', 'VLANId',
  -- NetDev section.
  'Description', 'Name', 'Kind', 'MTUBytes', 'MACAddress',
  -- Bridge (netdev) section.
  'HelloTimeSec', 'MaxAgeSec', 'ForwardDelaySec',
  -- VLAN section.
  'Id',
  -- MACVLAN MACVTAP and IPVLAN section.
  'Mode',
  -- VXLAN section.
  'Id', 'Group', 'TOS', 'TTL', 'MacLearning', 'FDBAgeingSec', 'MaximumFDBEntries', 'ARPProxy',
  'L2MissNotification', 'L3MissNotification', 'RouteShortCircuit', 'UDPCheckSum',
  'UDP6ZeroChecksumTx', 'UDP6ZeroCheckSumRx', 'GroupPolicyExtension', 'DestinationPort',
  'PortRange',
  -- Tunnel section.
  'Local', 'Remote', 'TOS', 'TTL', 'DiscoverPathMTU', 'IPv6FlowLabel', 'CopyDSCP',
  'EncapsulationLimit', 'Mode',
  -- Peer section.
  'Name', 'MACAddress',
  -- Tun and Tap section.
  'OneQueue', 'MultiQueue', 'PacketInfo', 'VNetHeader', 'User', 'Group',
  -- Bond section.
  'Mode', 'TransmitHashPolicy', 'LACPTransmitRate', 'MIIMonitorSec', 'UpDelaySec', 'DownDelaySec',
  'LearnPacketIntervalSec', 'AdSelect', 'FailOverMACPolicy', 'ARPValidate', 'ARPIntervalSec',
  'ARPIPTargets', 'ARPAllTargets', 'PrimaryReselectPolicy', 'ResendIGMP', 'PacketsPerSlave',
  'GratuitousARP', 'AllSlavesActive', 'MinLinks'
}))

-- Identifiers.
lex:add_rule('identifier', token(lexer.IDENTIFIER, (lexer.alpha + '_') * (lexer.alnum + S('_.'))^0))

-- Strings.
local sq_str = lexer.range("'")
local dq_str = lexer.range('"')
lex:add_rule('string', token(lexer.STRING, sq_str + dq_str))

-- Sections.
lex:add_rule('section', token(lexer.LABEL, '[' * word_match{
  'Address', 'Link', 'Match', 'Network', 'Route', 'DHCP', 'DHCPServer', 'Bridge', 'BridgeFDB',
  'NetDev', 'VLAN', 'MACVLAN', 'MACVTAP', 'IPVLAN', 'VXLAN', 'Tunnel', 'Peer', 'Tun', 'Tap', 'Bond'
} * ']'))

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
