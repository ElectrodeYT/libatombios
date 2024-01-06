#pragma once

#include <map>
#include <memory>
#include <vector>
#include <stdint.h>


class AtomBios {
public:
	AtomBios(const std::vector<uint8_t>& data);

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

	// What table does what is standard across cards;
	// this enum maps them.
	// Not all of these may exist on each card!
	// The names are taken from the linux driver.
	// TODO: do we want to this like this?
	enum CommandTables {
		ASIC_Init = 0,
		GetDisplaySurfaceSize,
		ASIC_RegistersInit,
		VRAM_BlockVenderDetection,
		DIGxEncoderControl,
		MemoryControllerInit,
		EnableCRTCMemReq,
		MemoryParamAdjust,
		DVOEncoderControl,
		GPIOPinControl,
		SetEngineClock,
		SetMemoryClock,
		SetPixelClock,
		EnableDispPowerGating,
		ResetMemoryDLL,
		ResetMemoryDevice,
		MemoryPLLInit,
		AdjustDisplayPll,
		AdjustMemoryController,
		EnableASIC_StaticPwrMgt,
		SetUniphyInstance,
		DAC_LoadDetection,
		LVTMAEncoderControl,
		HW_Misc_Operation,
		DAC1EncoderControl,
		DAC2EncoderControl,
		DVOOutputControl,
		CV1OutputControl,
		GetConditionalGoldenSetting,
		TVEncoderControl,
		PatchMCSetting,
		MC_SEQ_Control,
		Gfx_Harvesting,
		EnableScaler,
		BlankCRTC,
		EnableCRTC,
		GetPixelClock,
		EnableVGA_Render,
		GetSCLKOverMCLKRatio,
		SetCRTC_Timing,
		SetCRTC_OverScan,
		SetCRTC_Replication,
		SelectCRTC_Source,
		EnableGraphSurfaces,
		UpdateCRTC_DoubleBufferRegisters,
		LUT_AutoFill,
		EnableHW_IconCursor,
		GetMemoryClock,
		GetEngineClock,
		SetCRTC_UsingDTDTiming,
		ExternalEncoderControl,
		LVTMAOutputControl,
		VRAM_BlockDetectionByStrap,
		MemoryCleanUp,
		ProcessI2cChannelTransaction,
		WriteOneByteToHWAssistedI2C,
		ReadHWAssistedI2CStatus,
		SpeedFanControl,
		PowerConnectorDetection,
		MC_Synchronization,
		ComputeMemoryEnginePLL,
		MemoryRefreshConversion,
		VRAM_GetCurrentInfoBlock,
		DynamicMemorySettings,
		MemoryTraining,
		EnableSpreadSpectrumOnPPLL,
		TMDSAOutputControl,
		SetVoltage,
		DAC1OutputControl,
		DAC2OutputControl,
		ComputeMemoryClockParam,
		ClockSource,
		MemoryDeviceInit,
		GetDispObjectInfo,
		DIG1EncoderControl,
		DIG2EncoderControl,
		DIG1TransmitterControl,
		DIG2TransmitterControl,
		ProcessAuxChannelTransaction,
		DPEncoderService,
		GetVoltageInfo
	};

	struct Command {
		CommonHeader commonHeader;
		// These are in bytes, but should always be multiples of a "long"
		// (which, for AtomBios, is 32 bit)
		uint8_t workSpaceSize;
		uint8_t parameterSpaceSize;
		// I dont really know what this means other than that something has changed this routine,
		// and it probably isnt that important to track, but lets track it anyway.
		bool updatedByUtility;

		Command(const std::vector<uint8_t>& data, int index, uint16_t offset);

		int _i;
		uint16_t _offset;
		std::vector<uint8_t> _bytecode;
	};

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
	static const char* OpcodeArgEncodingToString(OpcodeArgEncoding arg);

	enum JumpArgEncoding {
		Above = 0,
		AboveOrEqual,
		Always,
		Below,
		BelowOrEqual,
		Equal,
		NotEqual
	};

	enum IOMode {
		MM,
		PCI,
		SYSIO,
		IIO
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
	static const char* SrcEncodingToString(SrcEncoding align);

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

		uint32_t swizleSrc(uint32_t in) {
			in &= atom_arg_mask[srcAlign];
			in >>= atom_arg_shift[srcAlign];
			return in;
		}

		uint32_t swizleDst(uint32_t in) {
			in &= atom_arg_mask[dstAlign];
			in >>= atom_arg_shift[dstAlign];
			return in;
		}

		uint32_t combineSaved(uint32_t in, uint32_t saved) {
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

		SET_ATI_PORT = 0x37,
		SET_PCI_PORT = 0x38,
		SET_SYSIO_PORT = 0x39,

		COMPARE_FROM_REG = 0x3C,
		COMPARE_FROM_PS = 0x3D,
		COMPARE_FROM_WS = 0x3E,
		COMPARE_FROM_FB = 0x3F,
		COMPARE_FROM_PLL = 0x40,
		COMPARE_FROM_MC = 0x41,

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
		SET_DATA_TABLE = 0x66
	};

	// TODO: do we want to do this like this?
	struct CommandTable {
		CommonHeader commonHeader;

		void readCommands(const std::vector<uint8_t>& data, uint16_t offset);
		std::map<int, std::shared_ptr<Command>> commands;
	};

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

	// TODO: this should lock
	void runCommand(CommandTables table, std::vector<uint32_t> params);

private:
	uint16_t read16(size_t offset) {
		return static_cast<uint16_t>(_data[offset]) |
			(static_cast<uint16_t>(_data[offset + 1]) << 8);
	}
	uint32_t read32(size_t offset) {
		return read16(offset) | (static_cast<uint32_t>(read16(offset + 2)) << 16);
	}

	void copyStructure(void* dest, size_t offset, size_t maxSize);
	void _runBytecode(std::shared_ptr<Command> command, std::vector<uint32_t>& params, int params_shift);

	std::vector<uint8_t> _data;
	size_t _atomRomTableBase = 0;
	AtomRomTable _atomRomTable;
	CommandTable _commandTable;
	DataTable _dataTable;

	int _activeDataTable;
	uint32_t _getDataTableOffset();

	// Current IO mode.
	IOMode _ioMode = IOMode::MM;
	// Port used in IIO mode.
	uint16_t _iioPort = 0;
	uint32_t _doIORead(uint32_t reg);
	void _doIOWrite(uint32_t reg, uint32_t val);

	// Flags.
	bool _flagAbove = false;
	bool _flagEqual = false;
	bool _flagBelow = false;
};
