#include <libatombios/extern-funcs.hpp>
#include "atom-private.hpp"

void* operator new(size_t size) {
	return lilrad_alloc(size);
}
void* operator new[](size_t size) {
	return lilrad_alloc(size);
}

void operator delete(void* ptr) noexcept {
	lilrad_free(ptr);
}
void operator delete(void* ptr, size_t) noexcept {
	lilrad_free(ptr);
}
void operator delete[](void* ptr) noexcept {
	lilrad_free(ptr);
}
void operator delete[](void* ptr, size_t) noexcept {
	lilrad_free(ptr);
}
