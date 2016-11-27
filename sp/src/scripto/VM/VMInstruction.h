#include <cstdint>

typedef uint32_t Reg;

// Instructions are 32 bits
typedef uint32_t Instruction;

// The "structure" of an instruction is as follows:
//		OPCODE	- 6  bits
//		ARGS	- 26 bits
//			A	- 8 bits
//			B	- 9 bits
//			C	- 9 bits
//			BC	- 18 bits
//			sBC	- signed BC
// B and C are signed to allow them to refer to registers or constants

// Op-Code
#define SIZE_OP	6
#define POS_OP	0

// A
#define SIZE_A	8
#define POS_A	(POS_OP + SIZE_OP)

// B
#define SIZE_B	9
#define POS_B	(POS_A + SIZE_A)

// C
#define SIZE_C	9
#define POS_C	(POS_B + SIZE_B)

// BC
#define SIZE_BC	(SIZE_B + SIZE_C)
#define POS(BC)	(POS_B)


#define GET_OP(x) x

///////////////////////////////////////////////////////////////////////


// R(x)		Register #x
// C(x)		Constant #x
// RC(x)	Register #x or constant #x,
// constant if IsC(x), otherwise register

// topmost bit of B or A (determines if reg or const)
// 1 - const, 0 - reg
#define RC_BIT	(1 << (SIZE_B - 1))

// is x a constant?
#define IsC(x)	((x) & RC_BIT)


enum OpCode {
/************************************************************
name			args	description
************************************************************/
OP_MOVE,//		A B		R(A) = R(B)
OP_LOAD,//		A B C	R(A) = C(BC)


OP_DEL,//		A B		R(A), R(A+1), ..., R(A+B) = NULL

OP_JUMP,//		sBC		PC += sBC	

OP_EQ,//		A B C	if ((RC(B) == RC(C)) != A) PC++	
OP_LT,//		A B C	if ((RC(B) <  RC(C)) != A) PC++
OP_LE,//		A B C	if ((RC(B) <= RC(C)) != A) PC++

OP_ADD,/*		A B C	R(A) = RC(B) + RC(x)	*/
OP_SUB,/*		A B C	R(A) = RC(B) - RC(x)	*/
OP_MUL,/*		A B C	R(A) = RC(B) * RC(x)	*/
OP_DIV,/*		A B C	R(A) = RC(B) / RC(x)	*/
OP_MOD,/*		A B C	R(A) = RC(B) % RC(x)	*/
OP_POW /*		A B C	R(A) = RC(B) ^ RC(x)	*/

};