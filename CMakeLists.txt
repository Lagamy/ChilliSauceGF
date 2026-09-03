cmake_minimum_required(VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
# if(POLICY CMP0141)
  # cmake_policy(SET CMP0141 NEW)
  # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
# endif()

project("ChilliSauceGF")

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
add_executable(ChilliSauceGF
	# Core 
	"src/Demo/main.cpp" "src/Framework/Core.cpp" "src/Framework/Utilities.cpp" "src/Framework/Api.cpp" 

	# Managers
	# "src/Core/Managers/AssetsManager.cpp"
	# "src/Core/Managers/InputManager.cpp" 

	# Graphics
	"src/Framework/Graphics/FrameResources.cpp" "src/Framework/Graphics/Backend/Base/Shader.cpp"
	"src/Framework/Graphics/Backend/Base/Device.cpp" "src/Framework/Graphics/Backend/Base/Instance.cpp" 
   	"src/Framework/Graphics/Backend/Base/Swapchain.cpp" "src/Framework/Graphics/Renderer.cpp"	
	"src/Framework/Graphics/Backend/Base/Surface.cpp" "src/Framework/Graphics/Backend/Image/Image.cpp"
	"src/Framework/Graphics/Backend/Base/MemoryBlock.cpp" "src/Framework/Graphics/Backend/Base/Buffer.cpp" 
	"src/Framework/Graphics/Backend/Base/RenderPass.cpp" "src/Framework/Graphics/Backend/Base/Framebuffer.cpp"
	# "src/Graphics/Backend/DescriptorSet/DescriptorSetLayout.cpp" "src/Graphics/Backend/DescriptorSet/BindingLayout.cpp"
	"src/Framework/Graphics/Backend/Base/SubPass.cpp"	"src/Framework/Graphics/Backend/CommandBuffer/FrameCommandPool.cpp" 
	"src/Framework/Graphics/Backend/CommandBuffer/OneShotCommandPool.cpp" "src/Framework/Graphics/GPUScene.cpp" 
	"src/Framework/Graphics/Backend/Base/GraphicsPipeline.cpp" "src/Framework/Graphics/Backend/Base/PipelineLayout.cpp" 
	"src/Framework/Graphics/Backend/Synchronisation/Semaphore.cpp" "src/Framework/Graphics/Backend/Synchronisation/Fence.cpp" 	
	# "src/Graphics/Backend/DescriptorSet/DescriptorSet.cpp" 
	 "src/Framework/Graphics/SyncManager.cpp"
	
	"src/Framework/Graphics/FrameResources.cpp" "src/Framework/Graphics/MemoryManager.cpp" "src/Framework/Graphics/Backend/ValidationLayers/DebugMessenger.cpp" 
	"src/Framework/Graphics/ShadersManager.cpp" "src/Framework/Graphics/Resources/Mesh.cpp" "src/Framework/Graphics/Pass/Task.cpp" "src/Framework/Graphics/Pass/Pass.cpp"
	"src/Framework/Graphics/PassesGraph.cpp" "src/Framework/Graphics/PipelinesManager.cpp" "src/Framework/Graphics/Pass/SubmissionBatch.cpp"

	# Demos
	"src/Demo/Triangle.cpp"

	# Misc 
	"src/Framework/Misc/Bool.cpp"

	# Reflection 
	"src/Framework/Graphics/Reflection/Layout.cpp" "src/Framework/Graphics/Reflection/MemberBlueprint.cpp"

	#Memory Managment 
	"src/Framework/Graphics/MemoryManagment/DynamicAllocator.cpp" "src/Framework/Graphics/MemoryManagment/StaticAllocator.cpp"
 	"src/Framework/Graphics/MemoryManagment/Pages/GPULocalPage.cpp" "src/Framework/Graphics/MemoryManagment/Pages/CPUSharedPage.cpp"
 	"src/Framework/Graphics/MemoryManagment/Heaps/GPULocalHeap.cpp" "src/Framework/Graphics/MemoryManagment/Heaps/CPUSharedHeap.cpp"
	"src/Framework/Graphics/MemoryManagment/Heaps/StagingHeap.cpp"
)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(FRAMEWORK_DIR "${SRC_DIR}/Framework")
set(DEMO_DIR "${SRC_DIR}/Demo")

set(GRAPHICS_DIR "${FRAMEWORK_DIR}/Graphics")
set(MISC_DIR "${FRAMEWORK_DIR}/Misc")

# Core
# set(ASSETS_DIR "${CORE_DIR}/Assets")
set(MANAGERS_DIR "${DEMO_DIR}/Managers")

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

#Memory Managment 
set(HEAPS_DIR "${MEMORY_MANAGMENT_DIR}/Heaps")
set(PAGES_DIR "${MEMORY_MANAGMENT_DIR}/Pages")

# Link dependencies
target_link_libraries(ChilliSauceGF
    PUBLIC glfw3
    PUBLIC Vulkan::Vulkan
    PUBLIC glm
)

target_include_directories(ChilliSauceGF PRIVATE 
	# Src
	${SRC_DIR} ${FRAMEWORK_DIR} ${DEMO_DIR}
	${GRAPHICS_DIR} ${MISC_DIR} ${TEST_DIR}

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

	# Memory Managment 
	${HEAPS_DIR} ${PAGES_DIR}
)

# Put the executable into /bin
set_target_properties(ChilliSauceGF PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_definitions(ChilliSauceGF PRIVATE
	$<$<CONFIG:Debug>:ENGINE_DEBUG>
)
