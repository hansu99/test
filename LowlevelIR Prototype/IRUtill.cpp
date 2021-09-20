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

typedef struct _ValueTable
{
	CPT_NC_UINT_X index;
	CPT_NC_UINT_X eip;

}VALUE_TABLE, *PVALUE_TABLE;

extern std::map<CPT_NC_UINT_X, Instruction*> InstList;
extern std::map<ZydisRegister, VALUE_TABLE> regValueNumberTable;
extern std::map<CPT_NC_UINT_X, VALUE_TABLE> memValueNumberTable;

bool desc(VALUE_TABLE a, VALUE_TABLE b)
{
	return a.eip > b.eip;
}

void findRegDef(ZydisDecodedOperand *pOperand, std::vector<Instruction*>& toUseList)
{
	VALUE_TABLE idx32BitVT;
	VALUE_TABLE idx16BitVT;
	VALUE_TABLE idx8BitHighVT;
	VALUE_TABLE idx8BitLowVT;
	std::vector<VALUE_TABLE> defList;

	if (pOperand->size == 32)
	{
		ZeroMemory(&idx32BitVT, sizeof(VALUE_TABLE));
		ZeroMemory(&idx16BitVT, sizeof(VALUE_TABLE));
		ZeroMemory(&idx8BitHighVT, sizeof(VALUE_TABLE));
		ZeroMemory(&idx8BitLowVT, sizeof(VALUE_TABLE));

		// 32비트 레지스터가 마지막으로 정의된 명령어를 찾는다.
		if (regValueNumberTable.find(pOperand->reg.value) != regValueNumberTable.end())
			idx32BitVT = regValueNumberTable[pOperand->reg.value];

		if (idx32BitVT.eip != 0)
			defList.push_back(idx32BitVT);

		//if (regValueNumberTable.find(pOperand->reg.value) != regValueNumberTable.end())
		{
			if (pOperand->reg.value == ZYDIS_REGISTER_EAX)
			{
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_AX];
			}
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBX)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_BX];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ECX)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_CX];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDX)
			{
				//printf("EDX 레지스터이므로 EDX, DX, DH, DL에 대한 Def를 찾는다.\n");
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_DX];
			}
			else if (pOperand->reg.value == ZYDIS_REGISTER_ESP)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_SP];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBP)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_BP];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ESI)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_SI];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDI)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_DI];
			else
				;
			if (idx16BitVT.eip != 0)
				defList.push_back(idx16BitVT);

			// 8비트 상위
			if (pOperand->reg.value == ZYDIS_REGISTER_EAX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_AH];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_BH];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ECX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_CH];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_DH];
			else
				;
			if (idx8BitHighVT.eip != 0)
				defList.push_back(idx8BitHighVT);

			// 8비트 하위
			if (pOperand->reg.value == ZYDIS_REGISTER_EAX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_AL];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_BL];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ECX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_CL];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_DL];
			else
				;
			if (idx8BitLowVT.eip != 0)
				defList.push_back(idx8BitLowVT);

			std::sort(defList.begin(), defList.end(), desc);
			CPT_NC_UINT32 toFill32Bit = 0;

			for (int i = 0; i < defList.size(); i++)
			{
				if (toFill32Bit == 0xffffffff)
					break;

				if (InstList[defList[i].eip]->Size == 32)
				{
					toUseList.push_back(InstList[defList[i].eip]);
					toFill32Bit |= 0xffffffff;
				}

				else if (InstList[defList[i].eip]->Size == 16)
				{
					toUseList.push_back(InstList[defList[i].eip]);
					toFill32Bit |= 0xffff;
				}

				else if (InstList[defList[i].eip]->Size == 8)
				{
					if (ZYDIS_REGISTER_AH <= InstList[defList[i].eip]->Reg &&InstList[defList[i].eip]->Reg <= ZYDIS_REGISTER_BH)
					{
						toUseList.push_back(InstList[defList[i].eip]);
						toFill32Bit |= 0xf0;
						//printf("High :%x\n", InstList[defList[i].eip]->address);
					}

					else if (ZYDIS_REGISTER_AL <= InstList[defList[i].eip]->Reg &&InstList[defList[i].eip]->Reg <= ZYDIS_REGISTER_BL)
					{
						toUseList.push_back(InstList[defList[i].eip]);
						toFill32Bit |= 0x0f;
						//printf("Low :%x\n", InstList[defList[i].eip]->address);
					}
				}
			}
			//printf("idx32BitVT.eip: %p, idx16BitVT.eip: %p, idx8BitHighVT.eip: %p, idx8BitLowVT.eip: %p\n", idx32BitVT.eip, idx16BitVT.eip, idx8BitHighVT.eip, idx8BitLowVT.eip);
		}

	}

}

