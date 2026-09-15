#pragma once 
#include "Shader.h"
#include "PoolMap.h"

namespace Graphics
{
    struct ShadersManager { 
        PoolMap<Shader> shaders = ("Shaders"); 
        void destroy();
    }; 
}