// Custom bool struct to solve both std140 bool size being 4 bytes, and vector<bool> being a bitset.  
#pragma once
#include <cstdint>

struct Bool
{
	uint32_t value = 0;

	Bool();
	Bool(bool value_);
	Bool& operator=(bool value_);
	operator bool() const;
};