void findMemDef(ZydisDecodedOperand *pOperand, CPT_NC_UINT_X _memAddress, std::vector<Instruction*>& toUseList)
{
	VALUE_TABLE idx32BitVT;
	VALUE_TABLE idx16BitVT;
	VALUE_TABLE idx8BitHighVT;
	VALUE_TABLE idx8BitLowVT;
	std::vector<VALUE_TABLE> defList;

	if (pOperand->size == 32)
	{
		ZeroMemory(&idx32BitVT, sizeof(VALUE_TABLE));
		ZeroMemory(&idx16BitVT, sizeof(VALUE_TABLE));
		ZeroMemory(&idx8BitHighVT, sizeof(VALUE_TABLE));
		ZeroMemory(&idx8BitLowVT, sizeof(VALUE_TABLE));

		// 32비트 레지스터가 마지막으로 정의된 명령어를 찾는다.
		if (memValueNumberTable.find(_memAddress) != memValueNumberTable.end())
			idx32BitVT = memValueNumberTable[_memAddress];

		if (idx32BitVT.eip != 0)
			defList.push_back(idx32BitVT);

		//if (regValueNumberTable.find(pOperand->reg.value) != regValueNumberTable.end())
		{
			if (pOperand->reg.value == ZYDIS_REGISTER_EAX)
			{
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_AX];
			}
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBX)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_BX];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ECX)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_CX];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDX)
			{
				//printf("EDX 레지스터이므로 EDX, DX, DH, DL에 대한 Def를 찾는다.\n");
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_DX];
			}
			else if (pOperand->reg.value == ZYDIS_REGISTER_ESP)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_SP];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBP)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_BP];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ESI)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_SI];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDI)
				idx16BitVT = regValueNumberTable[ZYDIS_REGISTER_DI];
			else
				;
			if (idx16BitVT.eip != 0)
				defList.push_back(idx16BitVT);

			// 8비트 상위
			if (pOperand->reg.value == ZYDIS_REGISTER_EAX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_AH];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_BH];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ECX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_CH];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDX)
				idx8BitHighVT = regValueNumberTable[ZYDIS_REGISTER_DH];
			else
				;
			if (idx8BitHighVT.eip != 0)
				defList.push_back(idx8BitHighVT);

			// 8비트 하위
			if (pOperand->reg.value == ZYDIS_REGISTER_EAX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_AL];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EBX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_BL];
			else if (pOperand->reg.value == ZYDIS_REGISTER_ECX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_CL];
			else if (pOperand->reg.value == ZYDIS_REGISTER_EDX)
				idx8BitLowVT = regValueNumberTable[ZYDIS_REGISTER_DL];
			else
				;
			if (idx8BitLowVT.eip != 0)
				defList.push_back(idx8BitLowVT);

			std::sort(defList.begin(), defList.end(), desc);
			CPT_NC_UINT32 toFill32Bit = 0;

			for (int i = 0; i < defList.size(); i++)
			{
				if (toFill32Bit == 0xffffffff)
					break;

				if (InstList[defList[i].eip]->Size == 32)
				{
					toUseList.push_back(InstList[defList[i].eip]);
					toFill32Bit |= 0xffffffff;
				}

				else if (InstList[defList[i].eip]->Size == 16)
				{
					toUseList.push_back(InstList[defList[i].eip]);
					toFill32Bit |= 0xffff;
				}

				else if (InstList[defList[i].eip]->Size == 8)
				{
					if (ZYDIS_REGISTER_AH <= InstList[defList[i].eip]->Reg &&InstList[defList[i].eip]->Reg <= ZYDIS_REGISTER_BH)
					{
						toUseList.push_back(InstList[defList[i].eip]);
						toFill32Bit |= 0xf0;
						//printf("High :%x\n", InstList[defList[i].eip]->address);
					}

					else if (ZYDIS_REGISTER_AL <= InstList[defList[i].eip]->Reg &&InstList[defList[i].eip]->Reg <= ZYDIS_REGISTER_BL)
					{
						toUseList.push_back(InstList[defList[i].eip]);
						toFill32Bit |= 0x0f;
						//printf("Low :%x\n", InstList[defList[i].eip]->address);
					}
				}
			}
			//printf("idx32BitVT.eip: %p, idx16BitVT.eip: %p, idx8BitHighVT.eip: %p, idx8BitLowVT.eip: %p\n", idx32BitVT.eip, idx16BitVT.eip, idx8BitHighVT.eip, idx8BitLowVT.eip);
		}

	}

}