#pragma once

#include <stdint.h>

#include <libatombios/atom.hpp>
#include "libatombios-frigg.hpp"

enum OpcodeArgEncoding {
	Reg = 0,
	ParameterSpace,
	WorkSpace,
	FrameBuffer,
	ID,  // Data Tables
	Imm,
	PLL, // PLL is better than PhaseLockedLoop
	MC   // MemoryController?
};

enum JumpArgEncoding {
	Above = 0,
	AboveOrEqual,
	Always,
	Below,
	BelowOrEqual,
	Equal,
	NotEqual
};

enum SrcEncoding {
	SrcDword = 0,
	SrcWord0,
	SrcWord8,
	SrcWord16,
	SrcByte0,
	SrcByte8,
	SrcByte16,
	SrcByte24
};

enum WorkSpaceSpecialAddresses {
	WS_QUOTIENT = 0x40,
	WS_REMAINDER = 0x41,
	WS_DATAPTR = 0x42,
	WS_SHIFT = 0x43,
	WS_OR_MASK = 0x44,
	WS_AND_MASK = 0x45,
	WS_FB_WINDOW = 0x46,
	WS_ATTRIBUTES = 0x47,
	WS_REGPTR = 0x48
};

enum Opcodes {
	MOVE_TO_REG = 0x01,
	MOVE_TO_PS = 0x02,
	MOVE_TO_WS = 0x03,
	MOVE_TO_FB = 0x04,
	MOVE_TO_PLL = 0x05,
	MOVE_TO_MC = 0x06,
	
	AND_INTO_REG = 0x07,
	AND_INTO_PS = 0x08,
	AND_INTO_WS = 0x09,
	AND_INTO_FB = 0x0A,
	AND_INTO_PLL = 0x0B,
	AND_INTO_MC = 0x0C,
	
	OR_INTO_REG = 0x0D,
	OR_INTO_PS = 0x0E,
	OR_INTO_WS = 0x0F,
	OR_INTO_FB = 0x10,
	OR_INTO_PLL = 0x11,
	OR_INTO_MC = 0x12,

	SHIFT_LEFT_IN_REG = 0x13,
	SHIFT_LEFT_IN_PS = 0x14,
	SHIFT_LEFT_IN_WS = 0x15,
	SHIFT_LEFT_IN_FB = 0x16,
	SHIFT_LEFT_IN_PLL = 0x17,
	SHIFT_LEFT_IN_MC = 0x18,

	SHIFT_RIGHT_IN_REG = 0x19,
	SHIFT_RIGHT_IN_PS = 0x1A,
	SHIFT_RIGHT_IN_WS = 0x1B,
	SHIFT_RIGHT_IN_FB = 0x1C,
	SHIFT_RIGHT_IN_PLL = 0x1D,
	SHIFT_RIGHT_IN_MC = 0x1E,

	MUL_WITH_REG = 0x1F,
	MUL_WITH_PS = 0x20,
	MUL_WITH_WS = 0x21,
	MUL_WITH_FB = 0x22,
	MUL_WITH_PLL = 0x23,
	MUL_WITH_MC = 0x24,

	DIV_WITH_REG = 0x25,
	DIV_WITH_PS = 0x26,
	DIV_WITH_WS = 0x27,
	DIV_WITH_FB = 0x28,
	DIV_WITH_PLL = 0x29,
	DIV_WITH_MC = 0x2A,

	ADD_INTO_REG = 0x2B,
	ADD_INTO_PS = 0x2C,
	ADD_INTO_WS = 0x2D,
	ADD_INTO_FB = 0x2E,
	ADD_INTO_PLL = 0x2F,
	ADD_INTO_MC = 0x30,

	SUB_INTO_REG = 0x31,
	SUB_INTO_PS = 0x32,
	SUB_INTO_WS = 0x33,
	SUB_INTO_FB = 0x34,
	SUB_INTO_PLL = 0x35,
	SUB_INTO_MC = 0x36,

