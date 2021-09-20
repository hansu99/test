#pragma once
#include "User.h"

template <typename SubClass, unsigned ARITY>
struct FixedNumOperandTraits
{
	static Use *op_begin(SubClass* U)
	{
		//printf("FixedNumOperandTraits op_begin :%p\n", reinterpret_cast<Use*>(U) - ARITY);
		return reinterpret_cast<Use*>(U) - ARITY;
	}
	static Use *op_end(SubClass* U)
	{
		return reinterpret_cast<Use*>(U);
	}
	static unsigned operands(const User*)
	{
		return ARITY;
	}
};

template <typename SubClass, unsigned ARITY = 1>
struct OptionalOperandTraits : public FixedNumOperandTraits<SubClass, ARITY>
{
	static unsigned operands(const User *U)
	{
		return U->getNumOperands();
	}
};

template <unsigned MINARITY = 1>
struct HungoffOperandTraits
{
	static Use *op_begin(User* U)
	{
		return U->getOperandList();
	}
	static Use *op_end(User* U)
	{
		return U->getOperandList() + U->getNumOperands();
	}
	static unsigned operands(const User *U)
	{
		return U->getNumOperands();
	}
};

/// Macro for generating in-class operand accessor declarations.
/// It should only be called in the public section of the interface.
///
#define DECLARE_TRANSPARENT_OPERAND_ACCESSORS(VALUECLASS) \
  public: \
  inline VALUECLASS *getOperand(unsigned) const; \
  inline void setOperand(unsigned, VALUECLASS*); \
  inline op_iterator op_begin(); \
  inline const_op_iterator op_begin() const; \
  inline op_iterator op_end(); \
  inline const_op_iterator op_end() const; \
  protected: \
  template <int> inline Use &Op(); \
  template <int> inline const Use &Op() const; \
  public: \
  inline unsigned getNumOperands() const

/// Macro for generating out-of-class operand accessor definitions
#define DEFINE_TRANSPARENT_OPERAND_ACCESSORS(CLASS, VALUECLASS) \
CLASS::op_iterator CLASS::op_begin() { \
  return OperandTraits<CLASS>::op_begin(this); \
} \
CLASS::const_op_iterator CLASS::op_begin() const { \
  return OperandTraits<CLASS>::op_begin(const_cast<CLASS*>(this)); \
} \
CLASS::op_iterator CLASS::op_end() { \
  return OperandTraits<CLASS>::op_end(this); \
} \
CLASS::const_op_iterator CLASS::op_end() const { \
  return OperandTraits<CLASS>::op_end(const_cast<CLASS*>(this)); \
} \
VALUECLASS *CLASS::getOperand(unsigned i_nocapture) const { \
  assert(i_nocapture < OperandTraits<CLASS>::operands(this) \
         && "getOperand() out of range!"); \
  return cast_or_null<VALUECLASS>( \
    OperandTraits<CLASS>::op_begin(const_cast<CLASS*>(this))[i_nocapture].get()); \
} \
void CLASS::setOperand(unsigned i_nocapture, VALUECLASS *Val_nocapture) { \
  assert(i_nocapture < OperandTraits<CLASS>::operands(this) \
         && "setOperand() out of range!"); \
  OperandTraits<CLASS>::op_begin(this)[i_nocapture] = Val_nocapture; \
} \
unsigned CLASS::getNumOperands() const { \
  return OperandTraits<CLASS>::operands(this); \
} \
template <int Idx_nocapture> Use &CLASS::Op() { \
  return this->OpFrom<Idx_nocapture>(this); \
} \
template <int Idx_nocapture> const Use &CLASS::Op() const { \
  return this->OpFrom<Idx_nocapture>(this); \
}