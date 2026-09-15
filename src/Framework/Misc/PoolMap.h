#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <unordered_map>
#include <vector> 
#include <cstdint>
#include <stdexcept>
#include <sstream>
#include <format>

template <typename T> 
struct PoolMap {
	std::vector<T> objects;
	std::vector<bool> alive;
	std::vector<uint32_t> generation; 
	std::vector<uint32_t> freeSlots;	
	std::vector<std::string> names;
	std::unordered_map<std::string, PoolId> nameToId;
	std::string name;
	uint32_t elementCount; 

	PoolMap(const char* name_) : name(name_){};
	void isPoolIdValid(PoolId pId_)
	{
		if(pId_ == UninitializedPoolId)
		{
			throw std::runtime_error(std::format("{} Pool: UninitializedPoolId can't be used in get function.", this->name));
		}

		if(pId_.id >= this->objects.size() || !alive[pId_.id])
		{
			throw std::runtime_error(std::format("{} Pool: object with id: {} doesn't exist.",this->name, pId_.id));
		}
		if(this->generation[pId_.id] != pId_.generation)
		{
			throw std::runtime_error(std::format("{} Pool: slot with id: {} has bigger generation({}), than passed one({}).", this->name, pId_.id, pId_.generation, this->generation[pId_.id]));
		}
	}

	template <typename... Args>
    PoolId add(const char* name_, Args&&... args)
    {
        PoolId id = {0, 0};

        if (freeSlots.empty())
        {
            id.id = static_cast<uint32_t>(objects.size());
            this->objects.emplace_back(std::forward<Args>(args)...);
            this->generation.emplace_back(id.generation);
			this->names.emplace_back(name_);
			this->alive.emplace_back(true); 
        }
        else
        {
            id.id = freeSlots.back();
            this->freeSlots.pop_back();
            // reconstruct in-place
			this->objects[id.id] = T(std::forward<Args>(args)...);
            id.generation = generation[id.id];
			this->names[id.id] = name_;
			this->alive[id.id] = true; 
        }
		nameToId.emplace(name_, id);
		this->elementCount++;
        return id;
    }

	void remove(PoolId pId_) {

		#ifdef ENGINE_DEBUG
			this->isPoolIdValid(pId_);
		#endif
		this->generation[pId_.id]++; 
		if constexpr (requires { T::destroy(nullptr); })
        {
        	    T::destroy(&objects[pId_.id]);
		}
		this->freeSlots.emplace_back(pId_.id);
		this->nameToId.erase(this->names[pId_.id]);
		this->names[pId_.id] = ""; 
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
			this->nameToId.erase(this->names[id_]);
			this->names[id_] = ""; 
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
	
	T& operator[](PoolId pId_)
	{
		#ifdef ENGINE_DEBUG
			this->isPoolIdValid(pId_); 
		#endif
		return this->objects[pId_.id];
    }

    const T& operator[](PoolId pId_) const 
	{
    	#ifdef ENGINE_DEBUG
			this->isPoolIdValid(pId_); 
		#endif
		return this->objects[pId_.id];
	}

	T& back()
	{
		
		#ifdef ENGINE_DEBUG
			if(this->objects.empty())
			{
				throw std::runtime_error(std::format("{} Pool: Can't use back() on empty Pool.", this->name));
			}
		#endif

		uint32_t id = this->allocatedSize() - 1;
		while(!this->alive[id])
		{
			#ifdef ENGINE_DEBUG
				if(id == 0)
				{
					throw std::runtime_error(std::format("{} Pool: Can't use back() on empty Pool.", this->name));
				}
				id--;
			#endif 
		}
		return this->objects[id]; 
	}

	PoolId getIdByName(const char* name_)
	{
		const auto& iterator = this->nameToId.find(name_);
		
		#ifdef ENGINE_DEBUG
			if (iterator == this->nameToId.end()) // doesn't exist 
			{
				throw std::runtime_error(std::format("{} Pool: no object with name {} was found.", this->name, name_));
			}
		#endif
		return iterator->second;
	}

	std::string& getName(PoolId pId_)
	{
		
		#ifdef ENGINE_DEBUG
			this->isPoolIdValid(pId_);
		#endif
		return this->names[pId_.id]; 
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

	const size_t allocatedSizeInBytes()
	{
		return this->objects.size() * sizeof(T);
	}

};


