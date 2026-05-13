#pragma once 
#include <unordered_map>
#include <vector> 
#include <queue>
#include <cstdint>
#include <stdexcept>
#include <sstream>

template <typename T> 
struct Pool {
	std::vector<T> objects;
	std::queue<uint32_t> freeSlots;
	std::string name; 

	uint32_t add(const T&& object_) { 

		uint32_t id; 
		if(this->freeSlots.empty())
		{
			id = this->objects.size(); 
			this->objects.emplace_back(object_); 
		}
		else 
		{
			id = this->freeSlots.front(); 
			this->objects[this->freeSlots.front()] = object_; 
			this->freeSlots.pop(); 
		}

		return id; 
	}

	Pool(const char* name_) : name(name_) {}; 

	void remove(uint32_t id_) { 
		if(id_ == this->objects.size() - 1)
		{
			this->objects.erase(id_); // So we get rid of extra unused slot
		}
		else 
		{
			this->freeSlots.emplace(id_);
			this->objects[id_].exists = false;
		}
	}

	T& get(uint32_t id_)
	{
		if(id_ > this->objects.size() || this->objects[id_].exists == false)
		{
			std::stringstream errorMessageStream;
			errorMessageStream << name << ": object with id " << id_ << " doesn't exist";
			throw std::runtime_error(errorMessageStream.str());
		}
		return this->objects[id_];
	}
};

