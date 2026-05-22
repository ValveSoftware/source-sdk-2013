//========= Copyright Valve Corporation, All rights reserved. ============//
// Frog Bomber arena minimap — always drawn via CHudElement (not debug overlays).
//=============================================================================
#include "cbase.h"

#ifdef SOURCESDK

#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "tf_gamerules.h"
#include "bm_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
class CHudBomberArena : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudBomberArena, Panel );

public:
	CHudBomberArena( const char *pElementName );

	virtual bool ShouldDraw( void );
	virtual void Paint( void );
};

DECLARE_HUDELEMENT( CHudBomberArena );

//-----------------------------------------------------------------------------
CHudBomberArena::CHudBomberArena( const char *pElementName )
	: CHudElement( pElementName ), BaseClass( NULL, "HudBomberArena" )
{
	SetParent( g_pClientMode->GetViewport() );
	SetHiddenBits( 0 );
}

//-----------------------------------------------------------------------------
bool CHudBomberArena::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
	{
		return false;
	}

	return TFGameRules() && TFGameRules()->IsBombermanMode();
}

//-----------------------------------------------------------------------------
void CHudBomberArena::Paint( void )
{
	BM_ClientPaintArenaHUD();
}

#endif // SOURCESDK
