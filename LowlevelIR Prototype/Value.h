#pragma once
#include <iostream>
#include <new>
#include "CPT_CommonType.h"
#include "Use.h"
#include <Zydis/Zydis.h>
#include <set>

class Value
{
public:
	Value()
		: UseList(nullptr), NumUserOperands(0)
	{
		//printf("Value Constructor (this:%p)\n",this);
	};

	enum OPERANDTYPE
	{
		VALUE_OPERANDTYPE_REGISTER,
		VALUE_OPERANDTYPE_MEMORY,
		VALUE_OPERANDTYPE_CONSTANT
	};

	enum DATATYPE
	{
		VALUE_DATATYPE_INTEGER8, VALUE_DATATYPE_INTEGER16,
		VALUE_DATATYPE_INTEGER32, VALUE_DATATYPE_INTEGER64,
		
		VALUE_DATATYPE_SIGNED8, VALUE_DATATYPE_SIGNED16,
		VALUE_DATATYPE_SIGNED32, VALUE_DATATYPE_SIGNED64,

		VALUE_DATATYPE_FLOAT, VALUE_DATATYPE_DOUBLE
	};

	OPERANDTYPE ValueType; // Register, Memory, Constant
	DATATYPE DataType; // ÀÚ·áÇü

	char Size;
	ZydisRegister Reg;
	CPT_NC_UINT_X MemAddress;
	CPT_NC_UINT_X Constant;
	CPT_NC_UINT_X index;

	Use *UseList;
	std::set<Use*> listOfUse;
	CPT_NC_UINT_X affectedBit;

	enum : unsigned { NumUserOperandsBits = 27 };
	unsigned NumUserOperands : NumUserOperandsBits;

	unsigned IsUsedByMD : 1;
	unsigned HasName : 1;
	unsigned HasMetadata : 1; // Has metadata attached to this?
	unsigned HasHungOffUses : 1;
	unsigned HasDescriptor : 1;


	void addUse(Use &U) 
	{ 
		U.addToList(&UseList); 
		listOfUse.insert(&U);
	}

};

void Use::set(Value *V)
{
	if (Val)
		removeFromList();

	Val = V;
	if (V) V->addUse(*this);
}

Value *Use::operator=(Value *RHS)
{
	set(RHS);
	return RHS;
}

const Use &Use::operator=(const Use &RHS)
{
	set(RHS.Val);
}

class Register:Value
{
	CPT_NC_UINT_X value;
};