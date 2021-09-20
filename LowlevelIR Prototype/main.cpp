#include <Windows.h>
#include <psapi.h>
#include <stdio.h>
#include <time.h>
#include <Zycore/LibC.h>
#include <Zydis/Zydis.h>
#include "InstTypes.h"
#include "Value.h"
#include "Instruction.h"
#include "CPT_CommonFunction.h"
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>

#define MAX_OPERAND_SIZE_COUNT 3

typedef struct _ValueTable
{
	CPT_NC_UINT_X index;
	CPT_NC_UINT_X eip;

}VALUE_TABLE, *PVALUE_TABLE;

typedef struct _OPERANDINFO
{
	CPT_NC_UINT_X memAddress;
	ZydisRegister reg;
}OPERAND_INFO, *POPERAND_INFO;

std::map<CPT_NC_UINT_X, Instruction*> InstList;
std::map<ZydisRegister, VALUE_TABLE> regValueNumberTable;
std::map<CPT_NC_UINT_X, VALUE_TABLE> memValueNumberTable;
std::vector<Value*> valuePool;

std::map<CPT_NC_UINT_X, Value *> regValueList[ZYDIS_REGISTER_MAX_VALUE];
std::map <CPT_NC_UINT_X, std::map<CPT_NC_UINT_X, Value*>> memValueList;

CPT_NC_UINT_X g_stackAddress=0x80000000;
void findRegDef(ZydisDecodedOperand *pOperand, std::vector<Instruction*>& toUseList);

