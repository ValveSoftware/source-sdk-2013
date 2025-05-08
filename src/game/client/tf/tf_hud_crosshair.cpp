//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "clientmode.h"
#include "c_tf_player.h"
#include "tf_hud_crosshair.h"
#include "hud_crosshair.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "tf_logic_halloween_2014.h"
#include "tf_gamerules.h"
#include "mathlib/mathlib.h"
#include "tf_weapon_passtime_gun.h"
#include "tf_hud_passtime_reticle.h"

//ConVar cl_crosshair_red( "cl_crosshair_red", "200", FCVAR_ARCHIVE );
//ConVar cl_crosshair_green( "cl_crosshair_green", "200", FCVAR_ARCHIVE );
//ConVar cl_crosshair_blue( "cl_crosshair_blue", "200", FCVAR_ARCHIVE );

ConVar cl_crosshair_file( "cl_crosshair_file", "", FCVAR_ARCHIVE );

ConVar cl_crosshair_scale( "cl_crosshair_scale", "32.0", FCVAR_ARCHIVE );

void OnCrosshairColorChanged( IConVar *var, const char *pOldValue, float flOldValue );
ConVar cl_crosshair_color( "cl_crosshair_color", "200 200 200", FCVAR_ARCHIVE, "Crosshair color in RGB format (\"r g b\")", OnCrosshairColorChanged );
ConVar pf_ballindicator( "pf_ballindicator", "1", FCVAR_ARCHIVE, "Enable/disable the HUD indicator when holding the ball." );
ConVar pf_ballindicator_file( "pf_ballindicator_file", "vgui/crosshairs/ballindicator", FCVAR_ARCHIVE, "Change the material for the HUD indicator when holding the ball." );
ConVar pf_ballindicator_color( "pf_ballindicator_color", "255 255 255", FCVAR_ARCHIVE, "Change the color of the HUD indicator when holding the ball.", OnCrosshairColorChanged );
ConVar pf_ballindicator_teamcolored( "pf_ballindicator_teamcolored", "1", FCVAR_ARCHIVE, "Overrides the color of the HUD indicator when holding the ball to always use your team's color." );
ConVar pf_ballindicator_scale( "pf_ballindicator_scale", "1.0", FCVAR_ARCHIVE, "Scale factor of the HUD indicator when holding the ball." );
using namespace vgui;

