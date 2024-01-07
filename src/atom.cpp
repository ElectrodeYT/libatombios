#include <cstdio>
#include <cstring>
#include <cassert>

#include <libatombios/atom.hpp>
#include <libatombios/atom-debug.hpp>
#include <libatombios/extern-funcs.hpp>

AtomBios::AtomBios(const std::vector<uint8_t>& data)
: _data{data} {
	// Verify the BIOS magic.
	{
		uint16_t biosMagic = read16(0);
		libatombios_printf_dbg("biosMagic is %x\n", biosMagic);
		assert(biosMagic == 0xAA55);
	}

	// Verify the ATI magic.
	{
		char* atiMagic = new char[11];
		memcpy(atiMagic, _data.data() + 0x30, 10);
		atiMagic[10] = '\0';
		libatombios_printf_dbg("ATI Magic is %x\n", atiMagic);
		assert(strcmp(atiMagic, " 761295520") == 0);
		delete[] atiMagic;
	}

	_atomRomTableBase = read16(0x48);
	libatombios_printf_dbg("Atom ROM Table Base is %x\n", _atomRomTableBase);
	
	// Copy the Atom ROM Table.
	{
		memset(&_atomRomTable, 0, sizeof(AtomRomTable));
		copyStructure(&_atomRomTable, _atomRomTableBase, sizeof(AtomRomTable));
		_atomRomTable.dumpToConsole();
	}

	// Verify the Atom ROM Table Magic.
	{
		char* atomRomTableMagic = new char[5];
		memcpy(atomRomTableMagic, _atomRomTable.atomMagic, 4);
		atomRomTableMagic[4] = '\0';
		libatombios_printf_dbg("Atom ROM Table Magic is %s\n", atomRomTableMagic);
		assert(strcmp(atomRomTableMagic, "ATOM") == 0);
		delete[] atomRomTableMagic;
	}

	// Copy the data table.
	libatombios_printf_dbg("Atom Data Table Base is %x\n", _atomRomTable.dataTableBase);
	copyStructure(&_dataTable, _atomRomTable.dataTableBase, sizeof(DataTable));

	// Initialize the command table.
	_commandTable.readCommands(_data, _atomRomTable.commandTableBase);

	// Index the IIO commands.
	_indexIIO(_dataTable.indirectIOAccess + 4);
}

void AtomBios::copyStructure(void* dest, size_t offset, size_t maxSize) {
	size_t commonHeaderSize = read16(offset);
	size_t copySize = commonHeaderSize < maxSize ? commonHeaderSize : maxSize;
	
	if(copySize != maxSize) {
		libatombios_printf_warn("copyStructure max size exceded! CommonHeader lists size as 0x%x, but max size is 0x%x\n", commonHeaderSize, maxSize);
	}

	memcpy(dest, _data.data() + offset, copySize);
}

static const int iioInstructionLengths[] = { 1, 2, 3, 3, 3, 3, 4, 4, 4, 3 };

void AtomBios::_indexIIO(uint32_t base) {
	uint32_t ptr = base;
	_iioIndexes.resize(255, 0);

	// There is no complete table of IIO functions; there are all in a single data table.
	// They have a "header" (the START opcode) which has the index in it.
	// Therefore, to obtain a list of IIO functions, we iterate by walking through this, and skipping over instructions.
	while(_data[ptr] == IIOOpcodes::START) {
		uint8_t id = _data[ptr + 1];
		_iioIndexes[id] = ptr + 2;
		ptr += 2;

		if(AtomBIOSDebugSettings::logIIOIndex) {
			libatombios_printf_dbg("IIO Index: _iioIndexes[%02x] = %04x (%04x into data table)\n", id, ptr, ptr - base);
		}

		while(_data[ptr] != IIOOpcodes::END) {
			assert(_data[ptr] < std::size(iioInstructionLengths));
			ptr += iioInstructionLengths[_data[ptr]];
		}
		ptr += 3;
	}
}

