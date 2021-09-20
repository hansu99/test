#pragma once

class User;
class Value;

/// A Use represents the edge between a Value definition and its users.
///
/// This is notionally a two-dimensional linked list. It supports traversing
/// all of the uses for a particular value definition. It also supports jumping
/// directly to the used value when we arrive from the User's operands, and
/// jumping directly to the User when we arrive from the Value's uses.
class Use {
public:
	Use(const Use &U) = delete;

private:
	/// Destructor - Only for zap()
	~Use() {
		if (Val)
			removeFromList();
	}

	/// Constructor
	Use(User *Parent) : Parent(Parent) { /*printf("Use Constructor\n");*/ }

public:
	friend class Value;
	friend class User;

	operator Value *() const { return Val; }
	Value *get() const { return Val; }

	/// Returns the User that contains this Use.
	///
	/// For an instruction operand, for example, this will return the
	/// instruction.
	User *getUser() const { return Parent; };

	inline void set(Value *Val);

	inline Value *operator=(Value *RHS);
	inline const Use &operator=(const Use &RHS);

	Value *operator->() { return Val; }
	const Value *operator->() const { return Val; }

	Use *getNext() const { return Next; }

private:

	Value *Val = nullptr;
	Use *Next = nullptr;
	Use **Prev = nullptr;
	User *Parent = nullptr;

	void addToList(Use **List) {
		//printf("Use addToList %p this:%p\n", List ,this);
		Next = *List;
		if (Next)
			Next->Prev = &Next;
		Prev = List;
		*Prev = this;
	}

	void removeFromList() {
		*Prev = Next;
		if (Next)
			Next->Prev = Prev;
	}
};