bool simpleDeadStoreFinder(Instruction* pInst)
{
	if (pInst->ValueType == Value::VALUE_OPERANDTYPE_REGISTER)
	{
		//printf("Reg IDX%d\n", pInst->index);
		if (pInst->index == regValueNumberTable[pInst->Reg].index)
			return false;
		else
			return true;
	}
	else if (pInst->ValueType == Value::VALUE_OPERANDTYPE_MEMORY)
	{
		//printf("Mem IDX: %d memValueNumberTable: %d\n", pInst->index, memValueNumberTable[pInst->MemAddress].index);
		if (pInst->index == memValueNumberTable[pInst->MemAddress].index)
			return false;
		else
			return true;
	}
	else
	{
		//printf("Not defined Value Operand Type\n");
	}
}
/*
	1. x86 명령어의 오퍼랜드들을 파싱한다.
	2. 각 오퍼랜드의 Def를 찾은 후 Def에 해당되는 Value 클래스를 찾는다
	3. 2에서 찾은 Value 클래스의 UseList에 
*/
Instruction *CreateBinaryIR(BinaryOp::BinaryOps _opType, ZydisDecodedInstruction* ptr_di, CPT_NC_UINT_X _offset)
{
	Value *Op1;
	Value *Op2;
	std::vector<Instruction*> toUseList;
	if (ptr_di->operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
	{
		CPT_NC_UINT_X regIdx = 0;
		findRegDef(&ptr_di->operands[0], toUseList);

		if (regValueNumberTable.find(ptr_di->operands[0].reg.value) != regValueNumberTable.end())
			regIdx = regValueNumberTable[ptr_di->operands[0].reg.value].index;
		Op1= regValueList[ptr_di->operands[0].reg.value][regIdx];

		if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER)
		{
			CPT_NC_UINT_X regIdx = 0;
			if (regValueNumberTable.find(ptr_di->operands[1].reg.value) != regValueNumberTable.end())
				regIdx = regValueNumberTable[ptr_di->operands[1].reg.value].index;
			Op2 = regValueList[ptr_di->operands[1].reg.value][regIdx];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
		{
			CPT_NC_UINT_X stackAddress = 0;
			CPT_NC_UINT_X memIdx = 0;
			
			if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_NONE || (ptr_di->operands[1].mem.scale==0)))
			{
				stackAddress = g_stackAddress + ptr_di->operands[1].mem.disp.value;
				
			}
			else if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_NONE) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress*(ptr_di->operands[1].mem.scale) + ptr_di->operands[1].mem.disp.value;
			}
			else if((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress + g_stackAddress + ptr_di->operands[1].mem.disp.value;
			}
			else
			{
				return nullptr;
			}

			// 해당

			if (memValueNumberTable.find(stackAddress) != memValueNumberTable.end())
			{
				memIdx = memValueNumberTable[stackAddress].index;
			}

			// 해당 StackMemory에 대한 Def가 없는 경우 새로 정의한다.
			else
			{
				//printf("Operand2 Memory %p def is null Operand Size:%d\n", stackAddress, ptr_di->operands[1].size);
				memIdx = 0;
				Value * newMem = new Value;

				if (ptr_di->operands[1].size == 64)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER64;
				else if(ptr_di->operands[1].size==32)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER32; 
				else if(ptr_di->operands[1].size == 16)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER16;
				else if (ptr_di->operands[1].size == 8)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER8;

				newMem->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_MEMORY;
				newMem->MemAddress = stackAddress;
				valuePool.push_back(newMem);
				VALUE_TABLE vt;
				vt.index = 0;
				vt.eip = _offset;
				memValueNumberTable.insert(std::pair<CPT_NC_UINT_X, VALUE_TABLE>(g_stackAddress, vt));
				memValueList[stackAddress][vt.index] = newMem;
			}

			Op2 = memValueList[stackAddress][memIdx];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE )
		{
			Op2 = new Value;
			Op2->Constant = ptr_di->operands[1].imm.value.u;
			Op2->ValueType = Value::VALUE_OPERANDTYPE_CONSTANT;
			Op2->Size = ptr_di->operands[1].size;
		}

		regIdx += 1;
		regValueNumberTable[ptr_di->operands[0].reg.value].index += 1;
		regValueNumberTable[ptr_di->operands[0].reg.value].eip = _offset;
		BinaryOp *rst = BinaryOp::Create(_opType, Op1, Op2);
		rst->ValueType = Value::VALUE_OPERANDTYPE_REGISTER;
		rst->Reg = ptr_di->operands[0].reg.value;
		rst->address = _offset;
		rst->Size = ptr_di->operands[0].size;
		rst->index = regValueNumberTable[ptr_di->operands[0].reg.value].index;

		for (int i = 0; i < toUseList.size(); i++)
		{
			//printf("toUseList Address:%x\n", toUseList[i]->address);
			toUseList[i]->listOfUse.insert(reinterpret_cast<Use*>(rst->op_begin()));
		}

		regValueList[ptr_di->operands[0].reg.value][regValueNumberTable[ptr_di->operands[0].reg.value].index] = rst;
		InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));

		//printf("------------------------------------------\n");
		//printf("CreateIR REG, OP2 Finish offset:%p IDX:%d, OP1 Value:%p\n", _offset, regValueNumberTable[ptr_di->operands[0].reg.value].index, rst->op_begin()->get());
		//printf("------------------------------------------\n");
		return rst;
	}

	else if (ptr_di->operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
	{
		CPT_NC_UINT_X memIdx = 0;
		CPT_NC_UINT_X stackAddress = 0;
		// [ESP] 또는 [ESP + Disp] 단, ESP는 Base Register
		if ((ptr_di->operands[0].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[0].mem.index == ZYDIS_REGISTER_NONE || (ptr_di->operands[0].mem.scale == 0)))
		{
			stackAddress = g_stackAddress + ptr_di->operands[1].mem.disp.value;
			
		}

		// [ESP] 또는 [ESP*Scale + Disp] 단, ESP는 IndexRegister
		else if ((ptr_di->operands[0].mem.base == ZYDIS_REGISTER_NONE) && (ptr_di->operands[0].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.index == ZYDIS_REGISTER_RSP))
		{
			stackAddress = g_stackAddress*(ptr_di->operands[0].mem.scale) + ptr_di->operands[0].mem.disp.value;
		}

		// [ESP + ESP*Scale + Disp]
		else if ((ptr_di->operands[0].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[0].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.index == ZYDIS_REGISTER_RSP))
		{
			stackAddress = g_stackAddress + g_stackAddress*(ptr_di->operands[0].mem.scale) + ptr_di->operands[0].mem.disp.value;
		}
		else
		{
			return nullptr;
		}
		
		if (memValueNumberTable.find(stackAddress) != memValueNumberTable.end())
		{
			//printf("Stack:%p\n", stackAddress);
			memIdx = memValueNumberTable[stackAddress].index;
		}

		else
		{
			//printf("Operand2 Memory %p def is null Operand Size:%d\n", stackAddress, ptr_di->operands[0].size);
			memIdx = 0;
			Value * newMem = new Value;
			std::map<CPT_NC_UINT_X, Value*> tempMemTable;

			if (ptr_di->operands[0].size == 64)
				newMem->DataType = Value::VALUE_DATATYPE_INTEGER64;
			else if (ptr_di->operands[0].size == 32)
				newMem->DataType = Value::VALUE_DATATYPE_INTEGER32;
			else if (ptr_di->operands[0].size == 16)
				newMem->DataType = Value::VALUE_DATATYPE_INTEGER16;
			else if (ptr_di->operands[0].size == 8)
				newMem->DataType = Value::VALUE_DATATYPE_INTEGER8;
			
			newMem->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_MEMORY;
			newMem->MemAddress = stackAddress;
			newMem->Size = ptr_di->operands[0].size;
			valuePool.push_back(newMem);
			
			VALUE_TABLE vt;
			vt.index = 0;
			vt.eip = _offset;
			memValueNumberTable.insert(std::pair<CPT_NC_UINT_X, VALUE_TABLE>(stackAddress, vt));
			tempMemTable.insert(std::pair<CPT_NC_UINT_X, Value*>(0, newMem));
			memValueList.insert(std::pair<CPT_NC_UINT_X, std::map<CPT_NC_UINT_X, Value*>>(stackAddress, tempMemTable));
		}
		//printf("Mem Idx\n");
		Op1 = memValueList[stackAddress][memIdx];

		if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER)
		{
			CPT_NC_UINT_X regIdx = 0;
			if (regValueNumberTable.find(ptr_di->operands[0].reg.value) != regValueNumberTable.end())
				regIdx = regValueNumberTable[ptr_di->operands[0].reg.value].index;
			Op2 = regValueList[ptr_di->operands[1].reg.value][regIdx];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
		{
			CPT_NC_UINT_X stackAddress = 0;
			CPT_NC_UINT_X memIdx2 = 0;

			// [ESP] 또는 [ESP + Disp] 단, ESP는 Base Register
			if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_NONE || (ptr_di->operands[1].mem.scale == 0)))
			{
				stackAddress = g_stackAddress + ptr_di->operands[1].mem.disp.value;
			}

			// [ESP] 또는 [ESP*Scale + Disp] 단, ESP는 IndexRegister
			else if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_NONE) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress*(ptr_di->operands[1].mem.scale) + ptr_di->operands[1].mem.disp.value;
			}

			// [ESP + ESP*Scale + Disp]
			else if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress + g_stackAddress*(ptr_di->operands[1].mem.scale) + ptr_di->operands[1].mem.disp.value;
			}
			else
			{
				return nullptr;
			}

			if (memValueNumberTable.find(stackAddress) != memValueNumberTable.end())
				memIdx2 = memValueNumberTable[ptr_di->operands[1].size].index;
			else
			{
				//printf("Operand2 Memory %p def is null\n", stackAddress);
				MessageBoxA(0, "Error", "Error", 0);
				return nullptr;
			}

			Op2 = memValueList[stackAddress][memIdx2];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
			Op2 = new Value;
			Op2->Constant = ptr_di->operands[1].imm.value.u;
			Op2->ValueType = Value::VALUE_OPERANDTYPE_CONSTANT;
			Op2->Size = ptr_di->operands[1].size;
		}

		memIdx += 1;
		memValueNumberTable[stackAddress].index += 1;
		memValueNumberTable[stackAddress].eip += _offset;
		BinaryOp *rst = BinaryOp::Create(_opType, Op1, Op2);
		rst->ValueType = Value::VALUE_OPERANDTYPE_MEMORY;
		if (stackAddress != 0)
			rst->MemAddress = stackAddress;
		rst->HasHungOffUses = 0;
		rst->address = _offset;
		rst->Size = ptr_di->operands[0].size;	
		rst->index = memValueNumberTable[stackAddress].index;
		memValueList[stackAddress][memIdx] = rst;

		InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));
		//printf("------------------------------------------\n");
		//printf("CreateIR MEM, OP2 Finish offset:%p %p OP1 Value:%p IDX:%d(%d)\n", _offset, InstList.begin()->first, rst->op_begin()->get(), rst->index, memValueNumberTable[stackAddress].index);
		//printf("------------------------------------------\n");

		if (stackAddress != 0)
		{
			printf("------------------------------------------\n");
			if (Op2->ValueType == Value::VALUE_OPERANDTYPE_REGISTER)
				printf("[%p] [%p](%d) = %s(%d)\n", _offset, g_stackAddress, memValueNumberTable[g_stackAddress].index, ZydisRegisterGetString(Op2->Reg), regValueNumberTable[Op2->Reg].index);
			printf("------------------------------------------\n");
		}

		return rst;
	}


	return nullptr;
}

