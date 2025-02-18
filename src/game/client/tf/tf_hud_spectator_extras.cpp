//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_team.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "iclientmode.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "tf_hud_target_id.h"
#include "c_baseobject.h"
#include "tf_hud_spectator_extras.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

using namespace vgui;

ConVar tf_spec_xray_disable( "tf_spec_xray_disable", "0", FCVAR_ARCHIVE, "Disable the spectator xray mode." );
ConVar tf_enable_glows_after_respawn( "tf_enable_glows_after_respawn", "1", FCVAR_ARCHIVE, "Enable teammate glow effects after respawn." );

DECLARE_HUDELEMENT( CTFHudSpectatorExtras );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudSpectatorExtras::CTFHudSpectatorExtras( const char *pszElementName ) : CHudElement( pszElementName ), EditablePanel( NULL, "HudSpectatorExtras" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( 0 );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudSpectatorExtras::ShouldDraw( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudSpectatorExtras::Reset( void )
{
	if ( !g_PR )
		return;

	FOR_EACH_VEC( m_vecEntitiesToDraw, i )
	{
		int nEntIndex = m_vecEntitiesToDraw[i].m_nEntIndex;
		if ( IsPlayerIndex( nEntIndex ) && !g_PR->IsConnected( nEntIndex ) )
			continue;

		CBaseCombatCharacter *pEnt = dynamic_cast< CBaseCombatCharacter* >( cl_entitylist->GetEnt( nEntIndex ) );
		if ( !pEnt )
			continue;

		if ( pEnt->IsClientSideGlowEnabled() )
		{
			pEnt->SetClientSideGlowEnabled( false );
		}
	}

	m_vecEntitiesToDraw.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudSpectatorExtras::RemoveEntity( int nRemove )
{
	FOR_EACH_VEC( m_vecEntitiesToDraw, i )
	{
		if ( m_vecEntitiesToDraw[i].m_nEntIndex == nRemove )
		{
			m_vecEntitiesToDraw.Remove( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudSpectatorExtras::OnTick()
{
	BaseClass::OnTick();

	if ( !g_PR )
		return;

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
	{
		Reset();
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	int nLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
	bool bIsHLTV = engine->IsHLTV();

	if ( tf_spec_xray_disable.GetBool() || ( !bIsHLTV && ( nLocalPlayerTeam < TEAM_SPECTATOR ) ) )
	{
		Reset();
		return;
	}

	if ( nLocalPlayerTeam >= FIRST_GAME_TEAM )
	{
		if ( pLocalPlayer->IsAlive() && !pLocalPlayer->m_Shared.InCond( TF_COND_TEAM_GLOWS ) )
		{
			if ( m_vecEntitiesToDraw.Count() > 0 )
			{
				Reset();
			}
			return;
		}
	}

	if ( bIsHLTV || 
		( tf_spec_xray.GetBool() && ( ( nLocalPlayerTeam == TEAM_SPECTATOR ) || ( pLocalPlayer->GetObserverMode() > OBS_MODE_FREEZECAM ) || ( pLocalPlayer->m_Shared.InCond( TF_COND_TEAM_GLOWS ) && tf_enable_glows_after_respawn.GetBool() ) ) ) )
	{
		bool bShowEveryone = ( bIsHLTV || 
							   ( ( nLocalPlayerTeam == TEAM_SPECTATOR ) && tf_spec_xray.GetBool() ) ||
							   ( ( nLocalPlayerTeam >= FIRST_GAME_TEAM ) && ( pLocalPlayer->GetObserverMode() > OBS_MODE_FREEZECAM ) && ( tf_spec_xray.GetInt() > 1 ) ) );

		// loop through the players
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			if ( !g_PR->IsConnected( i ) )
			{
				RemoveEntity( i );
				continue;
			}

			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer || ( pPlayer == pLocalPlayer ) )
			{
				RemoveEntity( i );
				continue;
			}

			int nPlayerTeamNumber = pPlayer->GetTeamNumber();
			
			// remove the entities we don't want to draw anymore
			if ( pPlayer->IsDormant() ||
				( nPlayerTeamNumber < FIRST_GAME_TEAM ) ||
				( !pPlayer->IsAlive() ) ||
				( pPlayer->m_Shared.IsStealthed() && ( nLocalPlayerTeam >= FIRST_GAME_TEAM ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) ) ||
				( !bShowEveryone && !pPlayer->IsPlayerClass( TF_CLASS_SPY ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) ) ||
				( !bShowEveryone && pPlayer->IsPlayerClass( TF_CLASS_SPY ) && !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) ) ||
				( !bShowEveryone && pPlayer->IsPlayerClass( TF_CLASS_SPY ) && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) && ( pPlayer->m_Shared.GetDisguiseTeam() != nLocalPlayerTeam ) ) )
			{
				if ( pPlayer->IsClientSideGlowEnabled() )
				{
					pPlayer->SetClientSideGlowEnabled( false );
				}
				RemoveEntity( i );
				continue;
			}

			// passed all of the tests, so make sure they're in the list
			int nVecIndex = -1;
			FOR_EACH_VEC( m_vecEntitiesToDraw, nTemp )
			{
				if ( m_vecEntitiesToDraw[nTemp].m_nEntIndex == i )
				{
					nVecIndex = nTemp;
					break;
				}
			}
			if ( nVecIndex == -1 )
			{
				nVecIndex = m_vecEntitiesToDraw.AddToTail();
			}

			// set the player index
			m_vecEntitiesToDraw[nVecIndex].m_nEntIndex = i;

			// don't draw their name if we're currently spectating them, but we still want them to glow
			m_vecEntitiesToDraw[nVecIndex].m_bDrawName = true;
			if ( !pLocalPlayer->IsAlive() )
			{
				CSpectatorTargetID *pSpecTargetID = (CSpectatorTargetID *)GET_HUDELEMENT( CSpectatorTargetID );
				if ( ( pLocalPlayer->GetObserverTarget() == pPlayer ) || ( pSpecTargetID && pSpecTargetID->GetTargetIndex() == i ) )
				{
					m_vecEntitiesToDraw[nVecIndex].m_bDrawName = false;

					// if we're in chase mode, just remove them entirely
					if ( pLocalPlayer->GetObserverMode() == OBS_MODE_CHASE )
					{
						if ( pPlayer->IsClientSideGlowEnabled() )
						{
							pPlayer->SetClientSideGlowEnabled( false );
						}
						RemoveEntity( i );
						continue;
					}
				}
			}

			// disguised Spy?
			C_TFPlayer *pDisguiseTarget = NULL;
			if ( !bIsHLTV && ( nLocalPlayerTeam >= FIRST_GAME_TEAM ) )
			{
				if ( pPlayer->IsPlayerClass( TF_CLASS_SPY ) && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && ( nPlayerTeamNumber != nLocalPlayerTeam ) )
				{
					pDisguiseTarget = pPlayer->m_Shared.GetDisguiseTarget();
				}
			}

			// use actual name or disguised name?
			int nNameIndex = pDisguiseTarget ? pDisguiseTarget->entindex() : i;
			g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( nNameIndex ), m_vecEntitiesToDraw[nVecIndex].m_wszName, sizeof( m_vecEntitiesToDraw[nVecIndex].m_wszName ) );
			m_vecEntitiesToDraw[nVecIndex].m_nNameWidth = UTIL_ComputeStringWidth( m_hNameFont, m_vecEntitiesToDraw[nVecIndex].m_wszName );

			m_vecEntitiesToDraw[nVecIndex].m_nOffset = ( VEC_HULL_MAX_SCALED( pPlayer ).z );

			// use actual health or disguised health?
			float flHealth = 1.0f;

			if ( pDisguiseTarget )
			{
				flHealth = (float)( pPlayer->m_Shared.GetDisguiseHealth() ) / (float)( pPlayer->m_Shared.GetDisguiseMaxHealth() );
			}
			else
			{
				flHealth = (float)( pPlayer->GetHealth() ) / (float)( pPlayer->GetMaxHealth() );
			}
			// don't show buffed health for this simple bar
			if ( flHealth > 1.0f )
			{
				flHealth = 1.0f;
			}
			m_vecEntitiesToDraw[nVecIndex].m_flHealth = flHealth;

 			// what color should we use?
			float r, g, b;
			pPlayer->GetGlowEffectColor( &r, &g, &b );
			m_vecEntitiesToDraw[nVecIndex].m_clrGlowColor = Color( r * 255, g * 255, b * 255, 255 );

			if ( !pPlayer->IsClientSideGlowEnabled() )
			{
				pPlayer->SetClientSideGlowEnabled( true );
			}
		}

		// loop through the buildings
		for ( int nCount = 0; nCount < IBaseObjectAutoList::AutoList().Count(); nCount++ )
		{
			bool bDraw = false;
			C_BaseObject *pObject = static_cast<C_BaseObject*>( IBaseObjectAutoList::AutoList()[nCount] );
			if ( !pObject->IsDormant() && !pObject->IsMapPlaced() && !pObject->IsEffectActive( EF_NODRAW ) )
			{
				if ( bShowEveryone || ( ( nLocalPlayerTeam >= FIRST_GAME_TEAM ) && ( nLocalPlayerTeam == pObject->GetTeamNumber() ) ) )
				{
					bDraw = true;
				}
			}

			if ( bDraw )
			{
				int nVecIndex = -1;
				FOR_EACH_VEC( m_vecEntitiesToDraw, nTemp )
				{
					if ( m_vecEntitiesToDraw[nTemp].m_nEntIndex == pObject->entindex() )
					{
						nVecIndex = nTemp;
						break;
					}
				}
				if ( nVecIndex == -1 )
				{
					nVecIndex = m_vecEntitiesToDraw.AddToTail();
				}

				// set the player index
				m_vecEntitiesToDraw[nVecIndex].m_nEntIndex = pObject->entindex();

				// don't draw the name if we're currently spectating this building, but we still want it to glow
				m_vecEntitiesToDraw[nVecIndex].m_bDrawName = true;
				if ( pLocalPlayer->GetObserverTarget() == pObject )
				{
					m_vecEntitiesToDraw[nVecIndex].m_bDrawName = false;
				}

				if ( pObject->GetType() == OBJ_TELEPORTER )
				{
					m_vecEntitiesToDraw[nVecIndex].m_nOffset = 30;
				}
				else if ( pObject->GetType() == OBJ_DISPENSER )
				{
					m_vecEntitiesToDraw[nVecIndex].m_nOffset = 70;
				}
				else
				{
					switch ( pObject->GetUpgradeLevel() )
					{
					case 1:
						m_vecEntitiesToDraw[nVecIndex].m_nOffset = 50;
						break;
					case 2:
						m_vecEntitiesToDraw[nVecIndex].m_nOffset = 65;
						break;
					case 3:
					default:
						m_vecEntitiesToDraw[nVecIndex].m_nOffset = 80;
						break;
					}
				}

				pObject->GetTargetIDString( m_vecEntitiesToDraw[nVecIndex].m_wszName, sizeof( m_vecEntitiesToDraw[nVecIndex].m_wszName ), true );
				m_vecEntitiesToDraw[nVecIndex].m_nNameWidth = UTIL_ComputeStringWidth( m_hNameFont, m_vecEntitiesToDraw[nVecIndex].m_wszName );

				float flHealth = 1.0f;
				flHealth = (float)( pObject->GetHealth() ) / (float)( pObject->GetMaxHealth() );
				if ( flHealth > 1.0f )
				{
					flHealth = 1.0f;
				}
				m_vecEntitiesToDraw[nVecIndex].m_flHealth = flHealth;

				// what color should we use?
				float r, g, b;
				pObject->GetGlowEffectColor( &r, &g, &b );
				m_vecEntitiesToDraw[nVecIndex].m_clrGlowColor = Color( r * 255, g * 255, b * 255, 255 );

				if ( !pObject->IsClientSideGlowEnabled() )
				{
					pObject->SetClientSideGlowEnabled( true );
				}
			}
			else
			{
				if ( pObject->IsClientSideGlowEnabled() )
				{
					pObject->SetClientSideGlowEnabled( false );
				}
				RemoveEntity( pObject->entindex() );
			}
		}
	}
	else
	{
		Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudSpectatorExtras::Paint()
{
	BaseClass::Paint();

	if ( !g_PR )
		return;

	if ( tf_spec_xray_disable.GetBool() )
		return;

	int nNameOffset = 35;
	int nHealthWidth = 70;
	int nHealthHeight = 6;

	FOR_EACH_VEC( m_vecEntitiesToDraw, i )
	{
		if ( !m_vecEntitiesToDraw[i].m_bDrawName )
			continue;

		int nEntIndex = m_vecEntitiesToDraw[i].m_nEntIndex;
		if ( IsPlayerIndex( nEntIndex ) && !g_PR->IsConnected( nEntIndex ) )
			continue;

		C_BaseEntity *pEnt = cl_entitylist->GetEnt( nEntIndex );
		if ( !pEnt )
			continue;

		Vector vecPos = pEnt->GetAbsOrigin();
		vecPos.z += m_vecEntitiesToDraw[i].m_nOffset;

		int iX, iY;
		Vector vecWorld( vecPos.x, vecPos.y, vecPos.z );
		if ( GetVectorInHudSpace( vecWorld, iX, iY ) )
		{
 			// draw the name
 			vgui::surface()->DrawSetTextFont( m_hNameFont );
			vgui::surface()->DrawSetTextPos( iX - ( m_vecEntitiesToDraw[i].m_nNameWidth / 2 ), iY - nNameOffset );
			vgui::surface()->DrawSetTextColor( m_vecEntitiesToDraw[i].m_clrGlowColor );
			vgui::surface()->DrawPrintText( m_vecEntitiesToDraw[i].m_wszName, wcslen( m_vecEntitiesToDraw[i].m_wszName ), vgui::FONT_DRAW_NONADDITIVE );

			int xHealthPos = iX - 35;
			int yHealthPos = iY - 10;

			// draw the health bar background
			vgui::surface()->DrawSetColor( Color( 127, 127, 127, 255 ) );
			vgui::surface()->DrawFilledRect( xHealthPos, yHealthPos, xHealthPos + nHealthWidth, yHealthPos + nHealthHeight );

			// draw the health bar
			vgui::surface()->DrawSetColor( m_vecEntitiesToDraw[i].m_clrGlowColor );
			vgui::surface()->DrawFilledRect( xHealthPos, yHealthPos, xHealthPos + ( nHealthWidth * m_vecEntitiesToDraw[i].m_flHealth ), yHealthPos + nHealthHeight );
		}
	}
}
