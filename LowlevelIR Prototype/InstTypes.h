#pragma once
#include "iterator_range.h"
#include "Instruction.h"
#include "OperandTraits.h"
#include "User.h"
#include "Value.h"

class BinaryOp :public Instruction
{
	BinaryOp(BinaryOps iType, Value *LHS, Value *RHS);
	//friend class Instruction;
public:
	//inline Value *getOperand(unsigned) const;
	int dpffkdladl;
	inline void setOperand(unsigned, Value*);
	inline op_iterator op_begin();
	inline const_op_iterator op_begin() const;
	inline op_iterator op_end();
	inline const_op_iterator op_end() const;
protected:
	template <int> inline Use &Op();
	template <int> inline const Use &Op() const;
public:
	inline unsigned getNumOperands() const;

	void *operator new(size_t S) 
	{ 
		//printf("BinaryOP new operator\n"); 
		return User::operator new(S, 2); 
	}
	static BinaryOp* Create(BinaryOps Bo, Value *S1, Value *S2);
};

class UnaryOp :public Instruction
{
	UnaryOp(UnaryOps iType, Value *LHS);
	//friend class Instruction;
public:
	//inline Value *getOperand(unsigned) const;
	int dpffkdladl;
	inline void setOperand(unsigned, Value*);
	inline op_iterator op_begin();
	inline const_op_iterator op_begin() const;
	inline op_iterator op_end();
	inline const_op_iterator op_end() const;
protected:
	template <int> inline Use &Op();
	template <int> inline const Use &Op() const;
public:
	inline unsigned getNumOperands() const;

	void *operator new(size_t S) 
	{ 
		//printf("UnaryOp new operator\n"); 
		return User::operator new(S, 1); 
	}
	static UnaryOp* Create(UnaryOps Uo, Value *S1);
};
// 템플릿 특수화

//template <typename T>
//struct OperandTraits;

template <>
struct OperandTraits<BinaryOp> :
	public FixedNumOperandTraits<BinaryOp, 2>
{
};

template <>
struct OperandTraits<UnaryOp> :
	public FixedNumOperandTraits<UnaryOp, 1>
{
};



//DEFINE_TRANSPARENT_OPERAND_ACCESSORS(BinaryOp, Value)

BinaryOp::op_iterator BinaryOp::op_begin()
{
	return OperandTraits<BinaryOp>::op_begin(this);
}
BinaryOp::const_op_iterator BinaryOp::op_begin() const
{

	return OperandTraits<BinaryOp>::op_begin(const_cast<BinaryOp*>(this));
}
BinaryOp::op_iterator BinaryOp::op_end()
{

	return OperandTraits<BinaryOp>::op_end(this);
}
BinaryOp::const_op_iterator BinaryOp::op_end() const
{

	return OperandTraits<BinaryOp>::op_end(const_cast<BinaryOp*>(this));
}

/*
Value *BinaryOp::getOperand(unsigned i_nocapture) const {
return cast_or_null<Value>(
OperandTraits<BinaryOp>::op_begin(const_cast<BinaryOp*>(this))[i_nocapture].get());
}
*/

void BinaryOp::setOperand(unsigned i_nocapture, Value *Val_nocapture)
{
	OperandTraits<BinaryOp>::op_begin(this)[i_nocapture] = Val_nocapture;
}
unsigned BinaryOp::getNumOperands() const
{
	return OperandTraits<BinaryOp>::operands((User*)this);
}

template <int Idx_nocapture> Use &BinaryOp::Op() {
	return this->OpFrom<Idx_nocapture>(this);
}
template <int Idx_nocapture> const Use &BinaryOp::Op() const {
	return this->OpFrom<Idx_nocapture>(this);
}
////////////////////////////////////////////////////////////////////

UnaryOp::op_iterator UnaryOp::op_begin()
{
	return OperandTraits<UnaryOp>::op_begin(this);
}
UnaryOp::const_op_iterator UnaryOp::op_begin() const
{

	return OperandTraits<UnaryOp>::op_begin(const_cast<UnaryOp*>(this));
}
UnaryOp::op_iterator UnaryOp::op_end()
{

	return OperandTraits<UnaryOp>::op_end(this);
}
UnaryOp::const_op_iterator UnaryOp::op_end() const
{

	return OperandTraits<UnaryOp>::op_end(const_cast<UnaryOp*>(this));
}

/*
Value *BinaryOp::getOperand(unsigned i_nocapture) const {
return cast_or_null<Value>(
OperandTraits<BinaryOp>::op_begin(const_cast<BinaryOp*>(this))[i_nocapture].get());
}
*/

void UnaryOp::setOperand(unsigned i_nocapture, Value *Val_nocapture)
{
	OperandTraits<UnaryOp>::op_begin(this)[i_nocapture] = Val_nocapture;
}
unsigned UnaryOp::getNumOperands() const
{
	return OperandTraits<UnaryOp>::operands((User*)this);
}

template <int Idx_nocapture> Use &UnaryOp::Op() {
	return this->OpFrom<Idx_nocapture>(this);
}
template <int Idx_nocapture> const Use &UnaryOp::Op() const {
	return this->OpFrom<Idx_nocapture>(this);
}