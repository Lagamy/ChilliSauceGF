#pragma once 
#include "PoolId.h"
#include "Pool.h"
#include "Utilities.h"
#include <unordered_map>
#include <vector> 
#include <queue>
#include <cstdint>
#include <stdexcept>
#include <sstream>
#include <optional>


template <typename T> 
struct PoolNameless {
	std::vector<T> objects;
	std::vector<bool> alive;
	std::vector<uint32_t> generation; 
	std::vector<uint32_t> freeSlots;	
	std::string name;
	uint32_t elementCount; 

	PoolNameless(const char* name_) : name(name_){};
	PoolNameless() {};
	void isPoolIdValid(PoolId pId_)
	{
		if(pId_ == UninitializedPoolId)
		{
			std::stringstream errorMessageStream;
			errorMessageStream << name << " Pool: UninitializedPoolId can't be used in get function.\n";
			throw std::runtime_error(errorMessageStream.str());
		}

		if(pId_.id >= this->objects.size() || !alive[pId_.id])
		{
			std::stringstream errorMessageStream;
			errorMessageStream << name << " Pool: object with id: " << pId_.id << " doesn't exist.\n";
			throw std::runtime_error(errorMessageStream.str());
		}
		if(this->generation[pId_.id] != pId_.generation)
		{
			std::stringstream errorMessageStream;
			errorMessageStream << name << " Pool: slot with id: " << pId_.id << " has bigger generation(" << pId_.generation << "), than passed one(" << this->generation[pId_.id] << ").\n";
			throw std::runtime_error(errorMessageStream.str());
		}
	}

	template <typename... Args>
    PoolId add(Args&&... args)
    {
        PoolId id = {0, 0};

        if (freeSlots.empty())
        {
            id.id = static_cast<uint32_t>(objects.size());
            this->objects.emplace_back(std::forward<Args>(args)...);
            this->generation.emplace_back(id.generation);
			this->alive.emplace_back(true); 
        }
        else
        {
            id.id = freeSlots.back();
            this->freeSlots.pop_back();
            // reconstruct in-place
			this->objects[id.id] = T(std::forward<Args>(args)...);
            id.generation = generation[id.id];
			this->alive[id.id] = true; 
        }
		this->elementCount++; 
        return id;
    }

	void remove(PoolId pId_) {
		this->isPoolIdValid(pId_);
		this->generation[pId_.id]++; 
		if constexpr (requires { T::destroy(nullptr); })
        {
        	    T::destroy(&objects[pId_.id]);
		}
		this->freeSlots.emplace_back(pId_.id);
		this->alive[pId_.id] = false; 
		this->elementCount--; 
	}

	
	void removeInternal(uint32_t id_) {
		if(this->alive[id_])
		{
			this->generation[id_]++; 
			if constexpr (requires (T& obj) { obj.destroy(); })
			{
    			objects[id_].destroy();
			}
			this->freeSlots.emplace_back(id_);
			this->alive[id_] = false; 
		}
	}

	void clear() 
	{
		for(uint32_t i = 0; i < this->objects.size(); i++)
		{
			this->removeInternal(i); 
		}
		this->elementCount = 0; 
	}

	T& getInternal(uint32_t id_)
	{
		return this->objects[id_];
	}
	
	T& get(PoolId pId_)
	{
		this->isPoolIdValid(pId_); 
		return this->objects[pId_.id];
	}

	T& back()
	{
		if(this->objects.empty())
		{
			std::stringstream errorMessageStream; 
			errorMessageStream << this->name << ": Can't use back() on empty Pool."; 
			throw std::runtime_error(errorMessageStream.str());
		}

		uint32_t id = this->allocatedSize() - 1;
		while(!this->alive[id])
		{
			if(id == 0)
			{
				std::stringstream errorMessageStream; 
				errorMessageStream << this->name << ": Can't use back() on empty Pool."; 
				throw std::runtime_error(errorMessageStream.str());
			}
			id--; 
		}
		return this->objects[id]; 
	}

	T* data()
	{
		return this->objects.data();
	}

	const uint32_t size()
	{
		return this->elementCount; 
	}

	const size_t allocatedSize()
	{
		return this->objects.size();
	}

	const size_t sizeInBytes()
	{
		return this->objects.size() * sizeof(T);
	}
};


