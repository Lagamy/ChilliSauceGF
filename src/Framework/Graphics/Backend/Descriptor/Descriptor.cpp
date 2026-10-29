#include "Descriptor.h"
#include "Api.h"

namespace Graphics
{
	/* The blueprint lives in the DescriptorSetLayout this descriptor was derived from. Both ids are validated by Pool::operator[] under ENGINE_DEBUG. */
	DescriptorBlueprint& Descriptor::getBlueprint()
	{
		return getDescriptorsManager().layouts[this->DSlayoutId].getDescriptorBlueprint(this->BlueprintId);
	}
}
