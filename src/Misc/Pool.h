#pragma once 
#include "PoolId.h"
#include "Utilities.h"
#include <unordered_map>
#include <vector> 
#include <queue>
#include <cstdint>
#include <stdexcept>
#include <sstream>
#include <optional>

inline bool operator==(const PoolId& a, const PoolId& b)
{
    return a.id == b.id && a.generation == b.generation;
}

inline bool operator!=(const PoolId& a, const PoolId& b)
{
    return !(a == b);
}

template <typename T> 
struct Pool {
	std::vector<T> objects;
	std::vector<bool> alive;
	std::vector<uint32_t> generation; 
	std::vector<uint32_t> freeSlots;	
	std::vector<std::string> names;
	std::unordered_map<std::string, PoolId> nameToId;
	std::string name;

	Pool(){};
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
		this->nameToId.erase(this->names[pId_.id]);
		this->names[pId_.id] = ""; 
		this->alive[pId_.id] = false; 
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

		uint32_t id = this->size() - 1;
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

	PoolId getIdByName(const char* name_)
	{
		const auto& iterator = this->nameToId.find(name_);
		if (iterator == this->nameToId.end()) // doesn't exist 
		{
			std::stringstream errorMessageStream;
			errorMessageStream << name << " Pool: no object with name " << name_ << " was found.\n";
			throw std::runtime_error(errorMessageStream.str());
		}
		return iterator->second;
	}

	std::string& getName(PoolId pId_)
	{
		this->isPoolIdValid(pId_);
		return this->names[pId_.id]; 
	}

	Pool(const char* name_) : name(name_) {};

	T* data()
	{
		return this->objects.data();
	}

	const size_t size()
	{
		return this->objects.size();
	}

	const size_t sizeInBytes()
	{
		return this->objects.size() * sizeof(T);
	}
};


