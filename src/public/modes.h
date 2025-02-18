//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#if !defined( MODES_H )
#define MODES_H
#ifdef _WIN32
#pragma once
#endif

typedef struct vmode_s
{
	int			width;
	int			height;
	int			bpp;
	int			refreshRate;
} vmode_t;

#endif // MODES_H
