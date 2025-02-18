//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectTeleporter
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "vgui/ILocalize.h"
#include "c_obj_teleporter.h"
#include "soundenvelope.h"
#include "vgui/ILocalize.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define TELEPORTER_MINS			Vector( -24, -24, 0)
#define TELEPORTER_MAXS			Vector( 24, 24, 12)	

//-----------------------------------------------------------------------------
// Purpose: Teleporter object
//-----------------------------------------------------------------------------

IMPLEMENT_CLIENTCLASS_DT(C_ObjectTeleporter, DT_ObjectTeleporter, CObjectTeleporter)
	RecvPropInt( RECVINFO(m_iState) ),
	RecvPropTime( RECVINFO(m_flRechargeTime) ),
	RecvPropTime( RECVINFO(m_flCurrentRechargeDuration) ),
	RecvPropInt( RECVINFO(m_iTimesUsed) ),
	RecvPropFloat( RECVINFO(m_flYawToExit) ),
	RecvPropBool( RECVINFO(m_bMatchBuilding) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ObjectTeleporter::C_ObjectTeleporter()
{
	m_hChargedEffect = NULL;
	m_hDirectionEffect = NULL;
	m_hChargedLeftArmEffect = NULL;
	m_hChargedRightArmEffect = NULL;

	m_iDirectionArrowPoseParam = 0;

	m_pSpinSound = NULL;

	m_bMatchBuilding = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::UpdateOnRemove( void )
{
	StopActiveEffects();
	StopChargedEffects();

	if ( m_pSpinSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSpinSound );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldState = m_iState;
	m_bOldMatchBuilding = m_bMatchBuilding;
}

void C_ObjectTeleporter::StartBuildingEffects()
{
	StopBuildingEffects();
	char szEffect[128];

	// arm glow effects
	Q_snprintf( szEffect, sizeof(szEffect), "teleporter_arms_circle_%s_blink", ( GetTeamNumber() == TF_TEAM_RED ) ? "red" : "blue" );

	Assert( m_hBuildingLeftArmEffect.m_pObject == NULL );
	m_hBuildingLeftArmEffect = ParticleProp()->Create( szEffect, PATTACH_POINT_FOLLOW, 1 );

	Assert( m_hBuildingRightArmEffect.m_pObject == NULL );
	m_hBuildingRightArmEffect = ParticleProp()->Create( szEffect, PATTACH_POINT_FOLLOW, 3 );
}

void C_ObjectTeleporter::StartChargedEffects()
{
	StopChargedEffects();
	char szEffect[128];

	Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_charged_level%d", 
		( GetTeamNumber() == TF_TEAM_RED ) ? "red" : "blue", GetUpgradeLevel() );

	Assert( m_hChargedEffect.m_pObject == NULL );
	m_hChargedEffect = ParticleProp()->Create( szEffect, PATTACH_ABSORIGIN );
}

void C_ObjectTeleporter::StartActiveEffects()
{
	StopActiveEffects();
	char szEffect[128];

	Q_snprintf( szEffect, sizeof(szEffect), "teleporter_%s_%s_level%d", 
		( GetTeamNumber() == TF_TEAM_RED ) ? "red" : "blue",
		GetObjectMode() == MODE_TELEPORTER_ENTRANCE ? "entrance" : "exit",
		GetUpgradeLevel() );

	Assert( m_hDirectionEffect.m_pObject == NULL );
	m_hDirectionEffect = ParticleProp()->Create( szEffect, PATTACH_ABSORIGIN );

	// arm glow effects
	Q_snprintf( szEffect, sizeof(szEffect), "teleporter_arms_circle_%s",
		( GetTeamNumber() == TF_TEAM_RED ) ? "red" : "blue" );

	Assert( m_hChargedLeftArmEffect.m_pObject == NULL );
	m_hChargedLeftArmEffect = ParticleProp()->Create( szEffect, PATTACH_POINT_FOLLOW, 1 );

	Assert( m_hChargedRightArmEffect.m_pObject == NULL );
	m_hChargedRightArmEffect = ParticleProp()->Create( szEffect, PATTACH_POINT_FOLLOW, 3 );

	// always reinitializes sound since this only gets called when the sound needs to start or change
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pSpinSound )
	{
		controller.SoundDestroy( m_pSpinSound );
		m_pSpinSound = NULL;
	}
	char szSound[128];
	Q_snprintf( szSound, sizeof(szSound), "Building_Teleporter.SpinLevel%d", GetUpgradeLevel());

	CLocalPlayerFilter filter;
	m_pSpinSound = controller.SoundCreate( filter, entindex(), szSound );
	controller.Play( m_pSpinSound, 1.0, 100 );
}

