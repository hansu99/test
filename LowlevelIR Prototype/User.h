#pragma once
#include "iterator_range.h"
#include "Use.h"
#include "Value.h"


template <typename T> class ArrayRef;
template <typename T> class MutableArrayRef;

/// Compile-time customization of User operands.
///
/// Customizes operand-related allocators and accessors.
template <typename T>
struct OperandTraits
{
	static Use *op_begin(T* U)
	{
		//printf("OperandTraits op_begin :%p\n", <Use*>(U) - 2);
		return reinterpret_cast<Use*>(U) - 2;
	}
	static Use *op_end(T* U)
	{
		return reinterpret_cast<Use*>(U);
	}
	static unsigned operands(const User*)
	{
		return 2;
	}
};

class User : public Value
{
	template <unsigned>
	friend struct HungoffOperandTraits;

	inline static void *
		allocateFixedOperandUser(size_t, unsigned);

protected:
	/// Allocate a User with an operand pointer co-allocated.
	///
	/// This is used for subclasses which need to allocate a variable number
	/// of operands, ie, 'hung off uses'.
	//void *operator new(size_t Size);

	/// Allocate a User with the operands co-allocated.
	///
	/// This is used for subclasses which have a fixed number of operands.
	void *operator new(size_t Size, unsigned Us);

	/// Allocate a User with the operands co-allocated.  If DescBytes is non-zero
	/// then allocate an additional DescBytes bytes before the operands. These
	/// bytes can be accessed by calling getDescriptor.
	///
	/// DescBytes needs to be divisible by sizeof(void *).  The allocated
	/// descriptor, if any, is aligned to sizeof(void *) bytes.
	///
	/// This is used for subclasses which have a fixed number of operands.
	//void *operator new(size_t Size, unsigned Us, unsigned DescBytes);

	User(int *ty, unsigned vty, Use *, unsigned NumOps)
		: Value()//ty, vty)
	{
		//printf("User Constructor\n");
		NumUserOperands = NumOps;
	}

	/// Allocate the array of Uses, followed by a pointer
	/// (with bottom bit set) to the User.
	/// \param IsPhi identifies callers which are phi nodes and which need
	/// N BasicBlock* allocated along with N
	//void allocHungoffUses(unsigned N, bool IsPhi = false);

	/// Grow the number of hung off uses.  Note that allocHungoffUses
	/// should be called if there are no uses.
	//void growHungoffUses(unsigned N, bool IsPhi = false);

protected:
	~User() = default; // Use deleteValue() to delete a generic Instruction.

public:
	User(const User &) = delete;


protected:
	template <int Idx, typename U> static Use &OpFrom(const U *that)
	{
		//printf("User:OpFrom Idx:%d Operand Address:%p\n", Idx, &OperandTraits<U>::op_begin(const_cast<U*>(that))[Idx]);
		return Idx < 0
			? OperandTraits<U>::op_end(const_cast<U*>(that))[Idx]
			: OperandTraits<U>::op_begin(const_cast<U*>(that))[Idx];
	}

	template <int Idx> Use &Op()
	{
		return OpFrom<Idx>(this);
	}
	template <int Idx> const Use &Op() const
	{
		return OpFrom<Idx>(this);
	}

private:
	const Use *getHungOffOperands() const
	{
		//printf("getHungOffOperands\n");
		return *(reinterpret_cast<const Use *const *>(this) - 1);
	}

	Use *&getHungOffOperands() { return *(reinterpret_cast<Use **>(this) - 1); }

	const Use *getIntrusiveOperands() const
	{
		//printf("getIntrusiveOperands\n");
		return reinterpret_cast<const Use *>(this) - NumUserOperands;
	}

	Use *getIntrusiveOperands()
	{
		return reinterpret_cast<Use *>(this) - NumUserOperands;
	}

	void setOperandList(Use *NewList)
	{
		getHungOffOperands() = NewList;
	}

public:
	const Use *getOperandList() const
	{
		//return HasHungOffUses ? getHungOffOperands() : getIntrusiveOperands();
		return getIntrusiveOperands();
	}
	Use *getOperandList()
	{
		return const_cast<Use *>(static_cast<const User *>(this)->getOperandList());
	}

	Value *getOperand(unsigned i) const
	{
		return getOperandList()[i];
	}

	void setOperand(unsigned i, Value *Val)
	{
		getOperandList()[i] = Val;
	}

	const Use &getOperandUse(unsigned i) const
	{
		return getOperandList()[i];
	}
	Use &getOperandUse(unsigned i)
	{
		return getOperandList()[i];
	}

	unsigned getNumOperands() const { return NumUserOperands; }

	/// Set the number of operands on a GlobalVariable.
	///
	/// GlobalVariable always allocates space for a single operands, but
	/// doesn't always use it.
	///
	/// FIXME: As that the number of operands is used to find the start of
	/// the allocated memory in operator delete, we need to always think we have
	/// 1 operand before delete.
	void setGlobalVariableNumOperands(unsigned NumOps)
	{
		NumUserOperands = NumOps;
	}

	/// Subclasses with hung off uses need to manage the operand count
	/// themselves.  In these instances, the operand count isn't used to find the
	/// OperandList, so there's no issue in having the operand count change.
	void setNumHungOffUseOperands(unsigned NumOps)
	{
		NumUserOperands = NumOps;
	}

	// ---------------------------------------------------------------------------
	// Operand Iterator interface...
	//
	using op_iterator = Use*;
	using const_op_iterator = const Use*;
	using op_range = iterator_range<op_iterator>;
	using const_op_range = iterator_range<const_op_iterator>;

	op_iterator       op_begin() { return getOperandList(); }
	const_op_iterator op_begin() const { return getOperandList(); }
	op_iterator       op_end()
	{
		return getOperandList() + NumUserOperands;
	}
	const_op_iterator op_end()   const
	{
		return getOperandList() + NumUserOperands;
	}
	op_range operands()
	{
		return op_range(op_begin(), op_end());
	}
	const_op_range operands() const
	{
		return const_op_range(op_begin(), op_end());
	}
};