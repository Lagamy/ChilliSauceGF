cmake_minimum_required(VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
# if(POLICY CMP0141)
  # cmake_policy(SET CMP0141 NEW)
  # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
# endif()

project("PBRVulkanDemo")

# Generate compile_commands.json for clangd
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")

# GLFW 
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/GLFW")

add_library(glfw3 STATIC IMPORTED)
set_target_properties(glfw3 PROPERTIES
    IMPORTED_LOCATION "${GLFW_DIR}/lib/libglfw3.a"
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_DIR}/include"
)

# GLM 
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party/glm
)

# Vulkan 
find_package(Vulkan REQUIRED)


# Add source to this project's executable.
add_executable(PBRVulkanDemo
	# Core 
	"src/main.cpp" "src/Core.cpp" "src/Utilities.cpp" "src/Api.cpp" 

	# Managers
	# "src/Core/Managers/AssetsManager.cpp"
	# "src/Core/Managers/InputManager.cpp" 

	# Graphics
	"src/Graphics/FrameResources.cpp" "src/Graphics/Backend/Base/Shader.cpp"
	"src/Graphics/Backend/Base/Device.cpp" "src/Graphics/Backend/Base/Instance.cpp" 
   	"src/Graphics/Backend/Base/Swapchain.cpp" "src/Graphics/Renderer.cpp"	
	"src/Graphics/Backend/Base/Surface.cpp" "src/Graphics/Backend/Image/Image.cpp"
	"src/Graphics/Backend/Base/MemoryBlock.cpp" "src/Graphics/Backend/Base/Buffer.cpp" 
	"src/Graphics/Backend/Base/RenderPass.cpp" "src/Graphics/Backend/Base/Framebuffer.cpp"
	# "src/Graphics/Backend/DescriptorSet/DescriptorSetLayout.cpp" "src/Graphics/Backend/DescriptorSet/BindingLayout.cpp"
	"src/Graphics/Backend/Base/SubPass.cpp"	"src/Graphics/Backend/CommandBuffer/FrameCommandPool.cpp" 
	"src/Graphics/Backend/CommandBuffer/OneShotCommandPool.cpp" "src/Graphics/GPUSceneManager.cpp" 
	"src/Graphics/Backend/Base/GraphicsPipeline.cpp" "src/Graphics/Backend/Base/PipelineLayout.cpp" 
	"src/Graphics/Backend/Synchronisation/Semaphore.cpp" "src/Graphics/Backend/Synchronisation/Fence.cpp" 	
	# "src/Graphics/Backend/DescriptorSet/DescriptorSet.cpp" 
	"src/Graphics/MemoryManagment/GPUHeap.cpp" "src/Graphics/SyncManager.cpp"
	"src/Graphics/MemoryManagment/StagingHeap.cpp" "src/Graphics/MemoryManagment/StaticAllocator.cpp"
	"src/Graphics/FrameResources.cpp" "src/Graphics/MemoryManager.cpp" "src/Graphics/Backend/ValidationLayers/DebugMessenger.cpp" 
	"src/Graphics/ShadersManager.cpp" "src/Graphics/Resources/Mesh.cpp" "src/Graphics/Pass/Task.cpp" "src/Graphics/Pass/Pass.cpp"
	"src/Graphics/PassesGraph.cpp" "src/Graphics/PipelinesManager.cpp" "src/Graphics/Pass/SubmissionBatch.cpp"

	# Demos
	"src/Demos/Triangle.cpp"

	# Misc 
	"src/Misc/Bool.cpp"

	# Reflection 
	"src/Graphics/Reflection/Layout.cpp" "src/Graphics/Reflection/MemberBlueprint.cpp"
)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(CORE_DIR "${SRC_DIR}/Core")
set(GRAPHICS_DIR "${SRC_DIR}/Graphics")
set(MISC_DIR "${SRC_DIR}/Misc")
set(DEMOS_DIR "${SRC_DIR}/Demos")

# Core
# set(ASSETS_DIR "${CORE_DIR}/Assets")
set(MANAGERS_DIR "${CORE_DIR}/Managers")


# Graphics
set(RESOURCES_DIR "${GRAPHICS_DIR}/Resources")
set(BACKEND_DIR "${GRAPHICS_DIR}/Backend")
set(PASS_DIR "${GRAPHICS_DIR}/Pass")
set(IMAGE_DIR "${BACKEND_DIR}/Image")
set(BASE_DIR "${BACKEND_DIR}/Base")
set(COMMANDBUFFER_DIR "${BACKEND_DIR}/CommandBuffer")
set(SUBMISSION_DIR "${BACKEND_DIR}/Submission") 

# set(DESCRIPTORSET_DIR "${BACKEND_DIR}/DescriptorSet")
set(SYNCHRONISATION_DIR "${BACKEND_DIR}/Synchronisation")
set(VALIDATION_DIR "${BACKEND_DIR}/ValidationLayers")
set(MEMORY_MANAGMENT_DIR "${GRAPHICS_DIR}/MemoryManagment")
set(REFLECTION_DIR "${GRAPHICS_DIR}/Reflection")

# Link dependencies
target_link_libraries(PBRVulkanDemo
    PUBLIC glfw3
    PUBLIC Vulkan::Vulkan
    PUBLIC glm
)

target_include_directories(PBRVulkanDemo PRIVATE 
	# Src
	${SRC_DIR} ${CORE_DIR} ${GRAPHICS_DIR}
	${MISC_DIR} ${TEST_DIR}

	# Core
	# ${ASSETS_DIR} 
	${MANAGERS_DIR} 

	# Graphics
	${BACKEND_DIR} ${IMAGE_DIR} ${BASE_DIR} ${COMMANDBUFFER_DIR} ${RESOURCES_DIR} ${PASS_DIR}
	# ${DESCRIPTORSET_DIR} 
	${SYNCHRONISATION_DIR} ${VALIDATION_DIR} ${MEMORY_MANAGMENT_DIR} ${SUBMISSION_DIR}

	# Demos
	${DEMOS_DIR}

	# Reflection
	${REFLECTION_DIR}
)

# Put the executable into /bin
set_target_properties(PBRVulkanDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Create a folder with Media
# add_custom_command(TARGET PBRVulkanDemo POST_BUILD
  # COMMAND ${CMAKE_COMMAND} -E copy_directory
  # "${CMAKE_SOURCE_DIR}/Assets"
  # "${RUNTIME_OUTPUT_DIRECTORY}/Assets"
# )

target_compile_definitions(PBRVulkanDemo PRIVATE
	$<$<CONFIG:Debug>:ENGINE_DEBUG>
)
