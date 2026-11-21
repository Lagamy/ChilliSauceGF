#pragma once
#include <cstdint>
#include "PoolId.h"

namespace Graphics
{
	enum VariableSelectorKindEnum : uint8_t
	{
		SELECT_BY_ID, // the id addMemberBlueprint / addVariable returned
		SELECT_BY_POSITION, // the orderPosition of a variable directly in the block (or of a variable of a vertex layout)
		SELECT_BY_STRUCT_POSITION // the orderPosition of a struct member in the block and the orderPosition of the variable inside the struct
	};

	/*
		Picks the variable a setter or a push constant command writes: by the id the add function returned, or by the orderPosition it was added at (selector 1 = the variable at position 1, selector {0, 1} = the variable at position 1 of the struct at position 0).
		It converts implicitly from all three, so a setter takes any of them in the same argument. A position is looked up in the layout every time it is used, so it follows what the layout is at that moment, an id keeps pointing at the same variable when positions move.
	*/
	struct VariableSelector
	{
		VariableSelectorKindEnum kind;
		PoolId id;
		uint32_t structPosition = 0;
		uint32_t position = 0;

		VariableSelector() : kind(SELECT_BY_ID) {};
		VariableSelector(PoolId id_) : kind(SELECT_BY_ID), id(id_) {};
		VariableSelector(uint32_t position_) : kind(SELECT_BY_POSITION), position(position_) {};
		VariableSelector(uint32_t structPosition_, uint32_t position_) : kind(SELECT_BY_STRUCT_POSITION), structPosition(structPosition_), position(position_) {};
	};
}
