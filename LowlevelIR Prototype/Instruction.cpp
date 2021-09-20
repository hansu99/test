#include "InstTypes.h"
#include "Instruction.h"
#include "OperandTraits.h"
#include "User.h"
#include "Value.h"


Instruction::Instruction(int iType, Use * Ops, unsigned NumOps)
	: User(&iType, iType, Ops, NumOps)
{
	//printf("Instruction Constructor\n");
}

BinaryOp::BinaryOp(BinaryOps iType, Value *LHS, Value *RHS) :
	Instruction(iType,
		OperandTraits<BinaryOp>::op_begin(this),
		OperandTraits<BinaryOp>::operands(this))
{
	Op<0>() = LHS;
	Op<1>() = RHS;
	//printf("[BinaryOp] InstType :%d Operand Count: %d\n", iType, OperandTraits<BinaryOp>::operands(this));
}

UnaryOp::UnaryOp(UnaryOps iType, Value *LHS) :
	Instruction(iType,
		OperandTraits<UnaryOp>::op_begin(this),
		OperandTraits<UnaryOp>::operands(this))
{
	Op<0>() = LHS;
	//printf("[UnaryOp] InstType :%d Operand Count: %d\n", iType, OperandTraits<UnaryOp>::operands(this));
}

BinaryOp* BinaryOp::Create(BinaryOps Bo, Value *S1, Value *S2)
{
	//printf("[BinaryOp] Create.\n");
	return new BinaryOp(Bo, S1, S2);
}

UnaryOp* UnaryOp::Create(UnaryOps Uo, Value *S1)
{
	//printf("[UnaryOp] Create.\n");
	return new UnaryOp(Uo, S1);
}