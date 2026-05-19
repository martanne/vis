-- Copyright 2026 Jamie Drinkell. See LICENSE.
-- Arduino LPeg lexer.
-- Reference: https://docs.arduino.cc/language-reference/

local lexer = lexer
local lex = lexer.new(..., {inherit = lexer.load('c')})

lex:set_word_list(lexer.KEYWORD, {
	-- Additional C++ Keywords supported by Arduino.
	'catch', 'class', 'const_cast', 'delete', 'dynamic_cast', 'explicit', 'export', 'friend',
	'mutable', 'namespace', 'new', 'operator', 'private', 'protected', 'public', 'reinterpret_cast',
	'static_cast', 'template', 'this', 'throw', 'try', 'typeid', 'typename', 'using', 'virtual',
	'and', 'not', 'or', 'xor', -- Operators
	'final', 'override', -- C++11.
	'PROGMEM' -- Arduino
}, true)

lex:set_word_list(lexer.TYPE, 'byte word String', true)

lex:set_word_list(lexer.FUNCTION_BUILTIN, {
	-- I/O
	'digitalRead', 'digitalWrite', 'pinMode', 'analogRead', 'analogReadResolution', 'analogReference',
	'analogWrite', 'analogWriteResolution',
	-- Adv I/O
	'noTone', 'pulseIn', 'pulseInLong', 'shiftIn', 'shiftOut', 'tone',
	-- Time
	'delay', 'delayMicroseconds', 'micros', 'millis',
	-- Maths (most are covered by cmath, just adding the missing ones)
	'abs', 'constrain', 'map', 'max', 'min', 'pow', 'sq', 'exp', 'bit',
	-- Characters
	'isAlpha', 'isAlphaNumeric', 'isAscii', 'isControl', 'isDigit', 'isGraph', 'isHexadecimalDigit',
	'isLowerCase', 'isPrintable', 'isPunct', 'isSpace', 'isUpperCase', 'isWhitespace',
	-- Random
	'random', 'randomSeed',
	-- Bits/Bytes
	'bit', 'bitClear', 'bitRead', 'bitSet', 'bitWrite', 'highByte', 'lowByte',
	-- Interrupts
	'attachInterrupt', 'detachInterrupt', 'digitalPinToInterrupt', 'interrupts', 'noInterrupts',
	-- Serial
	'Serial.available', 'Serial.availableForWrite', 'Serial.begin', 'Serial.end', 'Serial.find',
	'Serial.findUntil', 'Serial.flush', 'Serial.parseFloat', 'Serial.parseInt', 'Serial.peek',
	'Serial.print', 'Serial.println', 'Serial.read', 'Serial.readBytes', 'Serial.readBytesUntil',
	'Serial.readString', 'Serial.readStringUntil', 'Serial.setTimeout', 'Serial.write',
	'Serial.serialEvent',
	-- EEPROM
	'EEPROM.read', 'EEPROM.write', 'EEPROM.update', 'EEPROM.get', 'EEPROM.put', 'EEPROM.length',
	-- SPI
	'SPISettings', 'SPI.begin', 'SPI.end', 'SPI.beginTransaction', 'SPI.endTransaction',
	'SPI.usingInterrupt', 'SPI.transfer',
	-- Wire
	'Wire.begin', 'Wire.end', 'Wire.setClock', 'Wire.beginTransmission', 'Wire.write',
	'Wire.endTransmission', 'Wire.requestFrom', 'Wire.available', 'Wire.read', 'Wire.onReceive',
	'Wire.onRequest', 'Wire.setWireTimeout', 'Wire.getWireTimeoutFlag', 'Wire.clearWireTimeoutFlag',
	-- Mouse
	'Mouse.begin', 'Mouse.click', 'Mouse.end', 'Mouse.move', 'Mouse.press', 'Mouse.release',
	'Mouse.isPressed',
	-- Keyboard
	'Keyboard.begin', 'Keyboard.end', 'Keyboard.press', 'Keyboard.print', 'Keyboard.println',
	'Keyboard.release', 'Keyboard.releaseAll', 'Keyboard.write',
	-- WiFi
	-- WiFiClient and WiFiServer require new objects to be made and are hence ommitted
	'WiFi.begin', 'WiFi.disconnect', 'WiFi.config', 'WiFi.setDNS', 'WiFi.SSID', 'WiFi.BSSID',
	'WiFi.RSSI', 'WiFi.encryptionType', 'WiFi.scanNetworks', 'WiFi.status', 'WiFi.getSocket',
	'WiFi.macAddress', 'IPAddress.localIP', 'IPAddress.subnetMask', 'IPAddress.gatewayIP',
	'WiFiUDP.begin', 'WiFiUDP.available', 'WiFiUDP.beginPacket', 'WiFiUDP.endPacket', 'WiFiUDP.write',
	'WiFiUDP.parsePacket', 'WiFiUDP.peek', 'WiFiUDP.read', 'WiFiUDP.flush', 'WiFiUDP.stop',
	'WiFiUDP.remoteIP', 'WiFiUDP.remotePort',
	-- BLE
	'BLE.begin', 'BLE.end', 'BLE.poll', 'BLE.setEventHandler', 'BLE.connected', 'BLE.disconnect',
	'BLE.address', 'BLE.rssi', 'BLE.setAdvertisedServiceUuid', 'BLE.setAdvertisedService',
	'BLE.setManufacturerData', 'BLE.setLocalName', 'BLE.setDeviceName', 'BLE.setAppearance',
	'BLE.addService', 'BLE.advertise', 'BLE.stopAdvertise', 'BLE.central',
	'BLE.setAdvertisingInterval', 'BLE.setConnectionInterval', 'BLE.setConnectable', 'BLE.scan',
	'BLE.scanForName', 'BLE.scanForAddress', 'BLE.scanForUuid', 'BLE.stopScan', 'BLE.available'
}, true)

lex:set_word_list(lexer.CONSTANT_BUILTIN, 'HIGH LOW INPUT INPUT_PULLUP OUTPUT LED_BUILTIN', true)

lexer.property['scintillua.comment'] = '//'

return lex
