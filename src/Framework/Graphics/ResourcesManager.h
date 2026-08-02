#pragma once 
#include "Mesh.h"
#include "Image.h"
#include "Pool.h"

namespace Graphics 
{
struct ResourceManager 
{
    Pool<Mesh> meshes = ("Meshes"); 
    Pool<Image> textures = ("Textures"); 
};
} 