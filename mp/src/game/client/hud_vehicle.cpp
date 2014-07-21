//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_vehicle.h"
#include "iclientmode.h"
#include "view.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include "iclientvehicle.h"
#include "c_prop_vehicle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudVehicle );

CHudVehicle::CHudVehicle( const char *pElementName ) :
  CHudElement( pElementName ), BaseClass( NULL, "HudVehicle" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_VEHICLE_CROSSHAIR );
}

void CHudVehicle::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
	SetForceStereoRenderToFrameBuffer( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : IClientVehicle
//-----------------------------------------------------------------------------
IClientVehicle *CHudVehicle::GetLocalPlayerVehicle()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer ||  !pPlayer->IsInAVehicle() )
	{
		return NULL;
	}

	return pPlayer->GetVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudVehicle::ShouldDraw()
{
	// Don't draw if we're getting into/out of the vehicle
	IClientVehicle *pVehicle = GetLocalPlayerVehicle();
	if ( pVehicle )
	{
		C_PropVehicleDriveable *pDrivable = dynamic_cast<C_PropVehicleDriveable*>(pVehicle);
		
		if ( ( pDrivable ) && ( pDrivable->IsRunningEnterExitAnim() ) )
			return false;

		return CHudElement::ShouldDraw();
	}

	return false;
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudVehicle::Paint( void )
{
	IClientVehicle *v = GetLocalPlayerVehicle();
	if ( !v )
		return;

	// Vehicle-based hud...
	v->DrawHudElements();
}