// Everything else is expecting to find "CHudCrosshair"
DECLARE_NAMED_HUDELEMENT( CHudTFCrosshair, CHudCrosshair );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTFCrosshair::CHudTFCrosshair( const char *pName ) :
	CHudCrosshair ( pName )
{
	m_szPreviousCrosshair[0] = '\0';
	m_iCrosshairTextureID = -1;
	m_iBallIndicatorTextureID = -1;
	m_flTimeToHideUntil = -1.f;

	ListenForGameEvent( "restart_timer_time" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTFCrosshair::~CHudTFCrosshair( void )
{
	if ( vgui::surface() && m_iCrosshairTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iCrosshairTextureID );
		m_iCrosshairTextureID = -1;
		if ( m_iBallIndicatorTextureID != -1 )
		{
			vgui::surface()->DestroyTextureID( m_iBallIndicatorTextureID );
			m_iBallIndicatorTextureID = -1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTFCrosshair::ShouldDraw( void )
{
	// turn off for the minigames
	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
		return false;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
		return false;

	// turn off if the local player is a ghost
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
			return false;

		if ( pPlayer->IsTaunting() )
			return false;
	}

	if ( m_flTimeToHideUntil > gpGlobals->curtime )
		return false;

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::LevelShutdown( void )
{
	m_szPreviousCrosshair[0] = '\0';

	if ( m_pCrosshairMaterial )
	{
		delete m_pCrosshairMaterial;
		m_pCrosshairMaterial = NULL;
	}
	
	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::Init()
{
	if ( m_iCrosshairTextureID == -1 )
	{
		m_iCrosshairTextureID = vgui::surface()->CreateNewTextureID();
	}

	if ( m_iBallIndicatorTextureID == -1 )
	{
		m_iBallIndicatorTextureID = vgui::surface()->CreateNewTextureID();
	}

	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "restart_timer_time", event->GetName() ) )
	{
		if ( TFGameRules() && TFGameRules()->IsCompetitiveMode() )
		{
			int nTime = event->GetInt( "time" );
			if ( ( nTime <= 10 ) && ( nTime > 0 ) )
			{
				m_flTimeToHideUntil = gpGlobals->curtime + nTime;
				return;
			}
		}
	}

	m_flTimeToHideUntil = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTFCrosshair::Paint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if( !pPlayer )
		return;

	const char *crosshairfile = cl_crosshair_file.GetString();
	if ( ( crosshairfile == NULL ) || ( Q_stricmp( m_szPreviousCrosshair, crosshairfile ) != 0 ) )
	{
		char buf[256];
		Q_snprintf( buf, sizeof(buf), "vgui/crosshairs/%s", crosshairfile );

		if ( m_iCrosshairTextureID != -1 )
		{
			vgui::surface()->DrawSetTextureFile( m_iCrosshairTextureID, buf, true, false );
		}

		if ( m_pCrosshairMaterial )
		{
			delete m_pCrosshairMaterial;
		}

		m_pCrosshairMaterial = vgui::surface()->DrawGetTextureMatInfoFactory( m_iCrosshairTextureID );

		if ( !m_pCrosshairMaterial )
			return;

		// save the name to compare with the cvar in the future
		Q_strncpy( m_szPreviousCrosshair, crosshairfile, sizeof(m_szPreviousCrosshair) );
	}

	

	if ( m_szPreviousCrosshair[0] == '\0' )
	{
		return BaseClass::Paint();
	}


	// This is somewhat cut'n'paste from CHudCrosshair::Paint(). Would be nice to unify them some more.
	float x, y;
	bool bBehindCamera;
	GetDrawPosition ( &x, &y, &bBehindCamera );

	if( bBehindCamera )
		return;

	float flWeaponScale = 1.f;
	int iTextureW = 32;
	int iTextureH = 32;
	C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
	vgui::ISurface *pSurf = vgui::surface();


	float flPlayerScale = 1.0f;
	float flBallIndicatorScale = 1.0f;
#ifdef TF_CLIENT_DLL
	//Color clr( cl_crosshair_red.GetInt(), cl_crosshair_green.GetInt(), cl_crosshair_blue.GetInt(), 255 );
    int r = 200, g = 200, b = 200;
	sscanf( cl_crosshair_color.GetString(), "%d %d %d", &r, &g, &b );
	Color clr( r, g, b, 255 );
	sscanf( pf_ballindicator_color.GetString(), "%d %d %d", &r, &g, &b );
	Color clrbi( r, g, b, 255 );
	flPlayerScale = cl_crosshair_scale.GetFloat() / 32.0f;  // the player can change the scale in the options/multiplayer tab
	flBallIndicatorScale = pf_ballindicator_scale.GetFloat(); // the player can change the scale in the options/multiplayer tab
#else
	Color clr = m_clrCrosshair;
	Color clrbi = m_clrCrosshair;
#endif
	float flWidth = flWeaponScale * flPlayerScale * (float)iTextureW;
	float flHeight = flWeaponScale * flPlayerScale * (float)iTextureH;
	float flBallIndicatorWidth = flBallIndicatorScale * (float)iTextureW;
	float flBallIndicatorHeight = flBallIndicatorScale * (float)iTextureH;
	int iWidth = (int)( flWidth + 0.5f );
	int iHeight = (int)( flHeight + 0.5f );
	int iBallIndicatorWidth = (int)( flBallIndicatorWidth + 0.5f );
	int iBallIndicatorHeight = (int)( flBallIndicatorHeight + 0.5f );
	int iX = (int)( x + 0.5f );
	int iY = (int)( y + 0.5f );

	if ( pWeapon )
	{
		pWeapon->GetWeaponCrosshairScale( flWeaponScale );

		bool bShouldDrawBallIndicator = pf_ballindicator.GetBool();
		if ( bShouldDrawBallIndicator ) 
		{
			if ( pPlayer->m_Shared.HasPasstimeBall())
			{
				const char *ballindicatorfile = pf_ballindicator_file.GetString();

				if ( m_iBallIndicatorTextureID != -1 )
				{
					pSurf->DrawSetTextureFile( m_iBallIndicatorTextureID, ballindicatorfile, true, false );
				}

				if ( pf_ballindicator_teamcolored.GetBool() )
				{
					Color teamColor;
					teamColor = GetTeamColor( pPlayer->GetTeamNumber() );
					clrbi = Color( teamColor.r(), teamColor.g(), teamColor.b(), 255 );
				}
				pSurf->DrawSetColor( clrbi );
				pSurf->DrawSetTexture( m_iBallIndicatorTextureID );
				pSurf->DrawTexturedRect( iX-iBallIndicatorWidth, iY-iBallIndicatorHeight, iX+iBallIndicatorWidth, iY+iBallIndicatorHeight );
				pSurf->DrawSetTexture(0);
			}
		}
	}
	
	pSurf->DrawSetColor( clr );
	pSurf->DrawSetTexture( m_iCrosshairTextureID );
	pSurf->DrawTexturedRect( iX-iWidth, iY-iHeight, iX+iWidth, iY+iHeight );
	pSurf->DrawSetTexture(0);


}

void OnCrosshairColorChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
    int r = 200, g = 200, b = 200;
    const char *value = ((ConVar*)var)->GetString();
    sscanf( value, "%d %d %d", &r, &g, &b );
    
    // Clamp values
    r = clamp( r, 0, 255 );
    g = clamp( g, 0, 255 );
    b = clamp( b, 0, 255 );
}