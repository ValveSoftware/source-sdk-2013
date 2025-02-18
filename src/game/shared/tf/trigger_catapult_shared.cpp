//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// copied from portal2 code; original code came with client-predicted counterpart,
// but implementing predictable triggers in tf2 wasn't trivial so this is just the
// server component. it works but causes prediction errors.
#include "cbase.h"

#include "movevars_shared.h"

#if defined( GAME_DLL )
#include "trigger_catapult.h"
#include "tf_player.h"
#include "vcollide_parse.h"
#include "props.h"
#else
#include "c_trigger_catapult.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar catapult_physics_drag_boost( "catapult_physics_drag_boost", "2.1", FCVAR_REPLICATED );


//-----------------------------------------------------------------------------
// Purpose: calculates the launch vector between the entity that touched the
//			catapult trigger and the catapult target
//-----------------------------------------------------------------------------
Vector CTriggerCatapult::CalculateLaunchVector( CBaseEntity *pVictim, CBaseEntity *pTarget  )
{
#if defined( CLIENT_DLL )
	if( !GetPredictable() || !pVictim->GetPredictable() )
		return vec3_origin;
#endif

	// Find where we're going
	Vector vecSourcePos = pVictim->GetAbsOrigin();
	Vector vecTargetPos = pTarget->GetAbsOrigin();

	// If victim is player, adjust target position so player's center will hit the target
	if ( pVictim->IsPlayer() )
	{
		vecTargetPos.z -= 32.0f;  
	}

	float flSpeed = (pVictim->IsPlayer()) ? (float)m_flPlayerVelocity : (float)m_flPhysicsVelocity;	// u/sec
	float flGravity = GetCurrentGravity();
	
	Vector vecVelocity = (vecTargetPos - vecSourcePos);

	// throw at a constant time
	float time = vecVelocity.Length( ) / flSpeed;
	vecVelocity = vecVelocity * (1.f / time); // CatapultLaunchVelocityMultiplier

	// adjust upward toss to compensate for gravity loss
	vecVelocity.z += flGravity * time * 0.5;

	return vecVelocity;
}

