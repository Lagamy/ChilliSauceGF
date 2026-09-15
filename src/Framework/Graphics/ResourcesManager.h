#pragma once 
#include "Mesh.h"
#include "Image.h"
#include "PoolMap.h"

namespace Graphics 
{
struct ResourceManager 
{
    PoolMap<Mesh> meshes = ("Meshes"); 
    PoolMap<Image> textures = ("Textures"); 
};
} 