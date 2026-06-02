#include "RenderTarget.h"

RenderTarget::RenderTarget()
{
	this->imageAvailable.create(); 
	this->drawFinished.create();
}

RenderTarget::~RenderTarget() 
{
	this->image.destroy(); 
	this->drawFinished.destroy(); 
	this->imageAvailable.destroy();
}
