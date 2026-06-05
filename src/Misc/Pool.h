#pragma once 
#include <unordered_map>
#include <vector> 
#include <queue>
#include <cstdint>
#include <stdexcept>
#include <sstream>


struct PoolId
{
	uint32_t id; 
	uint32_t generation;
};

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
	std::vector<uint32_t> generation; 
	std::vector<bool> alive; 
	std::queue<uint32_t> freeSlots;

	std::vector<std::string> names;
	std::unordered_map<std::string, PoolId> nameToId; 

	std::string name; 

	PoolId add(const T&& object_, const char* name_) { 
		PoolId pId;
		if(this->freeSlots.empty())
		{
			pId.id = this->objects.size(); 
			this->objects.emplace_back(object_); 
			this->alive.emplace_back(true);
			this->names.emplace_back(name_); 
			pId.generation = 1; 
			this->generation.emplace_back(pId.generation);
		}
		else 
		{
			pId.id = this->freeSlots.front(); 
			this->objects[pId.id] = object_;
			this->alive[pId.id] = true; 
			pId.generation = this->generation[pId.id];
			this->freeSlots.pop(); 
		}
		this->nameToId.emplace(name_, pId); 
		return pId; 
	}

	void remove(uint32_t id_) {
		this->alive[id_] = false;
		this->generation[id_]++; 
		this->freeSlots.emplace(id_);
		this->nameToId.erase(this->names[id_]);
	}

	T& get(uint32_t id_)
	{
		if(id_ > this->objects.size() || this->alive[id_] == false)
		{
			std::stringstream errorMessageStream;
			errorMessageStream << name << " Pool: object with id " << id_ << " doesn't exist.\n";
			throw std::runtime_error(errorMessageStream.str());
		}
		return this->objects[id_];
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

	Pool(const char* name_) : name(name_) {}; 
};


