//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef CRTMEMDEBUG_H
#define CRTMEMDEBUG_H
#pragma once

#ifdef USECRTMEMDEBUG

#include <crtdbg.h>
#define MEMCHECK CheckHeap()
void CheckHeap( void );

#else

#define MEMCHECK

#endif

void InitCRTMemDebug( void );


#endif // CRTMEMDEBUG_H
