#include "Instance.h"

namespace Graphics
{
void Instance::setup() {
	/* Application Info*/
// Most data here doesn't affect the program and is for developer convenience 
	VkApplicationInfo appInfo = {};  // Struct.
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "ChilliSauce engine";
	appInfo.apiVersion = VK_API_VERSION_1_4;


	/* Instance Info */
// Creation info for a VkInstance. Acts as params for VkInstance constructor, just more readable.
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// pNext - is a pointer for additional extension data structs. flags - self explanatory.
	createInfo.pApplicationInfo = &appInfo;

	/* Extensions Setup */
	// Create list to hold available instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// Setup extensions Instance will use 
	uint32_t glfwExtensionCount = 0; // GLFW may require multiple extensions. Size of an array basically. 

	// Extensions passed as array of cstrings.
	// "array" = multiple const char*
	// Each const char* = C-string
	// So const char** = pointer to first element of array of cstrings
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // fills array 

	//Add GLFW extensions to list of extensions 
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.emplace_back(glfwExtensions[i]); // glfwExtensions[i] == *(glfwExtensions + i). Compiler does i * sizeOf(const char*) automatically 
	}
	
	/* Validation layers setup */
	// Check if Debug or Release build(Enables Validation layers if Debug)
	#ifdef ENGINE_DEBUG
		this->validationLayers.debugMessenger.init(); 
		for(const char* extension : this->validationLayers.extensions)
		{
			instanceExtensions.emplace_back(extension);	
		}
		createInfo.enabledLayerCount = validationLayers.layers.size();
		createInfo.ppEnabledLayerNames = validationLayers.layers.data();
		createInfo.pNext = &validationLayers.debugMessenger.createInfo;
	#else 
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	#endif

	// Check if those Instance Extensions supported...
	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()); // Making room for our extensions in createInfo  
	createInfo.ppEnabledExtensionNames = instanceExtensions.data(); // Passing extensions to our createInfo
	

	// Create instance 
	VkResult result = vkCreateInstance(&createInfo, nullptr, &vkHandle); // Second argument is allocator. It is for manually defining where and how to store it in memory. nullptr = allocate and store it automatically    

	// Check if instance was created 
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan instance.");
	}
}

void Instance::destroy() {
	vkDestroyInstance(vkHandle, nullptr); // Second arg - deallocator
	vkHandle = VK_NULL_HANDLE;
}



bool Instance::checkInstanceExtensionSupport(std::vector<const char*>* extensionsForCheck_) {
	uint32_t extensionCount = 0;
	// Get number of extensions, so we can create an array of a correct size. 
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);


	// Create a list of VkExtensionProperties using count 
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

	// Check of given extensions are in list of available extensions 
	for (const auto& checkExtension : *extensionsForCheck_)
	{
		bool hasExtension = false;
		for (const auto& extension : availableExtensions)
		{
			if (strcmp(checkExtension, extension.extensionName))
			{
				hasExtension = true;
				break;
			}
		}

		// If even one exstension from our list isn't supported - return false. 
		if (!hasExtension)
		{
			return false;
		}
	}
	return true;
}

VkInstance Instance::get() const {
	return vkHandle;
}
}
