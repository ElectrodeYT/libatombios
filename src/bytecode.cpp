#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <libatombios/atom.hpp>
#include <libatombios/extern-funcs.hpp>

namespace {
	constexpr bool logOpcodes = true;
}

void AtomBios::_runBytecode(std::shared_ptr<Command> command) {
	assert(command->workSpaceSize % sizeof(uint32_t) == 0);
	assert(command->parameterSpaceSize % sizeof(uint32_t) == 0);

	libatombios_printf_dbg("running command %i\n", command->_i);

	uint32_t* workSpace = new uint32_t[command->workSpaceSize / 4];
	uint32_t* parameterSpace = new uint32_t[command->parameterSpaceSize / 4];

	auto getParameterSpace = [&command, &parameterSpace](uint32_t offset) -> uint32_t {
		assert(offset > 0);
		assert(offset < command->parameterSpaceSize);

		return parameterSpace[offset];
	};
	auto setParameterSpace = [&command, &parameterSpace](uint32_t offset, uint32_t data) {
		assert(offset > 0);
		assert(offset < command->parameterSpaceSize);

		parameterSpace[offset] = data;
	};

	uint32_t ip = 0;

	// Safely change the IP.
	auto performJump = [&command, &ip](uint32_t bytecodeIP) {
		assert((bytecodeIP - 0x6) < command->_bytecode.size());
		assert(((int32_t)bytecodeIP - 0x6) > 0);
		ip = bytecodeIP - 0x6;
	};

	// Safely consume command data.
	auto consumeByte = [&command, &ip]() -> uint8_t {
		assert(ip < command->_bytecode.size());
		return command->_bytecode[ip++];
	};
	auto consumeShort = [&command, &ip]() -> uint16_t {
		assert(ip < (command->_bytecode.size() - 1));
		uint8_t a = command->_bytecode[ip++];
		uint8_t b = command->_bytecode[ip++];
		return static_cast<uint16_t>(a) | (static_cast<uint16_t>(b) << 8);
	};

	// Safely skip command data.
	auto skip = [&command, &ip](uint32_t count) {
		if(!count) {
			return;
		}
		assert(ip < (command->_bytecode.size() - (count - 1)));
		ip += count;
	};

	// Get source data.
	auto getSrc = [&command, &consumeByte, &consumeShort, this, &getParameterSpace](uint8_t attr, uint32_t* saved) -> uint32_t {
		constexpr bool debugParameterSpace = false;
		constexpr bool debugID = false;

		constexpr bool debugFinalVal = false;

		uint32_t idx;
		uint32_t val = 0xCDCDCDCD;
		uint32_t align = (attr >> 3) & 7;
		uint32_t arg = attr & 7;

		switch(arg) {
		case OpcodeArgEncoding::Reg:
			idx = consumeShort();
			val = _doIORead(idx);
			break;
		case OpcodeArgEncoding::ParameterSpace:
			idx = consumeByte();
			if(debugParameterSpace) {
				libatombios_printf_dbg("getSrc, arg=ParameterSpace, idx=%x\n", idx);
			}
			val = getParameterSpace(idx);
			break;
		case OpcodeArgEncoding::ID:
			idx = consumeShort();
			val = read32(idx + _getDataTableOffset());
			if(debugID) {
				libatombios_printf_dbg("getSrc, arg=ID, idx=%x, actual idx=%x, val=%x\n", idx, idx + _getDataTableOffset(), val);
			}
			break;
		case OpcodeArgEncoding::WorkSpace:
		case OpcodeArgEncoding::FrameBuffer:
		case OpcodeArgEncoding::Imm:
		case OpcodeArgEncoding::PLL:
		case OpcodeArgEncoding::MC:
			libatombios_printf_panic("getSrc with arg=%i not implemented\n", arg);
			break;
		}

		if(saved) {
			*saved = val;
		}

		val &= atom_arg_mask[align];
		val >>= atom_arg_shift[align];
		if(debugFinalVal) {
			libatombios_printf_dbg("getSrc, final val=%x\n", val);
		}
		return val;
	};

	// Read destination data.
	auto readDst = [&consumeByte, &consumeShort](OpcodeArgEncoding arg) -> Dst {
		Dst dst;
		dst.arg = arg;
		switch(arg) {
		case OpcodeArgEncoding::Reg:
			dst.idx = consumeShort();
			break;
		case OpcodeArgEncoding::ParameterSpace:
		case OpcodeArgEncoding::WorkSpace:
		case OpcodeArgEncoding::FrameBuffer:
		case OpcodeArgEncoding::PLL:
		case OpcodeArgEncoding::MC:
			dst.idx = consumeByte();
			break;

		case OpcodeArgEncoding::ID:
		case OpcodeArgEncoding::Imm:
			libatombios_printf_panic("readDst with arg=%i not implemented\n", arg);
			break;
		};

		return dst;
	};

	// Get destination data.
	auto getDst = [&getSrc](Dst dst, uint8_t attr, uint32_t* saved) -> uint32_t {
		return getSrc(arg | atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3] << 3, saved);
	};

	// Skip source data.
	auto skipSrc = [&skip](uint8_t attr) {
		uint32_t align = (attr >> 3) & 7;
		uint32_t arg = attr & 7;
		switch(arg) {
		case OpcodeArgEncoding::Reg:
		case OpcodeArgEncoding::ID:
			skip(2);
			break;
		case OpcodeArgEncoding::PLL:
		case OpcodeArgEncoding::MC:
		case OpcodeArgEncoding::ParameterSpace:
		case OpcodeArgEncoding::WorkSpace:
		case OpcodeArgEncoding::FrameBuffer:
			skip(1);
			break;
		case OpcodeArgEncoding::Imm:
			switch(align) {
			case SrcEncoding::SrcDword:
				skip(4);
				break;
			case SrcEncoding::SrcWord0:
			case SrcEncoding::SrcWord8:
			case SrcEncoding::SrcWord16:
				skip(2);
				break;
			case SrcEncoding::SrcByte0:
			case SrcEncoding::SrcByte8:
			case SrcEncoding::SrcByte16:
			case SrcEncoding::SrcByte24:
				skip(1);
				break;
			default:
				assert(0 && "Should not be reached!");
			}
		default:
			assert(0 && "Should not be reached!");
		}
	};

	// Skip destination data.
	auto skipDst = [&skipSrc](OpcodeArgEncoding arg, uint8_t attr) {
		skipSrc(arg | atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3]);
	};

	// Put destination data.
	auto putDst = [&consumeByte, &setParameterSpace](OpcodeArgEncoding arg, uint8_t attr, uint32_t val, uint32_t saved) {
		uint32_t align = atom_dst_to_src[(attr >> 3) & 7][(attr >> 6) & 3];
		uint32_t old_val = val;
		uint32_t idx;

		old_val &= atom_arg_mask[align] >> atom_arg_shift[align];
		val <<= atom_arg_shift[align];
		val &= atom_arg_mask[align];
		saved &= ~atom_arg_mask[align];
		val |= saved;

		switch(arg) {
		case OpcodeArgEncoding::ParameterSpace:
    		idx = consumeByte();
    		setParameterSpace(idx, val);
    		libatombios_printf_dbg("putDst PS[%i] = %x\n", idx, val);
        	break;
        case OpcodeArgEncoding::Reg:
        case OpcodeArgEncoding::WorkSpace:
        case OpcodeArgEncoding::FrameBuffer:
        case OpcodeArgEncoding::ID:
        case OpcodeArgEncoding::Imm:
        case OpcodeArgEncoding::PLL:
        case OpcodeArgEncoding::MC:
        	libatombios_printf_panic("putDst with arg=%i not implemented\n", arg);
        	break;
        }
	};


	///
	/// Opcodes
	///
	auto moveOpcode = [&consumeByte, &getSrc, &getDst, &putDst, &skipDst](OpcodeArgEncoding arg) {
		uint8_t attr = consumeByte();

		// If we are not moving the entire destination, save the rest
		uint32_t saved;
		if(((attr >> 3) & 7) != SrcEncoding::SrcDword) {
			getDst(arg, attr, &saved);
		} else {
			// We are moving the entire destination, skip the dst encoding and poison the saved variable
			saved = 0xCDCDCDCD;
			skipDst(arg, attr);
		}

		// Read data
		uint32_t src = getSrc(attr, nullptr);
		libatombios_printf_dbg("moveOpcode, attr=%x, saved=%x, src=%x\n", attr, saved, src);
		
		// Put data
		putDst(arg, attr, src, saved);
	};

	while(ip < command->_bytecode.size()) {
		uint8_t opcode = consumeByte();
		switch(opcode) {
		case Opcodes::CALL_TABLE: {
			uint8_t table = consumeByte();

			if(logOpcodes) {
				libatombios_printf_dbg("opcode CALL_TABLE(%i)\n", table);
			}
			// runCommand(table);
			break;
		}
		case Opcodes::SET_DATA_TABLE: {
			uint8_t table = consumeByte();
			_activeDataTable = table;

			if(logOpcodes) {
				libatombios_printf_dbg("opcode SET_DATA_TABLE(%i)\n", table);
			}
			if(table == 255) {
				libatombios_printf_warn("handling of SET_DATA_TABLE(255) may not be correct\n");
			}
			break;
		} 
		
		// Moves
		case Opcodes::MOVE_TO_PS:
			if(logOpcodes) {
				libatombios_printf_dbg("opcode MOVE (todo: better logging)\n");
			}
			moveOpcode(OpcodeArgEncoding::ParameterSpace);
			break;

		default: {
			libatombios_printf_panic("unexpected opcode %x\n", opcode);
		}
		}
	}

	delete[] workSpace;
	delete[] parameterSpace;
}
