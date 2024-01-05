#include <iostream>
#include <fstream>

#include <stdarg.h>
#include <string.h>

#include <libatombios/atom.hpp>
#include <libatombios/extern-funcs.hpp>

extern "C" void libatombios_printf_dbg(const char* format, ...) {
	va_list arglist;
	va_start(arglist, format);

	static char buffer[1024];
	static const char prefix[] = "libatombios [DBG]: ";
	strcpy(buffer, prefix);
	strncat(buffer, format, 1023 - strlen(prefix));
	vprintf(buffer, arglist);

	va_end(arglist);
}

extern "C" void libatombios_printf_log(const char* format, ...) {
	va_list arglist;
	va_start(arglist, format);

	static char buffer[1024];
	static const char prefix[] = "libatombios [LOG]: ";
	strcpy(buffer, prefix);
	strncat(buffer, format, 1023 - strlen(prefix));
	vprintf(buffer, arglist);

	va_end(arglist);
}

extern "C" void libatombios_printf_warn(const char* format, ...) {
	va_list arglist;
	va_start(arglist, format);

	static char buffer[1024];
	static const char prefix[] = "libatombios [WRN]: ";
	strcpy(buffer, prefix);
	strncat(buffer, format, 1023 - strlen(prefix));
	vfprintf(stderr, buffer, arglist);

	va_end(arglist);
}

extern "C" void libatombios_printf_error(const char* format, ...) {
	va_list arglist;
	va_start(arglist, format);

	static char buffer[1024];
	static const char prefix[] = "libatombios [ERR]: ";
	strcpy(buffer, prefix);
	strncat(buffer, format, 1023 - strlen(prefix));
	vfprintf(stderr, buffer, arglist);

	va_end(arglist);
}

extern "C" [[gnu::weak]] void libatombios_card_reg_write(uint32_t reg, uint32_t val) {
	printf("aaa: reg=%x, val=%x\n", reg, val);
}
extern "C" [[gnu::weak]] uint32_t libatombios_card_reg_read(uint32_t reg) {
	uint32_t val = 0xAA;
	printf("aaa: reg=%x, val=%x\n", reg, val);
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
	
	atomBios.runCommand(AtomBios::CommandTables::ASIC_Init);
}
