//=========== Copyright Valve Corporation, All rights reserved. ===============//
//
// Purpose: 
//=============================================================================//

#include <Cocoa/Cocoa.h>

extern "C" void *CreateAutoReleasePool()
{
	return [[NSAutoreleasePool alloc] init];
}

extern "C" void ReleaseAutoReleasePool( void *pool )
{
	[pool release];	
}