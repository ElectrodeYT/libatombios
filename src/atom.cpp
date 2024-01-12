#include <libatombios/atom.hpp>
#include <libatombios/atom-debug.hpp>
#include <libatombios/extern-funcs.hpp>

#include "atom-private.hpp"

AtomBios::AtomBios(uint8_t* data, size_t size) {
	//_impl = static_cast<AtomBiosImpl*>(lilrad_alloc(sizeof(AtomBiosImpl)));
	_impl = new AtomBiosImpl(data, size);
} 

void AtomBios::runCommand(CommandTables table, uint32_t* params, size_t size) {
	libatombios_vector<uint32_t> paramVector;
	paramVector.resize(size);
	memcpy(paramVector.data(), params, size * sizeof(uint32_t));

	_impl->runCommand(table, paramVector);

	memcpy(params, paramVector.data(), size * sizeof(uint32_t));
}

const uint32_t AtomBios::maxPSIndex() {
	return _impl->maxPSIndex();
}
const uint32_t AtomBios::maxWSIndex() {
	return _impl->maxWSIndex();
}

AtomBiosImpl::AtomBiosImpl(uint8_t* data, size_t size) {
	// Copy the bios data.
	{
		_data.resize(size);
		memcpy(_data.data(), data, size);
	}

	// Verify the BIOS magic.
	{
		uint16_t biosMagic = read16(0);
		lilrad_log(DEBUG, "biosMagic is %x\n", biosMagic);
		assert(biosMagic == 0xAA55);
	}

	// Verify the ATI magic.
	{
		char atiMagic[11];
		memcpy(atiMagic, _data.data() + 0x30, 10);
		atiMagic[10] = '\0';
		lilrad_log(DEBUG, "ATI Magic is %s\n", atiMagic);
		assert(strcmp(atiMagic, " 761295520") == 0);
	}

	_atomRomTableBase = read16(0x48);
	lilrad_log(DEBUG, "Atom ROM Table Base is %zx\n", _atomRomTableBase);
	
	// Copy the Atom ROM Table.
	{
		memset(&_atomRomTable, 0, sizeof(AtomRomTable));
		copyStructure(&_atomRomTable, _atomRomTableBase, sizeof(AtomRomTable));
		_atomRomTable.dumpToConsole();
	}

	// Verify the Atom ROM Table Magic.
	{
		char atomRomTableMagic[5];
		memcpy(atomRomTableMagic, _atomRomTable.atomMagic, 4);
		atomRomTableMagic[4] = '\0';
		lilrad_log(DEBUG, "Atom ROM Table Magic is %s\n", atomRomTableMagic);
		assert(strcmp(atomRomTableMagic, "ATOM") == 0);
	}

	// Copy the data table.
	lilrad_log(DEBUG, "Atom Data Table Base is %x\n", _atomRomTable.dataTableBase);
	copyStructure(&_dataTable, _atomRomTable.dataTableBase, sizeof(DataTable));

	// Initialize the command table.
	_commandTable.readCommands(_data, _atomRomTable.commandTableBase);

	// Index the IIO commands.
	_indexIIO(_dataTable.indirectIOAccess + 4);
}

void AtomBiosImpl::copyStructure(void* dest, size_t offset, size_t maxSize) {
	size_t commonHeaderSize = read16(offset);
	size_t copySize = commonHeaderSize < maxSize ? commonHeaderSize : maxSize;
	
	if(copySize != maxSize) {
		lilrad_log(WARNING, "copyStructure max size exceded! CommonHeader lists size as 0x%zx, but max size is 0x%zx\n", commonHeaderSize, maxSize);
	}

	memcpy(dest, _data.data() + offset, copySize);
}

/// TODO: this is not the way we should do this lol
uint32_t AtomBiosImpl::_doIORead(uint32_t reg) {
	switch(_ioMode) {
	case IOMode::MM: return libatombios_card_reg_read(reg);

	case IOMode::PCI:
		lilrad_log(WARNING, "PCI reads are not implemented (requested reg: 0x%x)\n", reg);
		return 0;
	case IOMode::SYSIO:
		lilrad_log(WARNING, "SYSIO reads are not implemented (requested reg: 0x%x)\n", reg);
		return 0;
	case IOMode::IIO:
		if(_iioIndexes[_iioPort]) {
			return _runIIO(_iioIndexes[_iioPort], reg, 0);
		} else {
			lilrad_log(WARNING, "Invalid IIO port %02x (function does not exist, requested reg: %04x)\n", _iioPort, reg);
		}
		return 0;
	}

	return 0;
}

void AtomBiosImpl::_doIOWrite(uint32_t reg, uint32_t val) {
	switch(_ioMode) {
	case IOMode::MM:
		libatombios_card_reg_write(reg, val);
		return;
	case IOMode::PCI:
	case IOMode::SYSIO:
		lilrad_log(WARNING, "PCI / SYSIO writes are not implemented (requested reg/val: 0x%x <- 0x%x)\n", reg, val);
		return;
	case IOMode::IIO:
		if(_iioIndexes[_iioPort]) {
			_runIIO(_iioIndexes[_iioPort], reg, val);
		} else {
			lilrad_log(WARNING, "Invalid IIO port %02x (function does not exist, requested reg/val: %04x <- %x)\n", _iioPort, reg, val);
		}
		return;
	}
}

// TODO: make this more pretty
//   Putting them into the class definiton makes the class a bit more ugly,
//   but having them not be namespaced in any way also isn't the best.

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

const char* OpcodeArgEncodingToString(OpcodeArgEncoding arg) {
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

const char* SrcEncodingToString(SrcEncoding align) {
	assert(align >= SrcEncoding::SrcDword);
	assert(align <= SrcEncoding::SrcByte24);
	
	return srcEncodingStrings[align];
}
