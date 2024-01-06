#include <cstdio>
#include <cstring>
#include <cassert>

#include <libatombios/atom.hpp>
#include <libatombios/extern-funcs.hpp>

AtomBios::Command::Command(const std::vector<uint8_t>& data, int index, uint16_t offset)
: _i{index}, _offset{offset} {
	memcpy(&commonHeader, data.data() + offset, sizeof(CommonHeader));

	uint16_t infoShort = static_cast<uint16_t>(data[offset + sizeof(CommonHeader)]) |
			(static_cast<uint16_t>(data[offset + sizeof(CommonHeader) + 1]) << 8);
	workSpaceSize = infoShort & 0xFF;
	parameterSpaceSize = (infoShort >> 8) & 0x7F;
	updatedByUtility = (infoShort >> 15) & 1;

	size_t bytecodeLength = commonHeader.structureSize - 0x6;
	_bytecode.resize(bytecodeLength);
	memcpy(_bytecode.data(), data.data() + offset + 0x6, bytecodeLength);

	//libatombios_printf_dbg("command %i: workSpaceSize=%i, parameterSpaceSize=%i, total size=0x%x, bytecode size=0x%x\n",
	//	_i, workSpaceSize, parameterSpaceSize, commonHeader.structureSize, bytecodeLength);
}

void AtomBios::CommandTable::readCommands(const std::vector<uint8_t>& data, uint16_t offset) {
	memcpy(&commonHeader, data.data() + offset, sizeof(CommonHeader));

	size_t offsetIntoTable = offset + sizeof(CommonHeader);
	for(int i = 0; offsetIntoTable < (offset + commonHeader.structureSize); i++, offsetIntoTable += 2) {
		uint16_t commandOffset = static_cast<uint16_t>(data[offsetIntoTable]) |
			(static_cast<uint16_t>(data[offsetIntoTable + 1]) << 8);
		if(commandOffset) {
			commands[i] = std::make_shared<Command>(data, i, commandOffset);
		}
	}
}

void AtomBios::runCommand(CommandTables table, std::vector<uint32_t> params) {
	assert(_commandTable.commands[table]);
	_runBytecode(_commandTable.commands[table], params, 0);
}
