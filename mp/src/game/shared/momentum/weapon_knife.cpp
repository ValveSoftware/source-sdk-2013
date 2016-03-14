//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_knife.h"
#include "mom_player_shared.h"

#ifndef CLIENT_DLL 
	#include "ilagcompensationmanager.h"
#endif


#define	KNIFE_BODYHIT_VOLUME 128
#define	KNIFE_WALLHIT_VOLUME 512


Vector head_hull_mins( -16, -16, -18 );
Vector head_hull_maxs( 16, 16, 18 );

#ifndef CLIENT_DLL
	//-----------------------------------------------------------------------------
	// Purpose: Only send to local player if this weapon is the active weapon
	// Input  : *pStruct - 
	//			*pVarData - 
	//			*pRecipients - 
	//			objectID - 
	// Output : void*
	//-----------------------------------------------------------------------------
	void* SendProxy_SendActiveLocalKnifeDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
	{
		// Get the weapon entity
		CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pVarData;
		if ( pWeapon )
		{
			// Only send this chunk of data to the player carrying this weapon
			CBasePlayer *pPlayer = ToBasePlayer( pWeapon->GetOwner() );
			if ( pPlayer /*&& pPlayer->GetActiveWeapon() == pWeapon*/ )
			{
				pRecipients->SetOnly( pPlayer->GetClientIndex() );
				return (void*)pVarData;
			}
		}
		
		return NULL;
	}
	REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendActiveLocalKnifeDataTable );
#endif

// ----------------------------------------------------------------------------- //
// CKnife tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( Knife, DT_WeaponKnife )

BEGIN_NETWORK_TABLE_NOBASE( CKnife, DT_LocalActiveWeaponKnifeData )
	#if !defined( CLIENT_DLL )
		SendPropTime( SENDINFO( m_flSmackTime ) ),
	#else
		RecvPropTime( RECVINFO( m_flSmackTime ) ),
	#endif
END_NETWORK_TABLE()


BEGIN_NETWORK_TABLE( CKnife, DT_WeaponKnife )
	#if !defined( CLIENT_DLL )
		SendPropDataTable("LocalActiveWeaponKnifeData", 0, &REFERENCE_SEND_TABLE(DT_LocalActiveWeaponKnifeData), SendProxy_SendActiveLocalKnifeDataTable ),
	#else
		RecvPropDataTable("LocalActiveWeaponKnifeData", 0, 0, &REFERENCE_RECV_TABLE(DT_LocalActiveWeaponKnifeData)),
	#endif
END_NETWORK_TABLE()


#if defined CLIENT_DLL
BEGIN_PREDICTION_DATA( CKnife )
	DEFINE_PRED_FIELD( m_flSmackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif


LINK_ENTITY_TO_CLASS( weapon_knife, CKnife );
PRECACHE_WEAPON_REGISTER( weapon_knife );

#ifndef CLIENT_DLL

	BEGIN_DATADESC( CKnife )
		DEFINE_FUNCTION( Smack )
	END_DATADESC()

#endif

// ----------------------------------------------------------------------------- //
// CKnife implementation.
// ----------------------------------------------------------------------------- //

CKnife::CKnife()
{
}


bool CKnife::HasPrimaryAmmo()
{
	return true;
}


bool CKnife::CanBeSelected()
{
	return true;
}

void CKnife::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Weapon_Knife.Deploy" );
	PrecacheScriptSound( "Weapon_Knife.Slash" );
	PrecacheScriptSound( "Weapon_Knife.Stab" );
	PrecacheScriptSound( "Weapon_Knife.Hit" );
}

void CKnife::Spawn()
{
	Precache();

	m_iClip1 = -1;
	BaseClass::Spawn();
}


bool CKnife::Deploy()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();
	EmitSound( filter, entindex(), "Weapon_Knife.Deploy" );

	return BaseClass::Deploy();
}

void CKnife::Holster( int skiplocal )
{
	if ( GetPlayerOwner() )
	{
		GetPlayerOwner()->m_flNextAttack = gpGlobals->curtime + 0.5;
	}
}

