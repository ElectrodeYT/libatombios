#pragma once

#include <libatombios/extern-funcs.hpp>

#include <frg/allocation.hpp>
#include <frg/vector.hpp>
#include <frg/hash.hpp>
#include <frg/hash_map.hpp>

struct LibAtombiosAllocator {
		void* allocate(size_t size) {
			return lilrad_alloc(size);
		}

		void deallocate(void* ptr, size_t) {
			lilrad_free(ptr);
		}

		void free(void* ptr) {
			lilrad_free(ptr);
		}
};

template<typename T>
using libatombios_vector = frg::vector<T, LibAtombiosAllocator>;

template<typename Key, typename Value>
using libatombios_hashmap = frg::hash_map<Key, Value, frg::hash<Key>, LibAtombiosAllocator>;
