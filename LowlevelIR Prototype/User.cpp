#include "Value.h"
#include "User.h"
#include "Use.h"
#include "Instruction.h"
#include "InstTypes.h"

void * User::allocateFixedOperandUser(size_t Size, unsigned Us)
{
	uint8_t *Storage = static_cast<uint8_t *>(::operator new(Size + sizeof(Use) * Us));
	Use *Start = reinterpret_cast<Use *>(Storage);
	Use *End = Start + Us;
	User *Obj = reinterpret_cast<User*>(End);
	Obj->NumUserOperands = Us;
	int cnt = 1;
	for (; Start != End; Start++)
	{
		new (Start) Use(Obj);
		//reinterpret_cast<Use*>(Start)->Value = cnt * 10 - cnt;
		//printf("Use[%d] :%p\n", cnt - 1, Start);
		cnt++;
	}
	//printf("Obj:%p\n", Obj);
	return Obj;
}

void *User::operator new(size_t Size, unsigned Us)
{
	//printf("Size :%d User Class Size: %dInsturction Class Size: %d BinaryOp Class Size :%d\n", Size, sizeof(User), sizeof(Instruction), sizeof(BinaryOp));
	return allocateFixedOperandUser(Size, Us);
}