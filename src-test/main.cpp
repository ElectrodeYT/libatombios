#include <iostream>
#include <fstream>

#include <stdarg.h>
#include <string.h>

#include <libatombios/atom.hpp>
#include <libatombios/extern-funcs.hpp>

#include <map>

std::map<uint32_t, uint32_t> readRegisterLog;
std::map<uint32_t, uint32_t> writeRegisterLog;

constexpr bool suppressLogs = false;

const char* logTypeToString(enum LilradLogType type) {
	switch(type) {
	case DEBUG:
		return "libatombios [DEBG]: ";
	case VERBOSE:
		return "libatombios [VERB]: ";
	case INFO:
		return "libatombios [INFO]: ";
	case WARNING:
		return "libatombios [WARN]: ";
	case ERROR:
		return "libatombios [ERR ]: ";
	}

	__builtin_unreachable();
}

extern "C" void lilrad_log(enum LilradLogType type, const char* format, ...) {
	va_list arglist;
	va_start(arglist, format);

	if(suppressLogs) {
		va_end(arglist);
		return;
	}

	static char buffer[1024];
	strcpy(buffer, logTypeToString(type));
	strncat(buffer, format, 1023 - strlen(logTypeToString(type)));
	vprintf(buffer, arglist);

	va_end(arglist);
}

extern "C" [[gnu::weak]] void libatombios_card_reg_write(uint32_t reg, uint32_t val) {
	if(readRegisterLog.count(reg) == 0) {
		writeRegisterLog[reg] = 1;
	} else {
		writeRegisterLog[reg]++;
	}

	if(!suppressLogs) {
		printf("aaa: reg=%x, val=%x\n", reg, val);
	}
}
extern "C" [[gnu::weak]] uint32_t libatombios_card_reg_read(uint32_t reg) {
	uint32_t val = 0xAA; //0xAA;

	if(reg == 0x1b9c) {
		val = 0xFF01FFFF;
		//val = 0;
	} else if(reg == 0x394) {
		val = 0x00001F00;
	} else if(reg == 0x4ccd) {
		val = 0x00010000;
	} else if(reg == 0x4bcb) {
		val = 0x00010000;
	} else if(reg == 0x4ccc) {
		val = 0x00010000;
	} else if(reg == 0x83) {
		exit(0);
	}

	if(readRegisterLog.count(reg) == 0) {
		readRegisterLog[reg] = 1;
	} else {
		readRegisterLog[reg]++;
	}

	if(!suppressLogs) {
		printf("aaa: reg=%x, val=%x\n", reg, val);
	}
	return val;
}
extern "C" [[gnu::weak]] void libatombios_card_mc_write(uint32_t reg, uint32_t val) {
	printf("aaa: reg=%x, val=%x\n", reg, val);
}
extern "C" [[gnu::weak]] uint32_t libatombios_card_mc_read(uint32_t reg) {
	uint32_t val = 0xAA;
	printf("aaa: reg=%x, val=%x\n", reg, val);
	return val;
}
extern "C" [[gnu::weak]] void libatombios_card_pll_write(uint32_t reg, uint32_t val) {
	printf("aaa: reg=%x, val=%x\n", reg, val);
}
extern "C" [[gnu::weak]] uint32_t libatombios_card_pll_read(uint32_t reg) {
	uint32_t val = 0xAA;
	printf("aaa: reg=%x, val=%x\n", reg, val);
	return val;
}

extern "C" [[gnu::weak]] void libatombios_delay_microseconds(uint32_t microseconds) {
	
}
extern "C" [[gnu::weak]] void libatombios_delay_milliseconds(uint32_t milliseconds) {
	
}

int main(int argc, char** argv) {
	if(argc != 2) {
		std::cerr << "TODO: actual argument parsing lol" << std::endl;
		exit(1);
	}

	std::ifstream fileStream(argv[1], std::ios::in | std::ios::binary | std::ios::ate);
	size_t fileSize = fileStream.tellg();
	fileStream.seekg(0, std::ios::beg);

	std::vector<uint8_t> data;
	data.resize(fileSize);
	fileStream.read((char*)data.data(), fileSize);
	fileStream.close();

	AtomBios atomBios(data);
	
	//std::vector<uint32_t> params = {0xAABBCCDD, 0xEEFF0011};
	std::vector<uint32_t> params = {0, 0};
	atomBios.runCommand(AtomBios::CommandTables::ASIC_Init, params);

	std::cout << "Read register log:" << std::endl;
	for(auto const& [reg, count] : readRegisterLog) {
		std::cout << std::hex << reg << ": " << std::dec << count << std::endl;
	}

	std::cout << "Write register log:" << std::endl;
	for(auto const& [reg, count] : writeRegisterLog) {
		std::cout << std::hex << reg << ": " << std::dec << count << std::endl;
	}
}
