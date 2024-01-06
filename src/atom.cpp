#include <cstdio>
#include <cstring>
#include <cassert>

#include <libatombios/atom.hpp>
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
}

void AtomBios::copyStructure(void* dest, size_t offset, size_t maxSize) {
	size_t commonHeaderSize = read16(offset);
	size_t copySize = commonHeaderSize < maxSize ? commonHeaderSize : maxSize;
	
	if(copySize != maxSize) {
		libatombios_printf_warn("copyStructure max size exceded! CommonHeader lists size as 0x%x, but max size is 0x%x\n", commonHeaderSize, maxSize);
	}

	memcpy(dest, _data.data() + offset, copySize);
}

/// TODO: this is not the way we should do this lol
uint32_t AtomBios::_doIORead(uint32_t reg) {
	switch(_ioMode) {
	case IOMode::MM: return libatombios_card_reg_read(reg);

	case IOMode::IIO:
	case IOMode::PCI:
	case IOMode::SYSIO:
	default: {
		libatombios_printf_warn("io read of type %i is not implemented\n", reg);
		break;
	}
	}

	return 0;
}

void AtomBios::_doIOWrite(uint32_t reg, uint32_t val) {
	switch(_ioMode) {
	case IOMode::MM:
		libatombios_card_reg_write(reg, val);
		break;

	case IOMode::IIO:
	case IOMode::PCI:
	case IOMode::SYSIO:
	default: {
		libatombios_printf_warn("io write of type %i is not implemented\n", reg);
		break;
	}
	}
}

uint32_t AtomBios::_getDataTableOffset() {
	if(_activeDataTable == 0) {
		return 0;
	}

	assert(_activeDataTable < 34);
	return _dataTable.dataTables[_activeDataTable];
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
