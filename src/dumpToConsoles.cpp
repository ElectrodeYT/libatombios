#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <libatombios/atom.hpp>
#include <libatombios/extern-funcs.hpp>

void AtomBios::CommonHeader::dumpToConsole(bool header) {
	if(header) {
		lilrad_log(DEBUG, "CommonHeader dump:");
	}
	lilrad_log(DEBUG, "  commonHeader.structureSize = 0x%x\n", structureSize);
	lilrad_log(DEBUG, "  commonHeader.tableFormatRevision = %i\n", tableFormatRevision);
	lilrad_log(DEBUG, "  commonHeader.tableContentRevision = %i\n", tableContentRevision);
}

void AtomBios::AtomRomTable::dumpToConsole() {
	lilrad_log(DEBUG, "AtomRomTable dump:\n");
	commonHeader.dumpToConsole();
	lilrad_log(DEBUG, "  biosRuntimeSegmentAddress = 0x%x\n", biosRuntimeSegmentAddress);
	lilrad_log(DEBUG, "  protectedModeInfoOffset = 0x%x\n", protectedModeInfoOffset);
	lilrad_log(DEBUG, "  configFilenameOffset = 0x%x\n", configFilenameOffset);
	lilrad_log(DEBUG, "  nameStringOffset = 0x%x\n", nameStringOffset);
	lilrad_log(DEBUG, "  int10Offset = 0x%x\n", int10Offset);
	lilrad_log(DEBUG, "  pciBusDeviceInitCode = 0x%x\n", pciBusDeviceInitCode);
	lilrad_log(DEBUG, "  ioBaseAddress = 0x%x\n", ioBaseAddress);
	lilrad_log(DEBUG, "  subsystemVendorID = 0x%x\n", subsystemVendorID);
	lilrad_log(DEBUG, "  subsystemID = 0x%x\n", subsystemID);
	lilrad_log(DEBUG, "  pciInfoOffset = 0x%x\n", pciInfoOffset);
	lilrad_log(DEBUG, "  commandTableBase = 0x%x\n", commandTableBase);
	lilrad_log(DEBUG, "  dataTableBase = 0x%x\n", dataTableBase);
	lilrad_log(DEBUG, "  extendedFunctionCode = 0x%x\n", extendedFunctionCode);
	lilrad_log(DEBUG, "  reserved = 0x%x\n", reserved);
}
