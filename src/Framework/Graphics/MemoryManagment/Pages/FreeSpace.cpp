#include "FreeSpace.h"

namespace Graphics
{
	void FreeSpace::init(uint64_t size_)
	{
		PoolId wholeIntervalId = this->memoryIntervals.add(0, size_ - 1);
		this->aliveIntervals.emplace_back(wholeIntervalId);
		this->intervalByFirst.emplace(0, wholeIntervalId);
		this->intervalByLast.emplace(size_ - 1, wholeIntervalId);
	}

	bool FreeSpace::allocate(uint64_t size_, uint64_t& rOffset_)
	{
		/* Free intervals are inclusive [start, end]. The range is taken from the start of the first interval that is big enough. */
		for(uint32_t i = 0; i < this->aliveIntervals.size(); i++)
		{
			PoolId intervalId = this->aliveIntervals[i];
			MemoryInterval& rMemoryInterval = this->memoryIntervals[intervalId];
			uint64_t intervalSize = rMemoryInterval.end - rMemoryInterval.start + 1;
			if(intervalSize < size_)
			{
				continue;
			}

			rOffset_ = rMemoryInterval.start;
			this->intervalByFirst.erase(rMemoryInterval.start);
			if(intervalSize == size_)
			{
				this->intervalByLast.erase(rMemoryInterval.end);
				this->aliveIntervals.erase(this->aliveIntervals.begin() + i);
				this->memoryIntervals.remove(intervalId);
				return true;
			}

			rMemoryInterval.start += size_;
			this->intervalByFirst.emplace(rMemoryInterval.start, intervalId);
			return true;
		}
		return false;
	}

	void FreeSpace::free(uint64_t firstByte_, uint64_t lastByte_)
	{
		/* The freed space is merged with free intervals touching it, so two free intervals are never next to each other. */
		bool bordersLeft = firstByte_ > 0 && this->intervalByLast.contains(firstByte_ - 1);
		bool bordersRight = this->intervalByFirst.contains(lastByte_ + 1);

		if(bordersLeft && bordersRight)
		{
			PoolId leftId = this->intervalByLast.at(firstByte_ - 1);
			PoolId rightId = this->intervalByFirst.at(lastByte_ + 1);
			MemoryInterval& rLeft = this->memoryIntervals[leftId];
			MemoryInterval& rRight = this->memoryIntervals[rightId];

			this->intervalByLast.erase(rLeft.end);
			this->intervalByFirst.erase(rRight.start);
			this->intervalByLast.erase(rRight.end);
			rLeft.end = rRight.end;
			this->intervalByLast.emplace(rLeft.end, leftId);

			std::erase(this->aliveIntervals, rightId);
			this->memoryIntervals.remove(rightId);
		}
		else if(bordersLeft)
		{
			PoolId leftId = this->intervalByLast.at(firstByte_ - 1);
			MemoryInterval& rLeft = this->memoryIntervals[leftId];

			this->intervalByLast.erase(rLeft.end);
			rLeft.end = lastByte_;
			this->intervalByLast.emplace(lastByte_, leftId);
		}
		else if(bordersRight)
		{
			PoolId rightId = this->intervalByFirst.at(lastByte_ + 1);
			MemoryInterval& rRight = this->memoryIntervals[rightId];

			this->intervalByFirst.erase(rRight.start);
			rRight.start = firstByte_;
			this->intervalByFirst.emplace(firstByte_, rightId);
		}
		else
		{
			PoolId intervalId = this->memoryIntervals.add(firstByte_, lastByte_);
			this->aliveIntervals.emplace_back(intervalId);
			this->intervalByFirst.emplace(firstByte_, intervalId);
			this->intervalByLast.emplace(lastByte_, intervalId);
		}
	}

	void FreeSpace::destroy()
	{
		this->memoryIntervals.clear();
		this->aliveIntervals.clear();
		this->intervalByFirst.clear();
		this->intervalByLast.clear();
	}
}
