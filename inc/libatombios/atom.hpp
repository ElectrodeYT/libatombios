#pragma once

#include <stdint.h>
#include <stddef.h>

class AtomBiosImpl;

class AtomBios {
public:
	AtomBios(uint8_t* data, size_t size);

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

	void runCommand(CommandTables table, uint32_t* params, size_t size);

	const uint32_t maxPSIndex();
	const uint32_t maxWSIndex();
private:
	AtomBiosImpl* _impl;
};