uint32_t AtomBios::_runIIO(uint32_t offset, uint32_t index, uint32_t data) {
	uint32_t temp = 0xCDCDCDCD;
	uint32_t ip = offset;

	auto moveTemp = [this, &ip, &temp](uint32_t val) {
		temp &= ~((0xFFFFFFFF >> (32 - _data[ip + 1])) << _data[ip + 3]);
		temp |= ((val >> _data[ip + 2])) & (0xFFFFFFFF >> (32 - _data[ip + 1])) << _data[ip + 3];
	};

	if(AtomBIOSDebugSettings::logIIOOpcodes) {
		libatombios_printf_dbg("  Running IIO table at %04x (offset from begin: 0x%04x)\n", offset, offset - _dataTable.indirectIOAccess);
	}

	while(true) {
		uint8_t opcode = _data[ip];

		// START should not be encountered inside of a IIO function
		if(opcode > IIOOpcodes::END || opcode == IIOOpcodes::START) {
			libatombios_printf_warn("IIO Interpreter encountered invalid IIO opcode: %02x; breaking!\n", opcode);
			break;
		}

		switch(static_cast<IIOOpcodes>(opcode)) {
		case START:
			__builtin_unreachable();
		case IIOOpcodes::NOP:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode NOP()\n");
			}
			break;
		case IIOOpcodes::READ:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode READ(%04x)\n", read16(ip + 1));
			}
			temp = libatombios_card_reg_read(read16(ip + 1));
			break;
		case WRITE:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode WRITE(%04x)\n", read16(ip + 1));
			}
			libatombios_card_reg_write(read16(ip + 1), temp);
			break;
		case CLEAR:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode CLEAR(%02x, %02x)\n", _data[ip + 1], _data[ip + 2]);
			}
			temp &= ~((0xFFFFFFFF >> (32 - _data[ip + 1]))) << _data[ip + 2];
			break;
		case SET:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode SET(%02x, %02x)\n", _data[ip + 1], _data[ip + 2]);
			}
			temp |= (0xFFFFFFFF >> (32 - _data[ip + 1])) << _data[ip + 2];
			break;
		case MOVE_INDEX:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode MOVE_INDEX(%02x, %02x, %02x)\n", _data[ip + 1], _data[ip + 2], _data[ip + 3]);
			}
			moveTemp(index);
			break;
		case MOVE_DATA:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode MOVE_DATA(%02x, %02x, %02x)\n", _data[ip + 1], _data[ip + 2], _data[ip + 3]);
			}
			moveTemp(data);
			break;
		case MOVE_ATTR:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode MOVE_ATTR(%02x, %02x, %02x)\n", _data[ip + 1], _data[ip + 2], _data[ip + 3]);
			}
			moveTemp(_iioIOAttr);
			break;
		case END:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				libatombios_printf_dbg("  IIO: opcode END()\n");
			}
			break;
		}

		ip += iioInstructionLengths[opcode];
	}

	return temp;
}

/// TODO: this is not the way we should do this lol
uint32_t AtomBios::_doIORead(uint32_t reg) {
	switch(_ioMode) {
	case IOMode::MM: return libatombios_card_reg_read(reg);

	case IOMode::PCI:
		libatombios_printf_warn("PCI reads are not implemented (requested reg: 0x%x)\n", reg);
		return 0;
	case IOMode::SYSIO:
		libatombios_printf_warn("SYSIO reads are not implemented (requested reg: 0x%x)\n", reg);
		return 0;
	case IOMode::IIO:
		if(_iioIndexes[_iioPort]) {
			return _runIIO(_iioIndexes[_iioPort], reg, 0);
		} else {
			libatombios_printf_warn("Invalid IIO port %02x (function does not exist, requested reg: %04x)\n", _iioPort, reg);
		}
		return 0;
	}

	return 0;
}

void AtomBios::_doIOWrite(uint32_t reg, uint32_t val) {
	switch(_ioMode) {
	case IOMode::MM:
		libatombios_card_reg_write(reg, val);
		return;
	case IOMode::PCI:
	case IOMode::SYSIO:
		libatombios_printf_warn("PCI / SYSIO writes are not implemented (requested reg/val: 0x%x <- 0x%x)\n", reg, val);
		return;
	case IOMode::IIO:
		if(_iioIndexes[_iioPort]) {
			_runIIO(_iioIndexes[_iioPort], reg, val);
		} else {
			libatombios_printf_warn("Invalid IIO port %02x (function does not exist, requested reg/val: %04x <- %x)\n", _iioPort, reg, val);
		}
		return;
	}
}

const char* opcodeArgEncodingStrings[] = {
	"REG",
	"PS",
	"WS",
	"FB",
	"ID",
	"Imm",
	"PLL",
	"MC"
};

const char* AtomBios::OpcodeArgEncodingToString(OpcodeArgEncoding arg) {
	assert(arg >= OpcodeArgEncoding::Reg);
	assert(arg <= OpcodeArgEncoding::MC);
	
	return opcodeArgEncodingStrings[arg];
}

const char* srcEncodingStrings[] = {
	"[XXXX]",
	"[--XX]",
	"[-XX-]",
	"[XX--]",
	"[---X]",
	"[--X-]",
	"[-X--]",
	"[X---]"
};

const char* AtomBios::SrcEncodingToString(SrcEncoding align) {
	assert(align >= SrcEncoding::SrcDword);
	assert(align <= SrcEncoding::SrcByte24);
	
	return srcEncodingStrings[align];
}