void CKnife::WeaponAnimation ( int iAnimation )
{
	/*
	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usKnife,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 
		0.0,
		0.0,
		iAnimation, 2, 3, 4 );
	*/
}

void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity )
{
	int			i, j, k;
	float		distance;
	Vector minmaxs[2] = {mins, maxs};
	trace_t tmpTrace;
	Vector		vecHullEnd = tr.endpos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = (tmpTrace.endpos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


void CKnife::PrimaryAttack()
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer )
	{
#if !defined (CLIENT_DLL)
		// Move other players back to history positions based on local player's lag
		lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
		SwingOrStab( false );
#if !defined (CLIENT_DLL)
		lagcompensation->FinishLagCompensation( pPlayer );
#endif
	}
}

void CKnife::SecondaryAttack()
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer /*&& !pPlayer->m_bIsDefusing && !CSGameRules()->IsFreezePeriod()*/ )
	{
#if !defined (CLIENT_DLL)
		// Move other players back to history positions based on local player's lag
		lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif
		SwingOrStab( true );
#if !defined (CLIENT_DLL)
		lagcompensation->FinishLagCompensation( pPlayer );
#endif
	}
}

#include "effect_dispatch_data.h"

void CKnife::Smack( void )
{
	if ( !GetPlayerOwner() )
		return;

	m_trHit.m_pEnt = m_pTraceHitEnt;

	if ( !m_trHit.m_pEnt || (m_trHit.surface.flags & SURF_SKY) )
		return;

	if ( m_trHit.fraction == 1.0 )
		return;

	if ( m_trHit.m_pEnt )
	{
		CPASAttenuationFilter filter( this );
		filter.UsePredictionRules();

		if( m_trHit.m_pEnt->IsPlayer()  )
		{
			EmitSound( filter, entindex(), m_bStab?"Weapon_Knife.Stab":"Weapon_Knife.Hit" );
		}
		else
		{
			EmitSound( filter, entindex(), "Weapon_Knife.HitWall" );
		}
	}

	CEffectData data;
	data.m_vOrigin = m_trHit.endpos;
	data.m_vStart = m_trHit.startpos;
	data.m_nSurfaceProp = m_trHit.surface.surfaceProps;
	data.m_nDamageType = DMG_SLASH;
	data.m_nHitBox = m_trHit.hitbox;
#ifdef CLIENT_DLL
	data.m_hEntity = m_trHit.m_pEnt->GetRefEHandle();
#else
	data.m_nEntIndex = m_trHit.m_pEnt->entindex();
#endif

	CPASFilter filter( data.m_vOrigin );
	
#ifndef CLIENT_DLL
	filter.RemoveRecipient( GetPlayerOwner() );
#endif

	data.m_vAngles = GetPlayerOwner()->GetAbsAngles();
	data.m_fFlags = 0x1;	//IMPACT_NODECAL;
	te->DispatchEffect( filter, 0.0, data.m_vOrigin, "KnifeSlash", data );
}

void CKnife::WeaponIdle()
{
	if (m_flTimeWeaponIdle > gpGlobals->curtime)
		return;

	CMomentumPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	//if ( pPlayer->IsShieldDrawn() )
	//	 return;

	SetWeaponIdleTime( gpGlobals->curtime + 20 );

	// only idle if the slid isn't back
	SendWeaponAnim( ACT_VM_IDLE );
}




bool CKnife::SwingOrStab( bool bStab )
{
	CMomentumPlayer *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return false;

	float fRange = bStab ? 32 : 48; // knife range
	
	Vector vForward; AngleVectors( pPlayer->EyeAngles(), &vForward );
	Vector vecSrc	= pPlayer->Weapon_ShootPosition();
	Vector vecEnd	= vecSrc + vForward * fRange;

	trace_t tr;
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );

	//check for hitting glass - TODO - fix this hackiness, doesn't always line up with what FindHullIntersection returns
#ifndef CLIENT_DLL
	CTakeDamageInfo glassDamage( pPlayer, pPlayer, 42.0f, DMG_BULLET | DMG_NEVERGIB );
	TraceAttackToTriggers( glassDamage, tr.startpos, tr.endpos, vForward );
