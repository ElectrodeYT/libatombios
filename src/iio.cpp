#include <libatombios/atom.hpp>
#include <libatombios/atom-debug.hpp>
#include <libatombios/extern-funcs.hpp>

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
			lilrad_log(DEBUG, "IIO Index: _iioIndexes[%02x] = %04x (%04x into data table)\n", id, ptr, ptr - base);
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
		lilrad_log(DEBUG, "  Running IIO table at %04x (offset from begin: 0x%04x)\n", offset, offset - _dataTable.indirectIOAccess);
	}

	bool run = true;
	while(run) {
		uint8_t opcode = _data[ip];

		// START should not be encountered inside of a IIO function
		if(opcode > IIOOpcodes::END || opcode == IIOOpcodes::START) {
			lilrad_log(WARNING, "IIO Interpreter encountered invalid IIO opcode: %02x; breaking!\n", opcode);
			break;
		}

		switch(static_cast<IIOOpcodes>(opcode)) {
		case START:
			__builtin_unreachable();
		case IIOOpcodes::NOP:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode NOP()\n");
			}
			break;
		case IIOOpcodes::READ:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode READ(%04x)\n", read16(ip + 1));
			}
			temp = libatombios_card_reg_read(read16(ip + 1));
			break;
		case IIOOpcodes::WRITE:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode WRITE(%04x)\n", read16(ip + 1));
			}
			libatombios_card_reg_write(read16(ip + 1), temp);
			break;
		case IIOOpcodes::CLEAR:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode CLEAR(%02x, %02x)\n", _data[ip + 1], _data[ip + 2]);
			}
			temp &= ~((0xFFFFFFFF >> (32 - _data[ip + 1]))) << _data[ip + 2];
			break;
		case IIOOpcodes::SET:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode SET(%02x, %02x)\n", _data[ip + 1], _data[ip + 2]);
			}
			temp |= (0xFFFFFFFF >> (32 - _data[ip + 1])) << _data[ip + 2];
			break;
		case IIOOpcodes::MOVE_INDEX:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode MOVE_INDEX(%02x, %02x, %02x)\n", _data[ip + 1], _data[ip + 2], _data[ip + 3]);
			}
			moveTemp(index);
			break;
		case IIOOpcodes::MOVE_DATA:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode MOVE_DATA(%02x, %02x, %02x)\n", _data[ip + 1], _data[ip + 2], _data[ip + 3]);
			}
			moveTemp(data);
			break;
		case IIOOpcodes::MOVE_ATTR:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode MOVE_ATTR(%02x, %02x, %02x)\n", _data[ip + 1], _data[ip + 2], _data[ip + 3]);
			}
			moveTemp(_iioIOAttr);
			break;
		case IIOOpcodes::END:
			if(AtomBIOSDebugSettings::logIIOOpcodes) {
				lilrad_log(DEBUG, "  IIO: opcode END()\n");
			}
			run = false;
			break;
		}

		ip += iioInstructionLengths[opcode];
	}

	return temp;
}
