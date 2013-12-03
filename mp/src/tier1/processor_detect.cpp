//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: win32 dependant ASM code for CPU capability detection
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#if defined( _X360 ) || defined( WIN64 )

bool CheckMMXTechnology(void) { return false; }
bool CheckSSETechnology(void) { return false; }
bool CheckSSE2Technology(void) { return false; }
bool Check3DNowTechnology(void) { return false; }

#elif defined( _WIN32 ) && !defined( _X360 )

#pragma optimize( "", off )
#pragma warning( disable: 4800 ) //'int' : forcing value to bool 'true' or 'false' (performance warning)

// stuff from windows.h
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER       1
#endif

bool CheckMMXTechnology(void)
{
    int retval = true;
    unsigned int RegEDX = 0;

#ifdef CPUID
	_asm pushad;
#endif

    __try
	{
        _asm
		{
#ifdef CPUID
			xor edx, edx	// Clue the compiler that EDX is about to be used.
#endif
            mov eax, 1      // set up CPUID to return processor version and features
                            //      0 = vendor string, 1 = version info, 2 = cache info
            CPUID           // code bytes = 0fh,  0a2h
            mov RegEDX, edx // features returned in edx
		}
    } 
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
		retval = false; 
	}

	// If CPUID not supported, then certainly no MMX extensions.
    if (retval)
	{
		if (RegEDX & 0x800000)          // bit 23 is set for MMX technology
		{
		   __try 
		   { 
				// try executing the MMX instruction "emms"
			   _asm EMMS
		   } 
		   __except(EXCEPTION_EXECUTE_HANDLER) 
		   { 
			   retval = false; 
		   }
		}

		else
			retval = false;           // processor supports CPUID but does not support MMX technology

		// if retval == 0 here, it means the processor has MMX technology but
		// floating-point emulation is on; so MMX technology is unavailable
	}

#ifdef CPUID
	_asm popad;
#endif

    return retval;
}

bool CheckSSETechnology(void)
{
    int retval = true;
    unsigned int RegEDX = 0;

#ifdef CPUID
	_asm pushad;
#endif

	// Do we have support for the CPUID function?
    __try
	{
        _asm
		{
#ifdef CPUID
			xor edx, edx			// Clue the compiler that EDX is about to be used.
#endif
            mov eax, 1				// set up CPUID to return processor version and features
									//      0 = vendor string, 1 = version info, 2 = cache info
            CPUID					// code bytes = 0fh,  0a2h
            mov RegEDX, edx			// features returned in edx
		}
    } 
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
		retval = false; 
	}

	// If CPUID not supported, then certainly no SSE extensions.
    if (retval)
	{
		// Do we have support for SSE in this processor?
		if ( RegEDX & 0x2000000L )		// bit 25 is set for SSE technology
		{
			// Make sure that SSE is supported by executing an inline SSE instruction

// BUGBUG, FIXME - Visual C Version 6.0 does not support SSE inline code YET (No macros from Intel either)
// Fix this if VC7 supports inline SSE instructinons like "xorps" as shown below.
#if 1
			__try
			{
				_asm
				{
					// Attempt execution of a SSE instruction to make sure OS supports SSE FPU context switches
					xorps xmm0, xmm0
					// This will work on Win2k+ (Including masking SSE FPU exception to "normalized" values)
					// This will work on Win98+ (But no "masking" of FPU exceptions provided)
				}
			} 
			__except(EXCEPTION_EXECUTE_HANDLER) 
#endif

			{ 
				retval = false; 
			}
		}
		else
			retval = false;
	}
#ifdef CPUID
	_asm popad;
#endif

    return retval;
}

bool CheckSSE2Technology(void)
{
    int retval = true;
    unsigned int RegEDX = 0;

#ifdef CPUID
	_asm pushad;
#endif

	// Do we have support for the CPUID function?
    __try
	{
        _asm
		{
#ifdef CPUID
			xor edx, edx			// Clue the compiler that EDX is about to be used.
#endif
            mov eax, 1				// set up CPUID to return processor version and features
									//      0 = vendor string, 1 = version info, 2 = cache info
            CPUID					// code bytes = 0fh,  0a2h
            mov RegEDX, edx			// features returned in edx
		}
    } 
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
		retval = false; 
	}

	// If CPUID not supported, then certainly no SSE extensions.
    if (retval)
	{
		// Do we have support for SSE in this processor?
		if ( RegEDX & 0x04000000 )		// bit 26 is set for SSE2 technology
		{
			// Make sure that SSE is supported by executing an inline SSE instruction

			__try
			{
				_asm
				{
					// Attempt execution of a SSE2 instruction to make sure OS supports SSE FPU context switches
					xorpd xmm0, xmm0
				}
			} 
			__except(EXCEPTION_EXECUTE_HANDLER) 

			{ 
				retval = false; 
			}
		}
		else
			retval = false;
	}
#ifdef CPUID
	_asm popad;
#endif

    return retval;
}

bool Check3DNowTechnology(void)
{
    int retval = true;
    unsigned int RegEAX = 0;

#ifdef CPUID
	_asm pushad;
#endif

    // First see if we can execute CPUID at all
	__try
	{
        _asm
		{
#ifdef CPUID
//			xor edx, edx			// Clue the compiler that EDX is about to be used.
#endif
            mov eax, 0x80000000     // setup CPUID to return whether AMD >0x80000000 function are supported.
									// 0x80000000 = Highest 0x80000000+ function, 0x80000001 = 3DNow support
            CPUID					// code bytes = 0fh,  0a2h
            mov RegEAX, eax			// result returned in eax
		}
    } 
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
		retval = false; 
	}

	// If CPUID not supported, then there is definitely no 3DNow support
    if (retval)
	{
		// Are there any "higher" AMD CPUID functions?
		if (RegEAX > 0x80000000L )				
		{
		   __try 
			{
			_asm
				{
					mov			eax, 0x80000001		// setup to test for CPU features
					CPUID							// code bytes = 0fh,  0a2h
					shr			edx, 31				// If bit 31 is set, we have 3DNow support!
					mov			retval, edx			// Save the return value for end of function
				}
			}
			__except(EXCEPTION_EXECUTE_HANDLER) 
			{ 
				retval = false; 
			}
		}
		else
		{
			// processor supports CPUID but does not support AMD CPUID functions
			retval = false;					
		}
	}

#ifdef CPUID
	_asm popad;
#endif

    return retval;
}

#pragma optimize( "", on )

#endif // _WIN32
