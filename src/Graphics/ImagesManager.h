#pragma once
#include "Image.h"
#include "Pool.h"

namespace Graphics 
{
struct ImagesManager 
{
    Pool<Image> cubemaps; 
    Pool<Image> volumetrics; 
    Pool<Image> textures; 
    Pool<Image> renderTargets; 
};
}