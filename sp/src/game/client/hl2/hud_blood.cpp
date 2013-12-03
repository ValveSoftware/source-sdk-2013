//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "clienteffectprecachesystem.h"
#include "c_te_effect_dispatch.h"
#include "hud.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void BloodSplatCallback( const CEffectData & data )
{
/*
	Msg("SPLAT!\n");

	int		x,y;

	// Find our screen position to start from
	x = XRES(320);
	y = YRES(240);

	// Draw the ammo label
	CHudTexture	*pSplat = gHUD.GetIcon( "hud_blood1" );
	
  // FIXME:  This can only occur during vgui::Paint() stuff
	pSplat->DrawSelf( x, y, gHUD.m_clrNormal);
*/
}

DECLARE_CLIENT_EFFECT( "HudBloodSplat", BloodSplatCallback );
