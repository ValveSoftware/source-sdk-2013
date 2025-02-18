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
#if !defined( OVERLAYTEXT_H )
#define OVERLAYTEXT_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

class  OverlayText_t
{
public:
	OverlayText_t()
	{
		nextOverlayText = 0;
		origin.Init();
		bUseOrigin = false;
		lineOffset = 0;
		flXPos = 0;
		flYPos = 0;
		text[ 0 ] = 0;
		m_flEndTime = 0.0f;
		m_nServerCount = -1;
		m_nCreationTick = -1;
		r = g = b = a = 255;
	}

	bool			IsDead();
	void			SetEndTime( float duration );

	Vector			origin;
	bool			bUseOrigin;
	int				lineOffset;
	float			flXPos;
	float			flYPos;
	char			text[512];
	float			m_flEndTime;			// When does this text go away
	int				m_nCreationTick;		// If > 0, show only one server frame
	int				m_nServerCount;			// compare server spawn count to remove stale overlays
	int				r;
	int				g;
	int				b;
	int				a;
	OverlayText_t	*nextOverlayText;
};

#endif // OVERLAYTEXT_H