	SET_ATI_PORT = 0x37,
	SET_PCI_PORT = 0x38,
	SET_SYSIO_PORT = 0x39,
	SET_REG_BLOCK = 0x3A,

	COMPARE_FROM_REG = 0x3C,
	COMPARE_FROM_PS = 0x3D,
	COMPARE_FROM_WS = 0x3E,
	COMPARE_FROM_FB = 0x3F,
	COMPARE_FROM_PLL = 0x40,
	COMPARE_FROM_MC = 0x41,

	SWITCH = 0x42,

	JUMP_ALWAYS = 0x43,
	JUMP_EQUAL = 0x44,
	JUMP_BELOW = 0x45,
	JUMP_ABOVE = 0x46,
	JUMP_BELOWOREQUAL = 0x47,
	JUMP_ABOVEOREQUAL = 0x48,
	JUMP_NOTEQUAL = 0x49,

	TEST_FROM_REG = 0x4A,
	TEST_FROM_PS = 0x4B,
	TEST_FROM_WS = 0x4C,
	TEST_FROM_FB = 0x4D,
	TEST_FROM_PLL = 0x4E,
	TEST_FROM_MC = 0x4F,

	DELAY_MICROSECONDS = 0x51,

	CLEAR_IN_REG = 0x54,
	CLEAR_IN_PS = 0x55,
	CLEAR_IN_WS = 0x56,
	CLEAR_IN_FB = 0x57,
	CLEAR_IN_PLL = 0x58,
	CLEAR_IN_MC = 0x59,

	MASK_INTO_REG = 0x5C,
	MASK_INTO_PS = 0x5D,
	MASK_INTO_WS = 0x5E,
	MASK_INTO_FB = 0x5F,
	MASK_INTO_PLL = 0x60,
	MASK_INTO_MC = 0x61,

	CALL_TABLE = 0x52,
	END_OF_TABLE = 0x5B,
	SET_DATA_TABLE = 0x66,

	XOR_INTO_REG = 0x67,
	XOR_INTO_PS = 0x68,
	XOR_INTO_WS = 0x69,
	XOR_INTO_FB = 0x6A,
	XOR_INTO_PLL = 0x6B,
	XOR_INTO_MC = 0x6C
};

enum IIOOpcodes {
	NOP = 0,
	START = 1,
	READ = 2,
	WRITE = 3,
	CLEAR = 4,
	SET = 5,
	MOVE_INDEX = 6,
	MOVE_ATTR = 7,
	MOVE_DATA = 8,
	END = 9
};

/// Opcode structures.
/// These are used when parsing opcodes.

/// The following arrays were taken from the linux kernel; this is the most sane way to implement this.
// Translate destination alignment field to the source aligment encoding; taken from the linux amdgpu driver.
static constexpr int atom_dst_to_src[8][4] = {
	{0, 0, 0, 0},
	{1, 2, 3, 0},
	{1, 2, 3, 0},
	{1, 2, 3, 0},
	{4, 5, 6, 7},
	{4, 5, 6, 7},
	{4, 5, 6, 7},
	{4, 5, 6, 7},
};
// Mask arguments.
static constexpr uint32_t atom_arg_mask[8] =
	{ 0xFFFFFFFF, 0xFFFF, 0xFFFF00, 0xFFFF0000, 0xFF, 0xFF00, 0xFF0000,
	  0xFF000000 };
// Shift arguments.
static constexpr int atom_arg_shift[8] = { 0, 0, 8, 16, 0, 8, 16, 24 };

struct AttrByte {
	SrcEncoding dstAlign;
	SrcEncoding srcAlign;
	OpcodeArgEncoding srcArg;

	constexpr uint32_t swizleSrc(uint32_t in) {
		in &= atom_arg_mask[srcAlign];
		in >>= atom_arg_shift[srcAlign];
		return in;
	}

