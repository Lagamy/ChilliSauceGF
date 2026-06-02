#pragma once 
#include "Image.h"
#include "Semaphore.h"

struct RenderTarget {
	Image image;  
	Semaphore drawFinished; 
	Semaphore imageAvailable;
	RenderTarget(); 
	~RenderTarget();
};
