#pragma once
#include <concepts>
#include <cstdint>

namespace Graphics
{
	/* What ReflectionLayout::setMember needs from a container: a repeat count for the bounds check and a way to write N bytes at an offset. */
	template<typename C>
	concept WritableDataContainer = requires(C& rContainer_, uint64_t offset_, const void* pSrc_, uint64_t size_)
	{
		{ rContainer_.repeatCount } -> std::convertible_to<uint32_t>;
		rContainer_.write(offset_, pSrc_, size_);
	};

	/* What ReflectionLayout::getMember needs from a container: a repeat count for the bounds check and a way to read N bytes at an offset. */
	template<typename C>
	concept ReadableDataContainer = requires(const C& rContainer_, uint64_t offset_, void* pDst_, uint64_t size_)
	{
		{ rContainer_.repeatCount } -> std::convertible_to<uint32_t>;
		rContainer_.read(offset_, pDst_, size_);
	};
}
