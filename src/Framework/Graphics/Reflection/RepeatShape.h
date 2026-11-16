#pragma once
#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>
#include <vector>

namespace Graphics
{
	/* One dimension of a repeated struct: count elements, each stride bytes apart. */
	struct RepeatLayer
	{
		uint32_t count = 1;
		uint64_t stride = 0;
	};

	/*
		Byte offset of the element picked by idsPerLayer_ (one id per layer, outermost first): the sum of id * stride over the layers.
		Every other offset calculation of repeated structs goes through this, a plain repeat unit is the one-layer case.
		Throws if the number of ids differs from the number of layers or an id is out of range of its own layer.
	*/
	inline uint64_t computeRepeatOffset(std::span<const RepeatLayer> layers_, std::span<const uint32_t> idsPerLayer_)
	{
		if(layers_.size() != idsPerLayer_.size())
		{
			throw std::runtime_error(std::format("Repeat Shape: {} ids given for {} layers.", idsPerLayer_.size(), layers_.size()));
		}

		uint64_t offset = 0;
		for(size_t layerId = 0; layerId < layers_.size(); layerId++)
		{
			if(idsPerLayer_[layerId] >= layers_[layerId].count)
			{
				throw std::runtime_error(std::format("Repeat Shape: Id {} is out of bounds of layer {} ({} elements).", idsPerLayer_[layerId], layerId, layers_[layerId].count));
			}
			offset += idsPerLayer_[layerId] * layers_[layerId].stride;
		}
		return offset;
	}

	/* The layers of a repeated struct, outermost first. For objects[][] of a struct: layers[0] steps through objects, layers[1] through the inner array. */
	struct RepeatShape
	{
		std::vector<RepeatLayer> layers;
		uint64_t elementSize = 0; // the struct the innermost layer repeats

		/*
			counts_ is the element count per layer, outermost first. The innermost stride is elementSize_ (the struct), every outer stride is the stride of the layer inside it times that layer's count.
			Throws on no layers or a count of 0 (a buffer can't have size 0).
		*/
		static RepeatShape fromCounts(std::span<const uint32_t> counts_, uint64_t elementSize_)
		{
			if(counts_.empty())
			{
				throw std::runtime_error("Repeat Shape: Needs at least one layer.");
			}

			RepeatShape shape;
			shape.elementSize = elementSize_;
			shape.layers.resize(counts_.size());
			uint64_t stride = elementSize_;
			for(size_t layerId = counts_.size(); layerId-- > 0;)
			{
				if(counts_[layerId] == 0)
				{
					throw std::runtime_error(std::format("Repeat Shape: Layer {} has 0 elements.", layerId));
				}
				shape.layers[layerId] = {counts_[layerId], stride};
				stride *= counts_[layerId];
			}
			return shape;
		}

		/* Bytes of the whole repeated struct. */
		uint64_t getSize() const
		{
			return this->layers.front().count * this->layers.front().stride;
		}

		/* Bytes one element of layer layerId_ spans: the whole sub-array inside it (the struct for the innermost layer). A dynamic uniform binding's range is this for its layer. */
		uint64_t getElementSpan(size_t layerId_) const
		{
			if(layerId_ + 1 < this->layers.size())
			{
				return this->layers[layerId_ + 1].count * this->layers[layerId_ + 1].stride;
			}
			return this->elementSize;
		}

		/*
			Sets the stride of layerId_ (padding between its elements), e.g. a dynamic layer's aligned stride. Only the outermost layer may be changed: the inner strides have to stay packed,
			the outer one just grows. Throws if the stride is smaller than the span of the elements.
		*/
		void setOutermostStride(uint64_t stride_)
		{
			if(stride_ < this->getElementSpan(0))
			{
				throw std::runtime_error(std::format("Repeat Shape: Stride {} is smaller than the {} bytes of one element.", stride_, this->getElementSpan(0)));
			}
			this->layers[0].stride = stride_;
		}

		uint64_t getOffset(std::span<const uint32_t> idsPerLayer_) const
		{
			return computeRepeatOffset(this->layers, idsPerLayer_);
		}
	};
}
