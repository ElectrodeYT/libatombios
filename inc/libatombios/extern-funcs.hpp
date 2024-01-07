#pragma once

#include <stdint.h>

// Ensure libc functions are available
#ifndef __STDC_HOSTED__

__attribute__((nonnull(1, 2))) void *memcpy(void *dest, const void *src, size_t n);
__attribute__((nonnull(1, 2))) int memcmp(const void *s1, const void *s2, size_t n);
__attribute__((nonnull(1))) void *memset(void *s, int c, size_t n);
__attribute__((nonnull(1, 2))) int strcmp(const char *s1, const char *s2);

#else

#include <string.h>

#endif

// libatombios uses the printf definitions from lilrad, in order to reduce the amount of proxy work that needs to be done.
// This prevents multiple definitions of the functions from existing.
#ifndef __LILRAD_PRINTS_DEFINED
#define __LILRAD_PRINTS_DEFINED

enum LilradLogType {
	DEBUG = 0,
	VERBOSE,
	INFO,
	WARNING,
	ERROR
};

extern "C" __attribute__((format (printf, 2, 3))) void lilrad_log(enum LilradLogType type, const char* fmt, ...);

#endif

// These functions deal with card register reads and stuff.
// TODO: how do we best implement these?
extern "C" [[gnu::weak]] void libatombios_card_reg_write(uint32_t reg, uint32_t val);
extern "C" [[gnu::weak]] uint32_t libatombios_card_reg_read(uint32_t reg);
extern "C" [[gnu::weak]] void libatombios_card_mc_write(uint32_t reg, uint32_t val);
extern "C" [[gnu::weak]] uint32_t libatombios_card_mc_read(uint32_t reg);
extern "C" [[gnu::weak]] void libatombios_card_pll_write(uint32_t reg, uint32_t val);
extern "C" [[gnu::weak]] uint32_t libatombios_card_pll_read(uint32_t reg);

// These functions should delay for an amount of time.
extern "C" [[gnu::weak]] void libatombios_delay_microseconds(uint32_t microseconds);
extern "C" [[gnu::weak]] void libatombios_delay_milliseconds(uint32_t milliseconds);

#if __has_include("assert.h")
#include <assert.h>
#endif
