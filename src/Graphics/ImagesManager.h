#pragma once
#include "Image.h"
#include "Pool.h"

namespace Graphics 
{
struct ImagesManager 
{
    Pool<Image> cubemaps = ("Cubemap Images"); 
    Pool<Image> volumetrics = ("Volumetric Images"); 
    Pool<Image> textures = ("Texture Images"); 
    Pool<Image> renderTargets = ("RenderTarget Images"); 
};
}