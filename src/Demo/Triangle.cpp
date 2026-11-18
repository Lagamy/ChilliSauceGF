#include "Triangle.h"
#include "Api.h"

namespace Graphics
{
	/* Creates the mesh and fills the default vertices (CPU side only). positions and colors can be changed before upload. */
	Triangle::Triangle(PoolId vertexLayoutId_, const TriangleVertexVariableIds& vertexVariableIds_)
	{
		this->vertexVariableIds = vertexVariableIds_; 
		this->meshId = addMesh("Triangle", vertexLayoutId_, 3);
		getMesh(this->meshId).indices = { 
			0, 1, 2
		};

		this->positions = { glm::vec3(0.0, 0.5, 0.0), glm::vec3(0.5, -0.5, 0.0), glm::vec3(-0.5, -0.5, 0.0) };
		this->colors = { glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(0.1, 0.1, 0.1, 1.0), glm::vec4(0.5, 0.5, 1.5, 1.0) };
	}

	/* The vertex data is written straight into the upload staging range, commitUpload queues the copy. */
	void Triangle::upload(AllocatorTypeEnum allocatorType_, MemoryVisabilityEnum memoryVisability_)
	{
		Mesh& rMesh = getMesh(this->meshId); 
		MemoryAccessParams triangleAccess = {};
		triangleAccess.accessMode = MemoryAccessModeEnum::READ_ONLY;
		rMesh.queueUpload(allocatorType_, memoryVisability_, triangleAccess); 

		for(uint32_t verticeId = 0; verticeId < 3; verticeId++)
		{
			rMesh.setVerticeVariable(this->vertexVariableIds.positionId, verticeId, this->positions[verticeId]);
			rMesh.setVerticeVariable(this->vertexVariableIds.colorId, verticeId, this->colors[verticeId]);
		}

		rMesh.commitUpload();
	}
}
