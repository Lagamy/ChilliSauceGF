#include "DebugMessenger.h"

void DebugMessenger::init()
{
	this->createInfo.sType =
    	VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

	this->createInfo.messageSeverity =
    	VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    	VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	this->createInfo.messageType =
    	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    	VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	this->createInfo.pfnUserCallback = debugCallback;
}