	constexpr uint32_t swizleDst(uint32_t in) {
		in &= atom_arg_mask[dstAlign];
		in >>= atom_arg_shift[dstAlign];
		return in;
	}

	constexpr uint32_t combineSaved(uint32_t in, uint32_t saved) {
		if(dstAlign == SrcEncoding::SrcDword) {
			return in;
		}

		in <<= atom_arg_shift[dstAlign];
		in &= atom_arg_mask[dstAlign];
		saved &= ~atom_arg_mask[dstAlign];
		in |= saved;
		return in;
	}
};

const char* OpcodeArgEncodingToString(OpcodeArgEncoding arg);
const char* SrcEncodingToString(SrcEncoding align);


// The actual AtomBios implementation.
class AtomBiosImpl {
public:
	AtomBiosImpl(uint8_t* data, size_t size);

	// This header is prepended to almost all structures in the AtomBios.
	struct CommonHeader {
		uint16_t structureSize;
		uint8_t tableFormatRevision;
		uint8_t tableContentRevision;

		void dumpToConsole(bool header = false);
	} __attribute__((packed));
	static_assert(sizeof(CommonHeader) == 4, "CommonHeader must have a size of 4!");

	// This table has generally not changed in a breaking way between versions.
	struct AtomRomTable {
		CommonHeader commonHeader;
		char atomMagic[4]; // Should be "ATOM"
		uint16_t biosRuntimeSegmentAddress;
		uint16_t protectedModeInfoOffset;
		uint16_t configFilenameOffset;
		uint16_t crcBlockOffset;
		uint16_t nameStringOffset;
		uint16_t int10Offset;
		uint16_t pciBusDeviceInitCode;
		uint16_t ioBaseAddress;
		uint16_t subsystemVendorID;
		uint16_t subsystemID;
		uint16_t pciInfoOffset;
		uint16_t commandTableBase;
		uint16_t dataTableBase;
		uint8_t extendedFunctionCode;
		uint8_t reserved;

		void dumpToConsole();
	} __attribute__((packed));

	// This table has not changed in a way that concerns us; the actual indexes have some times,
	// but the bytecode always indexes this for us.
	struct DataTable {
		CommonHeader commonHeader;
		union {
			struct {
				uint16_t utilityPipeline;
				uint16_t multimediaCapabilityInfo;
				uint16_t multimediaConfigInfo;
				uint16_t standardVesaTiming;
				uint16_t firmwareInfo;
				uint16_t paletteData;
				uint16_t lcdInfo;
				uint16_t digTransmitterInfo;
				uint16_t analogTVInfo;
				uint16_t supportedDevicesInfo;
				uint16_t gpioI2CInfo;
				uint16_t vramUsageByFirmware;
				uint16_t gpioPinLUT;
				uint16_t vesaToInteralModeLUT;
				uint16_t componentVideoInfo;
				uint16_t powerPlayInfo;
				uint16_t compassionateData;
				uint16_t saveRestoreInfo;
				uint16_t ppllSSInfo;
				uint16_t oemInfo;
				uint16_t xtmdsInfo;
				uint16_t mclkSSInfo;
				uint16_t objectHeader;
				uint16_t indirectIOAccess;
				uint16_t mcInitParameter;
				uint16_t asicVDDCInfo;
				uint16_t asicInternalSSInfo;
				uint16_t tvVideoMode;
				uint16_t vramInfo;
				uint16_t memoryTrainingInfo;
				uint16_t integratedSystemInfo;
				uint16_t asicProfilingInfo;
				uint16_t voltageObjectInfo;
				uint16_t powerSourceInfo;
			} __attribute__((packed));
			struct {
				uint16_t dataTables[34];
			} __attribute__((packed));
		};
	} __attribute__((packed));

	struct Command {
		CommonHeader commonHeader;

		constexpr uint16_t bytecodeSize() {
			return commonHeader.structureSize - 0x6;
		}