Instruction *CreateMovIR(UnaryOp::UnaryOps _opType, ZydisDecodedInstruction* ptr_di, CPT_NC_UINT_X _offset)
{
	Value *Op1;
	Value *Op2;
	std::vector<Instruction*> toUseList;
	CPT_NC_UINT_X stackAddress = 0;

	if (ptr_di->operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
	{
		CPT_NC_UINT_X regIdx = 0;

		findRegDef(&ptr_di->operands[0], toUseList);

		if (ptr_di->operands[0].size == 16)
		{

		}

		if (ptr_di->operands[0].size == 8)
		{

		}


		if (regValueNumberTable.find(ptr_di->operands[0].reg.value) != regValueNumberTable.end())
			regIdx = regValueNumberTable[ptr_di->operands[0].reg.value].index;
		Op1 = regValueList[ptr_di->operands[0].reg.value][regIdx];

		if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER)
		{
			CPT_NC_UINT_X regIdx = 0;
			if (regValueNumberTable.find(ptr_di->operands[1].reg.value) != regValueNumberTable.end())
				regIdx = regValueNumberTable[ptr_di->operands[1].reg.value].index;
			Op2 = regValueList[ptr_di->operands[1].reg.value][regIdx];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
		{
			
			CPT_NC_UINT_X memIdx = 0;

			if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_NONE || (ptr_di->operands[1].mem.scale == 0)))
			{
				stackAddress = g_stackAddress + ptr_di->operands[1].mem.disp.value;
			}
			else if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_NONE) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress*(ptr_di->operands[1].mem.scale) + ptr_di->operands[1].mem.disp.value;
			}
			else if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress + g_stackAddress + ptr_di->operands[1].mem.disp.value;
			}
			else
			{
				return nullptr;
			}

			// 해당

			if (memValueNumberTable.find(stackAddress) != memValueNumberTable.end())
				memIdx = memValueNumberTable[stackAddress].index;

			// 해당 StackMemory에 대한 Def가 없는 경우 새로 정의한다.
			else
			{
				//printf("Operand2 Memory %p def is null Operand Size:%d\n", stackAddress, ptr_di->operands[1].size);
				memIdx = 0;
				Value * newMem = new Value;

				if (ptr_di->operands[1].size == 64)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER64;
				else if (ptr_di->operands[1].size == 32)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER32;
				else if (ptr_di->operands[1].size == 16)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER16;
				else if (ptr_di->operands[1].size == 8)
					newMem->DataType = Value::VALUE_DATATYPE_INTEGER8;

				newMem->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_MEMORY;
				newMem->MemAddress = stackAddress;
				valuePool.push_back(newMem);
				VALUE_TABLE vt;
				vt.index = 0;
				vt.eip = _offset;
				memValueNumberTable.insert(std::pair<CPT_NC_UINT_X, VALUE_TABLE>(g_stackAddress, vt));
			}
			
			Op2 = memValueList[stackAddress][memIdx];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
			Op2 = new Value;
			Op2->Constant = ptr_di->operands[1].imm.value.u;
			Op2->ValueType = Value::VALUE_OPERANDTYPE_CONSTANT;
			Op2->Size = ptr_di->operands[1].size;
		}

		for (int i = 0; i < toUseList.size(); i++)
		{
			//printf("toUseList Address:%x\n", toUseList[i]->address);
		}

		regIdx += 1;
		regValueNumberTable[ptr_di->operands[0].reg.value].index += 1;
		regValueNumberTable[ptr_di->operands[0].reg.value].eip = _offset;
		UnaryOp *rst = UnaryOp::Create(_opType, Op2);
		rst->ValueType = Value::VALUE_OPERANDTYPE_REGISTER;
		rst->Size = ptr_di->operands[0].size;
		rst->address = _offset;
		rst->Reg = ptr_di->operands[0].reg.value;
		rst->index = regValueNumberTable[ptr_di->operands[0].reg.value].index;
		//printf("------------------------------------------\n");
		//printf("CreateIR REG, OP2 Finish offset:%p Reg:%d Mem:%p\n", _offset, ptr_di->operands[0].reg.value, rst->op_begin()->get());
		//printf("------------------------------------------\n");



		for (int i = 0; i < toUseList.size(); i++)
		{
			//printf("toUseList Address:%x\n", toUseList[i]->address);
			toUseList[i]->listOfUse.insert(reinterpret_cast<Use*>(rst));
		}

		regValueList[ptr_di->operands[0].reg.value][regIdx] = rst;
		InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));

		printf("------------------------------------------\n");
		if (Op2->ValueType == Value::VALUE_OPERANDTYPE_REGISTER)
			printf("[%p] %s(%d) = %s(%d) op %s(%d)\n", _offset, ZydisRegisterGetString(rst->Reg), regValueNumberTable[rst->Reg].index, ZydisRegisterGetString(Op1->Reg), regValueNumberTable[Op1->Reg].index - 1, ZydisRegisterGetString(Op2->Reg), regValueNumberTable[Op2->Reg].index);
		else if (Op2->ValueType == Value::VALUE_OPERANDTYPE_MEMORY && Op2->MemAddress !=0)
			printf("[%p] %s(%d) = %s(%d) op %x(%d)\n", _offset, ZydisRegisterGetString(rst->Reg), regValueNumberTable[rst->Reg].index, ZydisRegisterGetString(Op1->Reg), regValueNumberTable[Op1->Reg].index - 1, Op2->MemAddress, memValueNumberTable[Op2->MemAddress].index);
		printf("------------------------------------------\n");
		return rst;
	}

	else if (ptr_di->operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
	{
		CPT_NC_UINT_X memIdx = 0;
		CPT_NC_UINT_X stackAddress = 0;

		// [ESP] 또는 [ESP + Disp] 단, ESP는 Base Register
		if ((ptr_di->operands[0].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[0].mem.index == ZYDIS_REGISTER_NONE || (ptr_di->operands[0].mem.scale == 0)))
		{
			stackAddress = g_stackAddress + ptr_di->operands[0].mem.disp.value;
		}

		// [ESP] 또는 [ESP*Scale + Disp] 단, ESP는 IndexRegister
		else if ((ptr_di->operands[0].mem.base == ZYDIS_REGISTER_NONE) && (ptr_di->operands[0].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.index == ZYDIS_REGISTER_RSP))
		{
			stackAddress = g_stackAddress*(ptr_di->operands[0].mem.scale) + ptr_di->operands[0].mem.disp.value;
		}

		// [ESP + ESP*Scale + Disp]
		else if ((ptr_di->operands[0].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[0].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[0].mem.index == ZYDIS_REGISTER_RSP))
		{
			stackAddress = g_stackAddress + g_stackAddress*(ptr_di->operands[0].mem.scale) + ptr_di->operands[0].mem.disp.value;
		}
		else
		{
			return nullptr;
		}

		if (memValueNumberTable.find(stackAddress) != memValueNumberTable.end())
			memIdx = memValueNumberTable[stackAddress].index;
		Op1 = memValueList[stackAddress][memIdx];

		if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_REGISTER)
		{
			CPT_NC_UINT_X regIdx = 0;
			if (regValueNumberTable.find(ptr_di->operands[0].reg.value) != regValueNumberTable.end())
				regIdx = regValueNumberTable[ptr_di->operands[0].reg.value].index;
			Op2 = regValueList[ptr_di->operands[1].reg.value][regIdx];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY)
		{
			CPT_NC_UINT_X stackAddress = 0;
			CPT_NC_UINT_X memIdx2 = 0;

			// [ESP] 또는 [ESP + Disp] 단, ESP는 Base Register
			if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_NONE || (ptr_di->operands[1].mem.scale == 0)))
			{
				stackAddress = g_stackAddress + ptr_di->operands[1].mem.disp.value;
			}

			// [ESP] 또는 [ESP*Scale + Disp] 단, ESP는 IndexRegister
			else if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_NONE) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress*(ptr_di->operands[1].mem.scale) + ptr_di->operands[1].mem.disp.value;
			}

			// [ESP + ESP*Scale + Disp]
			else if ((ptr_di->operands[1].mem.base == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.base == ZYDIS_REGISTER_RSP) && (ptr_di->operands[1].mem.index == ZYDIS_REGISTER_ESP || ptr_di->operands[1].mem.index == ZYDIS_REGISTER_RSP))
			{
				stackAddress = g_stackAddress + g_stackAddress*(ptr_di->operands[1].mem.scale) + ptr_di->operands[1].mem.disp.value;
			}
			else
			{
				return nullptr;
			}

			if (memValueNumberTable.find(stackAddress) != memValueNumberTable.end())
				memIdx2 = memValueNumberTable[stackAddress].index;
			else
			{
				//printf("Operand2 Memory %p def is null\n", stackAddress);
				MessageBoxA(0, "Error", "Error", 0);
				return nullptr;
			}

			Op2 = memValueList[stackAddress][memIdx2];
		}

		else if (ptr_di->operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
		{
			Op2 = new Value;
			Op2->Constant = ptr_di->operands[1].imm.value.u;
			Op2->ValueType = Value::VALUE_OPERANDTYPE_CONSTANT;
			Op2->Size = ptr_di->operands[1].size;
		}
		memIdx += 1;
		memValueNumberTable[stackAddress].index += 1;
		memValueNumberTable[stackAddress].eip = _offset;
		UnaryOp *rst = UnaryOp::Create(_opType, Op2);
		rst->ValueType = Value::VALUE_OPERANDTYPE_MEMORY;
		if (stackAddress != 0)
			rst->MemAddress = stackAddress;
		rst->HasHungOffUses = 0;
		rst->address = _offset;
		rst->Size = ptr_di->operands[0].size;
		rst->index = memValueNumberTable[stackAddress].index;
		memValueList[stackAddress][memIdx] = rst;

		InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));
		//printf("------------------------------------------\n");
		//printf("CreateIR MEM, OP2 Finish offset:%p %p IDX:%d(%d)\n", _offset, InstList.begin()->first, rst->index, memValueNumberTable[stackAddress].index);
		//printf("------------------------------------------\n");

		if (rst->MemAddress != 0)
		{
			printf("------------------------------------------\n");
			if(Op2->ValueType == Value::VALUE_OPERANDTYPE_REGISTER)
				printf("[%p] [%p](%d) = %s(%d)\n", _offset, rst->MemAddress, memValueNumberTable[rst->MemAddress].index, ZydisRegisterGetString(Op2->Reg), regValueNumberTable[Op2->Reg].index);
			if (Op2->ValueType == Value::VALUE_OPERANDTYPE_CONSTANT)
				printf("[%p] [%p](%d) = %x\n", _offset, rst->MemAddress, memValueNumberTable[rst->MemAddress].index, Op2->Constant);

			printf("------------------------------------------\n");
		}
		return rst;
	}

	return nullptr;
}

