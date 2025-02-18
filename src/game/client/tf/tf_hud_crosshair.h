//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_TF_CROSSHAIR_H
#define HUD_TF_CROSSHAIR_H
#ifdef _WIN32
#pragma once
#endif

#include "hud_crosshair.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTFCrosshair : public CHudCrosshair
{
public:
	DECLARE_CLASS_SIMPLE( CHudTFCrosshair, CHudCrosshair );

	CHudTFCrosshair( const char *name );
	virtual ~CHudTFCrosshair( void );

	virtual void Init() OVERRIDE;
	virtual void LevelShutdown( void ) OVERRIDE;
	virtual bool ShouldDraw() OVERRIDE;

protected:
	virtual void Paint() OVERRIDE;
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE;

private:
	int					m_iCrosshairTextureID;
	IVguiMatInfo		*m_pCrosshairMaterial;

	char				m_szPreviousCrosshair[256];	// name of the current crosshair
	float				m_flTimeToHideUntil;
};


#endif // HUD_TF_CROSSHAIR_H