		// These are in bytes, but should always be multiples of a "long"
		// (which, for AtomBios, is 32 bit)
		uint8_t workSpaceSize;
		uint8_t parameterSpaceSize;
		// I dont really know what this means other than that something has changed this routine,
		// and it probably isnt that important to track, but lets track it anyway.
		bool updatedByUtility;

		Command() = default;
		Command(const libatombios_vector<uint8_t>& data, int index, uint16_t offset);

		constexpr int i() {
			return _i;
		}

		constexpr uint16_t offset() {
			return _offset;
		}

		constexpr bool exists() {
			return _exists;
		}

	private:
		int _i;
		uint16_t _offset;
		bool _exists = false;
	};

	// TODO: do we want to do this like this?
	struct CommandTable {
		CommonHeader commonHeader;
		CommandTable();

		void readCommands(const libatombios_vector<uint8_t>& data, uint16_t offset);
		libatombios_hashmap<int, Command> commands;
	};

	// This follows the linux driver numbering, however this is not technically needed.
	// (Linux ORs the IIO port and the IO mode together; we do not do this.)
	enum IOMode {
		MM = 0,
		PCI = 1,
		SYSIO = 2,
		IIO = 0x80
	};

	// TODO: this should lock
	void runCommand(AtomBios::CommandTables table, libatombios_vector<uint32_t> params);

	/// Get various telementry metrics.
	constexpr uint32_t maxPSIndex() { return _maxPSIndex; }
	constexpr uint32_t maxWSIndex() { return _maxWSIndex; }

private:
	constexpr uint16_t read16(size_t offset) {
		return static_cast<uint16_t>(_data[offset]) |
			(static_cast<uint16_t>(_data[offset + 1]) << 8);
	}
	constexpr uint32_t read32(size_t offset) {
		return read16(offset) | (static_cast<uint32_t>(read16(offset + 2)) << 16);
	}

	void copyStructure(void* dest, size_t offset, size_t maxSize);
	void _runBytecode(Command& command, libatombios_vector<uint32_t>& params, int params_shift);

	libatombios_vector<uint8_t> _data;
	size_t _atomRomTableBase = 0;
	AtomRomTable _atomRomTable;
	CommandTable _commandTable;
	DataTable _dataTable;

	// Pointer into the ROM from which ID fetches are relative too.
	// Mapped into the WorkSpace.
	uint32_t _dataBlock = 0;

	// Current IO mode.
	IOMode _ioMode = IOMode::MM;
	// Port used in IIO mode.
	uint16_t _iioPort = 0;
	// ROM offsets for IIO functions.
	libatombios_vector<uint32_t> _iioIndexes;

	void _indexIIO(uint32_t base);
	uint32_t _runIIO(uint32_t offset, uint32_t index, uint32_t data);

	// Current reg block.
	uint16_t _regBlock = 0;
	uint32_t _doIORead(uint32_t reg);
	void _doIOWrite(uint32_t reg, uint32_t val);
	// Current FB block.
	uint16_t _fbBlock = 0;

	// Flags.
	bool _flagAbove = false;
	bool _flagEqual = false;
	bool _flagBelow = false;

	// the DIV/MUL registers.
	// Mapped in the WorkSpace, used by the DIV and MUL instructions.
	uint32_t _divMulQuotient = 0;
	uint32_t _divMulRemainder = 0;

	// IIO IO Attributes
	// TODO: what the hell?
	// Mapped into the WorkSpace.
	uint32_t _iioIOAttr = 0;

	// WorkSpace mask generator value.
	// Mapped into the WorkSpace; used to generate an OR and AND mask on two other registers.
	uint32_t _workSpaceMaskShift = 0;

	/// Various and metric-gathering variables.
	// The highest index into the parameter space reached.
	uint32_t _maxPSIndex = 0;
	// The highest index into the work space reached.
	uint32_t _maxWSIndex = 0;
};

void* operator new(size_t size);
void* operator new[](size_t size);

void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, size_t) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete[](void* ptr, size_t) noexcept;
