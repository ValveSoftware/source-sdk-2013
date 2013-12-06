//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#include "prediction.h"
#define CRecipientFilter C_RecipientFilter
#else
#include "hl2mp_player.h"
#endif

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_footsteps;

const char *g_ppszPlayerSoundPrefixNames[PLAYER_SOUNDS_MAX] =
{
	"NPC_Citizen",
	"NPC_CombineS",
	"NPC_MetroPolice",
};

const char *CHL2MP_Player::GetPlayerModelSoundPrefix( void )
{
	return g_ppszPlayerSoundPrefixNames[m_iPlayerSoundType];
}

void CHL2MP_Player::PrecacheFootStepSounds( void )
{
	int iFootstepSounds = ARRAYSIZE( g_ppszPlayerSoundPrefixNames );
	int i;

	for ( i = 0; i < iFootstepSounds; ++i )
	{
		char szFootStepName[128];

		Q_snprintf( szFootStepName, sizeof( szFootStepName ), "%s.RunFootstepLeft", g_ppszPlayerSoundPrefixNames[i] );
		PrecacheScriptSound( szFootStepName );

		Q_snprintf( szFootStepName, sizeof( szFootStepName ), "%s.RunFootstepRight", g_ppszPlayerSoundPrefixNames[i] );
		PrecacheScriptSound( szFootStepName );
	}
}

//-----------------------------------------------------------------------------
// Consider the weapon's built-in accuracy, this character's proficiency with
// the weapon, and the status of the target. Use this information to determine
// how accurately to shoot at the target.
//-----------------------------------------------------------------------------
Vector CHL2MP_Player::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	if ( pWeapon )
		return pWeapon->GetBulletSpread( WEAPON_PROFICIENCY_PERFECT );
	
	return VECTOR_CONE_15DEGREES;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : step - 
//			fvol - 
//			force - force sound to play
//-----------------------------------------------------------------------------
void CHL2MP_Player::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
	if ( gpGlobals->maxClients > 1 && !sv_footsteps.GetFloat() )
		return;

#if defined( CLIENT_DLL )
	// during prediction play footstep sounds only once
	if ( !prediction->IsFirstTimePredicted() )
		return;
#endif

	if ( GetFlags() & FL_DUCKING )
		return;

	m_Local.m_nStepside = !m_Local.m_nStepside;

	char szStepSound[128];

	if ( m_Local.m_nStepside )
	{
		Q_snprintf( szStepSound, sizeof( szStepSound ), "%s.RunFootstepLeft", g_ppszPlayerSoundPrefixNames[m_iPlayerSoundType] );
	}
	else
	{
		Q_snprintf( szStepSound, sizeof( szStepSound ), "%s.RunFootstepRight", g_ppszPlayerSoundPrefixNames[m_iPlayerSoundType] );
	}

	CSoundParameters params;
	if ( GetParametersForSound( szStepSound, params, NULL ) == false )
		return;

	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

#ifndef CLIENT_DLL
	// im MP, server removed all players in origins PVS, these players 
	// generate the footsteps clientside
	if ( gpGlobals->maxClients > 1 )
		filter.RemoveRecipientsByPVS( vecOrigin );
#endif

	EmitSound_t ep;
	ep.m_nChannel = CHAN_BODY;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = fvol;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );
}