void C_ObjectTeleporter::StopBuildingEffects()
{
	if ( m_hBuildingLeftArmEffect )
	{
		ParticleProp()->StopEmission( m_hBuildingLeftArmEffect );
		m_hBuildingLeftArmEffect = NULL;
	}

	if ( m_hBuildingRightArmEffect )
	{
		ParticleProp()->StopEmission( m_hBuildingRightArmEffect );
		m_hBuildingRightArmEffect = NULL;
	}
}

void C_ObjectTeleporter::StopChargedEffects()
{
	if ( m_hChargedEffect )
	{
		ParticleProp()->StopEmission( m_hChargedEffect );
		m_hChargedEffect = NULL;
	}
}

void C_ObjectTeleporter::StopActiveEffects()
{
	if ( m_hDirectionEffect )
	{
		ParticleProp()->StopEmission( m_hDirectionEffect );
		m_hDirectionEffect = NULL;
	}

	if ( m_hChargedLeftArmEffect )
	{
		ParticleProp()->StopEmission( m_hChargedLeftArmEffect );
		m_hChargedLeftArmEffect = NULL;
	}

	if ( m_hChargedRightArmEffect )
	{
		ParticleProp()->StopEmission( m_hChargedRightArmEffect );
		m_hChargedRightArmEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::SetInvisibilityLevel( float flValue )
{
	if ( IsEnteringOrExitingFullyInvisible( flValue ) )
	{
		UpdateTeleporterEffects();
	}

	BaseClass::SetInvisibilityLevel( flValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::UpdateTeleporterEffects( void )
{

	if ( m_bMatchBuilding )
	{
		StartBuildingEffects();
	}
	else
	{
		StopBuildingEffects();
	}

	// In MVM, teleporter from invaders act as spawn point. Always play active effect
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( m_iState != TELEPORTER_STATE_BUILDING && GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			StartChargedEffects();
			StartActiveEffects();
			return;
		}
	}

	if ( m_iState == TELEPORTER_STATE_READY )
	{
		StartChargedEffects();
	}
	else
	{
		StopChargedEffects();
	}

	if ( m_iState > TELEPORTER_STATE_IDLE && m_iOldState <= TELEPORTER_STATE_IDLE )
	{
		StartActiveEffects();
	}
	else if ( ( m_iState <= TELEPORTER_STATE_IDLE || m_iState == TELEPORTER_STATE_UPGRADING ) && m_iOldState > TELEPORTER_STATE_IDLE )
	{
		StopActiveEffects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bOldMatchBuilding != m_bMatchBuilding )
	{
		m_bOldMatchBuilding = m_bMatchBuilding;
		UpdateTeleporterEffects();
	}

	if ( m_iOldState != m_iState )
	{
		UpdateTeleporterEffects();
		m_iOldState = m_iState;
	}

	// update the pitch based on our playback rate
	if ( m_pSpinSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		controller.SoundChangePitch( m_pSpinSound, GetPlaybackRate() * 100.0f, 0.1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float C_ObjectTeleporter::GetChargeTime( void )
{
	float flTime = m_flRechargeTime - gpGlobals->curtime;

	if ( flTime < 0 )
		return 0;

	return flTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_ObjectTeleporter::GetTimesUsed( void )
{
	return m_iTimesUsed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr *C_ObjectTeleporter::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iDirectionArrowPoseParam = LookupPoseParameter( "direction" );

	SetNextClientThink( CLIENT_THINK_ALWAYS );

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Update the direction arrow
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::ClientThink( void )
{
	if ( m_iState >= TELEPORTER_STATE_READY )
	{
		SetPoseParameter( m_iDirectionArrowPoseParam, m_flYawToExit);
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::GetTargetIDDataString( OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sDataString, int iMaxLenInBytes )
{
	Assert( iMaxLenInBytes >= sizeof(sDataString[0]) );
	wchar_t wzBaseString[MAX_ID_STRING];
	BaseClass::GetTargetIDDataString( wzBaseString, sizeof( wzBaseString ) );

	sDataString[0] = '\0';
	if ( m_iState == TELEPORTER_STATE_RECHARGING && gpGlobals->curtime < m_flRechargeTime )
	{
		float flPercent = clamp( ( m_flRechargeTime - gpGlobals->curtime ) / m_flCurrentRechargeDuration, 0.0f, 1.0f );

		wchar_t wszRecharging[ 32 ];
		_snwprintf( wszRecharging, ARRAYSIZE(wszRecharging) - 1, L"%.0f", 100 - (flPercent * 100) );
		wszRecharging[ ARRAYSIZE(wszRecharging)-1 ] = '\0';

		const char *printFormatString = "#TF_playerid_object_recharging";

		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find(printFormatString),
			1,
			wszRecharging );
	}	
	else if ( m_iState == TELEPORTER_STATE_IDLE )
	{
		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find("#TF_playerid_teleporter_nomatch" ), 0 );
	}

	// Concatenate the base level string
	V_wcsncat( sDataString, L"   ", iMaxLenInBytes / sizeof( wchar_t ) );
	V_wcsncat( sDataString, wzBaseString, iMaxLenInBytes / sizeof( wchar_t ) );
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
{
	if ( m_hDamageEffects )
	{
		m_hDamageEffects->StopEmission( false, false );
		m_hDamageEffects = NULL;
	}

	const char *pszEffect = "";

	switch( damageLevel )
	{
	case BUILDING_DAMAGE_LEVEL_LIGHT:
		pszEffect = "tpdamage_1";
		break;
	case BUILDING_DAMAGE_LEVEL_MEDIUM:
		pszEffect = "tpdamage_2";
		break;
	case BUILDING_DAMAGE_LEVEL_HEAVY:
		pszEffect = "tpdamage_3";
		break;
	case BUILDING_DAMAGE_LEVEL_CRITICAL:
		pszEffect = "tpdamage_4";
		break;

	default:
		break;
	}

	if ( Q_strlen(pszEffect) > 0 )
	{
		m_hDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool C_ObjectTeleporter::IsPlacementPosValid( void )
{
	bool bResult = BaseClass::IsPlacementPosValid();

	if ( !bResult )
	{
		return false;
	}

	// m_vecBuildOrigin is the proposed build origin

	// start above the teleporter position
	Vector vecTestPos = m_vecBuildOrigin;
	vecTestPos.z += TELEPORTER_MAXS.z;

	// make sure we can fit a player on top in this pos
	trace_t tr;
	UTIL_TraceHull( vecTestPos, vecTestPos, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID | CONTENTS_PLAYERCLIP, this, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	return ( tr.fraction >= 1.0 );
}
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::UpgradeLevelChanged( void )
{
	StopActiveEffects();
	StopChargedEffects();

	if ( m_iState >= TELEPORTER_STATE_READY && m_iState != TELEPORTER_STATE_UPGRADING )
	{
		StartActiveEffects();
		if ( m_iState != TELEPORTER_STATE_RECHARGING )
		{
			StartChargedEffects();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectTeleporter::OnGoInactive( void )
{
	StopActiveEffects();
	StopBuildingEffects();
	StopChargedEffects();

	BaseClass::OnGoInactive();
}