//-----------------------------------------------------------------------------
// Purpose: calculates the launch vector between the entity that touched the
//			catapult trigger and the catapult target
//-----------------------------------------------------------------------------
Vector CTriggerCatapult::CalculateLaunchVectorPreserve( Vector vecInitialVelocity, CBaseEntity *pVictim, CBaseEntity *pTarget, bool bForcePlayer  )
{
#if defined( CLIENT_DLL )
	if( !GetPredictable() || !pVictim->GetPredictable() )
		return vec3_origin;
#endif

	// Find where we're going
	Vector vecSourcePos = pVictim->GetAbsOrigin();
	Vector vecTargetPos = pTarget->GetAbsOrigin();

	// If victim is player, adjust target position so player's center will hit the target
	if ( pVictim->IsPlayer() || bForcePlayer )
	{
		vecTargetPos.z -= 32.0f;  
	}

	Vector vecDiff = (vecTargetPos - vecSourcePos);

	float flHeight = vecDiff.z;
	float flDist = vecDiff.Length2D();
	float flVelocity = (pVictim->IsPlayer() || bForcePlayer ) ? (float)m_flPlayerVelocity : (float)m_flPhysicsVelocity;
	float flGravity = -1.0f*GetCurrentGravity();


	if( flDist == 0.f )
	{
		DevWarning( "Bad location input for catapult!\n" );
		return CalculateLaunchVector(pVictim, pTarget);
	}

	float flRadical = flVelocity*flVelocity*flVelocity*flVelocity - flGravity*(flGravity*flDist*flDist - 2.f*flHeight*flVelocity*flVelocity);

	if( flRadical <= 0.f )
	{
		DevWarning( "Catapult can't hit target! Add more speed!\n" );
		return CalculateLaunchVector(pVictim, pTarget);
	}

	flRadical = ( sqrt( flRadical ) );

	float flTestAngle1 = flVelocity*flVelocity;
	float flTestAngle2 = flTestAngle1;

	flTestAngle1 = -atan( (flTestAngle1 + flRadical) / (flGravity*flDist) );
	flTestAngle2 = -atan( (flTestAngle2 - flRadical) / (flGravity*flDist) );

	Vector vecTestVelocity1 = vecDiff;
	vecTestVelocity1.z = 0;
	vecTestVelocity1.NormalizeInPlace();

	Vector vecTestVelocity2 = vecTestVelocity1;

	vecTestVelocity1 *= flVelocity*cos( flTestAngle1 );
	vecTestVelocity1.z = flVelocity*sin( flTestAngle1 );

	vecTestVelocity2 *= flVelocity*cos( flTestAngle2 );
	vecTestVelocity2.z = flVelocity*sin( flTestAngle2 );

	vecInitialVelocity.NormalizeInPlace();

	if( m_ExactVelocityChoice == 1 )
	{
		return vecTestVelocity1;
	}
	else if( m_ExactVelocityChoice == 2 )
	{
		return vecTestVelocity2;
	}

	if( vecInitialVelocity.Dot( vecTestVelocity1 ) > vecInitialVelocity.Dot( vecTestVelocity2 ) )
	{
		return vecTestVelocity1;
	}
	return vecTestVelocity2;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::LaunchByTarget( CBaseEntity *pVictim, CBaseEntity *pTarget  )
{
#if defined( CLIENT_DLL )
	if( !GetPredictable() || !pVictim->GetPredictable() )
		return;
#endif

	Vector vecVictim;
	if ( pVictim->VPhysicsGetObject() )
	{
		pVictim->VPhysicsGetObject()->GetVelocity( &vecVictim, NULL );
	}
	else
	{
		vecVictim = pVictim->GetAbsVelocity();
	}
	// get the launch vector
	Vector vecVelocity = m_bUseExactVelocity ?
		CalculateLaunchVectorPreserve( vecVictim, pVictim, pTarget ):
		CalculateLaunchVector( pVictim, pTarget );


	// Handle a player
	if ( pVictim->IsPlayer() )
	{
		// Send us flying
		if ( pVictim->GetFlags() & FL_ONGROUND )
		{
			pVictim->SetGroundEntity( NULL );
			pVictim->SetGroundChangeTime( gpGlobals->curtime + 0.5f );
		}

		CTFPlayer *pPlayer = ToTFPlayer( pVictim );
		if ( pPlayer )
		{
			float flSupressionTimeInSeconds = 0.25f;
			if ( m_flAirControlSupressionTime > 0 )
			{
				// If set in the map, use this override time
				flSupressionTimeInSeconds = m_flAirControlSupressionTime;
			}
			//pPlayer->SetAirControlSupressionTime( flSupressionTimeInSeconds * 1000.0f ); // fix units, this method expects milliseconds
			pVictim->Teleport( NULL, NULL, &vecVelocity );
			OnLaunchedVictim( pVictim );

#if defined( GAME_DLL ) && !defined( _GAMECONSOLE ) && !defined( NO_STEAM )
			//g_PortalGameStats.Event_Catapult_LaunchByTarget( pPlayer, vecVelocity );
#endif
		}
	}
	else
	{
		if ( pVictim->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			// Launch!
			IPhysicsObject *pPhysObject = pVictim->VPhysicsGetObject();
			if ( pPhysObject )
			{
				AngularImpulse angImpulse = m_bApplyAngularImpulse ? RandomAngularImpulse( -150.0f, 150.0f ) : vec3_origin;
				pPhysObject->SetVelocityInstantaneous( &vecVelocity, &angImpulse );

				// UNDONE: don't mess with physics properties 

#if defined( GAME_DLL )
				CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pVictim);
				if ( pProp != NULL )
				{
					//HACK!
					pProp->OnPhysGunDrop( UTIL_GetLocalPlayer(), LAUNCHED_BY_CANNON );
				}
#endif
			}
		}
		OnLaunchedVictim( pVictim );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::LaunchByDirection( CBaseEntity *pVictim  )
{
#if defined( CLIENT_DLL )
	if( !GetPredictable() || !pVictim->GetPredictable() )
		return;
#endif

	Vector vecForward;
	AngleVectors( m_vecLaunchAngles, &vecForward, NULL, NULL );

	// Handle a player
	if ( pVictim->IsPlayer() )
	{
		// Simply push us forward
		Vector vecPush = vecForward * m_flPlayerVelocity;

		// Hack on top of magic
		if( CloseEnough( vecPush[0], 0.f ) && CloseEnough( vecPush[1],0.f ) )
		{
			vecPush[2] = m_flPlayerVelocity * 1.5f;	// FIXME: Magic!
		}

		// Send us flying
		if ( pVictim->GetFlags() & FL_ONGROUND )
		{
			pVictim->SetGroundEntity( NULL );
			pVictim->SetGroundChangeTime( gpGlobals->curtime + 0.5f );
		}

		pVictim->SetAbsVelocity( vecPush );
		OnLaunchedVictim( pVictim );

		// Do air control suppression
		if( m_bDirectionSuppressAirControl )
		{
			float flSupressionTimeInSeconds = 0.25f;
			if ( m_flAirControlSupressionTime > 0 )
			{
				// If set in the map, use this override time
				flSupressionTimeInSeconds = m_flAirControlSupressionTime;
			}

			//CTFPlayer* pTFPlayer = static_cast<CTFPlayer*>(pVictim);
			//pTFPlayer->SetAirControlSupressionTime( flSupressionTimeInSeconds * 1000.0f ); // fix units, this method expects milliseconds
		}

#if defined( GAME_DLL ) && !defined( _GAMECONSOLE ) && !defined( NO_STEAM )
		//g_PortalGameStats.Event_Catapult_LaunchByDirection( ToPortalPlayer(pVictim), vecPush );
#endif
	}
#if defined( GAME_DLL )
	else
	{
		if ( pVictim->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			// Launch!
			IPhysicsObject *pPhysObject = pVictim->VPhysicsGetObject();
			if ( pPhysObject )
			{
				Vector vecVelocity = vecForward * m_flPhysicsVelocity;
				vecVelocity[2] = m_flPhysicsVelocity;

				AngularImpulse angImpulse = RandomAngularImpulse( -50.0f, 50.0f );

				pPhysObject->SetVelocityInstantaneous( &vecVelocity, &angImpulse );

				// Force this!
				float flNull = 0.0f;
				pPhysObject->SetDragCoefficient( &flNull, &flNull );
				pPhysObject->SetDamping( &flNull, &flNull );

				CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pVictim);
				if ( pProp != NULL )
				{
					//HACK!
					pProp->OnPhysGunDrop( UTIL_GetLocalPlayer(), LAUNCHED_BY_CANNON );
				}
			}
		}
		OnLaunchedVictim( pVictim );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::OnLaunchedVictim( CBaseEntity *pVictim )
{
#if defined( CLIENT_DLL )
	if( !GetPredictable() || !pVictim->GetPredictable() )
		return;
#endif

#if defined( GAME_DLL )
	m_OnCatapulted.FireOutput( pVictim, this );
#endif

	if ( pVictim->IsPlayer() )
	{
		CTFPlayer *pPlayer = static_cast< CTFPlayer* >( pVictim );
		int nRefireIndex = pPlayer->entindex();
#if defined( GAME_DLL )
		m_flRefireDelay[ nRefireIndex ] = gpGlobals->curtime + 0.5f; // HACK!
#else
		m_flRefireDelay[ nRefireIndex ] = gpGlobals->curtime + 0.5f; // HACK!
#endif
	}
	else
	{
#if defined( GAME_DLL )
		m_flRefireDelay[ 0 ] = gpGlobals->curtime + 0.5f; // HACK!
#else
		m_flRefireDelay[ 0 ] = gpGlobals->curtime + 0.5f; // HACK!
#endif
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerCatapult::StartTouch( CBaseEntity *pOther )
{
	if ( pOther == NULL )
		return;

#if defined( CLIENT_DLL )
	if( !GetPredictable() || !pOther->GetPredictable() )
		return;
#endif

	//Warning( "CTriggerCatapult::StartTouch( %i %s %f )\n", entindex(), gpGlobals->IsClient() ? "client" : "server", gpGlobals->curtime );


#if defined( GAME_DLL )
	if ( PassesTriggerFilters( pOther ) == false )
#else
	if( !(pOther->IsPlayer() && m_bPlayersPassTriggerFilters) )
#endif
	{
		return;
	}

	// Don't refire too quickly
	int nRefireIndex = pOther->IsPlayer() ? static_cast< CBasePlayer* >( pOther )->entindex() : 0;
	if ( !IsIndexIntoPlayerArrayValid(nRefireIndex) )
	{
		Warning( "CTriggerCatapult::StartTouch Trying to store a refire index for an entity( %d ) outside the expected range ( < %d ).\n", nRefireIndex, MAX_PLAYERS_ARRAY_SAFE );
		nRefireIndex = 0;
	}

	if ( m_flRefireDelay[ nRefireIndex ] > gpGlobals->curtime )
	{
		// but also don't forget to try again
		if ( m_hAbortedLaunchees.Find( pOther ) == -1 )
		{
			m_hAbortedLaunchees.AddToTail( pOther );
		}
		SetThink( &CTriggerCatapult::LaunchThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
		return;
	}

#if defined( GAME_DLL )
	// Don't touch things the player is holding
	if ( pOther->VPhysicsGetObject() && (pOther->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD) )
	{
		if ( m_hAbortedLaunchees.Find( pOther ) == -1 )
		{
			m_hAbortedLaunchees.AddToTail( pOther );
		}
		SetThink( &CTriggerCatapult::LaunchThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
		return;
	}
	else if ( pOther->IsPlayer() )
	{
		// Always keep players in this list in case the were trapped under another player in the previous launch
		if ( m_hAbortedLaunchees.Find( pOther ) == -1 )
		{
			m_hAbortedLaunchees.AddToTail( pOther );
		}
		SetThink( &CTriggerCatapult::LaunchThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
	}
#endif

	// Get the target
	CBaseEntity *pLaunchTarget = m_hLaunchTarget;

	// See if we're attempting to hit a target
	if ( pLaunchTarget )
	{
		// See if we are using the threshold check
		if ( m_bUseThresholdCheck )
		{
			// Get the velocity of the physics objects / players touching the catapult
			Vector vecVictim;
			if ( pOther->IsPlayer() )
			{
				vecVictim = pOther->GetAbsVelocity();
			}
			else if( pOther->VPhysicsGetObject() )
			{
				pOther->VPhysicsGetObject()->GetVelocity( &vecVictim, NULL );
			}
			else
			{
//				DevMsg("Catapult fail!!  Object is not a player and has no physics object!  BUG THIS\n");
				vecVictim = vec3_origin;
			}

			float flVictimSpeed = vecVictim.Length();

			// get the speed needed to hit the target			
			Vector vecVelocity;
			if( m_bUseExactVelocity )
			{
				vecVelocity = CalculateLaunchVectorPreserve( vecVictim, pOther, pLaunchTarget );
			}
			else
			{
				vecVelocity = CalculateLaunchVector( pOther, pLaunchTarget );
			}
			float flLaunchSpeed = vecVelocity.Length();

			// is the victim facing the target?
			Vector vecDirection = ( pLaunchTarget->GetAbsOrigin() - pOther->GetAbsOrigin() );
			Vector necNormalizedVictim = vecVictim;
			Vector vecNormalizedDirection = vecDirection;

			necNormalizedVictim.NormalizeInPlace();
			vecNormalizedDirection.NormalizeInPlace();

			float flDot = DotProduct( necNormalizedVictim, vecNormalizedDirection );
			if ( flDot >= m_flEntryAngleTolerance )
			{
				// Is the victim speed within tolerance to launch them?
				if ( ( ( flLaunchSpeed - (flLaunchSpeed * m_flLowerThreshold ) ) < flVictimSpeed ) && ( ( flLaunchSpeed + (flLaunchSpeed * m_flUpperThreshold ) ) > flVictimSpeed )  )
				{
					if( m_bOnlyVelocityCheck )
					{
						OnLaunchedVictim( pOther );
					}
					else
					{
						// Launch!
						LaunchByTarget( pOther, pLaunchTarget );
//						DevMsg( 1, "Catapult \"%s\" is adjusting velocity of \"%s\" so it will hit the target. (Object Velocity: %.1f -- Object needed to be between %.1f and %.1f \n", STRING(GetEntityName()), pOther->GetClassname(), flVictimSpeed, flLaunchSpeed - (flLaunchSpeed * m_flLowerThreshold ), flLaunchSpeed + (flLaunchSpeed * m_flUpperThreshold ) );
					}
				}
				else
				{
//					DevMsg( 1, "Catapult \"%s\" ignoring object \"%s\" because its velocity is outside of the threshold. (Object Velocity: %.1f -- Object needed to be between %.1f and %.1f \n", STRING(GetEntityName()), pOther->GetClassname(), flVictimSpeed, flLaunchSpeed - (flLaunchSpeed * m_flLowerThreshold ), flLaunchSpeed + (flLaunchSpeed * m_flUpperThreshold ) );
					// since we attempted a fling set the refire delay
#if defined( GAME_DLL )
					m_flRefireDelay[ nRefireIndex ] = gpGlobals->curtime + 0.5f; // HACK!
#else
					m_flRefireDelay[ nRefireIndex ] = gpGlobals->curtime + 0.5f; // HACK!
#endif
				}
			}
			else
			{
				// we're facing the wrong way.  set the refire delay.
#if defined( GAME_DLL )
				m_flRefireDelay[ nRefireIndex ] = gpGlobals->curtime + 0.5f; // HACK!
#else
				m_flRefireDelay[ nRefireIndex ] = gpGlobals->curtime + 0.5f; // HACK!
#endif
			}
		}
		else
		{
			LaunchByTarget( pOther, pLaunchTarget );
		}
	}
	else
	{
#if defined( CLIENT_DLL )
		if( m_hLaunchTarget.IsValid() )
		{
			Warning( "Catapult launch target not networked to client! This will make prediction fail! Fix this in the map.\n"
				"Catapult launch target not networked to client! This will make prediction fail! Fix this in the map.\n"
				"Catapult launch target not networked to client! This will make prediction fail! Fix this in the map.\n"
				"Catapult launch target not networked to client! This will make prediction fail! Fix this in the map.\n"
				"Catapult launch target not networked to client! This will make prediction fail! Fix this in the map.\n" );
		}
#endif

		bool bShouldLaunch = true;

		if( m_bUseThresholdCheck )
		{
			// Get the velocity of the physics objects / players touching the catapult
			Vector vecVictim;
			if ( pOther->IsPlayer() )
			{
				vecVictim = pOther->GetAbsVelocity();
			}
			else if( pOther->VPhysicsGetObject() )
			{
				pOther->VPhysicsGetObject()->GetVelocity( &vecVictim, NULL );
			}
			else
			{
//				DevMsg("Catapult fail!!  Object is not a player and has no physics object!  BUG THIS\n");
				vecVictim = vec3_origin;
			}

			Vector vecForward;
			AngleVectors( m_vecLaunchAngles, &vecForward, NULL, NULL );

			float flDot = DotProduct( vecForward, vecVictim );
			float flLower = m_flPlayerVelocity - (m_flPlayerVelocity * m_flLowerThreshold);
			float flUpper = m_flPlayerVelocity + (m_flPlayerVelocity * m_flUpperThreshold);
			if( flDot < flLower || flDot > flUpper )
			{
				bShouldLaunch = false;
			}
		}

		if( bShouldLaunch )
		{
#if defined( CLIENT_DLL )
			CEG_PROTECT_VIRTUAL_FUNCTION ( CTriggerCatapult_StartTouch );
#endif
			if( m_bOnlyVelocityCheck )
			{
				OnLaunchedVictim( pOther );
			}
			else
			{
				LaunchByDirection( pOther );
			}
		}
	}
}
