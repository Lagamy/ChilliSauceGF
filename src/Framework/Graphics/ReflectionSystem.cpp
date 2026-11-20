#include "ReflectionSystem.h"
#include "Api.h"
#include <algorithm>
#include <format>
#include <stdexcept>

namespace Graphics
{
    PoolId ReflectionSystem::beginVertexLayoutUpdate(PoolId verticeLayoutId_)
    {
        ReflectionLayout& rLayout = this->layoutsPerBufferType[VERTEX_BUFFER - 1][verticeLayoutId_];
        if(rLayout.updateOpen)
        {
            throw std::runtime_error(std::format("Reflection System: Vertex layout {} already has an open update, commit it first.", this->layoutsPerBufferType[VERTEX_BUFFER - 1].getName(verticeLayoutId_)));
        }
        PoolId updateId = this->vertexLayoutUpdates.add(verticeLayoutId_, rLayout);
        rLayout.updateOpen = true;
        return updateId;
    }

    PoolId ReflectionSystem::addVariableToVerticeLayout(PoolId updateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->vertexLayoutUpdates[updateId_].staged.addVariableBlueprint(name_, orderPosition_, dataType_, repeatCountPerDimension_);
    }

    PoolId ReflectionSystem::addVariableToVerticeLayout(PoolId updateId_, const char* name_, uint32_t orderPosition_, DataTypeEnum dataType_, uint32_t rowCount_, uint32_t columnCount_, const std::vector<uint32_t>& repeatCountPerDimension_)
    {
        return this->vertexLayoutUpdates[updateId_].staged.addVariableBlueprint(name_, orderPosition_, dataType_, rowCount_, columnCount_, repeatCountPerDimension_);
    }

    void ReflectionSystem::removeVariableFromVerticeLayout(PoolId updateId_, PoolId variableId_)
    {
        this->vertexLayoutUpdates[updateId_].staged.removeVariableBlueprint(variableId_);
    }

    /*
        Meshes that use the layout and their vertex entries, and the created pipelines that use it, are looked up here (nothing registers them). Everything that can throw is checked before anything changes.
        A mesh whose entry has to be replaced needs the entry to be committed and, for a static one, can't be (static entries can't be removed). The device is waited for once when anything live changes: a pipeline can't be destroyed
        while a pending command buffer uses it (VUID-vkDestroyPipeline-pipeline-00765), and the old vertex buffers of CPU_SHARED meshes are destroyed at once.
        Meshes get their new entry (Mesh::restride) while the old layout is still live, then the staged layout takes the live layout's slot, then the pipelines are created again, which reads the layout.
    */
    void ReflectionSystem::commitVertexLayoutUpdate(PoolId updateId_)
    {
        VertexLayoutUpdate& rUpdate = this->vertexLayoutUpdates[updateId_];
        ReflectionLayout& rLive = this->layoutsPerBufferType[VERTEX_BUFFER - 1][rUpdate.layoutId];

        std::vector<PoolId> meshIds;
        bool replacesEntries = false;
        PoolMap<Mesh>& rMeshes = Globals::renderer.resourcesManager.meshes;
        for(uint32_t i = 0; i < rMeshes.allocatedSize(); i++)
        {
            if(!rMeshes.alive[i] || rMeshes.objects[i].verticeLayoutId != rUpdate.layoutId)
            {
                continue;
            }
            Mesh& rMesh = rMeshes.objects[i];
            if(rMesh.hasVertexEntry() && !vertexKeepsEntry(rLive, rUpdate.staged))
            {
                rMesh.throwIfCantReplaceVertexEntry();
                replacesEntries = true;
            }
            meshIds.push_back(PoolId(i, rMeshes.generation[i]));
        }

        std::vector<PoolId> pipelineIds;
        PoolMap<GraphicsPipeline>& rPipelines = getPipelinesManager().graphicsPipelines;
        for(uint32_t i = 0; i < rPipelines.allocatedSize(); i++)
        {
            if(rPipelines.alive[i] && rPipelines.objects[i].verticeLayoutId == rUpdate.layoutId && rPipelines.objects[i].vkHandle != VK_NULL_HANDLE)
            {
                pipelineIds.push_back(PoolId(i, rPipelines.generation[i]));
            }
        }

        if(replacesEntries || !pipelineIds.empty())
        {
            vkDeviceWaitIdle(getMainDevice().logicalDevice);
        }
        for(const PoolId& rMeshId : meshIds)
        {
            rMeshes[rMeshId].restride(rLive, rUpdate.staged);
        }

        rLive = rUpdate.staged;
        for(const PoolId& rPipelineId : pipelineIds)
        {
            rPipelines[rPipelineId].destroy();
            rPipelines[rPipelineId].create();
        }
        this->vertexLayoutUpdates.remove(updateId_);
    }
}
