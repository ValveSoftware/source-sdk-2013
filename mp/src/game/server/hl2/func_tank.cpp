//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_tank.h"
#include "Sprite.h"
#include "EnvLaser.h"
#include "basecombatweapon.h"
#include "explode.h"
#include "eventqueue.h"
#include "gamerules.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "grenade_beam.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "physics_cannister.h"
#include "decals.h"
#include "shake.h"
#include "particle_smokegrenade.h"
#include "player.h"
#include "entitylist.h"
#include "IEffects.h"
#include "ai_basenpc.h"
#include "ai_behavior_functank.h"
#include "weapon_rpg.h"
#include "effects.h"
#include "iservervehicle.h"
#include "soundenvelope.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "props.h"
#include "rumble_shared.h"
#include "particle_parse.h"
// NVNT turret recoil
#include "haptics/haptic_utils.h"

#ifdef HL2_DLL
#include "hl2_player.h"
#endif //HL2_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern Vector PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint);

ConVar mortar_visualize("mortar_visualize", "0" );

BEGIN_DATADESC( CFuncTank )
	DEFINE_KEYFIELD( m_yawRate, FIELD_FLOAT, "yawrate" ),
	DEFINE_KEYFIELD( m_yawRange, FIELD_FLOAT, "yawrange" ),
	DEFINE_KEYFIELD( m_yawTolerance, FIELD_FLOAT, "yawtolerance" ),
	DEFINE_KEYFIELD( m_pitchRate, FIELD_FLOAT, "pitchrate" ),
	DEFINE_KEYFIELD( m_pitchRange, FIELD_FLOAT, "pitchrange" ),
	DEFINE_KEYFIELD( m_pitchTolerance, FIELD_FLOAT, "pitchtolerance" ),
	DEFINE_KEYFIELD( m_fireRate, FIELD_FLOAT, "firerate" ),
	DEFINE_FIELD( m_fireTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_persist, FIELD_FLOAT, "persistence" ),
	DEFINE_KEYFIELD( m_persist2, FIELD_FLOAT, "persistence2" ),
	DEFINE_KEYFIELD( m_minRange, FIELD_FLOAT, "minRange" ),
	DEFINE_KEYFIELD( m_maxRange, FIELD_FLOAT, "maxRange" ),
	DEFINE_FIELD( m_flMinRange2, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxRange2, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_iAmmoCount, FIELD_INTEGER, "ammo_count" ),
	DEFINE_KEYFIELD( m_spriteScale, FIELD_FLOAT, "spritescale" ),
	DEFINE_KEYFIELD( m_iszSpriteSmoke, FIELD_STRING, "spritesmoke" ),
	DEFINE_KEYFIELD( m_iszSpriteFlash, FIELD_STRING, "spriteflash" ),
	DEFINE_KEYFIELD( m_bulletType, FIELD_INTEGER, "bullet" ),
	DEFINE_FIELD( m_nBulletCount, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_spread, FIELD_INTEGER, "firespread" ),
	DEFINE_KEYFIELD( m_iBulletDamage, FIELD_INTEGER, "bullet_damage" ),
	DEFINE_KEYFIELD( m_iBulletDamageVsPlayer, FIELD_INTEGER, "bullet_damage_vs_player" ),
	DEFINE_KEYFIELD( m_iszMaster, FIELD_STRING, "master" ),
	
#ifdef HL2_EPISODIC	
	DEFINE_KEYFIELD( m_iszAmmoType, FIELD_STRING, "ammotype" ),
	DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),