Instruction *CreatePushIR(UnaryOp::UnaryOps _opType, ZydisDecodedInstruction* ptr_di, CPT_NC_UINT_X _offset)
{
	Value *Op1;
	Value *Op2;
	int regIdx;
	std::vector<Instruction*> toUseList;
	CPT_NC_UINT_X memIdx = 0;

	// ESP(n+1) = ESP(n)-4 
	if (regValueNumberTable.find(ZYDIS_REGISTER_ESP) != regValueNumberTable.end())
		regIdx = regValueNumberTable[ZYDIS_REGISTER_ESP].index;
	Op1 = regValueList[ZYDIS_REGISTER_ESP][regIdx];
	Op2 = new Value;
	Op2->ValueType = Value::VALUE_OPERANDTYPE_CONSTANT;
	Op2->Size = 32;
	Op2->Constant = 4;

	BinaryOp *rst = BinaryOp::Create(BinaryOp::BinaryOps::Sub, Op1, Op2);
	regValueNumberTable[ZYDIS_REGISTER_ESP].index += 1;
	rst->ValueType = Value::VALUE_OPERANDTYPE_REGISTER;
	rst->Reg = ZYDIS_REGISTER_ESP;
	rst->address = _offset;
	rst->Size = ptr_di->operands[0].size;
	rst->index = regValueNumberTable[ZYDIS_REGISTER_ESP].index;
	g_stackAddress -= 4;

	regValueList[ZYDIS_REGISTER_ESP][regValueNumberTable[ZYDIS_REGISTER_ESP].index] = rst;

	//printf("PUSH 1 of 2 [%x]\n", _offset);
	printf("------------------------------------------\n");
	printf("[%p] ESP(%d) = ESP(%d) - %d\n", _offset, regValueNumberTable[ZYDIS_REGISTER_ESP].index, regValueNumberTable[ZYDIS_REGISTER_ESP].index - 1, Op2->Constant);
	InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));
	_offset += 1;
	
	if (ptr_di->operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
	{
		CPT_NC_UINT_X regIdx = 0;

		findRegDef(&ptr_di->operands[0], toUseList);

		if (ptr_di->operands[0].size == 16)
		{

		}

		if (ptr_di->operands[0].size == 8)
		{

		}


		if (regValueNumberTable.find(ptr_di->operands[0].reg.value) != regValueNumberTable.end())
			regIdx = regValueNumberTable[ptr_di->operands[0].reg.value].index;
		Op1 = regValueList[ptr_di->operands[0].reg.value][regIdx];

		memIdx += 1;
		memValueNumberTable[g_stackAddress].index += 1;
		memValueNumberTable[g_stackAddress].eip = _offset;

		UnaryOp *rst = UnaryOp::Create(_opType, Op1);
		rst->ValueType = Value::VALUE_OPERANDTYPE_MEMORY;
		if (g_stackAddress != 0)
			rst->MemAddress = g_stackAddress;
		rst->HasHungOffUses = 0;
		rst->address = _offset;
		rst->Size = ptr_di->operands[0].size;
		rst->index = memValueNumberTable[g_stackAddress].index;
		memValueList[g_stackAddress][memValueNumberTable[g_stackAddress].index] = rst;

		InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));
	}

	else if (ptr_di->operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
	{
		Op1 = new Value;
		Op1->Constant = ptr_di->operands[0].imm.value.u;
		Op1->ValueType = Value::VALUE_OPERANDTYPE_CONSTANT;
		Op1->Size = ptr_di->operands[0].size;
	}

		//printf("PUSH 2 of 2 [%x]\n", _offset);
		if (g_stackAddress != 0)
		{
			if (Op1->ValueType == Value::VALUE_OPERANDTYPE_REGISTER)
				printf("[%p] [%p](%d) = %s(%d)\n", _offset, g_stackAddress, memValueNumberTable[g_stackAddress].index, ZydisRegisterGetString(Op1->Reg), regValueNumberTable[Op1->Reg].index);
			else if(Op1->ValueType==Value::VALUE_OPERANDTYPE_CONSTANT)
				printf("[%p] [%p](%d) = %x\n", _offset, g_stackAddress, memValueNumberTable[g_stackAddress].index, Op1->Constant);
		}
		printf("------------------------------------------\n");
	
}

