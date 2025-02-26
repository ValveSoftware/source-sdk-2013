//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Add a specially formatted string to each debug library of the form 
//          "%LIBNAME%.lib is built debug!". We can search for this string via
//			a Perforce trigger to ensure that debug LIBs are not checked in.
//
//			Also added a global int that is emitted only in release builds so
//			that release DLLs and EXEs can ensure that they are linking to 
//			release versions of LIBs at link time. The global int adhere to the
//			naming convention "%LIBNAME%_lib_is_a_release_build".
//
//			To take advantage of this pattern a DLL or EXE should add code 
//			at global scope similar to the following in one of its modules:
//
//			#if !defined(_DEBUG)
//			extern int bitmap_lib_is_a_release_build;
//
//			void DebugTestFunction()
//			{
//				bitmap_lib_is_a_release_build = 0;
//			}
//			#endif
//
//=============================================================================//

#if defined(DEBUG) || defined(_DEBUG)
#define _DEBUGONLYSTRING(x) #x
#define DEBUGONLYSTRING(x) _DEBUGONLYSTRING(x) 
static volatile char const *pDebugString = DEBUGONLYSTRING(LIBNAME) ".lib is built debug!";
#else
#define _RELONLYINT(x) int x ## _lib_is_a_release_build = 0 
#define RELONLYINT(x) _RELONLYINT(x)
RELONLYINT(LIBNAME);
#endif