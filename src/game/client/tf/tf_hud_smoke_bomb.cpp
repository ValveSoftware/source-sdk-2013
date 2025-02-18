//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "c_tf_player.h"
#include "clientmode_tf.h"


class CHudSmokeBomb : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudSmokeBomb, vgui::Panel );

	CHudSmokeBomb( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void Init();

private:
	CHudTexture *m_pIcon;
};

DECLARE_HUDELEMENT( CHudSmokeBomb );

CHudSmokeBomb::CHudSmokeBomb( const char *pName ) :
	vgui::Panel( NULL, "HudSmokeBomb" ), CHudElement( pName )
{
	SetParent( g_pClientMode->GetViewport() );
	m_pIcon = NULL;

	SetHiddenBits( HIDEHUD_PLAYERDEAD );
}


void CHudSmokeBomb::Init()
{
}

bool CHudSmokeBomb::ShouldDraw()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// if we are spectating another player first person, check this player
	if ( pPlayer && ( pPlayer->GetObserverMode() == OBS_MODE_IN_EYE ) )
	{
		pPlayer = ToTFPlayer( pPlayer->GetObserverTarget() );
	}

	return ( pPlayer && pPlayer->IsAlive() && pPlayer->m_Shared.InCond( TF_COND_SMOKE_BOMB ) );
}

extern ConVar tf_smoke_bomb_time;

void CHudSmokeBomb::Paint()
{
	if ( !m_pIcon )
	{
		m_pIcon = gHUD.GetIcon( "cond_smoke_bomb" );
	}

	int x, y, w, h;
	GetBounds( x, y, w, h );

	if ( m_pIcon )
	{
		m_pIcon->DrawSelf( 0, 0, w, w, Color(255,255,255,255) );
	}

	// Draw a progress bar for time remaining
	int barX = XRES(5);
	int barW = w - XRES(10);
	int barY = w + YRES(5);
	int barH = YRES(10);

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;
    
	float flExpireTime = pPlayer->m_Shared.GetSmokeBombExpireTime();

	float flPercent = ( flExpireTime - gpGlobals->curtime ) / tf_smoke_bomb_time.GetFloat();

	surface()->DrawSetColor( Color(0,0,0,255) );
	surface()->DrawFilledRect( barX - 1, barY - 1, barX + barW + 1, barY + barH + 1 );

	surface()->DrawSetColor( Color(200,200,200,255) );
	surface()->DrawFilledRect( barX, barY, barX + (int)( (float)barW * flPercent ), barY + barH );
}