Instruction *CreatePopIR(UnaryOp::UnaryOps _opType, ZydisDecodedInstruction* ptr_di, CPT_NC_UINT_X _offset)
{
	Value *Op1;
	Value *Op2;
	int regIdx = 0;
	std::vector<Instruction*> toUseList;
	CPT_NC_UINT_X memIdx = 0;

	// REG = [ESP]
	if (memValueNumberTable.find(g_stackAddress) != memValueNumberTable.end())
	{
		memIdx = memValueNumberTable[g_stackAddress].index;
		Op1 = memValueList[g_stackAddress][memIdx];
	}

	if (ptr_di->operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
	{
		CPT_NC_UINT_X regIdx = 0;

		findRegDef(&ptr_di->operands[0], toUseList);

		if (ptr_di->operands[0].size == 16)
		{

		}

		if (ptr_di->operands[0].size == 8)
		{

		}

		regValueNumberTable[ptr_di->operands[0].reg.value].index += 1;
		regValueNumberTable[ptr_di->operands[0].reg.value].eip = _offset;
		UnaryOp *rst = UnaryOp::Create(_opType, Op1);
		rst->ValueType = Value::VALUE_OPERANDTYPE_REGISTER;
		rst->Reg = ptr_di->operands[0].reg.value;
		rst->address = _offset;
		rst->Size = ptr_di->operands[0].size;
		rst->index = regValueNumberTable[ptr_di->operands[0].reg.value].index;

		/*
		for (int i = 0; i < toUseList.size(); i++)
		{
			//printf("toUseList Address:%x\n", toUseList[i]->address);
			toUseList[i]->listOfUse.insert(reinterpret_cast<Use*>(rst->op_begin()));
		}
		*/

		regValueList[ptr_di->operands[0].reg.value][regValueNumberTable[ptr_di->operands[0].reg.value].index] = rst;
		InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));

		//printf("PUSH 2 of 2 [%x]\n", _offset);
		printf("------------------------------------------\n");
		printf("[%p] %s(%d) = %p(%d)\n", _offset, ZydisRegisterGetString(rst->Reg), rst->index, g_stackAddress, memValueNumberTable[g_stackAddress].index);
	}
	_offset += 1;
		// ESP(n+1) = ESP(n)-4 
	if (regValueNumberTable.find(ZYDIS_REGISTER_ESP) != regValueNumberTable.end())
		regIdx = regValueNumberTable[ZYDIS_REGISTER_ESP].index;
	Op1 = regValueList[ZYDIS_REGISTER_ESP][regIdx];
	Op2 = new Value;
	Op2->ValueType = Value::VALUE_OPERANDTYPE_CONSTANT;
	Op2->Size = 32;
	Op2->Constant = 4;

	BinaryOp *rst = BinaryOp::Create(BinaryOp::BinaryOps::Add, Op1, Op2);
	regValueNumberTable[ZYDIS_REGISTER_ESP].index += 1;
	rst->ValueType = Value::VALUE_OPERANDTYPE_REGISTER;
	rst->Reg = ptr_di->operands[0].reg.value;
	rst->address = _offset;
	rst->Size = ptr_di->operands[0].size;
	rst->index = regValueNumberTable[ZYDIS_REGISTER_ESP].index;
	g_stackAddress += 4;

	//printf("PUSH 1 of 2 [%x]\n", _offset);
	printf("[%p] ESP(%d) = ESP(%d) + %d\n", _offset, regValueNumberTable[ZYDIS_REGISTER_ESP].index, regValueNumberTable[ZYDIS_REGISTER_ESP].index - 1, Op2->Constant);
	printf("------------------------------------------\n");
	InstList.insert(std::pair<CPT_NC_UINT_X, Instruction*>(_offset, rst));
	_offset += 1;

}

