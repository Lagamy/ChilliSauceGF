#pragma once
#include <cstdint>
#include <limits>

namespace HelperGlobals { 
	inline const uint32_t UninitializedId = std::numeric_limits<uint32_t>::max(); 
	inline uint64_t assetDelayBeforeUnload; // Can change between scenes.(Flexability). Used for assets with STREAMING lifetimeType
}
