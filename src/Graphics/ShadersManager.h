#pragma once 
#include "Shader.h"
#include "Pool.h"

namespace Graphics
{
    struct ShadersManager { 
        Pool<Shader> shaders; 
        void destroy();
    }; 
}