int CreateIR(ZydisDecodedInstruction* ptr_di, CPT_NC_UINT_X _offset)
{
	switch (ptr_di->mnemonic)
	{
	case ZYDIS_MNEMONIC_ADD:
		CreateBinaryIR(BinaryOp::BinaryOps::Add, ptr_di, _offset);
		return 1;
		break;
	case ZYDIS_MNEMONIC_SUB:
		CreateBinaryIR(BinaryOp::BinaryOps::Sub, ptr_di, _offset);
		return 1;
		break;
	case ZYDIS_MNEMONIC_MOV:
		CreateMovIR(UnaryOp::UnaryOps::Mov, ptr_di, _offset);
		return 1;
		break;
	case ZYDIS_MNEMONIC_PUSH:
		CreatePushIR(UnaryOp::UnaryOps::Mov, ptr_di, _offset);
		return 2;
		break;
	case ZYDIS_MNEMONIC_POP:
		CreatePopIR(UnaryOp::UnaryOps::Mov, ptr_di, _offset);
		return 2;
		break;
	default:
		return 0;
		break;
	}
}

Value* eax0;
Value* ebx0;
Value* ecx0;
Value* edx0;
Value* ebp0;
Value* esp0;
Value* esi0;
Value* edi0;

int main()
{
	CPT_NC_UINT_X offset = 0;
	ZydisDecodedInstruction g_inst;
	ZydisDecoder decoder;
	CPT_NC_UINT8 buf[] = { 0x56,0x52,0xC7,0x04,0x24,0xA2,0x53,0x10,0x52,0x89,0x1C,0x24,0x68,0xB6,0x01,0x00,0x00,0x5B,0x89,0x5C,0x24,0x04,0x5B,0x51,0x56,0xBE,0xF2,0x63,0x78,0x05,0x89,0x74,0x24,0x04,0x5E,0x89,0x0C,0x24,0x52,0xBA,0x2C,0xC3,0x48,0x00,0x89,0x54,0x24,0x04,0x5A,0x51,0x89,0xE1,0x81,0xC1,0x04,0x00,0x00,0x00,0x81,0xE9,0x04,0x00,0x00,0x00 };//{ 0x89,0xca,0x66,0x89,0xda,0x88,0xc6, 0x01,0x04,0x24, 0x03,0x54,0x24,0x04, 0x01,0xf3,0x01,0xd9, 0x01,0x04,0x24,0x01,0xf3,0x89,0x1c,0x24, 0x89,0x3c,0x24,0x03,0x34,0x24,0x89,0x0c,0x24 };
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_ADDRESS_WIDTH_32);

	VALUE_TABLE vt;
	vt.eip = 0x401000;
	vt.index = 0;

	eax0 = new Value;
	eax0->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_REGISTER;
	eax0->Reg = ZYDIS_REGISTER_EAX;
	eax0->HasHungOffUses = 0;
	regValueNumberTable[ZYDIS_REGISTER_EAX].index = 0;
	regValueList[ZYDIS_REGISTER_EAX][regValueNumberTable[ZYDIS_REGISTER_EAX].index] = eax0;
	//regValueNumberTable[ZYDIS_REGISTER_EAX] = vt;

	ebx0 = new Value;
	ebx0->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_REGISTER;
	ebx0->Reg = ZYDIS_REGISTER_EBX;
	ebx0->HasHungOffUses = 0;
	regValueNumberTable[ZYDIS_REGISTER_EBX].index = 0;
	regValueList[ZYDIS_REGISTER_EBX][regValueNumberTable[ZYDIS_REGISTER_EBX].index] = ebx0;
	//regValueNumberTable[ZYDIS_REGISTER_EBX] = vt;

	ecx0 = new Value;
	ecx0->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_REGISTER;
	ecx0->Reg = ZYDIS_REGISTER_ECX;
	ecx0->HasHungOffUses = 0;
	regValueNumberTable[ZYDIS_REGISTER_ECX].index = 0;
	regValueList[ZYDIS_REGISTER_ECX][regValueNumberTable[ZYDIS_REGISTER_ECX].index] = ecx0;

	edx0 = new Value;
	edx0->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_REGISTER;
	edx0->Reg = ZYDIS_REGISTER_EDX;
	edx0->HasHungOffUses = 0;
	regValueNumberTable[ZYDIS_REGISTER_EDX].index = 0;
	regValueList[ZYDIS_REGISTER_EDX][regValueNumberTable[ZYDIS_REGISTER_EDX].index] = edx0;

	ebp0 = new Value;
	ebp0->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_REGISTER;
	ebp0->Reg = ZYDIS_REGISTER_EBP;
	edx0->HasHungOffUses = 0;
	regValueNumberTable[ZYDIS_REGISTER_EBP].index = 0;
	regValueList[ZYDIS_REGISTER_EBP][regValueNumberTable[ZYDIS_REGISTER_EBP].index] = ebp0;

	esp0 = new Value;
	esp0->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_REGISTER;
	esp0->Reg = ZYDIS_REGISTER_ESP;
	esp0->HasHungOffUses = 0;
	regValueNumberTable[ZYDIS_REGISTER_ESP].index = 0;
	regValueList[ZYDIS_REGISTER_ESP][regValueNumberTable[ZYDIS_REGISTER_ESP].index] = esp0;

	esi0 = new Value;
	esi0->HasHungOffUses = 0;
	esi0->Reg = ZYDIS_REGISTER_ESI;
	esp0->HasHungOffUses = 0;
	regValueNumberTable[ZYDIS_REGISTER_ESI].index =0;
	regValueList[ZYDIS_REGISTER_ESI][regValueNumberTable[ZYDIS_REGISTER_ESI].index] = esi0;

	edi0 = new Value;
	edx0->Reg = ZYDIS_REGISTER_EDI;
	edx0->HasHungOffUses = 0;

	Value *StackAddress = new Value;
	StackAddress->ValueType = Value::OPERANDTYPE::VALUE_OPERANDTYPE_MEMORY;
	StackAddress->MemAddress = g_stackAddress;
	StackAddress->HasHungOffUses = 0;
	printf("This Basic Block's init stackAddress: %p\n", g_stackAddress);
	//memValueNumberTable.insert(std::pair<CPT_NC_UINT_X, CPT_NC_UINT_X>(g_stackAddress, 0));

	vt.index = 0;
	vt.eip = 0;
	//memValueNumberTable[32].insert(std::pair<CPT_NC_UINT_X, VALUE_TABLE>(g_stackAddress, vt));

	ZydisFormatter formatter;
	ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);
	char buffer[256];
	int length = 0;
	int irIdx = 0;
	for (int i = 0; length< sizeof(buf); i++)
	{
		ZydisDecoderDecodeBuffer(&decoder, (CPT_NC_UINT8*)(buf + length), 15, &g_inst);
		printf("[%p] ", 0x401000 + length);
		ZydisFormatterFormatInstruction(&formatter, &g_inst, buffer, sizeof(buffer), 0);
		puts(buffer);
		irIdx+=CreateIR(&g_inst, irIdx);
		length += g_inst.length;
	}
	//printf("EBX:%d %p\n", regValueNumberTable[ZYDIS_REGISTER_EBX].index, regValueNumberTable[ZYDIS_REGISTER_EBX].eip);
	//printf("End: reg idx :%d(%p) useList :%p\n", regValueNumberTable[ZYDIS_REGISTER_EBX].index , regValueNumberTable[ZYDIS_REGISTER_EBX].eip,reinterpret_cast<Instruction*>(regValueList[ZYDIS_REGISTER_EBX][regValueNumberTable[ZYDIS_REGISTER_EBX].index-1]->UseList->getUser())->address);
	printf("test:%p\n", InstList.begin()->first);
	std::map<CPT_NC_UINT_X, Instruction*>::iterator j;

	length = 0;
	for (j = InstList.begin(); j != InstList.end(); j++)
	{
		printf("-----------------------------------------\n");
		ZydisDecoderDecodeBuffer(&decoder, (CPT_NC_UINT8*)(buf + length), 15, &g_inst);
		printf("[%p] [IR:%p]", 0x401000 + length, j->first);
		ZydisFormatterFormatInstruction(&formatter, &g_inst, buffer, sizeof(buffer), 0);
		puts(buffer);
		length += g_inst.length;
		if (j->second->listOfUse.size() == 0)
		{
			if(simpleDeadStoreFinder(j->second))
				printf("Dead Store: %x\n", j->second->address);
		}
		else
		{
			Value *it = j->second;
			for (Use* is: it->listOfUse)
			{
				printf("Use at %p\n", reinterpret_cast<Instruction*>(is->getUser())->address);
			}
		}
		printf("-----------------------------------------\n");
	}
	printf("End\n");
	delete eax0;
	delete ebx0;
	delete ecx0;
	delete edx0;
	delete ebp0;
	delete esp0;
	delete esi0;
	delete edi0;
}