#endif

	if ( tr.fraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, head_hull_mins, head_hull_maxs, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = tr.m_pEnt;
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer );
			vecEnd = tr.endpos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	bool bDidHit = tr.fraction < 1.0f;

#ifndef CLIENT_DLL
	bool bFirstSwing = (m_flNextPrimaryAttack + 0.4) < gpGlobals->curtime;
#endif

	float fPrimDelay, fSecDelay;

	if ( bStab )
	{
		SendWeaponAnim( bDidHit ? ACT_VM_HITCENTER : ACT_VM_MISSCENTER );

		fPrimDelay = fSecDelay = bDidHit ? 1.1f : 1.0f;

		//pPlayer->DoAnimationEvent( PLAYERANIMEVENT_FIRE_GUN_PRIMARY );
	}
	else // swing
	{
		SendWeaponAnim( bDidHit ? ACT_VM_HITCENTER : ACT_VM_MISSCENTER );

		fPrimDelay = bDidHit ? 0.5f : 0.4f;
		fSecDelay = bDidHit ? 0.5f : 0.5f;

		//pPlayer->DoAnimationEvent( PLAYERANIMEVENT_FIRE_GUN_SECONDARY );
	}

	//if ( pPlayer->HasShield() )
	//{
	//	fPrimDelay += 0.7f; // 0.7 seconds slower if we carry a shield
	//	fSecDelay += 0.7f;
	//}

	m_flNextPrimaryAttack = gpGlobals->curtime + fPrimDelay;
	m_flNextSecondaryAttack = gpGlobals->curtime + fSecDelay;
	SetWeaponIdleTime( gpGlobals->curtime + 2 );
	
	if ( !bDidHit )
	{
		// play wiff or swish sound
		CPASAttenuationFilter filter( this );
		filter.UsePredictionRules();
		EmitSound( filter, entindex(), "Weapon_Knife.Slash" );
	}

	
#ifndef CLIENT_DLL

	if ( bDidHit )
	{
		// play thwack, smack, or dong sound

		CBaseEntity *pEntity = tr.m_pEnt;
				
		// player "shoot" animation
		pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage();
		
		float flDamage = 42.0f;

		if ( bStab )
		{
			flDamage = 65.0f;

			if ( pEntity && pEntity->IsPlayer() )
			{
				Vector vTragetForward;

				AngleVectors( pEntity->GetAbsAngles(), &vTragetForward );
				
				Vector2D vecLOS = (pEntity->GetAbsOrigin() - pPlayer->GetAbsOrigin()).AsVector2D();
				Vector2DNormalize( vecLOS );

				float flDot = vecLOS.Dot( vTragetForward.AsVector2D() );

				//Triple the damage if we are stabbing them in the back.
				if ( flDot > 0.80f )
					 flDamage *= 3;
			}
		}
		else
		{
			if ( bFirstSwing )
			{
				// first swing does full damage
				flDamage = 20;
			}
			else
			{
				// subsequent swings do less	
				flDamage = 15;
			}
		}

		CTakeDamageInfo info( pPlayer, pPlayer, flDamage, DMG_BULLET | DMG_NEVERGIB );

		CalculateMeleeDamageForce( &info, vForward, tr.endpos, 1.0f/flDamage );
		pEntity->DispatchTraceAttack( info, vForward, &tr ); 
		ApplyMultiDamage();
	}

#endif

	if ( bDidHit )
	{
		// delay the decal a bit
		m_trHit = tr;
		
		// Store the ent in an EHANDLE, just in case it goes away by the time we get into our think function.
		m_pTraceHitEnt = tr.m_pEnt; 

		m_bStab = bStab;	//store this so we know what hit sound to play

		m_flSmackTime = gpGlobals->curtime + (bStab?0.2f:0.1f);
	}

	return bDidHit;
}

void CKnife::ItemPostFrame( void )
{
	if( m_flSmackTime > 0 && gpGlobals->curtime > m_flSmackTime )
	{
		Smack();
		m_flSmackTime = -1;
	}

	BaseClass::ItemPostFrame();
}

bool CKnife::CanDrop()
{
	return false;
}


