#include "GPUDataContainer.h"
#include "Api.h"
#include "MemoryManager.h"
#include <format>
#include <stdexcept>

namespace Graphics
{
	template<bool Readable>
	void GPUDataContainer<Readable>::write(uint64_t offset_, const void* pSrc_, uint64_t size_)
	{
		#ifdef ENGINE_DEBUG
			if(offset_ < this->validFirstOffset || offset_ + size_ > this->validFirstOffset + this->validSize)
			{
				throw std::runtime_error(std::format("GPU Data Container: Write of {} bytes at offset {} is outside the valid range [{}, {}).", size_, offset_, this->validFirstOffset, this->validFirstOffset + this->validSize));
			}
		#endif

		MemoryManager& rMemoryManager = getMemoryManager();
		if(this->isShared)
		{
			rMemoryManager.writeShared(rMemoryManager.getEntry(this->entryId), offset_, pSrc_, size_);
		}
		else
		{
			rMemoryManager.writeStaging(this->stagingData, offset_, pSrc_, size_);
		}
	}

	template<bool Readable>
	void GPUDataContainer<Readable>::read(uint64_t offset_, void* pDst_, uint64_t size_) const requires Readable
	{
		#ifdef ENGINE_DEBUG
			if(offset_ < this->validFirstOffset || offset_ + size_ > this->validFirstOffset + this->validSize)
			{
				throw std::runtime_error(std::format("GPU Data Container: Read of {} bytes at offset {} is outside the valid range [{}, {}).", size_, offset_, this->validFirstOffset, this->validFirstOffset + this->validSize));
			}
		#endif

		MemoryManager& rMemoryManager = getMemoryManager();
		if(this->isShared)
		{
			rMemoryManager.readShared(rMemoryManager.getEntry(this->entryId), offset_, pDst_, size_);
		}
		else
		{
			rMemoryManager.readStaging(this->stagingData, offset_, pDst_, size_);
		}
	}

	template struct GPUDataContainer<false>;
	template struct GPUDataContainer<true>;
}
