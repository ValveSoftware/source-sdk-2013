//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectSentrygun
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tf_player.h"
#include "vgui_bitmapbutton.h"
#include "vgui/ILocalize.h"
#include "tf_fx_muzzleflash.h"
#include "eventlist.h"
#include "hintsystem.h"
#include <vgui_controls/ProgressBar.h>
#include "igameevents.h"

#include "c_obj_sentrygun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static void RecvProxy_BooleanToShieldLevel( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// convert old boolean "m_bShielded" to uint32 "m_nShieldLevel"
	*(uint32*)pOut = ( pData->m_Value.m_Int != 0 ) ? 1 : 0;
}

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )

BEGIN_NETWORK_TABLE( C_TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( C_ObjectSentrygun, DT_SentrygunLocalData )
	RecvPropInt( RECVINFO(m_iKills) ),
	RecvPropInt( RECVINFO(m_iAssists) ),
END_NETWORK_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_ObjectSentrygun, DT_ObjectSentrygun, CObjectSentrygun)
	RecvPropInt( RECVINFO(m_iAmmoShells) ),
	RecvPropInt( RECVINFO(m_iAmmoRockets) ),
	RecvPropInt( RECVINFO(m_iState) ),
	RecvPropBool( RECVINFO(m_bPlayerControlled) ),
	RecvPropInt( RECVINFO(m_nShieldLevel) ),
	RecvPropInt( RECVINFO_NAME(m_nShieldLevel, m_bShielded), 0, RecvProxy_BooleanToShieldLevel ), // for demo compatibility only
	RecvPropEHandle( RECVINFO( m_hEnemy ) ),
	RecvPropEHandle( RECVINFO( m_hAutoAimTarget ) ),
	RecvPropDataTable( "SentrygunLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_SentrygunLocalData ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ObjectSentrygun::C_ObjectSentrygun()
{
	m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
	m_bPlayerControlled = false;
	m_bOldPlayerControlled = false;
	m_nShieldLevel = SHIELD_NONE;
	m_nOldShieldLevel = SHIELD_NONE;
	m_hLaserBeamEffect = NULL;
	m_pTempShield = NULL;
	m_bNearMiss = false;
	m_flNextNearMissCheck = 0.f;

	m_iOldModelIndex = 0;
	m_bOldCarried = false;
	m_bRecreateShield = false;
	m_bRecreateLaserBeam = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::UpdateOnRemove( void )
{
	DestroyLaserBeam();
	DestroyShield();
	DestroySiren();

	BaseClass::UpdateOnRemove();
}


void C_ObjectSentrygun::GetAmmoCount( int &iShells, int &iMaxShells, int &iRockets, int & iMaxRockets )
{
	iShells = m_iAmmoShells;
	iMaxShells = m_iMaxAmmoShells;
	iRockets = m_iAmmoRockets;
	iMaxRockets = SENTRYGUN_MAX_ROCKETS;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::UpgradeLevelChanged()
{
	switch( m_iUpgradeLevel )
	{
	case 1:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_1, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
			break;
		}
	case 2:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_2, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_2;
			break;
		}
	case 3:	
		{ 
			VectorCopy( SENTRYGUN_EYE_OFFSET_LEVEL_3, m_vecViewOffset );
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_3;
			break;
		}
	default: 
		{ 
			Assert( 0 ); 
			break;
		}
	}

	CreateLaserBeam();

	// Because the bounding box size changes when upgrading, force the shadow to be reprojected using the new bounds
	g_pClientShadowMgr->AddToDirtyShadowList( this, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldBodygroups = GetBody();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	// intercept bodygroup sets from the server
	// we aren't clientsideanimating, but we don't want the server setting our
	// bodygroup while we are placing
	if ( m_iOldBodygroups != GetBody() )
	{
		if ( IsPlacing() )
		{
			m_nBody = m_iOldBodygroups;
		}
	}

	if ( GetModelIndex() != m_iOldModelIndex )
	{
		m_iOldModelIndex = GetModelIndex();

		if ( IsMiniBuilding() )
		{
			CStudioHdr *pStudiohdr = GetModelPtr();
			int bodyGroup = FindBodygroupByName( "mini_sentry_light" );
			if ( bodyGroup < pStudiohdr->numbodyparts() )
			{
				mstudiobodyparts_t *pbodypart = pStudiohdr->pBodypart( bodyGroup );
				if ( pbodypart->base > 0 )
				{
					SetBodygroup( bodyGroup, 1 );
				}
			}
		}
	}

	if ( m_bPlayerControlled != m_bOldPlayerControlled || m_bRecreateLaserBeam )
	{
		if ( m_bPlayerControlled )
		{
			CreateLaserBeam();
		}
		else
		{
			DestroyLaserBeam();
		}
		m_bOldPlayerControlled = m_bPlayerControlled;
		m_bRecreateLaserBeam = false;
	}

	if ( m_nShieldLevel != m_nOldShieldLevel || m_bRecreateShield )
	{
		if ( m_nShieldLevel > 0 )
		{
			CreateShield();
		}
		else
		{
			DestroyShield();
		}
		m_nOldShieldLevel = m_nShieldLevel;
		m_bRecreateShield = false;
	}

	if ( IsCarried() != m_bOldCarried )
	{
		m_bOldCarried = IsCarried();
		if ( IsCarried() )
		{
			DestroySiren();
		}
	}

	if ( ShouldBeActive() && !IsDisabled() && IsMiniBuilding() && !m_hSirenEffect )
	{
		CreateSiren();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnGoActive( void )
{
	CreateSiren();

	BaseClass::OnGoActive();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnGoInactive( void )
{
	DestroySiren();

	BaseClass::OnGoInactive();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnStartDisabled( void )
{
	DestroySiren();

	BaseClass::OnStartDisabled();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnEndDisabled( void )
{
	CreateSiren();

	BaseClass::OnEndDisabled();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::CreateLaserBeam( void )
{
	if ( !m_bPlayerControlled )
		return;

	DestroyLaserBeam();

	int iAttachment = LookupAttachment( "laser_origin" );
	m_hLaserBeamEffect = ParticleProp()->Create( "laser_sight_beam", PATTACH_POINT_FOLLOW, iAttachment );
	if ( m_hLaserBeamEffect )
	{
		m_hLaserBeamEffect->SetSortOrigin( m_hLaserBeamEffect->GetRenderOrigin() );
	}

	SetNextClientThink( CLIENT_THINK_ALWAYS );

	if ( m_hLaserBeamEffect )
	{
		if ( GetTeamNumber() == TF_TEAM_BLUE )
		{
			m_hLaserBeamEffect->SetControlPoint( 2, Vector( 0, 0, 255 ) );
		}
		else
		{
			m_hLaserBeamEffect->SetControlPoint( 2, Vector( 255, 0, 0 ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::DestroyLaserBeam( void )
{
	if ( m_hLaserBeamEffect )
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
		ParticleProp()->StopEmissionAndDestroyImmediately( m_hLaserBeamEffect );
		m_hLaserBeamEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::SetDormant( bool bDormant )
{
	if ( IsDormant() && !bDormant )
	{
		// Make sure our shield is where we are. We may have moved since last seen.
		if ( m_pTempShield )
		{
			m_bRecreateShield = true;
			m_bRecreateLaserBeam = true;
		}
	}

	BaseClass::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::CreateShield( void )
{
	DestroyShield();

	model_t *pModel = (model_t *) engine->LoadModel( "models/buildables/sentry_shield.mdl" );
	m_pTempShield = tempents->SpawnTempModel( pModel, GetAbsOrigin(), GetAbsAngles(), Vector(0, 0, 0), 1, FTENT_NEVERDIE );
	if ( m_pTempShield )
	{
		m_pTempShield->ChangeTeam( GetTeamNumber() );
		m_pTempShield->m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
		//m_pTempShield->m_nRenderFX = kRenderFxDistort;
	}

	m_hShieldEffect = ParticleProp()->Create( "turret_shield", PATTACH_ABSORIGIN_FOLLOW, 0, Vector( 0,0,30) );
	if ( !m_hShieldEffect )
		return;
	if ( GetTeamNumber() == TF_TEAM_BLUE )
	{
		m_hShieldEffect->SetControlPoint( 1, Vector(50,150,255) );
	}
	else
	{
		m_hShieldEffect->SetControlPoint( 1, Vector(255,50,50) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::DestroyShield( void )
{
	if ( m_pTempShield )
	{
		m_pTempShield->flags = FTENT_FADEOUT;
		m_pTempShield->die = gpGlobals->curtime;
		m_pTempShield->fadeSpeed = 1.0f;
		m_pTempShield = NULL;
	}

	if ( m_hShieldEffect )
	{
		ParticleProp()->StopEmission( m_hShieldEffect );
		m_hShieldEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::CreateSiren( void )
{
	if ( !IsMiniBuilding() )
		return;

	if ( IsCarried() )
		return;

	if ( m_hSirenEffect )
		return;

	const char* flashlightName = "cart_flashinglight";
	if ( GetTeamNumber() == TF_TEAM_RED )
	{
		flashlightName = "cart_flashinglight_red";
	}
	m_hSirenEffect = ParticleProp()->Create( flashlightName, PATTACH_POINT_FOLLOW, "siren" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::DestroySiren( void )
{
	if ( m_hSirenEffect )
	{
		ParticleProp()->StopEmission( m_hSirenEffect );
		m_hSirenEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::ClientThink( void )
{
	if ( m_hLaserBeamEffect && m_hEnemy && GetBuilder() )
	{
		QAngle vecAngles;
		Vector vecMuzzleOrigin;
		int iAttachment = 0;
		switch ( GetUpgradeLevel() )
		{
		case 1:
			iAttachment = LookupAttachment( "muzzle" );
			break;
		case 2:
			iAttachment = LookupAttachment( "muzzle_l" );
			break;
		case 3:
			iAttachment = LookupAttachment( "rocket_l" );
			break;
		}
		GetAttachment( iAttachment, vecMuzzleOrigin, vecAngles );

		Vector vForward;
		AngleVectors( vecAngles, &vForward );

		Vector vEnd = m_hEnemy->WorldSpaceCenter();
		if ( m_hAutoAimTarget )
		{
			vEnd = m_hAutoAimTarget->GetAbsOrigin() + m_hAutoAimTarget->GetClassEyeHeight()*0.75f;
		}

		trace_t	trace;
		CTraceFilterIgnoreTeammatesAndTeamObjects filter( GetBuilder(), COLLISION_GROUP_NONE, GetBuilder()->GetTeamNumber() );
		UTIL_TraceLine( vecMuzzleOrigin, vEnd, MASK_SOLID, &filter, &trace );

		Vector vecInterpBeamPos;
		InterpolateVector( gpGlobals->frametime * 25.f, m_vecLaserBeamPos, trace.endpos, vecInterpBeamPos );

		m_hLaserBeamEffect->SetControlPoint( 1, vecInterpBeamPos );
		m_vecLaserBeamPos = vecInterpBeamPos;

		// Perform a near-miss check.
		// This works pretty well as a threat indicator for the arrow, let's try it for our laser.
		if ( gpGlobals->curtime > m_flNextNearMissCheck )
		{
//			CheckNearMiss( vecMuzzleOrigin, m_hEnemy->GetAbsOrigin() );
			m_flNextNearMissCheck = gpGlobals->curtime + 0.2f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::CheckNearMiss( Vector vecStart, Vector vecEnd )
{
	// Check against the local player. If the laser sweeps near him, play the near miss sound...
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
		return;

	// Can't hear near miss sounds from friendly guns.
//	if ( pLocalPlayer->GetTeamNumber() == GetTeamNumber() )
//		return;

	Vector vecPlayerPos = pLocalPlayer->GetAbsOrigin();
	Vector vecClosestPoint;
	float dist;
	CalcClosestPointOnLineSegment( vecPlayerPos, vecStart, vecEnd, vecClosestPoint, &dist );
	dist = vecPlayerPos.DistTo( vecClosestPoint );
	if ( dist > 120 )
	{
		StopSound( "Building_Sentrygun.ShaftLaserPass" );
		return;
	}

	if ( !m_bNearMiss )
	{
		// We're good for a near miss!
		float soundlen = 0;
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pSoundName = "Building_Sentrygun.ShaftLaserPass";
		params.m_pflSoundDuration = &soundlen;
		params.m_flVolume = 1.f - (dist / 120.f);
		CSingleUserRecipientFilter localFilter( pLocalPlayer );
		EmitSound( localFilter, pLocalPlayer->entindex(), params );

		m_bNearMiss = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::DisplayHintTo( C_BasePlayer *pPlayer )
{
	bool bHintPlayed = false;

	C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if ( InSameTeam( pPlayer ) )
	{
		// We're looking at a friendly object. 
		if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// If the sentrygun can be upgraded, and I can afford it, let me know
			if ( GetHealth() == GetMaxHealth() && GetUpgradeLevel() < 3 )
			{
				if ( pTFPlayer->GetBuildResources() >= SENTRYGUN_UPGRADE_COST )
				{
					bHintPlayed = pTFPlayer->HintMessage( HINT_ENGINEER_UPGRADE_SENTRYGUN, false, true );
				}
				else
				{
					bHintPlayed = pTFPlayer->HintMessage( HINT_ENGINEER_METAL_TO_UPGRADE, false, true );
				}
			}
		}
	}

	if ( !bHintPlayed )
	{
		BaseClass::DisplayHintTo( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *C_ObjectSentrygun::GetHudStatusIcon( void )
{
	const char *pszResult;

	switch( m_iUpgradeLevel )
	{
	case 1:
	default:
		pszResult = "obj_status_sentrygun_1";
		break;
	case 2:
		pszResult = "obj_status_sentrygun_2";
		break;
	case 3:
		pszResult = "obj_status_sentrygun_3";
		break;
	}

	return pszResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
BuildingHudAlert_t C_ObjectSentrygun::GetBuildingAlertLevel( void )
{
	BuildingHudAlert_t baseAlertLevel = BaseClass::GetBuildingAlertLevel();

	// Just warn on low shells.

	float flShellPercent = (float)m_iAmmoShells / (float)m_iMaxAmmoShells;

	BuildingHudAlert_t alertLevel = BUILDING_HUD_ALERT_NONE;

	if ( !IsCarried() )
	{
		if ( !IsBuilding() && flShellPercent < 0.25 )
		{
			alertLevel = BUILDING_HUD_ALERT_VERY_LOW_AMMO;
		}
		else if ( !IsBuilding() && flShellPercent < 0.50 )
		{
			alertLevel = BUILDING_HUD_ALERT_LOW_AMMO;
		}
	}

	return MAX( baseAlertLevel, alertLevel );
}

//-----------------------------------------------------------------------------
// Purpose: During placement, only use the smaller bbox for shadow calc, don't include the range bodygroup
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	if ( IsPlacing() )
	{
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();

		// HACK: The collision prop bounding box doesn't quite cover the blueprint model, so we bloat it a little
		Vector bbBloat( 10.0f, 10.0f, 0.0f );
		mins -= bbBloat;
		maxs += bbBloat;
	}
	else
	{
		BaseClass::GetShadowRenderBounds( mins, maxs, shadowType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Re-calc our damage particles when we get a new model
//-----------------------------------------------------------------------------
CStudioHdr *C_ObjectSentrygun::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	UpdateDamageEffects( m_damageLevel );

	// Reset Bodygroups
	for ( int i = GetNumBodyGroups()-1; i >= 0; i-- )
	{
		SetBodygroup( i, 0 );
	}

	m_iPlacementBodygroup = FindBodygroupByName( "sentry1_range" );
	m_iPlacementBodygroup_Mini = FindBodygroupByName( "sentry1_range_mini" );

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
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
		pszEffect = "sentrydamage_1";
		break;
	case BUILDING_DAMAGE_LEVEL_MEDIUM:
		pszEffect = "sentrydamage_2";
		break;
	case BUILDING_DAMAGE_LEVEL_HEAVY:
		pszEffect = "sentrydamage_3";
		break;
	case BUILDING_DAMAGE_LEVEL_CRITICAL:
		pszEffect = "sentrydamage_4";
		break;

	default:
		break;
	}

	if ( Q_strlen(pszEffect) > 0 )
	{
		switch( m_iUpgradeLevel )
		{
		case 1:
		case 2:
			m_hDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "build_point_0" );
			break;

		case 3:
			m_hDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_POINT_FOLLOW, "sentrydamage" );
			break;
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: placement state has changed, update the model
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::OnPlacementStateChanged( bool bValidPlacement )
{
	if ( bValidPlacement && ( m_iPlacementBodygroup >= 0 ) && ( m_iPlacementBodygroup_Mini >= 0 ) )
	{
		if ( IsMiniBuilding() )
		{
			SetBodygroup( m_iPlacementBodygroup, 0 );
			SetBodygroup( m_iPlacementBodygroup_Mini, 1 );
		}
		else
		{
			SetBodygroup( m_iPlacementBodygroup, 1 );
			SetBodygroup( m_iPlacementBodygroup_Mini, 0 );
		}
	}
	else
	{
		SetBodygroup( m_iPlacementBodygroup, 0 );
		SetBodygroup( m_iPlacementBodygroup_Mini, 0 );
	}

	BaseClass::OnPlacementStateChanged( bValidPlacement );
}

void C_ObjectSentrygun::DebugDamageParticles( void )
{
	Msg( "Health %d\n", GetHealth() );

	BuildingDamageLevel_t damageLevel = CalculateDamageLevel();
	Msg( "Damage Level %d\n", (int)damageLevel );

	if ( m_hDamageEffects )
	{
		Msg( "m_hDamageEffects is valid\n" );
	}
	else
	{
		Msg( "m_hDamageEffects is NULL\n" );
	}

	// print all particles owned by particleprop
	ParticleProp()->DebugPrintEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ObjectSentrygun::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );

	if ( !IsMiniBuilding() )
		return;

	if ( IsBuilding() || IsPlacing() )
		return;

	
	//Vector position;
	//for ( int i=0; i<8; ++i )
	//{
	//	matrix3x4_t &transform = GetBoneForWrite( i );
	//	MatrixGetColumn( transform, 3, position );
	//	MatrixSetColumn( Vector(0,0,-4) + position, 3, transform );
	//}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* C_ObjectSentrygun::GetStatusName() const
{
	if ( IsDisposableBuilding() )
	{
		return "#TF_Object_Sentry_Disp";
	}
	
	return "#TF_Object_Sentry";
}


