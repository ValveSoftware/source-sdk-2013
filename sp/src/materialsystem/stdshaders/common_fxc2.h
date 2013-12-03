//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef COMMON_FXC2_H_
#define COMMON_FXC2_H_

// This file is here so you can add new utility functions without
// changing common_fxc.h and causing a recompile of the entire universe.

float LinearToMonochrome ( float3 r )
{
	return dot( r, float3( 0.299f, 0.587f, 0.114f ) );
}

#endif //#ifndef COMMON_FXC2_H_
