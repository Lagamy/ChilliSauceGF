#pragma once 
#include "Mesh.h"
#include "Image.h"
#include "Pool.h"

namespace Graphics 
{
struct ResourceManager 
{
    Pool<Mesh> meshes; 
    Pool<Image> textures; 
};
} 