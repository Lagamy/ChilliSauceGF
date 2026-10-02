#pragma once 


using ProjectFunc = void(*)();
struct ProjectManager 
{
    ProjectFunc setupEnvironment;  // set MemoryAllocator paging, maxDormantUpdateHeaps,  
}; 