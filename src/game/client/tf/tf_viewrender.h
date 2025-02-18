//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_VIEWRENDER_H
#define TF_VIEWRENDER_H
#ifdef _WIN32
#pragma once
#endif

#include "iviewrender.h"
#include "viewrender.h"

//-----------------------------------------------------------------------------
// Purpose: Implements the interview to view rendering for the client .dll
//-----------------------------------------------------------------------------
class CTFViewRender : public CViewRender
{
	DECLARE_CLASS( CTFViewRender, CViewRender );
public:
	CTFViewRender();

	virtual void Init( void );
	virtual void Render2DEffectsPostHUD( const CViewSetup &view );
};

#endif //TF_VIEWRENDER_H
