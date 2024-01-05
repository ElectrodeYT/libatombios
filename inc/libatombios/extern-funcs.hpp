#pragma once

#include <stdint.h>

extern "C" [[gnu::weak]] void libatombios_printf_dbg(const char* format, ...);
extern "C" [[gnu::weak]] void libatombios_printf_log(const char* format, ...);
extern "C" [[gnu::weak]] void libatombios_printf_warn(const char* format, ...);
extern "C" [[gnu::weak]] void libatombios_printf_error(const char* format, ...);

// This function is implemented with an implementation that is safe for userspace (uses exit), but can be overrriden.
// It should _not_ return to libatombios code, if possible, but returning _should_ generally be safe enough if this is not possible;
// any truly critical cases are handled with asserts.
extern "C" [[gnu::weak]] void libatombios_printf_panic(const char* format, ...);


// These functions deal with card register reads and stuff.
// TODO: how do we best implement these?
extern "C" [[gnu::weak]] void libatombios_card_reg_write(uint32_t reg, uint32_t val);
extern "C" [[gnu::weak]] uint32_t libatombios_card_reg_read(uint32_t reg);
extern "C" [[gnu::weak]] void libatombios_card_mc_write(uint32_t reg, uint32_t val);
extern "C" [[gnu::weak]] uint32_t libatombios_card_mc_read(uint32_t reg);
extern "C" [[gnu::weak]] void libatombios_card_pll_write(uint32_t reg, uint32_t val);
extern "C" [[gnu::weak]] uint32_t libatombios_card_pll_read(uint32_t reg);
