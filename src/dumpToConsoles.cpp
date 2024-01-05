#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <libatombios/atom.hpp>
#include <libatombios/extern-funcs.hpp>

void AtomBios::CommonHeader::dumpToConsole(bool header) {
	if(header) {
		libatombios_printf_dbg("CommonHeader dump:");
	}
	libatombios_printf_dbg("  commonHeader.structureSize = 0x%x\n", structureSize);
	libatombios_printf_dbg("  commonHeader.tableFormatRevision = %i\n", tableFormatRevision);
	libatombios_printf_dbg("  commonHeader.tableContentRevision = %i\n", tableContentRevision);
}

void AtomBios::AtomRomTable::dumpToConsole() {
	libatombios_printf_dbg("AtomRomTable dump:\n");
	commonHeader.dumpToConsole();
	libatombios_printf_dbg("  biosRuntimeSegmentAddress = 0x%x\n", biosRuntimeSegmentAddress);
	libatombios_printf_dbg("  protectedModeInfoOffset = 0x%x\n", protectedModeInfoOffset);
	libatombios_printf_dbg("  configFilenameOffset = 0x%x\n", configFilenameOffset);
	libatombios_printf_dbg("  nameStringOffset = 0x%x\n", nameStringOffset);
	libatombios_printf_dbg("  int10Offset = 0x%x\n", int10Offset);
	libatombios_printf_dbg("  pciBusDeviceInitCode = 0x%x\n", pciBusDeviceInitCode);
	libatombios_printf_dbg("  ioBaseAddress = 0x%x\n", ioBaseAddress);
	libatombios_printf_dbg("  subsystemVendorID = 0x%x\n", subsystemVendorID);
	libatombios_printf_dbg("  subsystemID = 0x%x\n", subsystemID);
	libatombios_printf_dbg("  pciInfoOffset = 0x%x\n", pciInfoOffset);
	libatombios_printf_dbg("  commandTableBase = 0x%x\n", commandTableBase);
	libatombios_printf_dbg("  dataTableBase = 0x%x\n", dataTableBase);
	libatombios_printf_dbg("  extendedFunctionCode = 0x%x\n", extendedFunctionCode);
	libatombios_printf_dbg("  reserved = 0x%x\n", reserved);
}


extern "C" void libatombios_printf_panic(const char* format, ...) {
	va_list arglist;
	va_start(arglist, format);

	static char buffer[1024];
	static const char prefix[] = "libatombios PANIC: ";
	strcpy(buffer, prefix);
	strncat(buffer, format, 1023 - strlen(prefix));
	vprintf(buffer, arglist);

	va_end(arglist);

	exit(-1);
}
