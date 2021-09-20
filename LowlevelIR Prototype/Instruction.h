#pragma once
#include "Value.h"
#include "User.h"
#include "Use.h"
#include "OperandTraits.h"
#include "CPT_CommonType.h"
//#include "InstTypes.h" // 이 헤더를 include 시키면 컴파일 에러 (Instruction 클래스가 정의되지 않음)가 발생하는데 이유를 알아야 한다.

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