#include "CMDsRecordFunctions.h"

void gameCmdGraphicsCommands(VkCommandBuffer& rCmdBuffer_) 
{ 
	// Begin swapchain renderpass 
	std::vector<BatchGroup>& rBatchGroups = Engine::renderer.batchGroups;  
	for(BatchGroup& batchGroup : rBatchGroups)
	{
		uint32_t currentMaterialTypeId = HelperGlobals::UninitializedId;
		uint32_t currentMaterialId = HelperGlobals::UninitializedId; 
		uint32_t currentMeshId = HelperGlobals::UninitializedId; 
		
		// Change MaterialType(Graphics Pipeline) 
		if(currentMaterialTypeId != batchGroup.materialTypeId)
		{
			// Bind new graphics pipeline
			currentMaterialTypeId = batchGroup.materialTypeId; 
			Engine::Project::graphicsObjects.materialTypes.get(currentMaterialTypeId).use(); 
		}
		// Change Material(Image Assets uploads(if wasn't before) included)
		if(currentMaterialId != batchGroup.materialId)
		{
			currentMaterialId = batchGroup.materialId;
			Engine::Project::graphicsObjects.materialTypes.get(currentMaterialTypeId).materials.get(currentMaterialId).use();
		}
		// Change Mesh(Mesh Asset upload(if it wasn't before) included)
		if(currentMeshId != batchGroup.meshId)
		{
			currentMeshId = batchGroup.meshId;
			Engine::Project::assetsManager.useGPUAsset(MESH, batchGroup.meshId);
		}
	}
}

void gameCmdTransferCommands() 
{

}

void gameCmdComputeCommands()
{

}

