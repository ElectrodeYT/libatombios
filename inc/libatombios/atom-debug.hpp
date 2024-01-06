#pragma once

// Generic opcode loggers.
// Used by the interpreter.
#define LOG_OPCODE(name) \
	if(AtomBIOSDebugSettings::logOpcodes) { \
		libatombios_printf_dbg("opcode " name "(%s[%02x] %s (savedVal: %x) <- %s[%02x] %s (val: %x, newVal: %x))\n", \
			OpcodeArgEncodingToString(arg), dstIdx, SrcEncodingToString(attrByte.dstAlign), \
			saved, \
			OpcodeArgEncodingToString(attrByte.srcArg), srcIdx, SrcEncodingToString(attrByte.srcAlign), \
			val, newVal); \
	}

#define LOG_OPCODE_DST_ONLY(name) \
	if(AtomBIOSDebugSettings::logOpcodes) { \
		libatombios_printf_dbg("opcode " name "(%s[%02x] %s (savedVal: %x, val: %x, newVal: %x))\n", \
			OpcodeArgEncodingToString(arg), dstIdx, SrcEncodingToString(attrByte.dstAlign), \
			saved, val, newVal); \
	}

#define LOG_FLAGS() \
if(AtomBIOSDebugSettings::logOpcodes) { \
	libatombios_printf_dbg("  flags after opcode: A%i E%i B%i\n", _flagAbove, _flagEqual, _flagBelow); \
}

namespace AtomBIOSDebugSettings {
	constexpr bool logOpcodes = true;
}