#else
	DEFINE_FIELD( m_iSmallAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMediumAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( m_iLargeAmmoType, FIELD_INTEGER ),
#endif // HL2_EPISODIC

	DEFINE_KEYFIELD( m_soundStartRotate, FIELD_SOUNDNAME, "rotatestartsound" ),
	DEFINE_KEYFIELD( m_soundStopRotate, FIELD_SOUNDNAME, "rotatestopsound" ),
	DEFINE_KEYFIELD( m_soundLoopRotate, FIELD_SOUNDNAME, "rotatesound" ),
	DEFINE_KEYFIELD( m_flPlayerGracePeriod, FIELD_FLOAT, "playergraceperiod" ),
	DEFINE_KEYFIELD( m_flIgnoreGraceUpto, FIELD_FLOAT, "ignoregraceupto" ),
	DEFINE_KEYFIELD( m_flPlayerLockTimeBeforeFire, FIELD_FLOAT, "playerlocktimebeforefire" ),
	DEFINE_FIELD( m_flLastSawNonPlayer, FIELD_TIME ),

	DEFINE_FIELD( m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( m_yawCenterWorld, FIELD_FLOAT ),
	DEFINE_FIELD( m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( m_pitchCenterWorld, FIELD_FLOAT ),
	DEFINE_FIELD( m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( m_sightOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_hFuncTankTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hController, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecControllerUsePos, FIELD_VECTOR ),
	DEFINE_FIELD( m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( m_targetEntityName, FIELD_STRING ),
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vTargetPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecNPCIdleTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_persist2burst, FIELD_FLOAT),
	//DEFINE_FIELD( m_parentMatrix, FIELD_MATRIX ), // DON'T SAVE
	DEFINE_FIELD( m_hControlVolume, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iszControlVolume, FIELD_STRING, "control_volume" ),
	DEFINE_FIELD( m_flNextControllerSearch, FIELD_TIME ),
	DEFINE_FIELD( m_bShouldFindNPCs, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNPCInRoute, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_iszNPCManPoint, FIELD_STRING, "npc_man_point" ),
	DEFINE_FIELD( m_bReadyToFire, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_bPerformLeading, FIELD_BOOLEAN, "LeadTarget" ),
	DEFINE_FIELD( m_flStartLeadFactor, FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartLeadFactorTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextLeadFactor, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextLeadFactorTime, FIELD_TIME ),

	// Used for when the gun is attached to another entity
	DEFINE_KEYFIELD( m_iszBaseAttachment, FIELD_STRING, "gun_base_attach" ),
	DEFINE_KEYFIELD( m_iszBarrelAttachment, FIELD_STRING, "gun_barrel_attach" ),
//	DEFINE_FIELD( m_nBarrelAttachment, FIELD_INTEGER ),

	// Used when the gun is actually a part of the parent entity, and pose params aim it
	DEFINE_KEYFIELD( m_iszYawPoseParam, FIELD_STRING, "gun_yaw_pose_param" ),
	DEFINE_KEYFIELD( m_iszPitchPoseParam, FIELD_STRING, "gun_pitch_pose_param" ),
	DEFINE_KEYFIELD( m_flYawPoseCenter, FIELD_FLOAT, "gun_yaw_pose_center" ),
	DEFINE_KEYFIELD( m_flPitchPoseCenter, FIELD_FLOAT, "gun_pitch_pose_center" ),
	DEFINE_FIELD( m_bUsePoseParameters, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_iEffectHandling, FIELD_INTEGER, "effecthandling" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFireRate", InputSetFireRate ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDamage", InputSetDamage ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetTargetPosition", InputSetTargetPosition ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetTargetDir", InputSetTargetDir ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetEntityName", InputSetTargetEntityName ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "SetTargetEntity", InputSetTargetEntity ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearTargetEntity", InputClearTargetEntity ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FindNPCToManTank", InputFindNPCToManTank ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopFindingNPCs", InputStopFindingNPCs ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartFindingNPCs", InputStartFindingNPCs ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceNPCOff", InputForceNPCOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMaxRange", InputSetMaxRange ),

	// Outputs
	DEFINE_OUTPUT(m_OnFire,					"OnFire"),
	DEFINE_OUTPUT(m_OnLoseTarget,			"OnLoseTarget"),
	DEFINE_OUTPUT(m_OnAquireTarget,			"OnAquireTarget"),
	DEFINE_OUTPUT(m_OnAmmoDepleted,			"OnAmmoDepleted"),
	DEFINE_OUTPUT(m_OnGotController,		"OnGotController"),
	DEFINE_OUTPUT(m_OnLostController,		"OnLostController"),
	DEFINE_OUTPUT(m_OnGotPlayerController,	"OnGotPlayerController"),
	DEFINE_OUTPUT(m_OnLostPlayerController,	"OnLostPlayerController"),
	DEFINE_OUTPUT(m_OnReadyToFire,			"OnReadyToFire"),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncTank::CFuncTank()
{
	m_nBulletCount = 0;

	m_bNPCInRoute = false;
	m_flNextControllerSearch = 0;
	m_bShouldFindNPCs = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncTank::~CFuncTank( void )
{
	if ( m_soundLoopRotate != NULL_STRING && ( m_spawnflags & SF_TANK_SOUNDON ) )
	{
		StopSound( entindex(), CHAN_STATIC, STRING(m_soundLoopRotate) );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
inline bool CFuncTank::CanFire( void )
{ 
	float flTimeDelay = gpGlobals->curtime - m_lastSightTime;

	// Fire when can't see enemy if time is less that persistence time
	if ( flTimeDelay <= m_persist )
		return true;

	// Fire when I'm in a persistence2 burst
	if ( flTimeDelay <= m_persist2burst )
		return true;

	// If less than persistence2, occasionally do another burst
	if ( flTimeDelay <= m_persist2 )
	{
		if ( random->RandomInt( 0, 30 ) == 0 )
		{
			m_persist2burst = flTimeDelay + 0.5f;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for activating the tank.
//------------------------------------------------------------------------------
void CFuncTank::InputActivate( inputdata_t &inputdata )
{	
	TankActivate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::TankActivate( void )
{
	m_spawnflags |= SF_TANK_ACTIVE; 
	SetNextThink( gpGlobals->curtime + 0.1f ); 
	m_fireLast = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for deactivating the tank.
//-----------------------------------------------------------------------------
void CFuncTank::InputDeactivate( inputdata_t &inputdata )
{
	TankDeactivate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::TankDeactivate( void )
{
	m_spawnflags &= ~SF_TANK_ACTIVE; 
	m_fireLast = 0; 
	StopRotSound();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for changing the name of the tank's target entity.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetEntityName( inputdata_t &inputdata )
{
	m_targetEntityName = inputdata.value.StringID();
	m_hTarget = FindTarget( m_targetEntityName, inputdata.pActivator );

	// No longer aim at target position if have one
	m_spawnflags &= ~SF_TANK_AIM_AT_POS; 
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting a new target entity by ehandle.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetEntity( inputdata_t &inputdata )
{
	if ( inputdata.value.Entity() != NULL )
	{
		m_targetEntityName = inputdata.value.Entity()->GetEntityName();
	}
	else
	{
		m_targetEntityName = NULL_STRING;
	}
	m_hTarget = inputdata.value.Entity();

	// No longer aim at target position if have one
	m_spawnflags &= ~SF_TANK_AIM_AT_POS; 
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for clearing the tank's target entity
//-----------------------------------------------------------------------------
void CFuncTank::InputClearTargetEntity( inputdata_t &inputdata )
{
	m_targetEntityName = NULL_STRING;
	m_hTarget = NULL;

	// No longer aim at target position if have one
	m_spawnflags &= ~SF_TANK_AIM_AT_POS; 
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the rate of fire in shots per second.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetFireRate( inputdata_t &inputdata )
{
	m_fireRate = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the damage
//-----------------------------------------------------------------------------
void CFuncTank::InputSetDamage( inputdata_t &inputdata )
{
	m_iBulletDamage = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the target as a position.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetPosition( inputdata_t &inputdata )
{
	m_spawnflags |= SF_TANK_AIM_AT_POS; 
	m_hTarget = NULL;

	inputdata.value.Vector3D( m_vTargetPosition );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the target as a position.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetDir( inputdata_t &inputdata )
{
	m_spawnflags |= SF_TANK_AIM_AT_POS; 
	m_hTarget = NULL;

	Vector vecTargetDir;
	inputdata.value.Vector3D( vecTargetDir );
	m_vTargetPosition = GetAbsOrigin() + m_barrelPos.LengthSqr() * vecTargetDir;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for telling the func_tank to find an NPC to man it.
//-----------------------------------------------------------------------------
void CFuncTank::InputFindNPCToManTank( inputdata_t &inputdata )
{
	// Verify the func_tank is controllable and available.
	if ( !IsNPCControllable() && !IsNPCSetController() )
		return;

	// If we have a controller already - don't look for one.
	if ( HasController() )
		return;

	// NPC assigned to man the func_tank?
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.StringID() );
	if ( pEntity )
	{
		CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();
		if ( pNPC )
		{
			// Verify the npc has the func_tank controller behavior.
			CAI_FuncTankBehavior *pBehavior;
			if ( pNPC->GetBehavior( &pBehavior ) )
			{
				m_hController = pNPC;
				pBehavior->SetFuncTank( this );
				NPC_SetInRoute( true );
				return;
			}
		}
	}

	// No controller? Find a nearby NPC who can man this func_tank.
	NPC_FindController();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputStopFindingNPCs( inputdata_t &inputdata )
{
	m_bShouldFindNPCs = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputStartFindingNPCs( inputdata_t &inputdata )
{
	m_bShouldFindNPCs = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputForceNPCOff( inputdata_t &inputdata )
{
	// Interrupt any npc in route (ally or not).
	if ( NPC_InRoute() )
	{
		// Interrupt the npc's route.
		NPC_InterruptRoute();
	}

	// If we don't have a controller - then the gun should be free.
	if ( !m_hController )
		return;

	CAI_BaseNPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC )
		return;

	CAI_FuncTankBehavior *pBehavior;
	if ( pNPC->GetBehavior( &pBehavior ) )
	{
		pBehavior->Dismount();
	}

	m_hController = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputSetMaxRange( inputdata_t &inputdata )
{
	m_maxRange = inputdata.value.Float();
	m_flMaxRange2 = m_maxRange * m_maxRange;
}

//-----------------------------------------------------------------------------
// Purpose: Find the closest NPC with the func_tank behavior.
//-----------------------------------------------------------------------------
void CFuncTank::NPC_FindController( void )
{
	// Not NPC controllable or controllable on by specified npc's return.
	if ( !IsNPCControllable() || IsNPCSetController() )
		return;

	// Initialize for finding closest NPC.
	CAI_BaseNPC *pClosestNPC = NULL;
	float flClosestDist2 = ( FUNCTANK_DISTANCE_MAX * FUNCTANK_DISTANCE_MAX );
	float flMinDistToEnemy2 = ( FUNCTANK_DISTANCE_MIN_TO_ENEMY * FUNCTANK_DISTANCE_MIN_TO_ENEMY );
	CAI_FuncTankBehavior *pClosestBehavior = NULL;

	// Get the mount position.
	Vector vecMountPos;
	NPC_FindManPoint( vecMountPos );

	// Search through the AI list for the closest NPC with the func_tank behavior.
	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAICount = g_AI_Manager.NumAIs();
	for ( int iAI = 0; iAI < nAICount; ++iAI )
	{
		CAI_BaseNPC *pNPC = ppAIs[iAI];
		if ( !pNPC )
			continue;
		
		if ( !pNPC->IsAlive() )
			continue;

		if ( pNPC->IsInAScript() )
			continue;

		CAI_FuncTankBehavior *pBehavior;
		if ( pNPC->GetBehavior( &pBehavior ) )
		{
			// Don't mount the func_tank if your "enemy" is within X feet or it or the npc.
			CBaseEntity *pEnemy = pNPC->GetEnemy();

			if ( pEnemy )
			{
				if ( !IsEntityInViewCone(pEnemy) )
				{
					// Don't mount the tank if the tank can't be aimed at the enemy.
					continue;
				}

				float flDist2 = ( pEnemy->GetAbsOrigin() - pNPC->GetAbsOrigin() ).LengthSqr();
				if ( flDist2 < flMinDistToEnemy2 )
					continue;

				flDist2 = ( vecMountPos - pEnemy->GetAbsOrigin() ).LengthSqr();
				if ( flDist2 < flMinDistToEnemy2 )
					continue;

				if ( !pNPC->FVisible( vecMountPos + pNPC->GetViewOffset() ) )
					continue;
			}

			trace_t tr;
			UTIL_TraceEntity( pNPC, vecMountPos, vecMountPos, MASK_NPCSOLID, this, pNPC->GetCollisionGroup(), &tr );
			if( tr.startsolid || tr.fraction < 1.0 )
			{
				// Don't mount the tank if someone/something is located on the control point.
				continue;
			}

			if ( !pBehavior->HasFuncTank() && !pBehavior->IsBusy() )
			{
				float flDist2 = ( vecMountPos - pNPC->GetAbsOrigin() ).LengthSqr();
				if ( flDist2 < flClosestDist2 )
				{
					pClosestNPC = pNPC;
					pClosestBehavior = pBehavior;
					flClosestDist2 = flDist2;
				}
			}
		}
	}

	// Set the closest NPC as controller.
	if ( pClosestNPC )
	{
		m_hController = pClosestNPC;
		pClosestBehavior->SetFuncTank( this );
		NPC_SetInRoute( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFuncTank::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// State
		// --------------
		char tempstr[255];
		if (IsActive()) 
		{
			Q_strncpy(tempstr,"State: Active",sizeof(tempstr));
		}
		else 
		{
			Q_strncpy(tempstr,"State: Inactive",sizeof(tempstr));
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;
		
		// -------------------
		// Print Firing Speed
		// --------------------
		Q_snprintf(tempstr,sizeof(tempstr),"Fire Rate: %f",m_fireRate);

		EntityText(text_offset,tempstr,0);
		text_offset++;
		
		// --------------
		// Print Target
		// --------------
		if (m_hTarget!=NULL) 
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Target: %s",m_hTarget->GetDebugName());
		}
		else
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Target:   -  ");
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// --------------
		// Target Pos
		// --------------
		if (m_spawnflags & SF_TANK_AIM_AT_POS) 
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Aim Pos: %3.0f %3.0f %3.0f",m_vTargetPosition.x,m_vTargetPosition.y,m_vTargetPosition.z);
		}
		else
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Aim Pos:    -  ");
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of fly direction
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CFuncTank::DrawDebugGeometryOverlays(void) 
{
	// Center
	QAngle angCenter;
	Vector vecForward;
	angCenter = QAngle( 0, YawCenterWorld(), 0 );
	AngleVectors( angCenter, &vecForward );
	NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + (vecForward * 64), 255,255,255, true, 0.1);

	// Draw the yaw ranges
	angCenter = QAngle( 0, YawCenterWorld() + m_yawRange, 0 );
	AngleVectors( angCenter, &vecForward );
	NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + (vecForward * 128), 0,255,0, true, 0.1);
	angCenter = QAngle( 0, YawCenterWorld() - m_yawRange, 0 );
	AngleVectors( angCenter, &vecForward );
	NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + (vecForward * 128), 0,255,0, true, 0.1);

	// Draw the pitch ranges
	angCenter = QAngle( PitchCenterWorld() + m_pitchRange, 0, 0 );
	AngleVectors( angCenter, &vecForward );
	NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + (vecForward * 128), 255,0,0, true, 0.1);
	angCenter = QAngle( PitchCenterWorld() - m_pitchRange, 0, 0 );
	AngleVectors( angCenter, &vecForward );
	NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + (vecForward * 128), 255,0,0, true, 0.1);

	BaseClass::DrawDebugGeometryOverlays();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pAttacker - 
//			flDamage - 
//			vecDir - 
//			ptr - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CFuncTank::TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType)
{
	if (m_spawnflags & SF_TANK_DAMAGE_KICK)
	{
		// Deflect the func_tank
		// Only adjust yaw for now
		if (pAttacker)
		{
			Vector vFromAttacker = (pAttacker->EyePosition()-GetAbsOrigin());
			vFromAttacker.z = 0;
			VectorNormalize(vFromAttacker);

			Vector vFromAttacker2 = (ptr->endpos-GetAbsOrigin());
			vFromAttacker2.z = 0;
			VectorNormalize(vFromAttacker2);


			Vector vCrossProduct;
			CrossProduct(vFromAttacker,vFromAttacker2, vCrossProduct);

			QAngle angles;
			angles = GetLocalAngles();
			if (vCrossProduct.z > 0)
			{
				angles.y		+= 10;
			}
			else
			{
				angles.y		-= 10;
			}

			// Limit against range in y
			if ( angles.y > m_yawCenter + m_yawRange )
			{
				angles.y = m_yawCenter + m_yawRange;
			}
			else if ( angles.y < (m_yawCenter - m_yawRange) )
			{
				angles.y = (m_yawCenter - m_yawRange);
			}

			SetLocalAngles( angles );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : targetName - 
//			pActivator - 
//-----------------------------------------------------------------------------
CBaseEntity *CFuncTank::FindTarget( string_t targetName, CBaseEntity *pActivator ) 
{
	return gEntList.FindEntityGenericNearest( STRING( targetName ), GetAbsOrigin(), 0, this, pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: Caches entity key values until spawn is called.
// Input  : szKeyName - 
//			szValue - 
// Output : 
//-----------------------------------------------------------------------------
bool CFuncTank::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "barrel"))
	{
		m_barrelPos.x = atof(szValue);
		return true;
	}
	
	if (FStrEq(szKeyName, "barrely"))
	{
		m_barrelPos.y = atof(szValue);
		return true;
	}
	
	if (FStrEq(szKeyName, "barrelz"))
	{
		m_barrelPos.z = atof(szValue);
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}


static Vector gTankSpread[] =
{
	Vector( 0, 0, 0 ),		// perfect
	Vector( 0.025, 0.025, 0.025 ),	// small cone
	Vector( 0.05, 0.05, 0.05 ),  // medium cone
	Vector( 0.1, 0.1, 0.1 ),	// large cone
	Vector( 0.25, 0.25, 0.25 ),	// extra-large cone
};
#define MAX_FIRING_SPREADS ARRAYSIZE(gTankSpread)


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::Spawn( void )
{
	Precache();

#ifdef HL2_EPISODIC
	m_iAmmoType = GetAmmoDef()->Index( STRING( m_iszAmmoType ) );
#else
	m_iSmallAmmoType	= GetAmmoDef()->Index("Pistol");
	m_iMediumAmmoType	= GetAmmoDef()->Index("SMG1");
	m_iLargeAmmoType	= GetAmmoDef()->Index("AR2");
#endif // HL2_EPISODIC

	SetMoveType( MOVETYPE_PUSH );  // so it doesn't get pushed by anything
	SetSolid( SOLID_VPHYSICS );
	SetModel( STRING( GetModelName() ) );
	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	if ( HasSpawnFlags(SF_TANK_NOTSOLID) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	m_hControlVolume	= NULL;

	if ( GetParent() && GetParent()->GetBaseAnimating() )
	{
		CBaseAnimating *pAnim = GetParent()->GetBaseAnimating();
		if ( m_iszBaseAttachment != NULL_STRING )
		{
			int nAttachment = pAnim->LookupAttachment( STRING( m_iszBaseAttachment ) );
			if ( nAttachment != 0 )
			{
				SetParent( pAnim, nAttachment );
				SetLocalOrigin( vec3_origin );
				SetLocalAngles( vec3_angle );
			}
		}

		m_bUsePoseParameters = (m_iszYawPoseParam != NULL_STRING) && (m_iszPitchPoseParam != NULL_STRING);

		if ( m_iszBarrelAttachment != NULL_STRING )
		{
			if ( m_bUsePoseParameters )
			{
				pAnim->SetPoseParameter( STRING( m_iszYawPoseParam ), 0 );
				pAnim->SetPoseParameter( STRING( m_iszPitchPoseParam ), 0 );
				pAnim->InvalidateBoneCache();
			}

			m_nBarrelAttachment = pAnim->LookupAttachment( STRING(m_iszBarrelAttachment) );

			Vector vecWorldBarrelPos;
			QAngle worldBarrelAngle;
			pAnim->GetAttachment( m_nBarrelAttachment, vecWorldBarrelPos, worldBarrelAngle );
			VectorITransform( vecWorldBarrelPos, EntityToWorldTransform( ), m_barrelPos );
		}

		if ( m_bUsePoseParameters )
		{
			// In this case, we're relying on the parent to have the gun model
			AddEffects( EF_NODRAW );
			QAngle localAngles( m_flPitchPoseCenter, m_flYawPoseCenter, 0 );
			SetLocalAngles( localAngles );
			SetSolid( SOLID_NONE );
			SetMoveType( MOVETYPE_NOCLIP );

			// If our parent is a prop_dynamic, make it use hitboxes for renderbox
			CDynamicProp *pProp = dynamic_cast<CDynamicProp*>(GetParent());
			if ( pProp )
			{
				pProp->m_bUseHitboxesForRenderBox = true;
			}
		}
	}

	// For smoothing out leading
	m_flStartLeadFactor = 1.0f;
	m_flNextLeadFactor = 1.0f;
	m_flStartLeadFactorTime = gpGlobals->curtime;
	m_flNextLeadFactorTime = gpGlobals->curtime + 1.0f;

	m_yawCenter			= GetLocalAngles().y;
	m_yawCenterWorld	= GetAbsAngles().y;
	m_pitchCenter		= GetLocalAngles().x;
	m_pitchCenterWorld	= GetAbsAngles().y;
	m_vTargetPosition	= vec3_origin;

	if ( IsActive() || (IsControllable() && !HasController()) )
	{
		// Think to find controllers.
		SetNextThink( gpGlobals->curtime + 1.0f );
		m_flNextControllerSearch = gpGlobals->curtime + 1.0f;
	}

	UpdateMatrix();

	m_sightOrigin = WorldBarrelPosition(); // Point at the end of the barrel

	if ( m_spread > MAX_FIRING_SPREADS )
	{
		m_spread = 0;
	}

	// No longer aim at target position if have one
	m_spawnflags		&= ~SF_TANK_AIM_AT_POS; 

	if (m_spawnflags & SF_TANK_DAMAGE_KICK)
	{
		m_takedamage = DAMAGE_YES;
	}

	// UNDONE: Do this?
	//m_targetEntityName = m_target;
	if ( GetSolid() != SOLID_NONE )
	{
		CreateVPhysics();
	}

	// Setup squared min/max range.
	m_flMinRange2 = m_minRange * m_minRange;
	m_flMaxRange2 = m_maxRange * m_maxRange;
	m_flIgnoreGraceUpto *= m_flIgnoreGraceUpto;

	m_flLastSawNonPlayer = 0;

	if( IsActive() )
	{
		m_OnReadyToFire.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::Activate( void )
{
	BaseClass::Activate();

	// Necessary for save/load
	if ( (m_iszBarrelAttachment != NULL_STRING) && (m_nBarrelAttachment == 0) )
	{
		if ( GetParent() && GetParent()->GetBaseAnimating() )
		{
			CBaseAnimating *pAnim = GetParent()->GetBaseAnimating();
			m_nBarrelAttachment = pAnim->LookupAttachment( STRING(m_iszBarrelAttachment) );
		}
	}
}

bool CFuncTank::CreateVPhysics()
{
	VPhysicsInitShadow( false, false );
	return true;
}


void CFuncTank::Precache( void )
{
	if ( m_iszSpriteSmoke != NULL_STRING )
		PrecacheModel( STRING(m_iszSpriteSmoke) );
	if ( m_iszSpriteFlash != NULL_STRING )
		PrecacheModel( STRING(m_iszSpriteFlash) );

	if ( m_soundStartRotate != NULL_STRING )
		PrecacheScriptSound( STRING(m_soundStartRotate) );
	if ( m_soundStopRotate != NULL_STRING )
		PrecacheScriptSound( STRING(m_soundStopRotate) );
	if ( m_soundLoopRotate != NULL_STRING )
		PrecacheScriptSound( STRING(m_soundLoopRotate) );

	PrecacheScriptSound( "Func_Tank.BeginUse" );
	
	// Precache the combine cannon
	if ( m_iEffectHandling == EH_COMBINE_CANNON )
	{
		PrecacheScriptSound( "NPC_Combine_Cannon.FireBullet" );
	}
}

void CFuncTank::UpdateOnRemove( void )
{
	if ( HasController() )
	{
		StopControl();
	}
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Barrel position
//-----------------------------------------------------------------------------
void CFuncTank::UpdateMatrix( void )
{
	m_parentMatrix.InitFromEntity( GetParent(), GetParentAttachment() );
}

	
//-----------------------------------------------------------------------------
// Barrel position
//-----------------------------------------------------------------------------
Vector CFuncTank::WorldBarrelPosition( void )
{
	if ( (m_nBarrelAttachment == 0) || !GetParent() )
	{
		EntityMatrix tmp;
		tmp.InitFromEntity( this );
		return tmp.LocalToWorld( m_barrelPos );
	}

	Vector vecOrigin;
	QAngle vecAngles;
	CBaseAnimating *pAnim = GetParent()->GetBaseAnimating();
	pAnim->GetAttachment( m_nBarrelAttachment, vecOrigin, vecAngles );
	return vecOrigin;
}


//-----------------------------------------------------------------------------
// Make the parent's pose parameters match the func_tank 
//-----------------------------------------------------------------------------
void CFuncTank::PhysicsSimulate( void )
{
	BaseClass::PhysicsSimulate();

	if ( m_bUsePoseParameters && GetParent() )
	{
		const QAngle &angles = GetLocalAngles();
		CBaseAnimating *pAnim = GetParent()->GetBaseAnimating();
		pAnim->SetPoseParameter( STRING( m_iszYawPoseParam ), angles.y );
		pAnim->SetPoseParameter( STRING( m_iszPitchPoseParam ), angles.x );
		pAnim->StudioFrameAdvance();
	}
}

//=============================================================================
//
// TANK CONTROLLING
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::OnControls( CBaseEntity *pTest )
{
	// Is the tank controllable.
	if ( !IsControllable() )
		return false;

	if ( !m_hControlVolume )
	{
		// Find our control volume
		if ( m_iszControlVolume != NULL_STRING )
		{
			m_hControlVolume = dynamic_cast<CBaseTrigger*>( gEntList.FindEntityByName( NULL, m_iszControlVolume ) );
		}

		if (( !m_hControlVolume ) && IsControllable() )
		{
			Msg( "ERROR: Couldn't find control volume for player-controllable func_tank %s.\n", STRING(GetEntityName()) );
			return false;
		}
	}

	if ( m_hControlVolume->IsTouching( pTest ) )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::StartControl( CBaseCombatCharacter *pController )
{
	// Check to see if we have a controller.
	if ( HasController() && GetController() != pController )
		return false;

	// Team only or disabled?
	if ( m_iszMaster != NULL_STRING )
	{
		if ( !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
			return false;
	}

	// Set func_tank as manned by player/npc.
	m_hController = pController;
	if ( pController->IsPlayer() )
	{
		m_spawnflags |= SF_TANK_PLAYER; 

		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( m_hController.Get() );
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;
	}
	else
	{
		m_spawnflags |= SF_TANK_NPC;
		NPC_SetInRoute( false );
	}

	// Holster player/npc weapon
	if ( m_hController->GetActiveWeapon() )
	{
		m_hController->GetActiveWeapon()->Holster();
	}

	// Set the controller's position to be the use position.
	m_vecControllerUsePos = m_hController->GetLocalOrigin();

	EmitSound( "Func_Tank.BeginUse" );
	
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	// Let the map maker know a controller has been found
	if ( m_hController->IsPlayer() )
	{
		m_OnGotPlayerController.FireOutput( this, this );
	}
	else
	{
		m_OnGotController.FireOutput( this, this );
	}

	OnStartControlled();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// TODO: bring back the controllers current weapon
//-----------------------------------------------------------------------------
void CFuncTank::StopControl()
{
	// Do we have a controller?
	if ( !m_hController )
		return;

	OnStopControlled();

	// Arm player/npc weapon.
	if ( m_hController->GetActiveWeapon() )
	{
		m_hController->GetActiveWeapon()->Deploy();
	}

	if ( m_hController->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( m_hController.Get() );
		pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
	}

	// Stop thinking.
	SetNextThink( TICK_NEVER_THINK );
	
	// Let the map maker know a controller has been lost.
	if ( m_hController->IsPlayer() )
	{
		m_OnLostPlayerController.FireOutput( this, this );
	}
	else
	{
		m_OnLostController.FireOutput( this, this );
	}

	// Reset the func_tank as unmanned (player/npc).
	if ( m_hController->IsPlayer() )
	{
		m_spawnflags &= ~SF_TANK_PLAYER;
	}
	else
	{		
		m_spawnflags &= ~SF_TANK_NPC;
	}

	m_hController = NULL;

	// Set think, if the func_tank can think on its own.
	if ( IsActive() || (IsControllable() && !HasController()) )
	{
		// Delay the think to find controllers a bit
#ifdef FUNCTANK_AUTOUSE
		m_flNextControllerSearch = gpGlobals->curtime + 1.0f;
#else
		m_flNextControllerSearch = gpGlobals->curtime + 5.0f;
#endif//FUNCTANK_AUTOUSE

		SetNextThink( m_flNextControllerSearch );
	}

	SetLocalAngularVelocity( vec3_angle );
}

//-----------------------------------------------------------------------------
// Purpose:
// Called each frame by the player's ItemPostFrame
//-----------------------------------------------------------------------------

// NVNT turret recoil
ConVar hap_turret_mag("hap_turret_mag", "5", 0);

void CFuncTank::ControllerPostFrame( void )
{
	// Make sure we have a contoller.
	Assert( m_hController != NULL );

	// Control the firing rate.
	if ( gpGlobals->curtime < m_flNextAttack )
		return;

	if ( !IsPlayerManned() )
		return;

	CBasePlayer *pPlayer = static_cast<CBasePlayer*>( m_hController.Get() );
	if ( ( pPlayer->m_nButtons & IN_ATTACK ) == 0 )
		return;

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	m_fireLast = gpGlobals->curtime - (1/m_fireRate) - 0.01;  // to make sure the gun doesn't fire too many bullets
	
	int bulletCount = (gpGlobals->curtime - m_fireLast) * m_fireRate;
	
	if( HasSpawnFlags( SF_TANK_AIM_ASSISTANCE ) )
	{
		// Trace out a hull and if it hits something, adjust the shot to hit that thing.
		trace_t tr;
		Vector start = WorldBarrelPosition();
		Vector dir = forward;
		
		UTIL_TraceHull( start, start + forward * 8192, -Vector(8,8,8), Vector(8,8,8), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		
		if( tr.m_pEnt && tr.m_pEnt->m_takedamage != DAMAGE_NO && (tr.m_pEnt->GetFlags() & FL_AIMTARGET) )
		{
			forward = tr.m_pEnt->WorldSpaceCenter() - start;
			VectorNormalize( forward );
		}
	}
	
	Fire( bulletCount, WorldBarrelPosition(), forward, pPlayer, false );
 
#if defined( WIN32 ) && !defined( _X360 ) 
	// NVNT apply a punch on the player each time fired
	HapticPunch(pPlayer,0,0,hap_turret_mag.GetFloat());
#endif	
	// HACKHACK -- make some noise (that the AI can hear)
	CSoundEnt::InsertSound( SOUND_COMBAT, WorldSpaceCenter(), FUNCTANK_FIREVOLUME, 0.2 );
	
	if( m_iAmmoCount > -1 )
	{
		if( !(m_iAmmoCount % 10) )
		{
			Msg("Ammo Remaining: %d\n", m_iAmmoCount );
		}
		
		if( --m_iAmmoCount == 0 )
		{
			// Kick the player off the gun, and make myself not usable.
			m_spawnflags &= ~SF_TANK_CANCONTROL;
			StopControl();
			return;				
		}
	}
	
	SetNextAttack( gpGlobals->curtime + (1/m_fireRate) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFuncTank::HasController( void )
{ 
	return (m_hController != NULL); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatCharacter
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CFuncTank::GetController( void ) 
{ 
	return m_hController; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::NPC_FindManPoint( Vector &vecPos )
{
	if ( m_iszNPCManPoint != NULL_STRING )
	{	
		CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, m_iszNPCManPoint );
		if ( pEntity )
		{
			vecPos = pEntity->GetAbsOrigin();
			return true;
		}
	}

	return false; 
}

//-----------------------------------------------------------------------------
// Purpose: The NPC manning this gun just saw a player for the first time since he left cover
//-----------------------------------------------------------------------------
void CFuncTank::NPC_JustSawPlayer( CBaseEntity *pTarget )
{
	SetNextAttack( gpGlobals->curtime + m_flPlayerLockTimeBeforeFire );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncTank::NPC_Fire( void )
{
	// Control the firing rate.
	if ( gpGlobals->curtime < m_flNextAttack )
		return;

	// Check for a valid npc controller.
	if ( !m_hController )
		return;

	CAI_BaseNPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC )
		return;

	// Setup for next round of firing.
	if ( m_nBulletCount == 0 )
	{
		m_nBulletCount = GetRandomBurst();
		m_fireTime = 1.0f;
	}

	// m_fireLast looks like it is only needed for Active non-controlled func_tank.
//		m_fireLast = gpGlobals->curtime - (1/m_fireRate) - 0.01;  // to make sure the gun doesn't fire too many bullets		

	Vector vecBarrelEnd = WorldBarrelPosition();		
	Vector vecForward;
	AngleVectors( GetAbsAngles(), &vecForward );

	if ( (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_SQUADMATES) && pNPC->IsInSquad() )
	{
		// Avoid shooting squadmates.
		if ( pNPC->IsSquadmateInSpread( vecBarrelEnd, vecBarrelEnd + vecForward * 2048, gTankSpread[m_spread].x, 8*12 ) )
		{
			return;
		}
	}

	if ( !HasSpawnFlags( SF_TANK_ALLOW_PLAYER_HITS ) && (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_PLAYER) )
	{
		// Avoid shooting player.
		if ( pNPC->PlayerInSpread( vecBarrelEnd, vecBarrelEnd + vecForward * 2048, gTankSpread[m_spread].x, 8*12 ) )
		{
			return;
		}
	}

	bool bIgnoreSpread = false;

  	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if ( HasSpawnFlags( SF_TANK_HACKPLAYERHIT ) && pEnemy && pEnemy->IsPlayer() )
	{
		// Every third shot should be fired directly at the player
		if ( m_nBulletCount%2 == 0 )
		{
			Vector vecBodyTarget = pEnemy->BodyTarget( vecBarrelEnd, false );
			vecForward = (vecBodyTarget - vecBarrelEnd);
			VectorNormalize( vecForward );
			bIgnoreSpread = true;
		}
	}

	// Fire the bullet(s).
	Fire( 1, vecBarrelEnd, vecForward, m_hController, bIgnoreSpread );
	--m_nBulletCount;

	// Check ammo counts and dismount when empty.
	if( m_iAmmoCount > -1 )
	{
		if( --m_iAmmoCount == 0 )
		{
			// Disable the func_tank.
			m_spawnflags &= ~SF_TANK_CANCONTROL;

			// Remove the npc.
			StopControl();
			return;				
		}
	}
	
	float flFireTime = GetRandomFireTime();
	if ( m_nBulletCount != 0 )
	{	
		m_fireTime -= flFireTime;
		SetNextAttack( gpGlobals->curtime + flFireTime );
	}
	else
	{
		SetNextAttack( gpGlobals->curtime + m_fireTime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncTank::NPC_HasEnemy( void )
{
	if ( !IsNPCManned() )
		return false;

	CAI_BaseNPC *pNPC = m_hController->MyNPCPointer();
	Assert( pNPC );

	return ( pNPC->GetEnemy() != NULL );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncTank::NPC_InterruptRoute( void )
{
	if ( !m_hController )
		return;

	CAI_BaseNPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC )
		return;

	CAI_FuncTankBehavior *pBehavior;
	if ( pNPC->GetBehavior( &pBehavior ) )
	{
		pBehavior->SetFuncTank( NULL );
	}

	// Reset the npc controller.
	m_hController = NULL;

	// No NPC's in route.
	NPC_SetInRoute( false );

	// Delay the think to find controllers a bit
	m_flNextControllerSearch = gpGlobals->curtime + 5.0f;

	if ( !HasController() )
	{
		// Start thinking to find controllers again
		SetNextThink( m_flNextControllerSearch );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::NPC_InterruptController( void )
{
	// If we don't have a controller - then the gun should be free.
	if ( !m_hController )
		return true;

	CAI_BaseNPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC || !pNPC->IsPlayerAlly() )
		return false;

	CAI_FuncTankBehavior *pBehavior;
	if ( pNPC->GetBehavior( &pBehavior ) )
	{
		pBehavior->Dismount();
	}

	m_hController = NULL;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CFuncTank::GetRandomFireTime( void )
{
	Assert( m_fireRate != 0 );
	float flOOFireRate = 1.0f / m_fireRate;
	float flOOFireRateBy2 = flOOFireRate * 0.5f;
	float flOOFireRateBy4 = flOOFireRate * 0.25f;
	return random->RandomFloat( flOOFireRateBy4, flOOFireRateBy2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CFuncTank::GetRandomBurst( void )
{
	return random->RandomInt( m_fireRate-2, m_fireRate+2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CFuncTank::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !IsControllable() )
		return;

	// player controlled turret
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	if ( value == 2 && useType == USE_SET )
	{
		ControllerPostFrame();
	}
	else if ( m_hController != pPlayer && useType != USE_OFF )
	{
		// The player must be within the func_tank controls
		if ( !m_hControlVolume )
		{
			// Find our control volume
			if ( m_iszControlVolume != NULL_STRING )
			{
				m_hControlVolume = dynamic_cast<CBaseTrigger*>( gEntList.FindEntityByName( NULL, m_iszControlVolume ) );
			}

			if (( !m_hControlVolume ) && IsControllable() )
			{
				Msg( "ERROR: Couldn't find control volume for player-controllable func_tank %s.\n", STRING(GetEntityName()) );
				return;
			}
		}

		if ( !m_hControlVolume->IsTouching( pPlayer ) )
			return;

		// Interrupt any npc in route (ally or not).
		if ( NPC_InRoute() )
		{
			// Interrupt the npc's route.
			NPC_InterruptRoute();
		}

		// Interrupt NPC - if possible (they must be allies).
		if ( IsNPCControllable() && HasController() )
		{
			if ( !NPC_InterruptController() )
				return;
		}

		pPlayer->SetUseEntity( this );
		StartControl( pPlayer );
	}
	else 
	{
		StopControl();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : range - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFuncTank::InRange( float range )
{
	if ( range < m_minRange )
		return FALSE;
	if ( (m_maxRange > 0) && (range > m_maxRange) )
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::InRange2( float flRange2 )
{
	if ( flRange2 < m_flMinRange2 )
		return false;

	if ( ( m_flMaxRange2 > 0.0f ) && ( flRange2 > m_flMaxRange2 ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncTank::Think( void )
{
	FuncTankPreThink();

	m_hFuncTankTarget = NULL;

	// Look for a new controller?
	if ( IsControllable() && !HasController() && (m_flNextControllerSearch <= gpGlobals->curtime) )
	{
		if ( m_bShouldFindNPCs && gpGlobals->curtime > 5.0f )
		{
			// Check for in route and timer.
			if ( !NPC_InRoute() )
			{
				NPC_FindController();
			}
		}

#ifdef FUNCTANK_AUTOUSE
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
		bool bThinkFast = false;

		if( pPlayer )
		{
			if ( !m_hControlVolume )
			{
				// Find our control volume
				if ( m_iszControlVolume != NULL_STRING )
				{
					m_hControlVolume = dynamic_cast<CBaseTrigger*>( gEntList.FindEntityByName( NULL, m_iszControlVolume ) );
				}

				if (( !m_hControlVolume ) && IsControllable() )
				{
					Msg( "ERROR: Couldn't find control volume for player-controllable func_tank %s.\n", STRING(GetEntityName()) );
					return;
				}
			}

			if ( m_hControlVolume )
			{
				if( m_hControlVolume->IsTouching( pPlayer ) && pPlayer->FInViewCone(WorldSpaceCenter()) )
				{
					// If my control volume is touching a player that's facing the mounted gun, automatically use the gun.
					// !!!BUGBUG - this only works in cases where the player can see the gun whilst standing in the control 
					// volume. (This works just fine for all func_tanks mounted on combine walls and small barriers)
					variant_t emptyVariant;
					AcceptInput( "Use", pPlayer, pPlayer, emptyVariant, USE_TOGGLE );
				}
				else
				{
					// If the player is nearby, think faster for snappier response to XBox auto mounting
					float flDistSqr = GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );

					if( flDistSqr <= Square(360) )
					{
						bThinkFast = true;
					}
				}
			}
		}

		// Keep thinking, in case they turn NPC finding back on
		if ( !HasController() )
		{
			if( bThinkFast )
			{
				SetNextThink( gpGlobals->curtime + 0.1f );
			}
			else
			{
				SetNextThink( gpGlobals->curtime + 2.0f );
			}
		}

		if( bThinkFast )
		{
			m_flNextControllerSearch = gpGlobals->curtime + 0.1f;
		}
		else
		{
			m_flNextControllerSearch = gpGlobals->curtime + 2.0f;
		}
#else
		// Keep thinking, in case they turn NPC finding back on
		if ( !HasController() )
		{
			SetNextThink( gpGlobals->curtime + 2.0f );
		}

		m_flNextControllerSearch = gpGlobals->curtime + 2.0f;
#endif//FUNCTANK_AUTOUSE
	}

	// refresh the matrix
	UpdateMatrix();

	SetLocalAngularVelocity( vec3_angle );
	TrackTarget();

	if ( fabs(GetLocalAngularVelocity().x) > 1 || fabs(GetLocalAngularVelocity().y) > 1 )
	{
		StartRotSound();
	}
	else
	{
		StopRotSound();
	}

	FuncTankPostThink();
}


//-----------------------------------------------------------------------------
// Purpose: Aim the offset barrel at a position in parent space
// Input  : parentTarget - the position of the target in parent space
// Output : Vector - angles in local space
//-----------------------------------------------------------------------------
QAngle CFuncTank::AimBarrelAt( const Vector &parentTarget )
{
	Vector target = parentTarget - GetLocalOrigin();
	float quadTarget = target.LengthSqr();
	float quadTargetXY = target.x*target.x + target.y*target.y;

	// Target is too close!  Can't aim at it
	if ( quadTarget <= m_barrelPos.LengthSqr() )
	{
		return GetLocalAngles();
	}
	else
	{
		// We're trying to aim the offset barrel at an arbitrary point.
		// To calculate this, I think of the target as being on a sphere with 
		// it's center at the origin of the gun.
		// The rotation we need is the opposite of the rotation that moves the target 
		// along the surface of that sphere to intersect with the gun's shooting direction
		// To calculate that rotation, we simply calculate the intersection of the ray 
		// coming out of the barrel with the target sphere (that's the new target position)
		// and use atan2() to get angles

		// angles from target pos to center
		float targetToCenterYaw = atan2( target.y, target.x );
		float centerToGunYaw = atan2( m_barrelPos.y, sqrt( quadTarget - (m_barrelPos.y*m_barrelPos.y) ) );

		float targetToCenterPitch = atan2( target.z, sqrt( quadTargetXY ) );
		float centerToGunPitch = atan2( -m_barrelPos.z, sqrt( quadTarget - (m_barrelPos.z*m_barrelPos.z) ) );
		return QAngle( -RAD2DEG(targetToCenterPitch+centerToGunPitch), RAD2DEG( targetToCenterYaw + centerToGunYaw ), 0 );
	}
}


//-----------------------------------------------------------------------------
// Aim the tank at the player crosshair 
//-----------------------------------------------------------------------------
void CFuncTank::CalcPlayerCrosshairTarget( Vector *pVecTarget )
{
	// Get the player.
	CBasePlayer *pPlayer = static_cast<CBasePlayer*>( m_hController.Get() );

	// Tank aims at player's crosshair.
	Vector vecStart, vecDir;
	trace_t	tr;
	
	vecStart = pPlayer->EyePosition();

	if ( !IsX360() )
	{
		vecDir = pPlayer->EyeDirection3D();
	}
	else
	{
		// Use autoaim as the eye dir.
		vecDir = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	}
	
	// Make sure to start the trace outside of the player's bbox!
	UTIL_TraceLine( vecStart + vecDir * 24, vecStart + vecDir * 8192, MASK_BLOCKLOS_AND_NPCS, this, COLLISION_GROUP_NONE, &tr );

	*pVecTarget = tr.endpos;
}

//-----------------------------------------------------------------------------
// Aim the tank at the player crosshair 
//-----------------------------------------------------------------------------
void CFuncTank::AimBarrelAtPlayerCrosshair( QAngle *pAngles )
{
	Vector vecTarget;
	CalcPlayerCrosshairTarget( &vecTarget );
	*pAngles = AimBarrelAt( m_parentMatrix.WorldToLocal( vecTarget ) );
}


//-----------------------------------------------------------------------------
// Aim the tank at the NPC's enemy
//-----------------------------------------------------------------------------
void CFuncTank::CalcNPCEnemyTarget( Vector *pVecTarget )
{
	Vector vecTarget;
	CAI_BaseNPC *pNPC = m_hController->MyNPCPointer();

	// Aim the barrel at the npc's enemy, or where the npc is looking.
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if ( pEnemy )
	{
		// Clear the idle target
		*pVecTarget = pEnemy->BodyTarget( GetAbsOrigin(), false );
		m_vecNPCIdleTarget = *pVecTarget;
	}
	else
	{
		if ( m_vecNPCIdleTarget != vec3_origin )
		{
			*pVecTarget = m_vecNPCIdleTarget;
		}
		else
		{
			Vector vecForward;
			QAngle angCenter( 0, m_yawCenterWorld, 0 );
			AngleVectors( angCenter, &vecForward );
			trace_t tr;
			Vector vecBarrel = GetAbsOrigin() + m_barrelPos;
			UTIL_TraceLine( vecBarrel, vecBarrel + vecForward * 8192, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			*pVecTarget = tr.endpos;
		}
	}
}

	
//-----------------------------------------------------------------------------
// Aim the tank at the NPC's enemy
//-----------------------------------------------------------------------------
void CFuncTank::AimBarrelAtNPCEnemy( QAngle *pAngles )
{
	Vector vecTarget;
	CalcNPCEnemyTarget( &vecTarget );
	*pAngles = AimBarrelAt( m_parentMatrix.WorldToLocal( vecTarget ) );
}

//-----------------------------------------------------------------------------
// Returns true if the desired angles are out of range 
//-----------------------------------------------------------------------------
bool CFuncTank::RotateTankToAngles( const QAngle &angles, float *pDistX, float *pDistY )
{
	bool bClamped = false;

	// Force the angles to be relative to the center position
	float offsetY = UTIL_AngleDistance( angles.y, m_yawCenter );
	float offsetX = UTIL_AngleDistance( angles.x, m_pitchCenter );

	float flActualYaw = m_yawCenter + offsetY;
	float flActualPitch = m_pitchCenter + offsetX;

	if ( ( fabs( offsetY ) > m_yawRange + m_yawTolerance ) ||
		 ( fabs( offsetX ) > m_pitchRange + m_pitchTolerance ) )
	{
		// Limit against range in x
		flActualYaw = clamp( flActualYaw, m_yawCenter - m_yawRange, m_yawCenter + m_yawRange );
		flActualPitch = clamp( flActualPitch, m_pitchCenter - m_pitchRange, m_pitchCenter + m_pitchRange );

		bClamped = true;
	}

	// Get at the angular vel
	QAngle vecAngVel = GetLocalAngularVelocity();

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( flActualYaw, GetLocalAngles().y );
	vecAngVel.y = distY * 10;
	vecAngVel.y = clamp( vecAngVel.y, -m_yawRate, m_yawRate );

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( flActualPitch, GetLocalAngles().x );
	vecAngVel.x = distX  * 10;
	vecAngVel.x = clamp( vecAngVel.x, -m_pitchRate, m_pitchRate );

	// How exciting! We're done
	SetLocalAngularVelocity( vecAngVel );

	if ( pDistX && pDistY )
	{
		*pDistX = distX;
		*pDistY = distY;
	}

	return bClamped;
}


//-----------------------------------------------------------------------------
// We lost our target! 
//-----------------------------------------------------------------------------
void CFuncTank::LostTarget( void )
{
	if (m_fireLast != 0)
	{
		m_OnLoseTarget.FireOutput(this, this);
		m_fireLast = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::ComputeLeadingPosition( const Vector &vecShootPosition, CBaseEntity *pTarget, Vector *pLeadPosition )
{
	Vector vecTarget = pTarget->BodyTarget( vecShootPosition, false );
	float flShotSpeed = GetShotSpeed();
	if ( flShotSpeed == 0 )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	Vector vecVelocity = pTarget->GetSmoothedVelocity();
	vecVelocity.z = 0.0f;
	float flTargetSpeed = VectorNormalize( vecVelocity );

	// Guesstimate...
	if ( m_flNextLeadFactorTime < gpGlobals->curtime )
	{
		m_flStartLeadFactor = m_flNextLeadFactor;
		m_flStartLeadFactorTime = gpGlobals->curtime;
		m_flNextLeadFactor = random->RandomFloat( 0.8f, 1.3f );
		m_flNextLeadFactorTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 4.0f );
	}

	float flFactor = (gpGlobals->curtime - m_flStartLeadFactorTime) / (m_flNextLeadFactorTime - m_flStartLeadFactorTime);
	float flLeadFactor = SimpleSplineRemapVal( flFactor, 0.0f, 1.0f, m_flStartLeadFactor, m_flNextLeadFactor );
	flTargetSpeed *= flLeadFactor;

	Vector vecDelta;
	VectorSubtract( vecShootPosition, vecTarget, vecDelta );
	float flTargetToShooter = VectorNormalize( vecDelta );
	float flCosTheta = DotProduct( vecDelta, vecVelocity );

	// Law of cosines... z^2 = x^2 + y^2 - 2xy cos Theta
	// where z = flShooterToPredictedTargetPosition = flShotSpeed * predicted time
	// x = flTargetSpeed * predicted time
	// y = flTargetToShooter
	// solve for predicted time using at^2 + bt + c = 0, t = (-b +/- sqrt( b^2 - 4ac )) / 2a
	float a = flTargetSpeed * flTargetSpeed - flShotSpeed * flShotSpeed;
	float b = -2.0f * flTargetToShooter * flCosTheta * flTargetSpeed;
	float c = flTargetToShooter * flTargetToShooter;
	
	float flDiscrim = b*b - 4*a*c;
	if (flDiscrim < 0)
	{
		*pLeadPosition = vecTarget;
		return;
	}

	flDiscrim = sqrt(flDiscrim);
	float t = (-b + flDiscrim) / (2.0f * a);
	float t2 = (-b - flDiscrim) / (2.0f * a);
	if ( t < t2 )
	{
		t = t2;
	}

	if ( t <= 0.0f )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	VectorMA( vecTarget, flTargetSpeed * t, vecVelocity, *pLeadPosition );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::AimFuncTankAtTarget( void )
{
	// Get world target position
	CBaseEntity *pTarget = NULL;
	trace_t tr;
	QAngle angles;
	bool bUpdateTime = false;

	CBaseEntity *pTargetVehicle = NULL;
	Vector barrelEnd = WorldBarrelPosition();
	Vector worldTargetPosition;
	if (m_spawnflags & SF_TANK_AIM_AT_POS)
	{
		worldTargetPosition = m_vTargetPosition;
	}
	else
	{
		CBaseEntity *pEntity = (CBaseEntity *)m_hTarget;
		if ( !pEntity || ( pEntity->GetFlags() & FL_NOTARGET ) )
		{
			if( m_targetEntityName != NULL_STRING )
			{
				m_hTarget = FindTarget( m_targetEntityName, NULL );
			}
			
			LostTarget();
			return;
		}

		pTarget = pEntity;

		// Calculate angle needed to aim at target
		worldTargetPosition = pEntity->EyePosition();
		if ( pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = assert_cast<CBasePlayer*>(pEntity);
			pTargetVehicle = pPlayer->GetVehicleEntity();
			if ( pTargetVehicle )
			{
				worldTargetPosition = pTargetVehicle->BodyTarget( GetAbsOrigin(), false );
			}
		}
	}

	float range2 = worldTargetPosition.DistToSqr( barrelEnd );
	if ( !InRange2( range2 ) )
	{
		if ( m_hTarget )
		{
			m_hTarget = NULL;
			LostTarget();
		}
		return;
	}

	Vector vecAimOrigin = m_sightOrigin;
	if (m_spawnflags & SF_TANK_AIM_AT_POS)
	{
		bUpdateTime		= true;
		m_sightOrigin	= m_vTargetPosition;
		vecAimOrigin = m_sightOrigin;
	}
	else
	{
		if ( m_spawnflags & SF_TANK_LINEOFSIGHT )
		{
			AI_TraceLOS( barrelEnd, worldTargetPosition, this, &tr );
		}
		else
		{
			tr.fraction = 1.0f;
			tr.m_pEnt = pTarget;
		}

		// No line of sight, don't track
		if ( tr.fraction == 1.0 || tr.m_pEnt == pTarget || (pTargetVehicle && (tr.m_pEnt == pTargetVehicle)) )
		{
			if ( InRange2( range2 ) && pTarget && pTarget->IsAlive() )
			{
				bUpdateTime = true;

				// Sight position is BodyTarget with no noise (so gun doesn't bob up and down)
				CBaseEntity *pInstance = pTargetVehicle ? pTargetVehicle : pTarget;
				m_hFuncTankTarget = pInstance;

				m_sightOrigin = pInstance->BodyTarget( GetAbsOrigin(), false );
				if ( m_bPerformLeading )
				{
					ComputeLeadingPosition( barrelEnd, pInstance, &vecAimOrigin );
				}
				else
				{
					vecAimOrigin = m_sightOrigin;
				}
			}
		}
	}

	// Convert targetPosition to parent
	Vector vecLocalOrigin = m_parentMatrix.WorldToLocal( vecAimOrigin );
	angles = AimBarrelAt( vecLocalOrigin );

	// FIXME: These need to be the clamped angles
	float distX, distY;
	bool bClamped = RotateTankToAngles( angles, &distX, &distY );
	if ( bClamped )
	{
		bUpdateTime = false;
	}

	if ( bUpdateTime )
	{
		if( (gpGlobals->curtime - m_lastSightTime >= 1.0) && (gpGlobals->curtime > m_flNextAttack) )
		{
			// Enemy was hidden for a while, and I COULD fire right now. Instead, tack a delay on.
			m_flNextAttack = gpGlobals->curtime + 0.5;
		}

		m_lastSightTime = gpGlobals->curtime;
		m_persist2burst = 0;
	}

	SetMoveDoneTime( 0.1 );

	if ( CanFire() && ( ( (fabs(distX) <= m_pitchTolerance) && (fabs(distY) <= m_yawTolerance) ) || (m_spawnflags & SF_TANK_LINEOFSIGHT) ) )
	{
		bool fire = false;
		Vector forward;
		AngleVectors( GetLocalAngles(), &forward );
		forward = m_parentMatrix.ApplyRotation( forward );

		if ( m_spawnflags & SF_TANK_LINEOFSIGHT )
		{
			AI_TraceLine( barrelEnd, pTarget->WorldSpaceCenter(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction == 1.0f || (tr.m_pEnt && tr.m_pEnt == pTarget) )
			{
				fire = true;
			}
		}
		else
		{
			fire = true;
		}

		if ( fire )
		{
			if (m_fireLast == 0)
			{
				m_OnAquireTarget.FireOutput(this, this);
			}
			FiringSequence( barrelEnd, forward, this );
		}
		else 
		{
			LostTarget();
		}
	}
	else 
	{
		LostTarget();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::TrackTarget( void )
{
	QAngle angles;

	if( !m_bReadyToFire && m_flNextAttack <= gpGlobals->curtime )
	{
		m_OnReadyToFire.FireOutput( this, this );
		m_bReadyToFire = true;
	}

	if ( IsPlayerManned() )
	{
		AimBarrelAtPlayerCrosshair( &angles );
		RotateTankToAngles( angles );
		SetNextThink( gpGlobals->curtime + 0.05f );
		SetMoveDoneTime( 0.1 );
		return;
	}

	if ( IsNPCManned() )
	{
		AimBarrelAtNPCEnemy( &angles );
		RotateTankToAngles( angles );
		SetNextThink( gpGlobals->curtime + 0.05f );
		SetMoveDoneTime( 0.1 );
		return;
	}

	if ( !IsActive() )
	{
		// If we're not active, but we're controllable, we need to keep thinking
		if ( IsControllable() && !HasController() )
		{
			// Think to find controllers.
			SetNextThink( m_flNextControllerSearch );
		}
		return;
	}

	// Clean room for unnecessarily complicated old code
	SetNextThink( gpGlobals->curtime + 0.1f );
	AimFuncTankAtTarget();
}


//-----------------------------------------------------------------------------
// Purpose: Start of firing sequence.  By default, just fire now.
// Input  : &barrelEnd - 
//			&forward - 
//			*pAttacker - 
//-----------------------------------------------------------------------------
void CFuncTank::FiringSequence( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	if ( m_fireLast != 0 )
	{
		int bulletCount = (gpGlobals->curtime - m_fireLast) * m_fireRate;
		
		if ( bulletCount > 0 )
		{
			// NOTE: Set m_fireLast first so that Fire can adjust it
			m_fireLast = gpGlobals->curtime;
			Fire( bulletCount, barrelEnd, forward, pAttacker, false );
		}
	}
	else
	{
		m_fireLast = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::DoMuzzleFlash( void )
{
	// If we're parented to something, make it play the muzzleflash
	if ( m_bUsePoseParameters && GetParent() )
	{
		CBaseAnimating *pAnim = GetParent()->GetBaseAnimating();
		pAnim->DoMuzzleFlash();

		// Do the AR2 muzzle flash
		if ( m_iEffectHandling == EH_COMBINE_CANNON )
		{
			CEffectData data;
			data.m_nAttachmentIndex = m_nBarrelAttachment;
			data.m_nEntIndex = pAnim->entindex();
			
			// FIXME: Create a custom entry here!
			DispatchEffect( "ChopperMuzzleFlash", data );
		}
		else
		{
			CEffectData data;
			data.m_nEntIndex = pAnim->entindex();
			data.m_nAttachmentIndex = m_nBarrelAttachment;
			data.m_flScale = 1.0f;
			data.m_fFlags = MUZZLEFLASH_COMBINE;

			DispatchEffect( "MuzzleFlash", data );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CFuncTank::GetTracerType( void )
{
	switch( m_iEffectHandling )
	{
	case EH_AR2:
		return "AR2Tracer";

	case EH_COMBINE_CANNON:
		return "HelicopterTracer";
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire targets and spawn sprites.
// Input  : bulletCount - 
//			barrelEnd - 
//			forward - 
//			pAttacker - 
//-----------------------------------------------------------------------------
void CFuncTank::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	// If we have a specific effect handler, apply it's effects
	if ( m_iEffectHandling == EH_AR2 )
	{
		DoMuzzleFlash();

		// Play the AR2 sound
		EmitSound( "Weapon_functank.Single" );
	}
	else if ( m_iEffectHandling == EH_COMBINE_CANNON )
	{
		DoMuzzleFlash();

		// Play the cannon sound
		EmitSound( "NPC_Combine_Cannon.FireBullet" );
	}
	else
	{
		if ( m_iszSpriteSmoke != NULL_STRING )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteSmoke), barrelEnd, TRUE );
			pSprite->AnimateAndDie( random->RandomFloat( 15.0, 20.0 ) );
			pSprite->SetTransparency( kRenderTransAlpha, m_clrRender->r, m_clrRender->g, m_clrRender->b, 255, kRenderFxNone );

			Vector vecVelocity( 0, 0, random->RandomFloat(40, 80) ); 
			pSprite->SetAbsVelocity( vecVelocity );
			pSprite->SetScale( m_spriteScale );
		}
		if ( m_iszSpriteFlash != NULL_STRING )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteFlash), barrelEnd, TRUE );
			pSprite->AnimateAndDie( 5 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetScale( m_spriteScale );
		}
	}

	if( pAttacker && pAttacker->IsPlayer() )
	{
		if ( IsX360() )
		{
			UTIL_PlayerByIndex(1)->RumbleEffect( RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART | RUMBLE_FLAG_RANDOM_AMPLITUDE );
		}
		else
		{
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY, barrelEnd + forward * 32.0f, 32.0f, 0.2f, pAttacker, SOUNDENT_CHANNEL_WEAPON );
		}
	}


	m_OnFire.FireOutput(this, this);
	m_bReadyToFire = false;
}


void CFuncTank::TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, trace_t &tr )
{
	Vector forward, right, up;

	AngleVectors( GetAbsAngles(), &forward, &right, &up );
	// get circular gaussian spread
	float x, y, z;
	do {
		x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);
	Vector vecDir = vecForward +
		x * vecSpread.x * right +
		y * vecSpread.y * up;
	Vector vecEnd;
	
	vecEnd = vecStart + vecDir * MAX_TRACE_LENGTH;
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
}

	
void CFuncTank::StartRotSound( void )
{
	if ( m_spawnflags & SF_TANK_SOUNDON )
		return;
	m_spawnflags |= SF_TANK_SOUNDON;
	
	if ( m_soundLoopRotate != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );
		filter.MakeReliable();

		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;
		ep.m_pSoundName = (char*)STRING(m_soundLoopRotate);
		ep.m_flVolume = 0.85;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}
	
	if ( m_soundStartRotate != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = (char*)STRING(m_soundStartRotate);
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}
}


void CFuncTank::StopRotSound( void )
{
	if ( m_spawnflags & SF_TANK_SOUNDON )
	{
		if ( m_soundLoopRotate != NULL_STRING )
		{
			StopSound( entindex(), CHAN_STATIC, (char*)STRING(m_soundLoopRotate) );
		}
		if ( m_soundStopRotate != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_BODY;
			ep.m_pSoundName = (char*)STRING(m_soundStopRotate);
			ep.m_flVolume = 1.0f;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}
	}
	m_spawnflags &= ~SF_TANK_SOUNDON;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::IsEntityInViewCone( CBaseEntity *pEntity )
{
	// First check to see if the enemy is in range.
	Vector vecBarrelEnd = WorldBarrelPosition();
	float flRange2 = ( pEntity->GetAbsOrigin() - vecBarrelEnd ).LengthSqr();

	if( !(GetSpawnFlags() & SF_TANK_IGNORE_RANGE_IN_VIEWCONE) )
	{
		if ( !InRange2( flRange2 ) )
			return false;
	}

	// If we're trying to shoot at a player, and we've seen a non-player recently, check the grace period
	if ( m_flPlayerGracePeriod && pEntity->IsPlayer() && (gpGlobals->curtime - m_flLastSawNonPlayer) < m_flPlayerGracePeriod )
	{
		// Grace period is ignored under a certain distance
		if ( flRange2 > m_flIgnoreGraceUpto )
			return false;
	}

	// Check to see if the entity center lies within the yaw and pitch constraints.
	// This isn't horribly accurate, but should do for now.
	QAngle angGun;
	angGun = AimBarrelAt( m_parentMatrix.WorldToLocal( pEntity->GetAbsOrigin() ) );
	
	// Force the angles to be relative to the center position
	float flOffsetY = UTIL_AngleDistance( angGun.y, m_yawCenter );
	float flOffsetX = UTIL_AngleDistance( angGun.x, m_pitchCenter );
	angGun.y = m_yawCenter + flOffsetY;
	angGun.x = m_pitchCenter + flOffsetX;

	if ( ( fabs( flOffsetY ) > m_yawRange + m_yawTolerance ) || ( fabs( flOffsetX ) > m_pitchRange + m_pitchTolerance ) )
		return false;

	// Remember the last time we saw a non-player
	if ( !pEntity->IsPlayer() )
	{
		m_flLastSawNonPlayer = gpGlobals->curtime;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this func tank can see the enemy
//-----------------------------------------------------------------------------
bool CFuncTank::HasLOSTo( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return false;

	// Get the barrel position
	Vector vecBarrelEnd = WorldBarrelPosition();
	Vector vecTarget = pEntity->BodyTarget( GetAbsOrigin(), false );
	trace_t tr;

	// Ignore the func_tank and any prop it's parented to
	CTraceFilterSkipTwoEntities traceFilter( this, GetParent(), COLLISION_GROUP_NONE );

	// UNDONE: Should this hit BLOCKLOS brushes?
	AI_TraceLine( vecBarrelEnd, vecTarget, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr );
	
	CBaseEntity	*pHitEntity = tr.m_pEnt;
	
	// Is entity in a vehicle? if so, verify vehicle is target and return if so (so npc shoots at vehicle)
	CBaseCombatCharacter *pCCEntity = pEntity->MyCombatCharacterPointer();
	if ( pCCEntity != NULL && pCCEntity->IsInAVehicle() )
	{
		// Ok, player in vehicle, check if vehicle is target we're looking at, fire if it is
		// Also, check to see if the owner of the entity is the vehicle, in which case it's valid too.
		// This catches vehicles that use bone followers.
		CBaseEntity	*pVehicle  = pCCEntity->GetVehicle()->GetVehicleEnt();
		if ( pHitEntity == pVehicle || ( pHitEntity != NULL && pHitEntity->GetOwnerEntity() == pVehicle ) )
			return true;
	}

	return ( tr.fraction == 1.0 || tr.m_pEnt == pEntity );
}

// #############################################################################
//   CFuncTankGun
// #############################################################################
class CFuncTankGun : public CFuncTank
{
public:
	DECLARE_CLASS( CFuncTankGun, CFuncTank );

	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );
};
LINK_ENTITY_TO_CLASS( func_tank, CFuncTankGun );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTankGun::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	int i;

	FireBulletsInfo_t info;
	info.m_iShots = 1;
	info.m_vecSrc = barrelEnd;
	info.m_vecDirShooting = forward;
	if ( bIgnoreSpread )
	{
		info.m_vecSpread = gTankSpread[0];
	}
	else
	{
		info.m_vecSpread = gTankSpread[m_spread];
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iTracerFreq = 1;
	info.m_flDamage = m_iBulletDamage;
	info.m_iPlayerDamage = m_iBulletDamageVsPlayer;
	info.m_pAttacker = pAttacker;
	info.m_pAdditionalIgnoreEnt = GetParent();

#ifdef HL2_EPISODIC
	if ( m_iAmmoType != -1 )
	{
		for ( i = 0; i < bulletCount; i++ )
		{
			info.m_iAmmoType = m_iAmmoType;
			FireBullets( info );
		}
	}
#else
	for ( i = 0; i < bulletCount; i++ )
	{
		switch( m_bulletType )
		{
		case TANK_BULLET_SMALL:
			info.m_iAmmoType = m_iSmallAmmoType;
			FireBullets( info );
			break;

		case TANK_BULLET_MEDIUM:
			info.m_iAmmoType = m_iMediumAmmoType;
			FireBullets( info );
			break;

		case TANK_BULLET_LARGE:
			info.m_iAmmoType = m_iLargeAmmoType;
			FireBullets( info );
			break;

		default:
		case TANK_BULLET_NONE:
			break;
		}
	}
#endif // HL2_EPISODIC

	CFuncTank::Fire( bulletCount, barrelEnd, forward, pAttacker, bIgnoreSpread );
}

// #############################################################################
//   CFuncTankPulseLaser
// #############################################################################
class CFuncTankPulseLaser : public CFuncTankGun
{
public:
	DECLARE_CLASS( CFuncTankPulseLaser, CFuncTankGun );
	DECLARE_DATADESC();

	void Precache();
	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );

	float		m_flPulseSpeed;
	float		m_flPulseWidth;
	color32		m_flPulseColor;
	float		m_flPulseLife;
	float		m_flPulseLag;
	string_t	m_sPulseFireSound;
};
LINK_ENTITY_TO_CLASS( func_tankpulselaser, CFuncTankPulseLaser );

BEGIN_DATADESC( CFuncTankPulseLaser )

	DEFINE_KEYFIELD( m_flPulseSpeed,	 FIELD_FLOAT,		"PulseSpeed" ),
	DEFINE_KEYFIELD( m_flPulseWidth,	 FIELD_FLOAT,		"PulseWidth" ),
	DEFINE_KEYFIELD( m_flPulseColor,	 FIELD_COLOR32,		"PulseColor" ),
	DEFINE_KEYFIELD( m_flPulseLife,	 FIELD_FLOAT,		"PulseLife" ),
	DEFINE_KEYFIELD( m_flPulseLag,		 FIELD_FLOAT,		"PulseLag" ),
	DEFINE_KEYFIELD( m_sPulseFireSound, FIELD_SOUNDNAME,	"PulseFireSound" ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CFuncTankPulseLaser::Precache(void)
{
	UTIL_PrecacheOther( "grenade_beam" );

	if ( m_sPulseFireSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_sPulseFireSound) );
	}
	BaseClass::Precache();
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CFuncTankPulseLaser::Fire( int bulletCount, const Vector &barrelEnd, const Vector &vecForward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	// --------------------------------------------------
	//  Get direction vectors for spread
	// --------------------------------------------------
	Vector vecUp = Vector(0,0,1);
	Vector vecRight;
	CrossProduct ( vecForward,  vecUp,		vecRight );	
	CrossProduct ( vecForward, -vecRight,   vecUp  );	

	for ( int i = 0; i < bulletCount; i++ )
	{
		// get circular gaussian spread
		float x, y, z;
		do {
			x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
			y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
			z = x*x+y*y;
		} while (z > 1);

		Vector vecDir = vecForward + x * gTankSpread[m_spread].x * vecRight + y * gTankSpread[m_spread].y * vecUp;

		CGrenadeBeam *pPulse =  CGrenadeBeam::Create( pAttacker, barrelEnd);
		pPulse->Format(m_flPulseColor, m_flPulseWidth);
		pPulse->Shoot(vecDir,m_flPulseSpeed,m_flPulseLife,m_flPulseLag,m_iBulletDamage);

		if ( m_sPulseFireSound != NULL_STRING )
		{
			CPASAttenuationFilter filter( this, 0.6f );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_WEAPON;
			ep.m_pSoundName = (char*)STRING(m_sPulseFireSound);
			ep.m_flVolume = 1.0f;
			ep.m_SoundLevel = SNDLVL_85dB;

			EmitSound( filter, entindex(), ep );
		}

	}
	CFuncTank::Fire( bulletCount, barrelEnd, vecForward, pAttacker, bIgnoreSpread );
}

// #############################################################################
//   CFuncTankLaser
// #############################################################################
class CFuncTankLaser : public CFuncTank
{
	DECLARE_CLASS( CFuncTankLaser, CFuncTank );
public:
	void	Activate( void );
	void	Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );
	void	Think( void );
	CEnvLaser *GetLaser( void );

	DECLARE_DATADESC();

private:
	CEnvLaser	*m_pLaser;
	float	m_laserTime;
	string_t m_iszLaserName;
};
LINK_ENTITY_TO_CLASS( func_tanklaser, CFuncTankLaser );

BEGIN_DATADESC( CFuncTankLaser )

	DEFINE_KEYFIELD( m_iszLaserName, FIELD_STRING, "laserentity" ),

	DEFINE_FIELD( m_pLaser, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_laserTime, FIELD_TIME ),

END_DATADESC()


void CFuncTankLaser::Activate( void )
{
	BaseClass::Activate();

	if ( !GetLaser() )
	{
		UTIL_Remove(this);
		Warning( "Laser tank with no env_laser!\n" );
	}
	else
	{
		m_pLaser->TurnOff();
	}
}


CEnvLaser *CFuncTankLaser::GetLaser( void )
{
	if ( m_pLaser )
		return m_pLaser;

	CBaseEntity *pLaser = gEntList.FindEntityByName( NULL, m_iszLaserName );
	while ( pLaser )
	{
		// Found the landmark
		if ( FClassnameIs( pLaser, "env_laser" ) )
		{
			m_pLaser = (CEnvLaser *)pLaser;
			break;
		}
		else
		{
			pLaser = gEntList.FindEntityByName( pLaser, m_iszLaserName );
		}
	}

	return m_pLaser;
}


void CFuncTankLaser::Think( void )
{
	if ( m_pLaser && (gpGlobals->curtime > m_laserTime) )
		m_pLaser->TurnOff();

	CFuncTank::Think();
}


void CFuncTankLaser::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	int i;
	trace_t tr;

	if ( GetLaser() )
	{
		for ( i = 0; i < bulletCount; i++ )
		{
			m_pLaser->SetLocalOrigin( barrelEnd );
			TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );
			
			m_laserTime = gpGlobals->curtime;
			m_pLaser->TurnOn();
			m_pLaser->SetFireTime( gpGlobals->curtime - 1.0 );
			m_pLaser->FireAtPoint( tr );
			m_pLaser->SetNextThink( TICK_NEVER_THINK );
		}
		CFuncTank::Fire( bulletCount, barrelEnd, forward, this, bIgnoreSpread );
	}
}

class CFuncTankRocket : public CFuncTank
{
public:
	DECLARE_CLASS( CFuncTankRocket, CFuncTank );

	void Precache( void );
	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );
	virtual float GetShotSpeed() { return m_flRocketSpeed; }

protected:
	float	m_flRocketSpeed;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CFuncTankRocket )

	DEFINE_KEYFIELD( m_flRocketSpeed, FIELD_FLOAT, "rocketspeed" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_tankrocket, CFuncTankRocket );

void CFuncTankRocket::Precache( void )
{
	UTIL_PrecacheOther( "rpg_missile" );
	CFuncTank::Precache();
}

void CFuncTankRocket::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	CMissile *pRocket = (CMissile *) CBaseEntity::Create( "rpg_missile", barrelEnd, GetAbsAngles(), this );
	
	pRocket->DumbFire();
	pRocket->SetNextThink( gpGlobals->curtime + 0.1f );
	pRocket->SetAbsVelocity( forward * m_flRocketSpeed );
	if ( GetController() && GetController()->IsPlayer() )
	{
		pRocket->SetDamage( m_iBulletDamage );
	}
	else
	{
		pRocket->SetDamage( m_iBulletDamageVsPlayer );
	}

	CFuncTank::Fire( bulletCount, barrelEnd, forward, this, bIgnoreSpread );
}


//-----------------------------------------------------------------------------
// Airboat gun
//-----------------------------------------------------------------------------
class CFuncTankAirboatGun : public CFuncTank
{
public:
	DECLARE_CLASS( CFuncTankAirboatGun, CFuncTank );
 	DECLARE_DATADESC();

	void Precache( void );
	virtual void Spawn();
	virtual void Activate();
	virtual void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );
	virtual void ControllerPostFrame();
	virtual void OnStopControlled();
	virtual const char *GetTracerType( void );
	virtual Vector WorldBarrelPosition( void );
	virtual void DoImpactEffect( trace_t &tr, int nDamageType );

private:
	void CreateSounds();
	void DestroySounds();
	void DoMuzzleFlash( );
	void StartFiring();
	void StopFiring();

	CSoundPatch *m_pGunFiringSound;
    float		m_flNextHeavyShotTime;
	bool		m_bIsFiring;

	string_t	m_iszAirboatGunModel;
	CHandle<CBaseAnimating> m_hAirboatGunModel;
	int			m_nGunBarrelAttachment;
	float		m_flLastImpactEffectTime;
};


//-----------------------------------------------------------------------------
// Save/load: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CFuncTankAirboatGun )

	DEFINE_SOUNDPATCH( m_pGunFiringSound ),
	DEFINE_FIELD( m_flNextHeavyShotTime,	FIELD_TIME ),
	DEFINE_FIELD( m_bIsFiring,				FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_iszAirboatGunModel,	FIELD_STRING, "airboat_gun_model" ),
//	DEFINE_FIELD( m_hAirboatGunModel,		FIELD_EHANDLE ),
//	DEFINE_FIELD( m_nGunBarrelAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flLastImpactEffectTime,	FIELD_TIME ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_tankairboatgun, CFuncTankAirboatGun );


//-----------------------------------------------------------------------------
// Precache: 
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::Precache( void )
{
	BaseClass::Precache();
	PrecacheScriptSound( "Airboat.FireGunLoop" );
	PrecacheScriptSound( "Airboat.FireGunRevDown");
	CreateSounds();
}


//-----------------------------------------------------------------------------
// Precache: 
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::Spawn( void )
{
	BaseClass::Spawn();
	m_flNextHeavyShotTime = 0.0f;
	m_bIsFiring = false;
	m_flLastImpactEffectTime = -1;
}


//-----------------------------------------------------------------------------
// Attachment indices
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::Activate()
{
	BaseClass::Activate();

	if ( m_iszAirboatGunModel != NULL_STRING )
	{
		m_hAirboatGunModel = dynamic_cast<CBaseAnimating*>( gEntList.FindEntityByName( NULL, m_iszAirboatGunModel ) );
		if ( m_hAirboatGunModel )
		{
			m_nGunBarrelAttachment = m_hAirboatGunModel->LookupAttachment( "muzzle" );
		}
	}
}


//-----------------------------------------------------------------------------
// Create/destroy looping sounds 
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::CreateSounds()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter( this );
	if (!m_pGunFiringSound)
	{
		m_pGunFiringSound = controller.SoundCreate( filter, entindex(), "Airboat.FireGunLoop" );
		controller.Play( m_pGunFiringSound, 0, 100 );
	}
}

void CFuncTankAirboatGun::DestroySounds()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	controller.SoundDestroy( m_pGunFiringSound );
	m_pGunFiringSound = NULL;
}


//-----------------------------------------------------------------------------
// Stop Firing
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::StartFiring()
{
	if ( !m_bIsFiring )
	{
		CSoundEnvelopeController *pController = &CSoundEnvelopeController::GetController();
		float flVolume = pController->SoundGetVolume( m_pGunFiringSound );
		pController->SoundChangeVolume( m_pGunFiringSound, 1.0f, 0.1f * (1.0f - flVolume) );
		m_bIsFiring = true;
	}
}

void CFuncTankAirboatGun::StopFiring()
{
	if ( m_bIsFiring )
	{
		CSoundEnvelopeController *pController = &CSoundEnvelopeController::GetController();
		float flVolume = pController->SoundGetVolume( m_pGunFiringSound );
		pController->SoundChangeVolume( m_pGunFiringSound, 0.0f, 0.1f * flVolume );
		EmitSound( "Airboat.FireGunRevDown" );
		m_bIsFiring = false;
	}
}


//-----------------------------------------------------------------------------
// Maintains airboat gun sounds
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::ControllerPostFrame( void )
{
	if ( IsPlayerManned() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( GetController() );
		if ( pPlayer->m_nButtons & IN_ATTACK )
		{
			StartFiring();
		}
		else
		{
			StopFiring();
		}
	}

	BaseClass::ControllerPostFrame();
}


//-----------------------------------------------------------------------------
// Stop controlled
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::OnStopControlled()
{
	StopFiring();
	BaseClass::OnStopControlled();
}


//-----------------------------------------------------------------------------
// Barrel position
//-----------------------------------------------------------------------------
Vector CFuncTankAirboatGun::WorldBarrelPosition( void )
{
	if ( !m_hAirboatGunModel || (m_nGunBarrelAttachment == 0) )
	{
		return BaseClass::WorldBarrelPosition();
	}

	Vector vecOrigin;
	m_hAirboatGunModel->GetAttachment( m_nGunBarrelAttachment, vecOrigin );
	return vecOrigin;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CFuncTankAirboatGun::GetTracerType( void ) 
{
	if ( gpGlobals->curtime >= m_flNextHeavyShotTime )
		return "AirboatGunHeavyTracer";

	return "AirboatGunTracer"; 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::DoMuzzleFlash( void )
{
	if ( m_hAirboatGunModel && (m_nGunBarrelAttachment != 0) )
	{
		CEffectData data;
		data.m_nEntIndex = m_hAirboatGunModel->entindex();
		data.m_nAttachmentIndex = m_nGunBarrelAttachment;
		data.m_flScale = 1.0f;
		DispatchEffect( "AirboatMuzzleFlash", data );
	}
}


//-----------------------------------------------------------------------------
// Allows the shooter to change the impact effect of his bullets
//-----------------------------------------------------------------------------
void CFuncTankAirboatGun::DoImpactEffect( trace_t &tr, int nDamageType )
{
	// The airboat spits out so much crap that we need to do cheaper versions
	// of the impact effects. Also, we need to do less of them.
	if ( m_flLastImpactEffectTime == gpGlobals->curtime )
		return;

	m_flLastImpactEffectTime = gpGlobals->curtime;
	UTIL_ImpactTrace( &tr, nDamageType, "AirboatGunImpact" );
} 


//-----------------------------------------------------------------------------
// Fires bullets
//-----------------------------------------------------------------------------
#define AIRBOAT_GUN_HEAVY_SHOT_INTERVAL	0.2f

void CFuncTankAirboatGun::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	CAmmoDef *pAmmoDef = GetAmmoDef();
	int ammoType = pAmmoDef->Index( "AirboatGun" );

	FireBulletsInfo_t info;
	info.m_vecSrc = barrelEnd;
	info.m_vecDirShooting = forward;
	info.m_flDistance = 4096;
	info.m_iAmmoType = ammoType;

	if ( gpGlobals->curtime >= m_flNextHeavyShotTime )
	{
		info.m_iShots = 1;
		info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
		info.m_flDamageForceScale = 1000.0f;
	}
	else
	{
		info.m_iShots = 2;
		info.m_vecSpread = VECTOR_CONE_5DEGREES;
	}

	FireBullets( info );

	DoMuzzleFlash();

	// NOTE: This must occur after FireBullets
	if ( gpGlobals->curtime >= m_flNextHeavyShotTime )
	{
		m_flNextHeavyShotTime = gpGlobals->curtime + AIRBOAT_GUN_HEAVY_SHOT_INTERVAL; 
	}
}


//-----------------------------------------------------------------------------
// APC Rocket 
//-----------------------------------------------------------------------------
#define DEATH_VOLLEY_MISSILE_COUNT 10
#define DEATH_VOLLEY_MIN_FIRE_RATE 3
#define DEATH_VOLLEY_MAX_FIRE_RATE 6

class CFuncTankAPCRocket : public CFuncTank
{
public:
	DECLARE_CLASS( CFuncTankAPCRocket, CFuncTank );

	void Precache( void );
	virtual void Spawn();
	virtual void UpdateOnRemove();
	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );
	virtual void Think();
	virtual float GetShotSpeed() { return m_flRocketSpeed; }

protected:
	void InputDeathVolley( inputdata_t &inputdata );
	void FireDying( const Vector &barrelEnd );

	EHANDLE	m_hLaserDot;
	float	m_flRocketSpeed;
	int 	m_nSide;
	int		m_nBurstCount;
	bool	m_bDying;

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CFuncTankAPCRocket )

	DEFINE_KEYFIELD( m_flRocketSpeed, FIELD_FLOAT, "rocketspeed" ),
	DEFINE_FIELD( m_hLaserDot, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nSide, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_nBurstCount, FIELD_INTEGER, "burstcount" ),
	DEFINE_FIELD( m_bDying, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DeathVolley", InputDeathVolley ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_tankapcrocket, CFuncTankAPCRocket );

void CFuncTankAPCRocket::Precache( void )
{
	UTIL_PrecacheOther( "apc_missile" );

	PrecacheScriptSound( "PropAPC.FireCannon" );

	CFuncTank::Precache();
}

void CFuncTankAPCRocket::Spawn( void )
{
	BaseClass::Spawn();
	AddEffects( EF_NODRAW );
	m_nSide = 0;
	m_bDying = false;
	m_hLaserDot = CreateLaserDot( GetAbsOrigin(), this, false );
	m_nBulletCount = m_nBurstCount;
	SetSolid( SOLID_NONE );
	SetLocalVelocity( vec3_origin );
}

void CFuncTankAPCRocket::UpdateOnRemove( void )
{
	if ( m_hLaserDot )
	{
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot = NULL;
	}
	BaseClass::UpdateOnRemove();
}

void CFuncTankAPCRocket::FireDying( const Vector &barrelEnd )
{
	Vector vecDir;
	vecDir.Random( -1.0f, 1.0f );
	if ( vecDir.z < 0.0f )
	{
		vecDir.z *= -1.0f;
	}

	VectorNormalize( vecDir );

	Vector vecVelocity;
	VectorMultiply( vecDir, m_flRocketSpeed * random->RandomFloat( 0.75f, 1.25f ), vecVelocity );

	QAngle angles;
	VectorAngles( vecDir, angles );

	CAPCMissile *pRocket = (CAPCMissile *) CAPCMissile::Create( barrelEnd, angles, vecVelocity, this );
	float flDeathTime = random->RandomFloat( 0.3f, 0.5f );
	if ( random->RandomFloat( 0.0f, 1.0f ) < 0.3f )
	{
		pRocket->ExplodeDelay( flDeathTime );
	}
	else
	{
		pRocket->AugerDelay( flDeathTime );
	}

	// Make erratic firing
	m_fireRate = random->RandomFloat( DEATH_VOLLEY_MIN_FIRE_RATE, DEATH_VOLLEY_MAX_FIRE_RATE ); 
	if ( --m_nBulletCount <= 0 )
	{
		UTIL_Remove( this );
	}
}

void CFuncTankAPCRocket::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	static float s_pSide[] = { 0.966, 0.866, 0.5, -0.5, -0.866, -0.966 };

	Vector vecDir;
	CrossProduct( Vector( 0, 0, 1 ), forward, vecDir );
	vecDir.z = 1.0f;
	vecDir.x *= s_pSide[m_nSide];
	vecDir.y *= s_pSide[m_nSide];
	if ( ++m_nSide >= 6 )
	{
		m_nSide = 0;
	}

	VectorNormalize( vecDir );

	Vector vecVelocity;
	VectorMultiply( vecDir, m_flRocketSpeed, vecVelocity );

	QAngle angles;
	VectorAngles( vecDir, angles );

	CAPCMissile *pRocket = (CAPCMissile *) CAPCMissile::Create( barrelEnd, angles, vecVelocity, this );
	pRocket->IgniteDelay();

	CFuncTank::Fire( bulletCount, barrelEnd, forward, this, bIgnoreSpread );

	if ( --m_nBulletCount <= 0 )
	{
		m_nBulletCount = m_nBurstCount;

		// This will cause it to wait for a little while before shooting
		m_fireLast += random->RandomFloat( 2.0f, 3.0f );
	}
	EmitSound( "PropAPC.FireCannon" );
}

void CFuncTankAPCRocket::Think()
{
	// Inert if we're carried...
	if ( GetMoveParent() && GetMoveParent()->GetMoveParent() )
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
		return;
	}

	BaseClass::Think();
	m_hLaserDot->SetAbsOrigin( m_sightOrigin );
	SetLaserDotTarget( m_hLaserDot, m_hFuncTankTarget );
	EnableLaserDot( m_hLaserDot, m_hFuncTankTarget != NULL );

	if ( m_bDying )
	{
		FireDying( WorldBarrelPosition() );
		return;
	}
}


void CFuncTankAPCRocket::InputDeathVolley( inputdata_t &inputdata )
{
	if ( !m_bDying )
	{
		m_fireRate = random->RandomFloat( DEATH_VOLLEY_MIN_FIRE_RATE, DEATH_VOLLEY_MAX_FIRE_RATE );
		SetNextAttack( gpGlobals->curtime + (1.0f / m_fireRate ) );
		m_nBulletCount = DEATH_VOLLEY_MISSILE_COUNT;
		m_bDying = true;
	}
}


//-----------------------------------------------------------------------------
// Mortar shell
//-----------------------------------------------------------------------------
class CMortarShell : public CBaseEntity
{
public:
	DECLARE_CLASS( CMortarShell, CBaseEntity );

	static CMortarShell *Create( const Vector &vecStart, const Vector &vecTarget, const Vector &vecShotDir, float flImpactDelay, float flWarnDelay, string_t warnSound );

	void	Spawn( void );
	void	Precache( void );
	void	Impact( void );
	void	Warn( void );
	void	FlyThink( void );
	void	FadeThink( void );
	int		UpdateTransmitState( void );

private:

	void		FixUpImpactPoint( const Vector &initialPos, const Vector &initialNormal, Vector *endPos, Vector *endNormal );

	float		m_flFadeTime;
	float		m_flImpactTime;
	float		m_flWarnTime;
	float		m_flNPCWarnTime;
	string_t	m_warnSound;
	int			m_iSpriteTexture;
	bool		m_bHasWarned;
	Vector		m_vecFiredFrom;
	Vector		m_vecFlyDir;
	float		m_flSpawnedTime;

	CHandle<CBeam>	m_pBeamEffect[4];

	CNetworkVar( float, m_flLifespan );
	CNetworkVar( float, m_flRadius );
	CNetworkVar( Vector, m_vecSurfaceNormal );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};

LINK_ENTITY_TO_CLASS( mortarshell, CMortarShell );

BEGIN_DATADESC( CMortarShell )
	DEFINE_FIELD( m_flImpactTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flFadeTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flWarnTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flNPCWarnTime, 	FIELD_TIME ),
	DEFINE_FIELD( m_warnSound,		FIELD_STRING ),
	DEFINE_FIELD( m_iSpriteTexture,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bHasWarned,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flLifespan,		FIELD_FLOAT ),
	DEFINE_FIELD( m_vecFiredFrom,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecFlyDir,		FIELD_VECTOR ),
	DEFINE_FIELD( m_flSpawnedTime,	FIELD_TIME ),
	DEFINE_AUTO_ARRAY( m_pBeamEffect,	FIELD_EHANDLE),
	DEFINE_FIELD( m_flRadius,		FIELD_FLOAT ),
	DEFINE_FIELD( m_vecSurfaceNormal, FIELD_VECTOR ),
	
	DEFINE_FUNCTION( FlyThink ),
	DEFINE_FUNCTION( FadeThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CMortarShell, DT_MortarShell )
	SendPropFloat( SENDINFO( m_flLifespan ), -1, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flRadius ), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecSurfaceNormal ), 0, SPROP_NORMAL ),
END_SEND_TABLE()

#define	MORTAR_TEST_RADIUS	16.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &initialPos - 
//			*endPos - 
//			*endNormal - 
//-----------------------------------------------------------------------------
void CMortarShell::FixUpImpactPoint( const Vector &initialPos, const Vector &initialNormal, Vector *endPos, Vector *endNormal )
{
	Vector vecStartOffset;

	vecStartOffset = initialPos + ( initialNormal * 1.0f );

	trace_t	tr;
	UTIL_TraceLine( vecStartOffset, vecStartOffset - Vector( 0, 0, 256 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0f )
	{
		if ( endPos )
		{
			*endPos = tr.endpos + ( initialNormal * 16.0f );
		}

		if ( endNormal )
		{
			*endNormal = tr.plane.normal;
		}
	}
	else
	{
		if ( endPos )
		{
			*endPos = initialPos;
		}

		if ( endNormal )
		{
			*endNormal = initialNormal;
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
#define MORTAR_BLAST_DAMAGE	50
#define	MORTAR_BLAST_HEIGHT	7500

CMortarShell *CMortarShell::Create( const Vector &vecStart, const Vector &vecTarget, const Vector &vecShotDir, float flImpactDelay, float flWarnDelay, string_t warnSound )
{
	CMortarShell *pShell = (CMortarShell *)CreateEntityByName("mortarshell" );

	// Place the mortar shell at the target location so that it can make the sound and explode.
	trace_t	tr;
	UTIL_TraceLine( vecTarget, vecTarget + ( vecShotDir * 128.0f ), MASK_SOLID_BRUSHONLY, pShell, COLLISION_GROUP_NONE, &tr );

	Vector	targetPos, targetNormal;
	pShell->FixUpImpactPoint( tr.endpos, tr.plane.normal, &targetPos, &targetNormal );

	UTIL_SetOrigin( pShell, targetPos );

	Vector	vecStartSkew, vecEndSkew;

	vecStartSkew = targetPos - vecStart;
	vecStartSkew[2] = 0.0f;
	float skewLength = VectorNormalize( vecStartSkew );

	vecEndSkew = -vecStartSkew * ( skewLength * 0.25f );
	vecStartSkew *= skewLength * 0.1f;

	// Muzzleflash beam
	pShell->m_pBeamEffect[0] = CBeam::BeamCreate( "sprites/laserbeam.vmt", 1 );
	pShell->m_pBeamEffect[0]->PointsInit( vecStart, vecStart + Vector( vecStartSkew[0], vecStartSkew[1], MORTAR_BLAST_HEIGHT ) );
	pShell->m_pBeamEffect[0]->SetColor( 16, 16, 8 );
	pShell->m_pBeamEffect[0]->SetBrightness( 0 );
	pShell->m_pBeamEffect[0]->SetNoise( 0 );
	pShell->m_pBeamEffect[0]->SetBeamFlag( FBEAM_SHADEOUT );
	pShell->m_pBeamEffect[0]->SetWidth( 64.0f );
	pShell->m_pBeamEffect[0]->SetEndWidth( 64.0f );

	pShell->m_pBeamEffect[1] = CBeam::BeamCreate( "sprites/laserbeam.vmt", 1 );
	pShell->m_pBeamEffect[1]->PointsInit( vecStart, vecStart + Vector( vecStartSkew[0], vecStartSkew[1], MORTAR_BLAST_HEIGHT ) );
	pShell->m_pBeamEffect[1]->SetColor( 255, 255, 255 );
	pShell->m_pBeamEffect[1]->SetBrightness( 0 );
	pShell->m_pBeamEffect[1]->SetNoise( 0 );
	pShell->m_pBeamEffect[1]->SetBeamFlag( FBEAM_SHADEOUT );
	pShell->m_pBeamEffect[1]->SetWidth( 8.0f );
	pShell->m_pBeamEffect[1]->SetEndWidth( 8.0f );

	trace_t	skyTrace;
	UTIL_TraceLine( targetPos, targetPos + Vector( vecEndSkew[0], vecEndSkew[1], MORTAR_BLAST_HEIGHT ), MASK_SOLID_BRUSHONLY, pShell, COLLISION_GROUP_NONE, &skyTrace );

	// We must touch the sky to make this beam
	if ( skyTrace.fraction <= 1.0f && skyTrace.surface.flags & SURF_SKY )
	{
		// Impact point beam
		pShell->m_pBeamEffect[2] = CBeam::BeamCreate( "sprites/laserbeam.vmt", 1 );
		pShell->m_pBeamEffect[2]->PointsInit( targetPos, targetPos + Vector( vecEndSkew[0], vecEndSkew[1], MORTAR_BLAST_HEIGHT ) );
		pShell->m_pBeamEffect[2]->SetColor( 16, 16, 8 );
		pShell->m_pBeamEffect[2]->SetBrightness( 0 );
		pShell->m_pBeamEffect[2]->SetNoise( 0 );
		pShell->m_pBeamEffect[2]->SetBeamFlag( FBEAM_SHADEOUT );
		pShell->m_pBeamEffect[2]->SetWidth( 32.0f );
		pShell->m_pBeamEffect[2]->SetEndWidth( 32.0f );

		pShell->m_pBeamEffect[3] = CBeam::BeamCreate( "sprites/laserbeam.vmt", 1 );
		pShell->m_pBeamEffect[3]->PointsInit( targetPos, targetPos + Vector( vecEndSkew[0], vecEndSkew[1], MORTAR_BLAST_HEIGHT ) );
		pShell->m_pBeamEffect[3]->SetColor( 255, 255, 255 );
		pShell->m_pBeamEffect[3]->SetBrightness( 0 );
		pShell->m_pBeamEffect[3]->SetNoise( 0 );
		pShell->m_pBeamEffect[3]->SetBeamFlag( FBEAM_SHADEOUT );
		pShell->m_pBeamEffect[3]->SetWidth( 4.0f );
		pShell->m_pBeamEffect[3]->SetEndWidth( 4.0f );
	}
	else
	{
		// Mark these as not being used
		pShell->m_pBeamEffect[2] = NULL;
		pShell->m_pBeamEffect[3] = NULL;
	}

	pShell->m_vecFiredFrom = vecStart;
	pShell->m_flLifespan = flImpactDelay;
	pShell->m_flImpactTime = gpGlobals->curtime + flImpactDelay;
	pShell->m_flWarnTime = pShell->m_flImpactTime - flWarnDelay;
	pShell->m_flNPCWarnTime = pShell->m_flWarnTime - 0.5;
	pShell->m_warnSound = warnSound;
	pShell->Spawn();

	// Save off the impact normal
	pShell->m_vecSurfaceNormal = targetNormal;
	pShell->m_flRadius = MORTAR_BLAST_RADIUS;

	return pShell;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CMortarShell::Precache()
{
	m_iSpriteTexture = PrecacheModel( "sprites/physbeam.vmt" );

	PrecacheScriptSound( "Weapon_Mortar.Impact" );
	PrecacheMaterial( "effects/ar2ground2" );

	if ( NULL_STRING != m_warnSound )
	{
		PrecacheScriptSound( STRING( m_warnSound ) );
	}
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CMortarShell::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CMortarShell::Spawn()
{
	Precache();

	AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );

	Vector mins( -MORTAR_BLAST_RADIUS, -MORTAR_BLAST_RADIUS, -MORTAR_BLAST_RADIUS );
	Vector maxs(  MORTAR_BLAST_RADIUS,  MORTAR_BLAST_RADIUS,  MORTAR_BLAST_RADIUS );

	UTIL_SetSize( this, mins, maxs );

	m_vecFlyDir = GetAbsOrigin() - m_vecFiredFrom;
	VectorNormalize( m_vecFlyDir );

	m_flSpawnedTime = gpGlobals->curtime;

	SetThink( &CMortarShell::FlyThink );
	SetNextThink( gpGlobals->curtime );

	// No model but we still need to force this!
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			steps - 
//			bias - 
//-----------------------------------------------------------------------------
ConVar curve_bias( "curve_bias", "0.5" );

enum
{
	CURVE_BIAS,
	CURVE_GAIN,
	CURVE_SMOOTH,
	CURVE_SMOOTH_TWEAK,
};

void UTIL_VisualizeCurve( int type, int steps, float bias )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	Vector vForward, vRight, vUp;
	
	pPlayer->EyeVectors( &vForward, &vRight, &vUp );

	Vector	renderOrigin = pPlayer->EyePosition() + ( vForward * 512.0f );

	float renderScale = 8.0f;
	float lastPerc, perc;

	Vector	renderOffs, lastRenderOffs = vec3_origin;

	for ( int i = 0; i < steps; i++ )
	{
		perc = RemapValClamped( i, 0, steps-1, 0.0f, 1.0f );
		
		switch( type )
		{
		case CURVE_BIAS:
			perc = Bias( perc, bias );
			break;

		case CURVE_GAIN:
			perc = Gain( perc, bias );
			break;

		case CURVE_SMOOTH:
			perc = SmoothCurve( perc );
			break;

		case CURVE_SMOOTH_TWEAK:
			perc = SmoothCurve_Tweak( perc, bias, 0.9f );
			break;
		}

		renderOffs = ( vRight * (-steps*0.5f) * renderScale ) + ( vUp * (renderScale*-(steps*0.5f)) )+ ( vRight * i * renderScale ) + ( vUp * perc * (renderScale*steps) );

		NDebugOverlay::Cross3D( renderOrigin + renderOffs, -Vector(2,2,2), Vector(2,2,2), 255, 0, 0, true, 0.05f );

		if ( i > 0 )
		{
			NDebugOverlay::Line( renderOrigin + renderOffs, renderOrigin + lastRenderOffs, 255, 0, 0, true, 0.05f );
		}

		lastRenderOffs = renderOffs;
		lastPerc = perc;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CMortarShell::FlyThink()
{
	SetNextThink( gpGlobals->curtime + 0.05 );

	if ( gpGlobals->curtime > m_flNPCWarnTime )
	{
		// Warn the AI. Make this radius a little larger than the explosion will be, and make the sound last a little longer.
		CSoundEnt::InsertSound ( SOUND_DANGER | SOUND_CONTEXT_MORTAR, GetAbsOrigin(), MORTAR_BLAST_RADIUS * 1.25, (m_flImpactTime - m_flNPCWarnTime) + 0.15 );
		m_flNPCWarnTime = FLT_MAX;
	}

	//UTIL_VisualizeCurve( CURVE_GAIN, 64, curve_bias.GetFloat() );

	float lifePerc = 1.0f - ( ( m_flImpactTime - gpGlobals->curtime ) / ( m_flImpactTime - m_flSpawnedTime ) );

	lifePerc = clamp( lifePerc, 0.0f, 1.0f );
	
	float curve1 = Bias( lifePerc, 0.75f );

	// Beam updates START

	m_pBeamEffect[0]->SetBrightness( 255 * curve1 );
	m_pBeamEffect[0]->SetWidth( 64.0f * curve1 );
	m_pBeamEffect[0]->SetEndWidth( 64.0f * curve1 );

	m_pBeamEffect[1]->SetBrightness( 255 * curve1 );
	m_pBeamEffect[1]->SetWidth( 8.0f * curve1 );
	m_pBeamEffect[1]->SetEndWidth( 8.0f * curve1 );

	float curve2 = Bias( lifePerc, 0.1f );

	if ( m_pBeamEffect[2] )
	{
		m_pBeamEffect[2]->SetBrightness( 255 * curve2 );
		m_pBeamEffect[2]->SetWidth( 32.0f * curve2 );
		m_pBeamEffect[2]->SetEndWidth( 32.0f * curve2 );
	}

	if ( m_pBeamEffect[3] )
	{
		m_pBeamEffect[3]->SetBrightness( 255 * curve2 );
		m_pBeamEffect[3]->SetWidth( 8.0f * curve2 );
		m_pBeamEffect[3]->SetEndWidth( 8.0f * curve2 );
	}

	// Beam updates END
		 
	if( !m_bHasWarned && gpGlobals->curtime > m_flWarnTime )
	{
		Warn();
	}

	if( gpGlobals->curtime > m_flImpactTime )
	{
		Impact();
	}

}

//---------------------------------------------------------
//---------------------------------------------------------
void CMortarShell::Warn( void )
{
	if ( m_warnSound != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_WEAPON;
		ep.m_pSoundName = (char*)STRING(m_warnSound);
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NONE;

		EmitSound( filter, entindex(), ep );
	}

	m_bHasWarned = true;
}		

//---------------------------------------------------------
//---------------------------------------------------------
void CMortarShell::Impact( void )
{
	// Fire the bullets
	Vector vecSrc, vecShootDir;

	float flRadius = MORTAR_BLAST_RADIUS;

	trace_t	tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 128 ), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	UTIL_DecalTrace( &tr, "Scorch" );

	// Send the effect over
	CEffectData	data;

	// Do an extra effect if we struck the world
	if ( tr.m_pEnt && tr.m_pEnt->IsWorld() )
	{
		data.m_flRadius = flRadius * 0.5f;
		data.m_vNormal	= tr.plane.normal;
		data.m_vOrigin	= tr.endpos;
		
		DispatchEffect( "AR2Explosion", data );
	}

	//Shockring
	CBroadcastRecipientFilter filter2;
	te->BeamRingPoint( filter2, 0, GetAbsOrigin(),	//origin
		8.0f,	//start radius
		flRadius * 2,		//end radius
		m_iSpriteTexture, //texture
		0,			//halo index
		0,			//start frame
		2,			//framerate
		0.2f,		//life
		32,			//width
		0,			//spread
		0,			//amplitude
		255,	//r
		255,	//g
		225,	//b
		32,		//a
		0,		//speed
		FBEAM_FADEOUT
		);

	//Shockring
	te->BeamRingPoint( filter2, 0, GetAbsOrigin(),	//origin
		8.0f,	//start radius
		flRadius,	//end radius
		m_iSpriteTexture, //texture
		0,			//halo index
		0,			//start frame
		2,			//framerate
		0.2f,		//life
		64,			//width
		0,			//spread
		0,			//amplitude
		255,	//r
		255,	//g
		225,	//b
		64,		//a
		0,		//speed
		FBEAM_FADEOUT
		);

	RadiusDamage( CTakeDamageInfo( this, GetOwnerEntity(), MORTAR_BLAST_DAMAGE, (DMG_BLAST|DMG_DISSOLVE) ), GetAbsOrigin(), MORTAR_BLAST_RADIUS, CLASS_NONE, NULL );

	EmitSound( "Weapon_Mortar.Impact" );

	UTIL_ScreenShake( GetAbsOrigin(), 10, 60, 1.0, 550, SHAKE_START, false );

	//Fade the beams over time!
	m_flFadeTime = gpGlobals->curtime;

	SetThink( &CMortarShell::FadeThink );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

#define	MORTAR_FADE_LENGTH 1.0f

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMortarShell::FadeThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.05f );

	float lifePerc = 1.0f - ( ( gpGlobals->curtime - m_flFadeTime  ) / MORTAR_FADE_LENGTH );

	lifePerc = clamp( lifePerc, 0.0f, 1.0f );
	
	float curve1 = Bias( lifePerc, 0.1f );

	// Beam updates START

	m_pBeamEffect[0]->SetBrightness( 255 * curve1 );
	m_pBeamEffect[0]->SetWidth( 64.0f * curve1 );
	m_pBeamEffect[0]->SetEndWidth( 64.0f * curve1 );

	m_pBeamEffect[1]->SetBrightness( 255 * curve1 );
	m_pBeamEffect[1]->SetWidth( 8.0f * curve1 );
	m_pBeamEffect[1]->SetEndWidth( 8.0f * curve1 );

	float curve2 = Bias( lifePerc, 0.25f );

	if ( m_pBeamEffect[2] )
	{
		m_pBeamEffect[2]->SetBrightness( 255 * curve2 );
		m_pBeamEffect[2]->SetWidth( 32.0f * curve2 );
		m_pBeamEffect[2]->SetEndWidth( 32.0f * curve2 );
	}

	if ( m_pBeamEffect[3] )
	{
		m_pBeamEffect[3]->SetBrightness( 255 * curve2 );
		m_pBeamEffect[3]->SetWidth( 8.0f * curve2 );
		m_pBeamEffect[3]->SetEndWidth( 8.0f * curve2 );
	}

	// Beam updates END

	if ( gpGlobals->curtime > ( m_flFadeTime + MORTAR_FADE_LENGTH ) )
	{
		UTIL_Remove( m_pBeamEffect[0] );
		UTIL_Remove( m_pBeamEffect[1] );
		UTIL_Remove( m_pBeamEffect[2] );
		UTIL_Remove( m_pBeamEffect[3] );

		SetThink(NULL);
		UTIL_Remove( this );
	}
}

//=========================================================
//=========================================================
class CFuncTankMortar : public CFuncTank
{
public:
	DECLARE_CLASS( CFuncTankMortar, CFuncTank );

	CFuncTankMortar() { m_fLastShotMissed = false; }

	void Precache( void );
	void FiringSequence( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker );
	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &vecForward, CBaseEntity *pAttacker, bool bIgnoreSpread );
	void ShootGun(void);
	void Spawn();
	void SetNextAttack( float flWait );
	
	// Input handlers.
	void InputShootGun( inputdata_t &inputdata );
	void InputFireAtWill( inputdata_t &inputdata );

	DECLARE_DATADESC();

	int			m_Magnitude;
	float		m_fireDelay;
	string_t	m_fireStartSound;
	//string_t	m_fireEndSound;

	string_t	m_incomingSound;
	float		m_flWarningTime;
	float		m_flFireVariance;

	bool		m_fLastShotMissed;

	// store future firing event
	CBaseEntity *m_pAttacker;
};

LINK_ENTITY_TO_CLASS( func_tankmortar, CFuncTankMortar );

BEGIN_DATADESC( CFuncTankMortar )

	DEFINE_KEYFIELD( m_Magnitude, FIELD_INTEGER, "iMagnitude" ),
	DEFINE_KEYFIELD( m_fireDelay, FIELD_FLOAT, "firedelay" ),
	DEFINE_KEYFIELD( m_fireStartSound, FIELD_STRING, "firestartsound" ),
	//DEFINE_KEYFIELD( m_fireEndSound, FIELD_STRING, "fireendsound" ),
	DEFINE_KEYFIELD( m_incomingSound, FIELD_STRING, "incomingsound" ),
	DEFINE_KEYFIELD( m_flWarningTime, FIELD_TIME, "warningtime" ),
	DEFINE_KEYFIELD( m_flFireVariance, FIELD_TIME, "firevariance" ),

	DEFINE_FIELD( m_fLastShotMissed, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_pAttacker, FIELD_CLASSPTR ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "ShootGun", InputShootGun ),
	DEFINE_INPUTFUNC( FIELD_VOID, "FireAtWill", InputFireAtWill ),
END_DATADESC()


void CFuncTankMortar::Spawn()
{
	BaseClass::Spawn();

	m_takedamage = DAMAGE_NO;
}

void CFuncTankMortar::Precache( void )
{
	if ( m_fireStartSound != NULL_STRING )
		PrecacheScriptSound( STRING(m_fireStartSound) );
	//if ( m_fireEndSound != NULL_STRING )
	//	PrecacheScriptSound( STRING(m_fireEndSound) );
	if ( m_incomingSound != NULL_STRING )
		PrecacheScriptSound( STRING(m_incomingSound) );
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CFuncTankMortar::SetNextAttack( float flWait )
{
	if ( m_flFireVariance > 0.09 )
		flWait += random->RandomFloat( -m_flFireVariance, m_flFireVariance );
	BaseClass::SetNextAttack( flWait );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler to make the tank shoot.
//-----------------------------------------------------------------------------
void CFuncTankMortar::InputShootGun( inputdata_t &inputdata )
{
	ShootGun();
}

//-----------------------------------------------------------------------------
// This mortar can fire the next round as soon as it is ready. This is not a 
// 'sticky' state, it just allows us to get the next shot off as soon as the 
// tank is on target. great for scripted applications where you need a shot as
// soon as you can get it.
//-----------------------------------------------------------------------------
void CFuncTankMortar::InputFireAtWill( inputdata_t &inputdata )
{
	SetNextAttack( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTankMortar::ShootGun( void )
{
	Vector forward;
	AngleVectors( GetLocalAngles(), &forward );
	UpdateMatrix();
	forward = m_parentMatrix.ApplyRotation( forward );

	Fire( 1, WorldBarrelPosition(), forward, m_pAttacker, false );
}


void CFuncTankMortar::FiringSequence( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker )
{
	if ( gpGlobals->curtime > GetNextAttack() )
	{
		ShootGun();
		m_fireLast = gpGlobals->curtime;
		SetNextAttack( gpGlobals->curtime + (1.0 / m_fireRate ) );
	}
	else
	{
		m_fireLast = gpGlobals->curtime;
	}
}	

void CFuncTankMortar::Fire( int bulletCount, const Vector &barrelEnd, const Vector &vecForward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	Vector vecProjectedPosition = vec3_invalid;
	trace_t tr;

	if ( m_hTarget )
	{
		float leadTime = (m_fireDelay * 1.1);

		if ( m_hTarget->IsNPC() ) // Give NPCs a little extra grace
			leadTime = 1.25;

		Vector vLead = m_hTarget->GetSmoothedVelocity() * leadTime;
		Vector vNoise;

		vecProjectedPosition = m_hTarget->WorldSpaceCenter() + vLead;
		vNoise.AsVector2D().Random( -6*12, 6*12);
		vNoise.z = 0;
		
		if( m_hTarget->Classify() != CLASS_BULLSEYE )
		{
			// Don't apply noise when attacking a bullseye.
			vecProjectedPosition += vNoise;
		}
	}
	else if ( IsPlayerManned() )
	{
		CalcPlayerCrosshairTarget( &vecProjectedPosition );
	}
	else if ( IsNPCManned() )
	{
		CalcNPCEnemyTarget(  &vecProjectedPosition );
		//vecProjectedPosition += GetEnemy()->GetSmoothedVelocity() * (m_fireDelay * 1.1);
	}
	else
		return;

	#define TARGET_SEARCH_DEPTH 100

	// find something interesting to shoot at near the projected position. 
	Vector delta;

	// Make a really rough approximation of the last half of the mortar trajectory and trace it. 
	// Do this so that mortars fired into windows land on rooftops, and that targets projected 
	// inside buildings (or out of the world) clip to the world. (usually a building facade)
	
	// Find halfway between the mortar and the target.
	Vector vecSpot = ( vecProjectedPosition + GetAbsOrigin() ) * 0.5;
	vecSpot.z = GetAbsOrigin().z;
	
	// Trace up to find the fake 'apex' of the shell. The skybox or 1024 units, whichever comes first. 
	UTIL_TraceLine( vecSpot, vecSpot + Vector(0, 0, 1024), MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );
	vecSpot = tr.endpos;

	//NDebugOverlay::Line( tr.startpos, tr.endpos, 0,255,0, false, 5 );

	// Now trace from apex to target
	UTIL_TraceLine( vecSpot, vecProjectedPosition, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	if( mortar_visualize.GetBool() )
	{
		NDebugOverlay::Line( tr.startpos, tr.endpos, 255,0,0, false, 5 );
	}

	if ( m_fireStartSound != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_WEAPON;
		ep.m_pSoundName = (char*)STRING(m_fireStartSound);
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NONE;

		EmitSound( filter, entindex(), ep );
	}

	Vector vecFinalDir = tr.endpos - tr.startpos;
	VectorNormalize( vecFinalDir );

	CMortarShell::Create( barrelEnd, tr.endpos, vecFinalDir, m_fireDelay, m_flWarningTime, m_incomingSound );
	BaseClass::Fire( bulletCount, barrelEnd, vecForward, this, bIgnoreSpread );
}

//-----------------------------------------------------------------------------
// Purpose: Func tank that fires physics cannisters placed on it
//-----------------------------------------------------------------------------
class CFuncTankPhysCannister : public CFuncTank
{
public:
	DECLARE_CLASS( CFuncTankPhysCannister, CFuncTank );
	DECLARE_DATADESC();

	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );

protected:
	string_t				m_iszBarrelVolume;
	CHandle<CBaseTrigger>	m_hBarrelVolume;
};

LINK_ENTITY_TO_CLASS( func_tankphyscannister, CFuncTankPhysCannister );

BEGIN_DATADESC( CFuncTankPhysCannister )

	DEFINE_KEYFIELD( m_iszBarrelVolume, FIELD_STRING, "barrel_volume" ),
	DEFINE_FIELD( m_hBarrelVolume, FIELD_EHANDLE ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTankPhysCannister::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	// Find our barrel volume
	if ( !m_hBarrelVolume )
	{
		if ( m_iszBarrelVolume != NULL_STRING )
		{
			m_hBarrelVolume = dynamic_cast<CBaseTrigger*>( gEntList.FindEntityByName( NULL, m_iszBarrelVolume ) );
		}

		if ( !m_hBarrelVolume )
		{
			Msg("ERROR: Couldn't find barrel volume for func_tankphyscannister %s.\n", STRING(GetEntityName()) );
			return;
		}
	}

	// Do we have a cannister in our barrel volume?
	CPhysicsCannister *pCannister = (CPhysicsCannister *)m_hBarrelVolume->GetTouchedEntityOfType( "physics_cannister" );
	if ( !pCannister )
	{
		// Play a no-ammo sound
		return;
	}

	// Fire the cannister!
	pCannister->CannisterFire( pAttacker );
}

//=========================================================
//=========================================================
static const char *s_pUpdateBeamThinkContext = "UpdateBeamThinkContext";
#define COMBINE_CANNON_BEAM "effects/blueblacklargebeam.vmt"
//#define COMBINE_CANNON_BEAM "sprites/strider_bluebeam.vmt"

class CFuncTankCombineCannon : public CFuncTankGun
{
	DECLARE_CLASS( CFuncTankCombineCannon, CFuncTankGun );

	void Precache();
	void Spawn();
	void CreateBeam();
	void DestroyBeam();
	void FuncTankPostThink();
	void AdjustRateOfFire();
	void UpdateBeamThink( void );
	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );
	void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	void TankDeactivate();

	void InputSetTargetEntity( inputdata_t &inputdata );
	void InputClearTargetEntity( inputdata_t &inputdata );

	void InputEnableHarrass( inputdata_t &inputdata );
	void InputDisableHarrass( inputdata_t &inputdata );

	COutputEvent m_OnShotAtPlayer;

	CHandle<CBeam>	m_hBeam;

	DECLARE_DATADESC();

private:
	float	m_originalFireRate;
	float	m_flTimeNextSweep;
	float	m_flTimeBeamOn;
	Vector	m_vecTrueForward;
	bool	m_bShouldHarrass;
	bool	m_bLastTargetWasNPC; // Tells whether the last entity we fired a shot at was an NPC (otherwise it was the player)
};

BEGIN_DATADESC( CFuncTankCombineCannon )
	DEFINE_FIELD( m_originalFireRate, FIELD_FLOAT ),
	DEFINE_THINKFUNC( UpdateBeamThink ),
	DEFINE_FIELD( m_flTimeNextSweep, FIELD_TIME ),
	DEFINE_FIELD( m_flTimeBeamOn, FIELD_TIME ),
	DEFINE_FIELD( m_hBeam, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecTrueForward, FIELD_VECTOR ),
	DEFINE_FIELD( m_bShouldHarrass, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLastTargetWasNPC, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableHarrass", InputEnableHarrass ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableHarrass", InputDisableHarrass ),

	DEFINE_OUTPUT( m_OnShotAtPlayer, "OnShotAtPlayer" ),

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::Precache()
{
	m_originalFireRate = m_fireRate;

	PrecacheModel(COMBINE_CANNON_BEAM);
	PrecacheParticleSystem( "Weapon_Combine_Ion_Cannon" );
	
	BaseClass::Precache();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::Spawn()
{
	BaseClass::Spawn();
	m_flTimeBeamOn = gpGlobals->curtime;
	CreateBeam();

	m_bShouldHarrass = true;

	GetVectors( &m_vecTrueForward, NULL, NULL );
	m_bLastTargetWasNPC = false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::CreateBeam()
{
	if (!m_hBeam && gpGlobals->curtime >= m_flTimeBeamOn )
	{
		m_hBeam = CBeam::BeamCreate( COMBINE_CANNON_BEAM, 1.0f );
		m_hBeam->SetColor( 255, 255, 255 );
		SetContextThink( &CFuncTankCombineCannon::UpdateBeamThink, gpGlobals->curtime, s_pUpdateBeamThinkContext );
	}
	else
	{
		// Beam seems to be on, or I'm not supposed to have it on at the moment.
		return;
	}

	Vector vecInitialAim;

	AngleVectors( GetAbsAngles(), &vecInitialAim, NULL, NULL );

	m_hBeam->PointsInit( WorldBarrelPosition(), WorldBarrelPosition() + vecInitialAim );
	m_hBeam->SetBrightness( 255 );
	m_hBeam->SetNoise( 0 );
	m_hBeam->SetWidth( 3.0f );
	m_hBeam->SetEndWidth( 0 );
	m_hBeam->SetScrollRate( 0 );
	m_hBeam->SetFadeLength( 60 ); // five feet to fade out
	//m_hBeam->SetHaloTexture( sHaloSprite );
	m_hBeam->SetHaloScale( 4.0f );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::DestroyBeam()
{
	if( m_hBeam )
	{
		UTIL_Remove( m_hBeam );
		m_hBeam.Set(NULL);
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::AdjustRateOfFire()
{
	// Maintain 1.5 rounds per second rate of fire.
	m_fireRate = 1.5;
/*
	if( m_hTarget.Get() != NULL && m_hTarget->IsPlayer() )
	{
		if( m_bLastTargetWasNPC )
		{
			// Cheat, and be able to fire RIGHT NOW if the target is a player and the 
			// last target I fired at was an NPC. This prevents the player from running
			// for it while the gun is busy dealing with NPCs
			SetNextAttack( gpGlobals->curtime );
		}
	}
*/
}

//---------------------------------------------------------
//---------------------------------------------------------
#define COMBINE_CANNON_BEAM_MAX_DIST	1900.0f
void CFuncTankCombineCannon::UpdateBeamThink()
{
	SetContextThink( &CFuncTankCombineCannon::UpdateBeamThink, gpGlobals->curtime + 0.025, s_pUpdateBeamThinkContext );

	// Always try to create the beam.
	CreateBeam();

	if( !m_hBeam )
		return;

	trace_t trBeam;
	trace_t trShot;
	trace_t trBlockLOS;

	Vector vecBarrel = WorldBarrelPosition();
	Vector vecAim;
	AngleVectors( GetAbsAngles(), &vecAim, NULL, NULL );

	AI_TraceLine( vecBarrel, vecBarrel + vecAim * COMBINE_CANNON_BEAM_MAX_DIST, MASK_SHOT, this, COLLISION_GROUP_NONE, &trBeam );

	m_hBeam->SetStartPos( trBeam.startpos );
	m_hBeam->SetEndPos( trBeam.endpos );

	if( !(m_spawnflags & SF_TANK_AIM_AT_POS) )
	{
		SetTargetPosition( trBeam.endpos );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::FuncTankPostThink()
{
	AdjustRateOfFire();

	if( m_hTarget.Get() == NULL )
	{
		if( gpGlobals->curtime > m_flTimeNextSweep )
		{
			AddSpawnFlags( SF_TANK_AIM_AT_POS );

			Vector vecTargetPosition = GetTargetPosition();
			CBasePlayer *pPlayer = AI_GetSinglePlayer();
			Vector vecToPlayer = pPlayer->WorldSpaceCenter() - GetAbsOrigin();
			vecToPlayer.NormalizeInPlace();

			bool bHarass = false;
			float flDot = DotProduct( m_vecTrueForward, vecToPlayer );

			if( flDot >= 0.9f && m_bShouldHarrass )
			{
				//Msg("%s Harrassing player\n", GetDebugName() );
				vecTargetPosition = pPlayer->EyePosition();
				bHarass = true;
			}
			else
			{
				//Msg( "%s Bored\n", GetDebugName() );
				// Just point off in the distance, more or less directly ahead of me.
				vecTargetPosition = GetAbsOrigin() + m_vecTrueForward * 1900.0f;
			}

			int i;
			Vector vecTest;
			bool bFoundPoint = false;
			for( i = 0 ; i < 5 ; i++ )
			{
				vecTest = vecTargetPosition;

				if( bHarass )
				{
					vecTest.x += random->RandomFloat( -48, 48 );
					vecTest.y += random->RandomFloat( -48, 48 );
					vecTest.z += random->RandomFloat( 16, 48 );
				}
				else
				{
					vecTest.x += random->RandomFloat( -48, 48 );
					vecTest.y += random->RandomFloat( -48, 48 );
					vecTest.z += random->RandomFloat( -48, 48 );
				}

				// Get the barrel position
				Vector vecBarrelEnd = WorldBarrelPosition();
				trace_t trLOS;
				trace_t trShoot;

				// Ignore the func_tank and any prop it's parented to, and check line of sight to the point
				// Trace to the point. If an opaque trace doesn't reach the point, that means the beam hit
				// something closer, (including a blockLOS), so try again.
				CTraceFilterSkipTwoEntities traceFilter( this, GetParent(), COLLISION_GROUP_NONE );
				AI_TraceLine( vecBarrelEnd, vecTest, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &trLOS );
				AI_TraceLine( vecBarrelEnd, vecTest, MASK_SHOT, &traceFilter, &trShoot );

				if( trLOS.fraction < trShoot.fraction )
				{
					// Damn block LOS brushes.
					continue;
				}

				//Msg("Point is visible in %d tries\n", i);
				bFoundPoint = true;
				break;
			}

			if( bFoundPoint )
			{
				vecTargetPosition = vecTest;
				SetTargetPosition( vecTargetPosition );
				//Msg("New place\n");
			}

			if( bHarass )
			{
				m_flTimeNextSweep = gpGlobals->curtime + random->RandomFloat( 0.25f, 0.75f );
			}
			else
			{
				m_flTimeNextSweep = gpGlobals->curtime + random->RandomFloat( 1, 3 );
			}
		}
	}
	else
	{
		//Msg("%d engaging: %s\n", entindex(), m_hTarget->GetClassname() );
		RemoveSpawnFlags( SF_TANK_AIM_AT_POS );
	}
}

//---------------------------------------------------------
// A normal func_tank uses a method of aiming the gun that will
// always follow a fast-moving player. This is because the func_tank
// turns the weapon by applying angular velocities in the early 
// phase of the func_tank's Think(). Because the bullet is fired 
// later in the same think, it is fired before the game physics have
// updated the func_tank's angles using the newly-computed angular
// velocity, so the bullet always trails the target slightly. 
// This is unacceptable for the Combine Cannon, as the cannon MUST
// strike a moving player with absolute certainty. As a quick 
// remedy, this code allows the combine cannon to fire a bullet
// at a slightly different angle than the gun is aiming, to 
// ensure a hit. Large discrepancies are ignored and we accept 
// the miss instead of presenting a bullet fired at an obviously
// adjusted angle.
//---------------------------------------------------------
void CFuncTankCombineCannon::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread )
{
	// Specifically do NOT fire in aim at pos mode. This is just for show.
	if( HasSpawnFlags(SF_TANK_AIM_AT_POS) )
		return;

	Vector vecAdjustedForward = forward;

	if( m_hTarget != NULL )
	{
		Vector vecToTarget = m_hTarget->BodyTarget( barrelEnd, false ) - barrelEnd;
		VectorNormalize( vecToTarget );

		float flDot = DotProduct( vecToTarget, forward );

		if( flDot >= 0.97 )
		{
			vecAdjustedForward = vecToTarget;
		}

		if( m_hTarget->IsNPC() )
			m_bLastTargetWasNPC = true;
		else
			m_bLastTargetWasNPC = false;

		if( m_hTarget->IsPlayer() )
			m_OnShotAtPlayer.FireOutput( this, this );
	}

	BaseClass::Fire( bulletCount, barrelEnd, vecAdjustedForward, pAttacker, bIgnoreSpread );

	// Turn off the beam and tell it to stay off for a bit. We want it to look like the beam became the
	// ion cannon 'rail gun' effect.
	DestroyBeam();
	m_flTimeBeamOn = gpGlobals->curtime + 0.2f;

	m_flTimeNextSweep = gpGlobals->curtime + random->RandomInt( 1.0f, 2.0f );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	// If the shot passed near the player, shake the screen.
	if( AI_IsSinglePlayer() )
	{
		Vector vecPlayer = AI_GetSinglePlayer()->EyePosition();

		Vector vecNearestPoint = PointOnLineNearestPoint( vecTracerSrc, tr.endpos, vecPlayer );

		float flDist = vecPlayer.DistTo( vecNearestPoint );

		if( flDist >= 10.0f && flDist <= 120.0f )
		{
			// Don't shake the screen if we're hit (within 10 inches), but do shake if a shot otherwise comes within 10 feet.
			UTIL_ScreenShake( vecNearestPoint, 10, 60, 0.3, 120.0f, SHAKE_START, false );
		}
	}

	// Send the railgun effect
	DispatchParticleEffect( "Weapon_Combine_Ion_Cannon", vecTracerSrc, tr.endpos, vec3_angle, NULL );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::TankDeactivate()
{
	DestroyBeam();
	m_flTimeBeamOn = gpGlobals->curtime + 1.0f;
	SetContextThink( NULL, 0, s_pUpdateBeamThinkContext );

	BaseClass::TankDeactivate();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::InputSetTargetEntity( inputdata_t &inputdata )
{
	BaseClass::InputSetTargetEntity( inputdata );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::InputClearTargetEntity( inputdata_t &inputdata )
{
/*
	m_targetEntityName = NULL_STRING;
	m_hTarget = NULL;

	// No longer aim at target position if have one
	m_spawnflags &= ~SF_TANK_AIM_AT_POS; 
*/
	BaseClass::InputClearTargetEntity( inputdata );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::InputEnableHarrass( inputdata_t &inputdata )
{
	m_bShouldHarrass = true;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CFuncTankCombineCannon::InputDisableHarrass( inputdata_t &inputdata )
{
	m_bShouldHarrass = false;
}


LINK_ENTITY_TO_CLASS( func_tank_combine_cannon, CFuncTankCombineCannon );
