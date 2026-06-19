#pragma once 
#include "Image.h"
#include "Semaphore.h"

namespace Graphics
{
struct RenderTarget {
	Image image;  
	Semaphore drawFinished; 
	Semaphore imageAvailable;
	RenderTarget(); 
	~RenderTarget();
};
}
