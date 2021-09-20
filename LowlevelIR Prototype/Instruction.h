#pragma once
#include "Value.h"
#include "User.h"
#include "Use.h"
#include "OperandTraits.h"
#include "CPT_CommonType.h"
//#include "InstTypes.h" // �� ����� include ��Ű�� ������ ���� (Instruction Ŭ������ ���ǵ��� ����)�� �߻��ϴµ� ������ �˾ƾ� �Ѵ�.

class Instruction : public User
{
public:
	Instruction(int iType, Use * Ops, unsigned NumOps);
	CPT_NC_UINT_X address;
	enum BinaryOps
	{
#define  FIRST_BINARY_INST(N)             BinaryOpsBegin = N,
#define HANDLE_BINARY_INST(N, OPC, CLASS) OPC = N,
#define   LAST_BINARY_INST(N)             BinaryOpsEnd = N+1
		Add, Sub
	};

	enum UnaryOps
	{
#define  FIRST_BINARY_INST(N)             BinaryOpsBegin = N,
#define HANDLE_BINARY_INST(N, OPC, CLASS) OPC = N,
#define   LAST_BINARY_INST(N)             BinaryOpsEnd = N+1
		Mov
	};
};