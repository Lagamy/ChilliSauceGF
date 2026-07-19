#pragma once 
#include "Shader.h"
#include "Pool.h"

namespace Graphics
{
    struct ShadersManager { 
        Pool<Shader> shaders = ("Shaders"); 
        void destroy();
    }; 
}