#pragma once
#include "Image.h"
#include "PoolMap.h"

namespace Graphics 
{
struct ImagesManager 
{
    PoolMap<Image> cubemaps = ("Cubemap Images"); 
    PoolMap<Image> volumetrics = ("Volumetric Images"); 
    PoolMap<Image> textures = ("Texture Images"); 
    PoolMap<Image> renderTargets = ("RenderTarget Images"); 
};
}