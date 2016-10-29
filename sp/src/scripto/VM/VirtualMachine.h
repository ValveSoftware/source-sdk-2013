// Scripto Virtual Machine CONCEPT
// Based off of Lua VM

#include "VMInstruction.h"
#include "VMState.h"
#include "VMTypes.h"

// Local (per-thread) state
struct VMState
{
	const Instruction* PC;
};


class CVirtualMachine
{
private:
	const Instruction* PC;
public:
	CVirtualMachine()
	{
		
	}

	void Init()
	{

	}

	// Execute from PC to next return
	void Exexcute(VMState* state)
	{
		for (;;)
		{
			state->PC++;
			switch (GET_OP(*PC))
			{
				case OP_MOVE: {

				} continue;

				// ...

				default: {

				} break;

			}
			
			
		}

	}


};