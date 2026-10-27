#include "Descriptor.h"
#include "Api.h"

namespace Graphics
{
	/* The blueprint lives in the DescriptorSetLayout this descriptor was derived from. DSlayoutId is an index into DescriptorsManager::layouts, so the lookup skips the Pool id validation. */
	DescriptorBlueprint& Descriptor::getBlueprint()
	{
		return getDescriptorsManager().layouts[this->DSlayoutId].getDescriptorBlueprint(this->BlueprintId);
	}
}
