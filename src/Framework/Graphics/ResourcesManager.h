#pragma once 
#include "Mesh.h"
#include "Image.h"
#include "PoolMap.h"
#include <vector>

namespace Graphics 
{
struct ResourceManager 
{
    PoolMap<Mesh> meshes = ("Meshes"); 
    PoolMap<Image> textures = ("Textures"); 
    std::vector<PoolId> meshLoadsToWaitFor; // meshes of WAIT_UNTIL_LOADED loaded this frame, see waitForLoads

    /*
        On demand loading of meshes (Mesh::load / unload), the counterpart of DescriptorsManager::updateLoading, runs right after it in Renderer::draw: it loads the meshes a draw or pass requested and unloads the ones nobody used for their residencyParams.absenceSecondsCount seconds.
        A mesh that is not committed yet is never unloaded. The decisions are collected first and applied after waiting for the submits that used the unloaded meshes (PassesGraph::waitForMeshUse: a buffer may not be destroyed while a pending command buffer uses it, VUID-vkDestroyBuffer-buffer-00922), then resolvePendingOperations queues the copies so the batch is submitted in the same frame.
    */
    void updateLoading();
    /* The same steps as updateLoading for one mesh: the application decides. An unload waits for the submits that used the mesh first, a load by hand honours the mesh's loadMode too. */
    void loadMesh(PoolId meshId_);
    void unloadMesh(PoolId meshId_);
    MemoryEntryId getMeshVertexEntry(PoolId meshId_); // the vertex entry of a loaded mesh, throws when it is unloaded
    void saveMeshToDisk(PoolId meshId_); // calls the mesh's saveToDisk (ownerId_ = the mesh id, partId_ = UninitializedPoolId), throws if it has none
    void changeMeshLayoutOnDisk(PoolId meshId_); // calls the mesh's changeLayoutOnDisk, then clears Residency::diskLayoutStale so a load may read the file again. Throws if the mesh has none
    void waitForLoads(); // after the frame's submit: blocks until the uploads of the meshes loaded this frame by a WAIT_UNTIL_LOADED mesh completed, see DescriptorsManager::waitForLoads
};
} 
