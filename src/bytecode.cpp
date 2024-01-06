#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>

#include <libatombios/atom.hpp>
#include <libatombios/atom-debug.hpp>
#include <libatombios/extern-funcs.hpp>

void AtomBios::_runBytecode(std::shared_ptr<Command> command, std::vector<uint32_t>& params, int params_shift) {
	assert(command->workSpaceSize % sizeof(uint32_t) == 0);
	assert(command->parameterSpaceSize % sizeof(uint32_t) == 0);

	libatombios_printf_dbg("running command %i\n", command->_i);

	std::vector<uint32_t> workSpace;
	workSpace.reserve(command->workSpaceSize / sizeof(uint32_t));

	auto getParameterSpace = [&command, &params, &params_shift](uint32_t offset) -> uint32_t {
		assert(offset >= 0);
		if(offset >= params.size()) {
			params.resize(offset + 1);
		}

		return params[offset + params_shift];
	};
	auto setParameterSpace = [&command, &params, &params_shift](uint32_t offset, uint32_t data) {
		assert(offset >= 0);
		if(offset >= params.size()) {
			params.resize(offset + 1);
		}

		params[offset + params_shift] = data;
	};

	auto getWorkSpace = [&command, &workSpace](uint32_t offset) -> uint32_t {
		assert(offset >= 0);

		switch(static_cast<WorkSpaceSpecialAddresses>(offset)) {
		case WS_QUOTIENT:
		case WS_REMAINDER:
		case WS_DATAPTR:
		case WS_SHIFT:
		case WS_OR_MASK:
		case WS_AND_MASK:
		case WS_FB_WINDOW:
		case WS_ATTRIBUTES:
		case WS_REGPTR:
			libatombios_printf_warn("getWorkspace: special address 0x%02x not implemented\n", offset);
			break;
		}

        if(offset >= workSpace.size()) {
			workSpace.resize(offset + 1);
		}

		return workSpace[offset];
	};
	auto setWorkSpace = [&command, &workSpace](uint32_t offset, uint32_t data) {
		assert(offset >= 0);

		switch(static_cast<WorkSpaceSpecialAddresses>(offset)) {
		case WS_QUOTIENT:
		case WS_REMAINDER:
		case WS_DATAPTR:
		case WS_SHIFT:
		case WS_OR_MASK:
		case WS_AND_MASK:
		case WS_FB_WINDOW:
		case WS_ATTRIBUTES:
		case WS_REGPTR:
			libatombios_printf_warn("setWorkSpace: special address 0x%02x not implemented\n", offset);
			break;
		}

		if(offset >= workSpace.size()) {
			workSpace.resize(offset + 1);
		}

		workSpace[offset] = data;
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
	auto consumeLong = [&command, &ip]() -> uint32_t {
		assert(ip < (command->_bytecode.size() - 3));
		uint8_t a = command->_bytecode[ip++];
		uint8_t b = command->_bytecode[ip++];
		uint8_t c = command->_bytecode[ip++];
		uint8_t d = command->_bytecode[ip++];
		return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
	};

	auto consumeAlignSize = [&consumeByte, &consumeShort, &consumeLong](SrcEncoding align) -> uint32_t {
		switch(align) {
		case SrcEncoding::SrcByte0 ... SrcEncoding::SrcByte24:
			return consumeByte();
		case SrcEncoding::SrcWord0 ... SrcEncoding::SrcWord16:
			return consumeShort();
		case SrcEncoding::SrcDword:
			return consumeLong();
		}

		__builtin_unreachable();
	};

	auto consumeAttrByte = [&consumeByte]() -> AttrByte {
		uint8_t byte = consumeByte();
		AttrByte attrByte;
		attrByte.srcArg = static_cast<OpcodeArgEncoding>(byte & 0b111);
		attrByte.srcAlign = static_cast<SrcEncoding>((byte >> 3) & 0b111);
		attrByte.dstAlign = static_cast<SrcEncoding>(atom_dst_to_src[attrByte.srcAlign][(byte >> 6) & 0b11]);
		return attrByte;
	};
	auto consumeIdx = [&consumeByte, &consumeShort](OpcodeArgEncoding arg) -> uint32_t {
		uint32_t idx;
		switch(arg) {
		case OpcodeArgEncoding::Reg:
		case OpcodeArgEncoding::ID:
			idx = consumeShort();
			break;

		case OpcodeArgEncoding::ParameterSpace:
		case OpcodeArgEncoding::WorkSpace:
		case OpcodeArgEncoding::FrameBuffer:
		case OpcodeArgEncoding::PLL:
		case OpcodeArgEncoding::MC:
			idx = consumeByte();
			break;

		case OpcodeArgEncoding::Imm:
			// No Idx is needed
			idx = 0;
			break;
		}
		return idx;
	};
	auto consumeVal = [this, &getParameterSpace, &getWorkSpace, &consumeByte, &consumeShort, &consumeLong](OpcodeArgEncoding arg, AttrByte attrByte, uint32_t idx) -> uint32_t {
		switch(arg) {
		case OpcodeArgEncoding::Reg:
			return _doIORead(idx);

		case OpcodeArgEncoding::ParameterSpace:
			return getParameterSpace(idx);

		case OpcodeArgEncoding::ID:
			return read32(idx + _getDataTableOffset());

		case OpcodeArgEncoding::Imm:
			// Immideates only make sense for source values.
			switch(static_cast<SrcEncoding>(attrByte.srcAlign)) {
			case SrcEncoding::SrcDword:
				return consumeLong();
			case SrcEncoding::SrcWord0:
			case SrcEncoding::SrcWord8:
			case SrcEncoding::SrcWord16:
				return consumeShort();
			case SrcEncoding::SrcByte0:
			case SrcEncoding::SrcByte8:
			case SrcEncoding::SrcByte16:
			case SrcEncoding::SrcByte24:
				return consumeByte();
			}
			break;
		case OpcodeArgEncoding::WorkSpace:
			return getWorkSpace(idx);

		case OpcodeArgEncoding::FrameBuffer:
		case OpcodeArgEncoding::PLL:
		case OpcodeArgEncoding::MC:
			libatombios_printf_panic("consumeVal with arg=%i (%s) is not implemented\n", arg, OpcodeArgEncodingToString(arg));
			break;
		};
		__builtin_unreachable();
	};

	auto putVal = [this, &setParameterSpace, &setWorkSpace](OpcodeArgEncoding arg, uint32_t idx, uint32_t val) {
		switch(arg) {
		case OpcodeArgEncoding::Reg:
			_doIOWrite(idx, val);
			break;

		case OpcodeArgEncoding::ParameterSpace:
			setParameterSpace(idx, val);
			break;
		case OpcodeArgEncoding::WorkSpace:
			setWorkSpace(idx, val);
			break;

		case OpcodeArgEncoding::ID:
		case OpcodeArgEncoding::FrameBuffer:
		case OpcodeArgEncoding::Imm:
		case OpcodeArgEncoding::PLL:
		case OpcodeArgEncoding::MC:
			libatombios_printf_panic("putVal with arg=%i is not implemented\n", arg);
			break;
		}
	};

	// Safely skip command data.
	auto skip = [&command, &ip](uint32_t count) {
		if(!count) {
			return;
		}
		assert(ip < (command->_bytecode.size() - (count - 1)));
		ip += count;
	};

	///
	/// Opcodes
	///
	auto moveOpcode = [&consumeAttrByte, &consumeIdx, &consumeVal, &putVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t srcIdx = consumeIdx(attrByte.srcArg);

		uint32_t saved = consumeVal(arg, attrByte, dstIdx);
		uint32_t val = attrByte.swizleSrc(consumeVal(attrByte.srcArg, attrByte, srcIdx));
		uint32_t newVal = val;

		LOG_OPCODE("MOVE");

		putVal(arg, dstIdx, attrByte.combineSaved(val, saved));
	};

	auto andOpcode = [&consumeAttrByte, &consumeIdx, &consumeVal, &putVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t srcIdx = consumeIdx(attrByte.srcArg);

		uint32_t saved = consumeVal(arg, attrByte, dstIdx);
		uint32_t val = attrByte.swizleSrc(consumeVal(attrByte.srcArg, attrByte, srcIdx));
		uint32_t newVal = attrByte.swizleDst(saved) & val;

		LOG_OPCODE("AND");

		putVal(arg, dstIdx, attrByte.combineSaved(newVal, saved));
	};


	auto orOpcode = [&consumeAttrByte, &consumeIdx, &consumeVal, &putVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t srcIdx = consumeIdx(attrByte.srcArg);

		uint32_t saved = consumeVal(arg, attrByte, dstIdx);
		uint32_t val = attrByte.swizleSrc(consumeVal(attrByte.srcArg, attrByte, srcIdx));
		uint32_t newVal = attrByte.swizleDst(saved) | val;

		LOG_OPCODE("OR");

		putVal(arg, dstIdx, attrByte.combineSaved(newVal, saved));
	};

	auto testOpcode = [this, &consumeAttrByte, &consumeIdx, &consumeVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t srcIdx = consumeIdx(attrByte.srcArg);

		uint32_t saved = consumeVal(arg, attrByte, dstIdx);
		uint32_t val = attrByte.swizleSrc(consumeVal(attrByte.srcArg, attrByte, srcIdx));
		_flagEqual = attrByte.swizleDst(saved) == val;

		// Keep the generic opcode logger happy
		uint32_t newVal = attrByte.swizleDst(saved);
		
		LOG_OPCODE("TEST");
		LOG_FLAGS();
	};

	auto jumpOpcode = [this, &ip, &consumeShort, &performJump](JumpArgEncoding jumpCond) {
		uint16_t target = consumeShort();
		bool shouldJump;

		switch(jumpCond) {
		case JumpArgEncoding::Above:
			shouldJump = _flagAbove;
			break;
		case JumpArgEncoding::AboveOrEqual:
			shouldJump = _flagAbove || _flagEqual;
			break;
		case JumpArgEncoding::Always:
			shouldJump = true;
			break;
		case JumpArgEncoding::Below:
			shouldJump = _flagBelow;
			break;
		case JumpArgEncoding::BelowOrEqual:
			shouldJump = _flagBelow || _flagEqual;
			break;
		case JumpArgEncoding::Equal:
			shouldJump = _flagEqual;
			break;
		case JumpArgEncoding::NotEqual:
			shouldJump = !_flagEqual;
			break;
		}

		if(AtomBIOSDebugSettings::logOpcodes) {
			libatombios_printf_dbg("opcode JUMP_* (shouldJump = %i, oldIP = %x, newIP = %x)\n",
				shouldJump, ip, shouldJump ? target : ip);
		}

        if(shouldJump) {
			performJump(target);
		}
	};

	auto clearOpcode = [&consumeAttrByte, &consumeIdx, &consumeVal, &putVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);

		uint32_t saved = consumeVal(arg, attrByte, dstIdx);
		uint32_t newVal = attrByte.combineSaved(0, saved);

		// Keep the logger happy
		constexpr uint32_t val = 0;
		LOG_OPCODE_DST_ONLY("CLEAR");

		putVal(arg, dstIdx, newVal);
	};

	// TODO: might be bugged, should test
	auto maskOpcode = [&consumeAttrByte, &consumeIdx, &consumeVal, &consumeAlignSize, &putVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t saved = consumeVal(arg, attrByte, dstIdx);

		uint32_t mask = consumeAlignSize(attrByte.dstAlign);

		uint32_t srcIdx = consumeIdx(attrByte.srcArg);
		uint32_t val = attrByte.swizleSrc(consumeVal(attrByte.srcArg, attrByte, srcIdx));
		uint32_t newVal = (attrByte.swizleDst(saved) & mask) | val;

		if(AtomBIOSDebugSettings::logOpcodes) { \
			libatombios_printf_dbg("opcode MASK(%s[%02x] %s (savedVal: %x) & %04x | %s[%02x] %s (val: %x, newVal: %x))\n", \
				OpcodeArgEncodingToString(arg), dstIdx, SrcEncodingToString(attrByte.dstAlign), \
				saved, mask, \
				OpcodeArgEncodingToString(attrByte.srcArg), srcIdx, SrcEncodingToString(attrByte.srcAlign), \
				val, newVal); \
		}

		putVal(arg, dstIdx, attrByte.combineSaved(newVal, saved));
	};

	auto compareOpcode = [this, &consumeAttrByte, &consumeIdx, &consumeVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t srcIdx = consumeIdx(attrByte.srcArg);

		uint32_t saved = consumeVal(arg, attrByte, dstIdx);
		uint32_t val = attrByte.swizleSrc(consumeVal(attrByte.srcArg, attrByte, srcIdx));
		_flagEqual = attrByte.swizleDst(saved) == val;
		_flagAbove = attrByte.swizleDst(saved) > val;
		_flagBelow = attrByte.swizleDst(saved) < val;

		// Keep the generic opcode logger happy
		uint32_t newVal = attrByte.swizleDst(saved);
		
		LOG_OPCODE("COMPARE");
		LOG_FLAGS();
	};

	auto shiftLeftOpcode = [&consumeAttrByte, &consumeByte, &consumeIdx, &consumeVal, &putVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t saved = consumeVal(arg, attrByte, dstIdx);

		uint32_t shift = consumeByte();
		uint32_t newVal = attrByte.swizleDst(saved) << shift;

		if(AtomBIOSDebugSettings::logOpcodes) {
			libatombios_printf_dbg("opcode SHIFT_LEFT(%s[%02x] %s (savedVal: %x) << %i (newVal: %x)\n",
				OpcodeArgEncodingToString(arg), dstIdx, SrcEncodingToString(attrByte.dstAlign),
				saved, shift, newVal
				);
		}

		putVal(arg, dstIdx, attrByte.combineSaved(newVal, saved));
	};

	auto shiftRightOpcode = [&consumeAttrByte, &consumeByte, &consumeIdx, &consumeVal, &putVal](OpcodeArgEncoding arg) {
		AttrByte attrByte = consumeAttrByte();
		uint32_t dstIdx = consumeIdx(arg);
		uint32_t saved = consumeVal(arg, attrByte, dstIdx);

		uint32_t shift = consumeByte();
		uint32_t newVal = attrByte.swizleDst(saved) >> shift;

		if(AtomBIOSDebugSettings::logOpcodes) {
			libatombios_printf_dbg("opcode SHIFT_RIGHT(%s[%02x] %s (savedVal: %x) << %i (newVal: %x)\n",
				OpcodeArgEncodingToString(arg), dstIdx, SrcEncodingToString(attrByte.dstAlign),
				saved, shift, newVal
				);
		}

		putVal(arg, dstIdx, attrByte.combineSaved(newVal, saved));
	};


	while(ip < command->_bytecode.size()) {
		uint8_t opcode = consumeByte();
		switch(opcode) {
		/// Misc. opcodes
		case Opcodes::CALL_TABLE: {
			uint8_t table = consumeByte();

			if(AtomBIOSDebugSettings::logOpcodes) {
				libatombios_printf_dbg("opcode CALL_TABLE(%x)\n", table);
			}
			assert(_commandTable.commands[table]);
			_runBytecode(_commandTable.commands[table], params, params_shift + (command->parameterSpaceSize / 4));
			break;
		}
		case Opcodes::SET_DATA_TABLE: {
			uint8_t table = consumeByte();
			_activeDataTable = table;

			if(AtomBIOSDebugSettings::logOpcodes) {
				libatombios_printf_dbg("opcode SET_DATA_TABLE(%i)\n", table);
			}
			if(table == 255) {
				libatombios_printf_warn("handling of SET_DATA_TABLE(255) may not be correct\n");
			}
			break;
		} 
		case Opcodes::SET_ATI_PORT: {
			uint16_t port = consumeShort();
			if(!port) {
				_ioMode = IOMode::MM;
			} else {
				_ioMode = IOMode::IIO;
				_iioPort = port;
			}

			if(AtomBIOSDebugSettings::logOpcodes) {
				libatombios_printf_dbg("opcode SET_ATI_PORT(%x)\n", port);
			}
			break;
		}
		case Opcodes::SET_PCI_PORT:
			_ioMode = IOMode::PCI;
			break;
		case Opcodes::SET_SYSIO_PORT:
			_ioMode = IOMode::SYSIO;
			break;

		/// Moves
		case Opcodes::MOVE_TO_REG:
			moveOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::MOVE_TO_PS:
			moveOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::MOVE_TO_WS:
			moveOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::MOVE_TO_FB:
			moveOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::MOVE_TO_PLL:
			moveOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::MOVE_TO_MC:
			moveOpcode(OpcodeArgEncoding::MC);
			break;

		/// AND
		case Opcodes::AND_INTO_REG:
			andOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::AND_INTO_PS:
			andOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::AND_INTO_WS:
			andOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::AND_INTO_FB:
			andOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::AND_INTO_PLL:
			andOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::AND_INTO_MC:
			andOpcode(OpcodeArgEncoding::MC);
			break;

		/// OR
		case Opcodes::OR_INTO_REG:
			orOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::OR_INTO_PS:
			orOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::OR_INTO_WS:
			orOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::OR_INTO_FB:
			orOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::OR_INTO_PLL:
			orOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::OR_INTO_MC:
			orOpcode(OpcodeArgEncoding::MC);
			break;

		/// SHIFT_LEFT
		case Opcodes::SHIFT_LEFT_IN_REG:
			shiftLeftOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::SHIFT_LEFT_IN_PS:
			shiftLeftOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::SHIFT_LEFT_IN_WS:
			shiftLeftOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::SHIFT_LEFT_IN_FB:
			shiftLeftOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::SHIFT_LEFT_IN_PLL:
			shiftLeftOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::SHIFT_LEFT_IN_MC:
			shiftLeftOpcode(OpcodeArgEncoding::MC);
			break;

		/// SHIFT_RIGHT
		case Opcodes::SHIFT_RIGHT_IN_REG:
			shiftRightOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::SHIFT_RIGHT_IN_PS:
			shiftRightOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::SHIFT_RIGHT_IN_WS:
			shiftRightOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::SHIFT_RIGHT_IN_FB:
			shiftRightOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::SHIFT_RIGHT_IN_PLL:
			shiftRightOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::SHIFT_RIGHT_IN_MC:
			shiftRightOpcode(OpcodeArgEncoding::MC);
			break;

		/// COMPARE
		case Opcodes::COMPARE_FROM_REG:
			compareOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::COMPARE_FROM_PS:
			compareOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::COMPARE_FROM_WS:
			compareOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::COMPARE_FROM_FB:
			compareOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::COMPARE_FROM_PLL:
			compareOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::COMPARE_FROM_MC:
			compareOpcode(OpcodeArgEncoding::MC);
			break;

		/// JUMP_*
		case Opcodes::JUMP_ALWAYS:
			jumpOpcode(JumpArgEncoding::Always);
			break;
		case Opcodes::JUMP_EQUAL:
			jumpOpcode(JumpArgEncoding::Equal);
			break;
		case Opcodes::JUMP_BELOW:
			jumpOpcode(JumpArgEncoding::Below);
			break;
		case Opcodes::JUMP_ABOVE:
			jumpOpcode(JumpArgEncoding::Above);
			break;
		case Opcodes::JUMP_BELOWOREQUAL:
			jumpOpcode(JumpArgEncoding::BelowOrEqual);
			break;
		case Opcodes::JUMP_ABOVEOREQUAL:
			jumpOpcode(JumpArgEncoding::AboveOrEqual);
			break;
		case Opcodes::JUMP_NOTEQUAL:
			jumpOpcode(JumpArgEncoding::NotEqual);
			break;

		/// TEST
		case Opcodes::TEST_FROM_REG:
			testOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::TEST_FROM_PS:
			testOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::TEST_FROM_WS:
			testOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::TEST_FROM_FB:
			testOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::TEST_FROM_PLL:
			testOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::TEST_FROM_MC:
			testOpcode(OpcodeArgEncoding::MC);
			break;

		/// CLEAR
		case Opcodes::CLEAR_IN_REG:
			clearOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::CLEAR_IN_PS:
			clearOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::CLEAR_IN_WS:
			clearOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::CLEAR_IN_FB:
			clearOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::CLEAR_IN_PLL:
			clearOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::CLEAR_IN_MC:
			clearOpcode(OpcodeArgEncoding::MC);
			break;

		/// MASK
		case Opcodes::MASK_INTO_REG:
			maskOpcode(OpcodeArgEncoding::Reg);
			break;
		case Opcodes::MASK_INTO_PS:
			maskOpcode(OpcodeArgEncoding::ParameterSpace);
			break;
		case Opcodes::MASK_INTO_WS:
			maskOpcode(OpcodeArgEncoding::WorkSpace);
			break;
		case Opcodes::MASK_INTO_FB:
			maskOpcode(OpcodeArgEncoding::FrameBuffer);
			break;
		case Opcodes::MASK_INTO_PLL:
			maskOpcode(OpcodeArgEncoding::PLL);
			break;
		case Opcodes::MASK_INTO_MC:
			maskOpcode(OpcodeArgEncoding::MC);
			break;

		case Opcodes::END_OF_TABLE: {
			if(AtomBIOSDebugSettings::logOpcodes) {
				libatombios_printf_dbg("opcode END_OF_TABLE\n");
			}
			// Force a drop out by setting IP to a really high value
			ip = INT32_MAX;
			break;
		}

		default: {
			libatombios_printf_panic("unexpected opcode 0x%x, in command table 0x%x, ip %x (%x including header)\n", opcode, command->_i, ip - 1, ip + 6 - 1);
		}
		}
	}
}
