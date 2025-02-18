// C_NextBot.cpp
// Client-side implementation of Next generation bot system
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "C_NextBot.h"
#include "debugoverlay_shared.h"
#include <bitbuf.h>
#include "viewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot

ConVar NextBotShadowDist( "nb_shadow_dist", "400" );

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_NextBotCombatCharacter, DT_NextBot, NextBotCombatCharacter )
END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_NextBotCombatCharacter::C_NextBotCombatCharacter()
{
	// Left4Dead have surfaces too steep for IK to work properly
	m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

	m_shadowType = SHADOWS_SIMPLE;
	m_forcedShadowType = SHADOWS_NONE;
	m_bForceShadowType = false;

	TheClientNextBots().Register( this );
	UseClientSideAnimation();
}


//-----------------------------------------------------------------------------
C_NextBotCombatCharacter::~C_NextBotCombatCharacter()
{
	TheClientNextBots().UnRegister( this );
}


//-----------------------------------------------------------------------------
void C_NextBotCombatCharacter::Spawn( void )
{
	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
void C_NextBotCombatCharacter::UpdateClientSideAnimation()
{
	if (IsDormant())
	{
		return;
	}

	BaseClass::UpdateClientSideAnimation();
}


//--------------------------------------------------------------------------------------------------------
void C_NextBotCombatCharacter::UpdateShadowLOD( void )
{
	ShadowType_t oldShadowType = m_shadowType;

	if ( m_bForceShadowType )
	{
		m_shadowType = m_forcedShadowType;
	}
	else
	{
#ifdef NEED_SPLITSCREEN_INTEGRATION
		FOR_EACH_VALID_SPLITSCREEN_PLAYER( hh )
		{
			C_BasePlayer *pl = C_BasePlayer::GetLocalPlayer(hh);
			if ( pl )
			{
				Vector delta = GetAbsOrigin() - C_BasePlayer::GetLocalPlayer(hh)->GetAbsOrigin();
#else
		{
			if ( C_BasePlayer::GetLocalPlayer() )
			{
				Vector delta = GetAbsOrigin() - C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
#endif
				if ( delta.IsLengthLessThan( NextBotShadowDist.GetFloat() ) )
				{
					m_shadowType = SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
				}
				else
				{
					m_shadowType = SHADOWS_SIMPLE;
				}
			}
			else
			{
				m_shadowType = SHADOWS_SIMPLE;
			}
		}
	}

	if ( oldShadowType != m_shadowType )
	{
		DestroyShadow();
	}
}


//--------------------------------------------------------------------------------------------------------
ShadowType_t C_NextBotCombatCharacter::ShadowCastType( void ) 
{
	if ( !IsVisible() )
		return SHADOWS_NONE;

	if ( m_shadowTimer.IsElapsed() )
	{
		m_shadowTimer.Start( 0.15f );
		UpdateShadowLOD();
	}

	return m_shadowType;
}


//--------------------------------------------------------------------------------------------------------
bool C_NextBotCombatCharacter::GetForcedShadowCastType( ShadowType_t* pForcedShadowType ) const
{
	if ( pForcedShadowType )
	{
		*pForcedShadowType = m_forcedShadowType;
	}
	return m_bForceShadowType;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Singleton accessor.
 * By returning a reference, we guarantee construction of the 
 * instance before its first use.
 */
C_NextBotManager &TheClientNextBots( void )
{
	static C_NextBotManager manager;
	return manager;
}


//--------------------------------------------------------------------------------------------------------
C_NextBotManager::C_NextBotManager( void )
{
}


//--------------------------------------------------------------------------------------------------------
C_NextBotManager::~C_NextBotManager()
{
}


//--------------------------------------------------------------------------------------------------------
void C_NextBotManager::Register( C_NextBotCombatCharacter *bot )
{
	m_botList.AddToTail( bot );
}


//--------------------------------------------------------------------------------------------------------
void C_NextBotManager::UnRegister( C_NextBotCombatCharacter *bot )
{
	m_botList.FindAndRemove( bot );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool C_NextBotManager::SetupInFrustumData( void )
{
#ifdef ENABLE_AFTER_INTEGRATION
	// Done already this frame.
	if ( IsInFrustumDataValid() )
		return true;

	// Can we use the view data yet?
	if ( !FrustumCache()->IsValid() )
		return false;

	// Get the number of active bots.
	int nBotCount = m_botList.Count();

	// Reset.
	for ( int iBot = 0; iBot < nBotCount; ++iBot )
	{
		// Get the current bot.
		C_NextBotCombatCharacter *pBot = m_botList[iBot];
		if ( !pBot )
			continue;

		pBot->InitFrustumData();
	}

	FOR_EACH_VALID_SPLITSCREEN_PLAYER( iSlot )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( iSlot );
		// Get the active local player.
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			continue;

		for ( int iBot = 0; iBot < nBotCount; ++iBot )
		{
			// Get the current bot.
			C_NextBotCombatCharacter *pBot = m_botList[iBot];
			if ( !pBot )
				continue;

			// Are we in the view frustum?
			Vector vecMin, vecMax;
			pBot->CollisionProp()->WorldSpaceAABB( &vecMin, &vecMax );
			bool bInFrustum = !FrustumCache()->m_Frustums[iSlot].CullBox( vecMin, vecMax );
		
			if ( bInFrustum )
			{
				Vector vecSegment;
				VectorSubtract( pBot->GetAbsOrigin(), pPlayer->GetAbsOrigin(), vecSegment );
				float flDistance = vecSegment.LengthSqr();
				if ( flDistance < pBot->GetInFrustumDistanceSqr() )
				{
					pBot->SetInFrustumDistanceSqr( flDistance );
				}	

				pBot->SetInFrustum( true );
			}
		}
	}

	// Mark as setup this frame.
	m_nInFrustumFrame = gpGlobals->framecount;
#endif

	return true;
}

//--------------------------------------------------------------------------------------------------------
