#include <cstdint>
#include <libatombios/atom-debug.hpp>
#include <libatombios/extern-funcs.hpp>

#include "atom-private.hpp"

AtomBiosImpl::Command::Command(const libatombios_vector<uint8_t>& data, int index, uint16_t offset)
: _i{index}, _offset{static_cast<uint16_t>(offset + 0x6)} {
	memcpy(&commonHeader, data.data() + offset - 0x6, sizeof(CommonHeader));

	uint16_t infoShort = static_cast<uint16_t>(data[offset + sizeof(CommonHeader)]) |
			(static_cast<uint16_t>(data[offset + sizeof(CommonHeader) + 1]) << 8);
	workSpaceSize = infoShort & 0xFF;
	parameterSpaceSize = (infoShort >> 8) & 0x7F;
	updatedByUtility = (infoShort >> 15) & 1;

	size_t bytecodeLength = commonHeader.structureSize - 0x6;

	if(AtomBIOSDebugSettings::logCommandTableCreation) {
		lilrad_log(DEBUG, "command %02x: workSpaceSize=%i, parameterSpaceSize=%i, total size=0x%x, bytecode size=0x%zx\n",
			_i, workSpaceSize, parameterSpaceSize, commonHeader.structureSize, bytecodeLength);
	}

	_exists = true;
}

AtomBiosImpl::CommandTable::CommandTable()
: commands{frg::hash<int>()} {
}

void AtomBiosImpl::CommandTable::readCommands(const libatombios_vector<uint8_t>& data, uint16_t offset) {
	memcpy(&commonHeader, data.data() + offset, sizeof(CommonHeader));

	size_t offsetIntoTable = offset + sizeof(CommonHeader);
	for(int i = 0; offsetIntoTable < (offset + commonHeader.structureSize); i++, offsetIntoTable += 2) {
		uint16_t commandOffset = static_cast<uint16_t>(data[offsetIntoTable]) |
			(static_cast<uint16_t>(data[offsetIntoTable + 1]) << 8);
		if(commandOffset) {
			commands[i] = Command(data, i, commandOffset);
		}
	}
}

void AtomBiosImpl::runCommand(AtomBios::CommandTables table, libatombios_vector<uint32_t> params) {
	assert(_commandTable.commands[table].exists());
	_runBytecode(_commandTable.commands[table], params, 0);
}
