//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "vcollide_parse.h"
#include "vehicle_base.h"
#include "npc_vehicledriver.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "soundenvelope.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "saverestore_utlvector.h"
#include "KeyValues.h"
#include "studio.h"
#include "bone_setup.h"
#include "collisionutils.h"
#include "animation.h"
#include "env_player_surface_trigger.h"
#include "rumble_shared.h"

#ifdef HL2_DLL
	#include "hl2_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar g_debug_vehiclesound( "g_debug_vehiclesound", "0", FCVAR_CHEAT );
ConVar g_debug_vehicleexit( "g_debug_vehicleexit", "0", FCVAR_CHEAT );

ConVar sv_vehicle_autoaim_scale("sv_vehicle_autoaim_scale", "8");

bool ShouldVehicleIgnoreEntity( CBaseEntity *pVehicle, CBaseEntity *pCollide );

#define HITBOX_SET	2

//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC_NO_BASE( vehicle_gear_t )

	DEFINE_FIELD( flMinSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( flMaxSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( flSpeedApproachFactor,FIELD_FLOAT ),

END_DATADESC()

BEGIN_DATADESC_NO_BASE( vehicle_crashsound_t )
	DEFINE_FIELD( flMinSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( flMinDeltaSpeed,		FIELD_FLOAT ),
	DEFINE_FIELD( iszCrashSound,		FIELD_STRING ),
	DEFINE_FIELD( gearLimit,			FIELD_INTEGER ),
END_DATADESC()

BEGIN_DATADESC_NO_BASE( vehiclesounds_t )

	DEFINE_AUTO_ARRAY( iszSound,		FIELD_STRING ),
	DEFINE_UTLVECTOR( pGears,			FIELD_EMBEDDED ),
	DEFINE_UTLVECTOR( crashSounds,		FIELD_EMBEDDED ),
	DEFINE_AUTO_ARRAY( iszStateSounds,	FIELD_STRING ),
	DEFINE_AUTO_ARRAY( minStateTime,	FIELD_FLOAT ),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( CPassengerInfo )
	DEFINE_FIELD( m_hPassenger,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_strRoleName,	FIELD_STRING ),
	DEFINE_FIELD( m_strSeatName,	FIELD_STRING ),
	// NOT SAVED
	// DEFINE_FIELD( m_nRole,	FIELD_INTEGER ),
	// DEFINE_FIELD( m_nSeat,	FIELD_INTEGER ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( CBaseServerVehicle )

// These are reset every time by the constructor of the owning class 
//	DEFINE_FIELD( m_pVehicle, FIELD_CLASSPTR ),
//	DEFINE_FIELD( m_pDrivableVehicle; ??? ),

	// Controls
	DEFINE_FIELD( m_nNPCButtons,		FIELD_INTEGER ),
	DEFINE_FIELD( m_nPrevNPCButtons,	FIELD_INTEGER ),
	DEFINE_FIELD( m_flTurnDegrees,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flVehicleVolume,	FIELD_FLOAT ),

	// We're going to reparse this data from file in Precache
	DEFINE_EMBEDDED( m_vehicleSounds ),

	DEFINE_FIELD( m_iSoundGear,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flSpeedPercentage,	FIELD_FLOAT ),
	DEFINE_SOUNDPATCH( m_pStateSound ),
	DEFINE_SOUNDPATCH( m_pStateSoundFade ),
	DEFINE_FIELD( m_soundState,			FIELD_INTEGER ),
	DEFINE_FIELD( m_soundStateStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_lastSpeed, FIELD_FLOAT ),
	
// NOT SAVED
//	DEFINE_FIELD( m_EntryAnimations, CUtlVector ),
//	DEFINE_FIELD( m_ExitAnimations, CUtlVector ),
//	DEFINE_FIELD( m_bParsedAnimations,	FIELD_BOOLEAN ),
//  DEFINE_UTLVECTOR( m_PassengerRoles, FIELD_EMBEDDED ),

	DEFINE_FIELD( m_iCurrentExitAnim,	FIELD_INTEGER ),
	DEFINE_FIELD( m_vecCurrentExitEndPoint, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_chPreviousTextureType, FIELD_CHARACTER ),

	DEFINE_FIELD( m_savedViewOffset, FIELD_VECTOR ),
	DEFINE_FIELD( m_hExitBlocker, FIELD_EHANDLE ),
	
	DEFINE_UTLVECTOR( m_PassengerInfo, FIELD_EMBEDDED ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Base class for drivable vehicle handling. Contain it in your 
//			drivable vehicle.
//-----------------------------------------------------------------------------
CBaseServerVehicle::CBaseServerVehicle( void )
{
	m_pVehicle = NULL;
	m_pDrivableVehicle = NULL;
	m_nNPCButtons = 0;
	m_nPrevNPCButtons = 0;
	m_flTurnDegrees = 0;

	m_bParsedAnimations = false;
	m_iCurrentExitAnim = 0;
	m_vecCurrentExitEndPoint = vec3_origin;

	m_flVehicleVolume = 0.5;
	m_iSoundGear = 0;
	m_pStateSound = NULL;
	m_pStateSoundFade = NULL;
	m_soundState = SS_NONE;
	m_flSpeedPercentage = 0;
	m_bUseLegacyExitChecks = false;

	m_vehicleSounds.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseServerVehicle::~CBaseServerVehicle( void )
{
	SoundShutdown(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::Precache( void )
{
	int i;
	// Precache our other sounds
	for ( i = 0; i < VS_NUM_SOUNDS; i++ )
	{
		if ( m_vehicleSounds.iszSound[i] != NULL_STRING )
		{
			CBaseEntity::PrecacheScriptSound( STRING(m_vehicleSounds.iszSound[i]) );
		}
	}
	for ( i = 0; i < m_vehicleSounds.crashSounds.Count(); i++ )
	{
		if ( m_vehicleSounds.crashSounds[i].iszCrashSound != NULL_STRING )
		{
			CBaseEntity::PrecacheScriptSound( STRING(m_vehicleSounds.crashSounds[i].iszCrashSound) );
		}
	}

	for ( i = 0; i < SS_NUM_STATES; i++ )
	{
		if ( m_vehicleSounds.iszStateSounds[i] != NULL_STRING )
		{
			CBaseEntity::PrecacheScriptSound( STRING(m_vehicleSounds.iszStateSounds[i]) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parses the vehicle's script for the vehicle sounds
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::Initialize( const char *pScriptName )
{
	// Attempt to parse our vehicle script
	if ( PhysFindOrAddVehicleScript( pScriptName, NULL, &m_vehicleSounds ) == false )
		return false;

	Precache();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SetVehicle( CBaseEntity *pVehicle )
{ 
	m_pVehicle = pVehicle; 
	m_pDrivableVehicle = dynamic_cast<IDrivableVehicle*>(m_pVehicle);
	Assert( m_pDrivableVehicle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IDrivableVehicle *CBaseServerVehicle::GetDrivableVehicle( void )
{
	Assert( m_pDrivableVehicle );
	return m_pDrivableVehicle;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the driver. Unlike GetPassenger(VEHICLE_ROLE_DRIVER), it will return
//			the NPC driver if it has one.
//-----------------------------------------------------------------------------
CBaseEntity	*CBaseServerVehicle::GetDriver( void )
{
	return GetPassenger( VEHICLE_ROLE_DRIVER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CBaseServerVehicle::GetPassenger( int nRole ) 
{ 
	Assert( nRole == VEHICLE_ROLE_DRIVER ); 
	CBaseEntity *pDriver = GetDrivableVehicle()->GetDriver();

	if ( pDriver == NULL )
		return NULL;

	return pDriver->MyCombatCharacterPointer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseServerVehicle::GetPassengerRole( CBaseCombatCharacter *pPassenger )
{
	if ( pPassenger == GetDrivableVehicle()->GetDriver() )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a passenger to the vehicle
// Input  : nSeat - seat to sit in
//			*pPassenger - character to enter
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::NPC_AddPassenger( CBaseCombatCharacter *pPassenger, string_t strRoleName, int nSeat )
{
	// Players cannot yet use this code! - jdw
	Assert( pPassenger != NULL && pPassenger->IsPlayer() == false );
	if ( pPassenger == NULL || pPassenger->IsPlayer() )
		return false;

	// Find our role
	int nRole = FindRoleIndexByName( strRoleName );
	if ( nRole == -1 )
		return false;

	// Cannot evict a passenger already in this position
	CBaseCombatCharacter *pCurrentPassenger = NPC_GetPassengerInSeat( nRole, nSeat );
	if ( pCurrentPassenger == pPassenger )
		return true;

	// If we weren't the same passenger, we need to be empty
	if ( pCurrentPassenger != NULL )
		return false;

	// Find the seat
	for ( int i = 0; i < m_PassengerInfo.Count(); i++ )
	{
		if ( m_PassengerInfo[i].GetSeat() == nSeat && m_PassengerInfo[i].GetRole() == nRole )
		{
			m_PassengerInfo[i].m_hPassenger = pPassenger;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Removes a passenger from the vehicle
// Input  : *pPassenger - Passenger to remove
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::NPC_RemovePassenger( CBaseCombatCharacter *pPassenger )
{
	// Players cannot yet use this code! - jdw
	Assert( pPassenger != NULL && pPassenger->IsPlayer() == false );
	if ( pPassenger == NULL || pPassenger->IsPlayer() )
		return false;

	// Find the seat
	for ( int i = 0; i < m_PassengerInfo.Count(); i++ )
	{
		if ( m_PassengerInfo[i].m_hPassenger == pPassenger )
		{
			m_PassengerInfo[i].m_hPassenger = NULL;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the attachment point index for the passenger's seat
// Input  : *pPassenger - Passenger in the seat
// Output : int - Attachment point index for the vehicle
//-----------------------------------------------------------------------------
int CBaseServerVehicle::NPC_GetPassengerSeatAttachment( CBaseCombatCharacter *pPassenger )
{
	// Get the role and seat the the supplied passenger
	for ( int i = 0; i < m_PassengerInfo.Count(); i++ )
	{
		// If this is the passenger, get the attachment it'll be at
		if ( m_PassengerInfo[i].m_hPassenger == pPassenger )
		{
			// The seat is the attachment point
			int nSeat = m_PassengerInfo[i].GetSeat();
			int nRole = m_PassengerInfo[i].GetRole();
			
			return m_PassengerRoles[nRole].m_PassengerSeats[nSeat].GetAttachmentID();
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Get the worldspace position and angles of the specified seat
// Input  : *pPassenger - Passenger's seat to use
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::NPC_GetPassengerSeatPosition( CBaseCombatCharacter *pPassenger, Vector *vecResultPos, QAngle *vecResultAngles )
{
	// Get our attachment point
	int nSeatAttachment = NPC_GetPassengerSeatAttachment( pPassenger );
	if ( nSeatAttachment == -1 )
		return false;

	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = dynamic_cast< CBaseAnimating * >( m_pVehicle );
	if ( pAnimating == NULL )
		return false;

	Vector vecPos;
	QAngle vecAngles;
	pAnimating->GetAttachment( nSeatAttachment, vecPos, vecAngles );

	if ( vecResultPos != NULL )
	{
		*vecResultPos = vecPos;
	}

	if ( vecResultAngles != NULL )
	{
		*vecResultAngles = vecAngles;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get the localspace position and angles of the specified seat
// Input  : *pPassenger - Passenger's seat to use
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::NPC_GetPassengerSeatPositionLocal( CBaseCombatCharacter *pPassenger, Vector *vecResultPos, QAngle *vecResultAngles )
{
	// Get our attachment point
	int nSeatAttachment = NPC_GetPassengerSeatAttachment( pPassenger );
	if ( nSeatAttachment == -1 )
		return false;

	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = m_pVehicle->GetBaseAnimating();
	if ( pAnimating == NULL )
		return false;

	Vector vecPos;
	QAngle vecAngles;
	pAnimating->InvalidateBoneCache(); // NOTE: We're moving with velocity, so we're almost always out of date
	pAnimating->GetAttachmentLocal( nSeatAttachment, vecPos, vecAngles );

	if ( vecResultPos != NULL )
	{
		*vecResultPos = vecPos;
	}

	if ( vecResultAngles != NULL )
	{
		*vecResultAngles = vecAngles;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieves a list of animations used to enter/exit the seat occupied by the passenger
// Input  : *pPassenger - Passenger who's seat anims to retrieve
//			nType - which set of animations to retrieve
//-----------------------------------------------------------------------------
const PassengerSeatAnims_t *CBaseServerVehicle::NPC_GetPassengerSeatAnims( CBaseCombatCharacter *pPassenger, PassengerSeatAnimType_t nType )
{
	// Get the role and seat the the supplied passenger
	for ( int i = 0; i < m_PassengerInfo.Count(); i++ )
	{
		if ( m_PassengerInfo[i].m_hPassenger == pPassenger )
		{
			int nSeat = m_PassengerInfo[i].GetSeat();
			int nRole = m_PassengerInfo[i].GetRole();
			switch( nType )
			{
			case PASSENGER_SEAT_ENTRY:
				return &m_PassengerRoles[nRole].m_PassengerSeats[nSeat].m_EntryTransitions;
				break;

			case PASSENGER_SEAT_EXIT:
				return &m_PassengerRoles[nRole].m_PassengerSeats[nSeat].m_ExitTransitions;
				break;

			default:
				return NULL;
				break;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get and set the current driver. Use PassengerRole_t enum in shareddefs.h for adding passengers
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SetPassenger( int nRole, CBaseCombatCharacter *pPassenger )
{
	// Baseclass only handles vehicles with a single passenger
	Assert( nRole == VEHICLE_ROLE_DRIVER ); 

	if ( pPassenger != NULL && pPassenger->IsPlayer() == false )
	{
		// Use NPC_AddPassenger() for NPCs at the moment, these will all be collapsed into one system -- jdw
		Assert( 0 );
		return;
	}

	// Getting in? or out?
	if ( pPassenger != NULL )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
		if ( pPlayer != NULL )
		{
			m_savedViewOffset = pPlayer->GetViewOffset();
			pPlayer->SetViewOffset( vec3_origin );
			pPlayer->ShowCrosshair( false );

			GetDrivableVehicle()->EnterVehicle( pPassenger );

#ifdef HL2_DLL
			// Stop the player sprint and flashlight.
			CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player*>( pPlayer );
			if ( pHL2Player )
			{
				if ( pHL2Player->IsSprinting() )
				{
					pHL2Player->StopSprinting();
				}

				if ( pHL2Player->FlashlightIsOn() )
				{
					pHL2Player->FlashlightTurnOff();
				}
			}
#endif
		}
	}
	else
	{
		CBasePlayer *pPlayer = ToBasePlayer( GetDriver() );
		if ( pPlayer )
		{
			// Restore the exiting player's view offset			
			pPlayer->SetViewOffset( m_savedViewOffset );
			pPlayer->ShowCrosshair( true );
		}

		GetDrivableVehicle()->ExitVehicle( nRole );
		GetDrivableVehicle()->SetVehicleEntryAnim( false );
		UTIL_Remove( m_hExitBlocker );
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: Get a position in *world space* inside the vehicle for the player to start at
//-----------------------------------------------------------------------------
void CBaseServerVehicle::GetPassengerSeatPoint( int nRole, Vector *pPoint, QAngle *pAngles )
{
	Assert( nRole == VEHICLE_ROLE_DRIVER ); 

	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( pAnimating )
	{
		char pAttachmentName[32];
		Q_snprintf( pAttachmentName, sizeof( pAttachmentName ), "vehicle_feet_passenger%d", nRole );
		int nFeetAttachmentIndex = pAnimating->LookupAttachment(pAttachmentName);
		int nIdleSequence = pAnimating->SelectWeightedSequence( ACT_IDLE );
		if ( nFeetAttachmentIndex > 0 && nIdleSequence != -1 )
		{
			// FIXME: This really wants to be a faster query than this implementation!
			Vector vecOrigin;
			QAngle vecAngles;
			if ( GetLocalAttachmentAtTime( nIdleSequence, nFeetAttachmentIndex, 0.0f, &vecOrigin, &vecAngles ) )
			{
				UTIL_ParentToWorldSpace( pAnimating, vecOrigin, vecAngles );
				if ( pPoint )
				{
					*pPoint = vecOrigin;
				}

				if ( pAngles )
				{
					*pAngles = vecAngles;
				}

				return;
			}
		}
	}

	// Couldn't find the attachment point, so just use the origin
	if ( pPoint )
	{
		*pPoint = m_pVehicle->GetAbsOrigin();
	}

	if ( pAngles )
	{
		*pAngles = m_pVehicle->GetAbsAngles();
	}
}

//---------------------------------------------------------------------------------
// Check Exit Point for leaving vehicle.
//
// Input: yaw/roll from vehicle angle to check for exit
//		  distance from origin to drop player (allows for different shaped vehicles
// Output: returns true if valid location, pEndPoint
//         updated with actual exit point
//---------------------------------------------------------------------------------
bool CBaseServerVehicle::CheckExitPoint( float yaw, int distance, Vector *pEndPoint )
{
	QAngle vehicleAngles = m_pVehicle->GetLocalAngles();
  	Vector vecStart = m_pVehicle->GetAbsOrigin();
  	Vector vecDir;
   
  	vecStart.z += 12;		// always 12" from ground
  	vehicleAngles[YAW] += yaw;	
  	AngleVectors( vehicleAngles, NULL, &vecDir, NULL );
	// Vehicles are oriented along the Y axis
	vecDir *= -1;
  	*pEndPoint = vecStart + vecDir * distance;
  
  	trace_t tr;
  	UTIL_TraceHull( vecStart, *pEndPoint, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, m_pVehicle, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0 )
		return false;
  
  	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Where does this passenger exit the vehicle?
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::GetPassengerExitPoint( int nRole, Vector *pExitPoint, QAngle *pAngles )
{ 
	Assert( nRole == VEHICLE_ROLE_DRIVER ); 

	// First, see if we've got an attachment point
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( pAnimating )
	{
		Vector vehicleExitOrigin;
		QAngle vehicleExitAngles;
		if ( pAnimating->GetAttachment( "vehicle_driver_exit", vehicleExitOrigin, vehicleExitAngles ) )
		{
			// Make sure it's clear
			trace_t tr;
			UTIL_TraceHull( vehicleExitOrigin + Vector(0, 0, 12), vehicleExitOrigin, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, m_pVehicle, COLLISION_GROUP_NONE, &tr );
			if ( !tr.startsolid )
			{
				*pAngles = vehicleExitAngles;
				*pExitPoint = tr.endpos;
				return true;
			}
		}
	}

	// left side 
	if( CheckExitPoint( 90, 90, pExitPoint ) )	// angle from car, distance from origin, actual exit point
		return true;

	// right side
	if( CheckExitPoint( -90, 90, pExitPoint ) )
		return true;

	// front
	if( CheckExitPoint( 0, 100, pExitPoint ) )
		return true;

	// back
	if( CheckExitPoint( 180, 170, pExitPoint ) )
		return true;

	// All else failed, try popping them out the top.
	Vector vecWorldMins, vecWorldMaxs;
	m_pVehicle->CollisionProp()->WorldSpaceAABB( &vecWorldMins, &vecWorldMaxs );
	pExitPoint->x = (vecWorldMins.x + vecWorldMaxs.x) * 0.5f;
	pExitPoint->y = (vecWorldMins.y + vecWorldMaxs.y) * 0.5f;
	pExitPoint->z = vecWorldMaxs.z + 50.0f;

	// Make sure it's clear
	trace_t tr;
	UTIL_TraceHull( m_pVehicle->CollisionProp()->WorldSpaceCenter(), *pExitPoint, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, m_pVehicle, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid )
	{
		return true;
	}

	// No clear exit point available!
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ParseExitAnim( KeyValues *pkvExitList, bool bEscapeExit )
{
	// Look through the entry animations list
	KeyValues *pkvExitAnim = pkvExitList->GetFirstSubKey();
	while ( pkvExitAnim )
	{
		// Add 'em to our list
		int iIndex = m_ExitAnimations.AddToTail();
		Q_strncpy( m_ExitAnimations[iIndex].szAnimName, pkvExitAnim->GetName(), sizeof(m_ExitAnimations[iIndex].szAnimName) );
		m_ExitAnimations[iIndex].bEscapeExit = bEscapeExit;
		if ( !Q_strncmp( pkvExitAnim->GetString(), "upsidedown", 10 ) )
		{
			m_ExitAnimations[iIndex].bUpright = false;
		}
		else 
		{
			m_ExitAnimations[iIndex].bUpright = true;
		}

		pkvExitAnim = pkvExitAnim->GetNextKey();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse the transition information
// Input  : *pTransitionKeyValues - key values to parse
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ParseNPCSeatTransition( KeyValues *pTransitionKeyValues, CPassengerSeatTransition *pTransition )
{
	// Store it
	const char *lpszAnimName = pTransitionKeyValues->GetString( "animation" );
	pTransition->m_strAnimationName = AllocPooledString( lpszAnimName );
	pTransition->m_nPriority = pTransitionKeyValues->GetInt( "priority" );
}

//-----------------------------------------------------------------------------
// Purpose: Sorting function for vehicle seat animation priorities
//-----------------------------------------------------------------------------
typedef CPassengerSeatTransition SortSeatPriorityType;
int __cdecl SeatPrioritySort( const SortSeatPriorityType *s1, const SortSeatPriorityType *s2 )
{
	return ( s1->GetPriority() > s2->GetPriority() );
}

//-----------------------------------------------------------------------------
// Purpose: Parse one set of entry/exit data
// Input  : *pSetKeyValues - Key values for this set
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ParseNPCPassengerSeat( KeyValues *pSetKeyValues, CPassengerSeat *pSeat )
{
	CBaseAnimating *pAnimating = (CBaseAnimating *) m_pVehicle;

	// Get our attachment name
	const char *lpszAttachmentName = pSetKeyValues->GetString( "target_attachment" );
	int nAttachmentID = pAnimating->LookupAttachment( lpszAttachmentName );
	pSeat->m_nAttachmentID = nAttachmentID;
	pSeat->m_strSeatName = AllocPooledString( lpszAttachmentName );

	KeyValues *pKey = pSetKeyValues->GetFirstSubKey();
	while ( pKey != NULL )
	{
		const char *lpszName = pKey->GetName();

		if ( Q_stricmp( lpszName, "entry" ) == 0 )
		{
			int nIndex = pSeat->m_EntryTransitions.AddToTail();
			Assert( pSeat->m_EntryTransitions.IsValidIndex( nIndex ) );

			ParseNPCSeatTransition( pKey, &pSeat->m_EntryTransitions[nIndex] );
		}
		else if ( Q_stricmp( lpszName, "exit" ) == 0 )
		{
			int nIndex = pSeat->m_ExitTransitions.AddToTail();
			Assert( pSeat->m_ExitTransitions.IsValidIndex( nIndex ) );

			ParseNPCSeatTransition( pKey, &pSeat->m_ExitTransitions[nIndex] );
		}

		// Advance
		pKey = pKey->GetNextKey();
	}

	// Sort the seats based on their priority
	pSeat->m_EntryTransitions.Sort( SeatPrioritySort );
	pSeat->m_ExitTransitions.Sort( SeatPrioritySort );
}

//-----------------------------------------------------------------------------
// Purpose: Find a passenger role (by name), or create a new one of that names
// Input  : strName - name of the role
//		  : *nIndex - the index into the CUtlBuffer where this role resides
// Output : CPassengerRole * - Role found or created
//-----------------------------------------------------------------------------
CPassengerRole *CBaseServerVehicle::FindOrCreatePassengerRole( string_t strName, int *nIndex )
{
	// Try to find an already created container of the same name
	for ( int i = 0; i < m_PassengerRoles.Count(); i++ )
	{
		// If we match, return it
		if ( FStrEq( STRING( m_PassengerRoles[i].m_strName ), STRING( strName ) ) )
		{
			// Supply the index, if requested
			if ( nIndex != NULL )
			{
				*nIndex = i;
			}

			return &m_PassengerRoles[i];
		}
	}

	// Create a new container
	int nNewIndex = m_PassengerRoles.AddToTail();
	Assert( m_PassengerRoles.IsValidIndex( nNewIndex ) );
	
	m_PassengerRoles[nNewIndex].m_strName = strName;
	
	// Supply the index, if requested
	if ( nIndex != NULL )
	{
		*nIndex = nNewIndex;
	}
	
	return &m_PassengerRoles[nNewIndex];
}

ConVar g_debug_npc_vehicle_roles( "g_debug_npc_vehicle_roles", "0" );

//-----------------------------------------------------------------------------
// Purpose: Parse NPC entry and exit anim data
// Input  : *pModelKeyValues - Key values from the vehicle model
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ParseNPCRoles( KeyValues *pkvPassengerList )
{
	// Get the definition section
	if ( pkvPassengerList == NULL )
		return;

	// Get our animating class
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	Assert( pAnimating != NULL );
	if ( pAnimating == NULL )
		return;

	// For attachment polling
	CStudioHdr *pStudioHdr = pAnimating->GetModelPtr();
	Assert( pStudioHdr != NULL );
	if ( pStudioHdr == NULL )
		return;

	// Parse all subkeys
	int nRoleIndex;
	KeyValues *pkvPassengerKey = pkvPassengerList->GetFirstSubKey();
	while ( pkvPassengerKey != NULL )
	{
		string_t strRoleName = AllocPooledString( pkvPassengerKey->GetName() );

		// Find or create the container
		CPassengerRole *pRole = FindOrCreatePassengerRole( strRoleName, &nRoleIndex );
		if ( pRole == NULL )
			continue;

		// Add a new role
		int nSeatIndex = pRole->m_PassengerSeats.AddToTail();
		Assert( pRole->m_PassengerSeats.IsValidIndex( nSeatIndex ) );

		// Parse the information
		ParseNPCPassengerSeat( pkvPassengerKey, &pRole->m_PassengerSeats[nSeatIndex] );
	
		// Add a matching entry into our passenger manifest
		int nPassengerIndex = m_PassengerInfo.AddToTail();
		m_PassengerInfo[nPassengerIndex].m_hPassenger = NULL;
		m_PassengerInfo[nPassengerIndex].m_nSeat = nSeatIndex;
		m_PassengerInfo[nPassengerIndex].m_nRole = nRoleIndex;
		
		// The following are used for index fix-ups after save game restoration
		m_PassengerInfo[nPassengerIndex].m_strRoleName = strRoleName;
		m_PassengerInfo[nPassengerIndex].m_strSeatName = pRole->m_PassengerSeats[nSeatIndex].m_strSeatName;

		// Advance to the next key
		pkvPassengerKey = pkvPassengerKey->GetNextKey();
	}

	// ======================================================================================================
	// Debug print

	if ( g_debug_npc_vehicle_roles.GetBool() )
	{
		Msg("Passenger Roles Parsed:\t%d\n\n", m_PassengerRoles.Count() );
		for ( int i = 0; i < m_PassengerRoles.Count(); i++ )
		{
			Msg("\tPassenger Role:\t%s (%d seats)\n", STRING(m_PassengerRoles[i].m_strName), m_PassengerRoles[i].m_PassengerSeats.Count() );
			
			// Iterate through all information sets under this name
			for ( int j = 0; j < m_PassengerRoles[i].m_PassengerSeats.Count(); j++ )
			{
				Msg("\t\tAttachment: %d\n", m_PassengerRoles[i].m_PassengerSeats[j].m_nAttachmentID );
				
				// Entries
				Msg("\t\tEntries:\t%d\n", m_PassengerRoles[i].m_PassengerSeats[j].m_EntryTransitions.Count() );
				Msg("\t\t=====================\n" );

				for ( int nEntry = 0; nEntry < m_PassengerRoles[i].m_PassengerSeats[j].m_EntryTransitions.Count(); nEntry++ )
				{
					Msg("\t\t\tAnimation:\t%s\t(Priority %d)\n", STRING(m_PassengerRoles[i].m_PassengerSeats[j].m_EntryTransitions[nEntry].m_strAnimationName), 
																 m_PassengerRoles[i].m_PassengerSeats[j].m_EntryTransitions[nEntry].m_nPriority );
				}
				
				Msg("\n");

				// Exits
				Msg("\t\tExits:\t%d\n", m_PassengerRoles[i].m_PassengerSeats[j].m_ExitTransitions.Count() );
				Msg("\t\t=====================\n" );

				for ( int nExits = 0; nExits < m_PassengerRoles[i].m_PassengerSeats[j].m_ExitTransitions.Count(); nExits++ )
				{
					Msg("\t\t\tAnimation:\t%s\t(Priority %d)\n", STRING(m_PassengerRoles[i].m_PassengerSeats[j].m_ExitTransitions[nExits].m_strAnimationName), 
																 m_PassengerRoles[i].m_PassengerSeats[j].m_ExitTransitions[nExits].m_nPriority );
				}
			}

			Msg("\n");
		}
	}

	// ======================================================================================================
}
//-----------------------------------------------------------------------------
// Purpose: Get an attachment point at a specified time in its cycle (note: not exactly a speedy query, use sparingly!)
// Input  : nSequence - sequence to test
//			nAttachmentIndex - attachment to test
//			flCyclePoint - 0.0 - 1.0
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::GetLocalAttachmentAtTime( int nQuerySequence, int nAttachmentIndex, float flCyclePoint, Vector *vecOriginOut, QAngle *vecAnglesOut )
{
	CBaseAnimating *pAnimating = m_pVehicle->GetBaseAnimating();
	if ( pAnimating == NULL )
		return false;

	// TODO: It's annoying to stomp and restore this off each time when we're just going to stomp it again later, but the function 
	//		 should really leave the car in an acceptable state to run this query -- jdw

	// Store this off for restoration later
	int nOldSequence = pAnimating->GetSequence();
	float flOldCycle = pAnimating->GetCycle();

	// Setup the model for the query
	pAnimating->SetSequence( nQuerySequence );
	pAnimating->SetCycle( flCyclePoint );
	pAnimating->InvalidateBoneCache();

	// Query for the point
	Vector vecOrigin;
	QAngle vecAngles;
	pAnimating->GetAttachmentLocal( nAttachmentIndex, vecOrigin, vecAngles );

	if ( vecOriginOut != NULL )
	{
		*vecOriginOut = vecOrigin;
	}

	if ( vecAnglesOut != NULL )
	{
		*vecAnglesOut = vecAngles;
	}

	// Restore the model after the query
	pAnimating->SetSequence( nOldSequence );
	pAnimating->SetCycle( flOldCycle );
	pAnimating->InvalidateBoneCache();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get an attachment point at a specified time in its cycle (note: not exactly a speedy query, use sparingly!)
// Input  : lpszAnimName - name of the sequence to test
//			nAttachmentIndex - attachment to test
//			flCyclePoint - 0.0 - 1.0
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::GetLocalAttachmentAtTime( const char *lpszAnimName, int nAttachmentIndex, float flCyclePoint, Vector *vecOriginOut, QAngle *vecAnglesOut )
{
	CBaseAnimating *pAnimating = m_pVehicle->GetBaseAnimating();
	if ( pAnimating == NULL )
		return false;

	int nQuerySequence = pAnimating->LookupSequence( lpszAnimName );
	if ( nQuerySequence < 0 )
		return false;

	return GetLocalAttachmentAtTime( nQuerySequence, nAttachmentIndex, flCyclePoint, vecOriginOut, vecAnglesOut );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::CacheEntryExitPoints( void )
{
	CBaseAnimating *pAnimating = m_pVehicle->GetBaseAnimating();
	if ( pAnimating == NULL )
		return;

	int nAttachment = pAnimating->LookupAttachment( "vehicle_driver_eyes" );
	
	// For each exit animation, determine where the end point is and cache it
	for ( int i = 0; i < m_ExitAnimations.Count(); i++ )
	{
		if ( GetLocalAttachmentAtTime( m_ExitAnimations[i].szAnimName, nAttachment, 1.0f, &m_ExitAnimations[i].vecExitPointLocal, &m_ExitAnimations[i].vecExitAnglesLocal ) == false )
		{
			Warning("Exit animation %s failed to cache target points properly!\n", m_ExitAnimations[i].szAnimName );
		}

		if ( g_debug_vehicleexit.GetBool() )
		{
			Vector vecExitPoint = m_ExitAnimations[i].vecExitPointLocal;
			QAngle vecExitAngles = m_ExitAnimations[i].vecExitAnglesLocal;
			UTIL_ParentToWorldSpace( pAnimating, vecExitPoint, vecExitAngles );

			NDebugOverlay::Box( vecExitPoint, -Vector(8,8,8), Vector(8,8,8), 0, 255, 0, 0, 20.0f );
			NDebugOverlay::Axis( vecExitPoint, vecExitAngles, 8.0f, true, 20.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ParseEntryExitAnims( void )
{
	// Try and find the right animation to play in the model's keyvalues
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( m_pVehicle->GetModel() ), modelinfo->GetModelKeyValueText( m_pVehicle->GetModel() ) ) )
	{
		// Do we have an entry section?
		KeyValues *pkvEntryList = modelKeyValues->FindKey("vehicle_entry");
		if ( pkvEntryList )
		{
			// Look through the entry animations list
			KeyValues *pkvEntryAnim = pkvEntryList->GetFirstSubKey();
			while ( pkvEntryAnim )
			{
				// Add 'em to our list
				int iIndex = m_EntryAnimations.AddToTail();
				Q_strncpy( m_EntryAnimations[iIndex].szAnimName, pkvEntryAnim->GetName(), sizeof(m_EntryAnimations[iIndex].szAnimName) );
				m_EntryAnimations[iIndex].iHitboxGroup = pkvEntryAnim->GetInt();

				pkvEntryAnim = pkvEntryAnim->GetNextKey();
			}
		}

		// Do we have an exit section?
		KeyValues *pkvExitList = modelKeyValues->FindKey("vehicle_exit");
		if ( pkvExitList )
		{
			ParseExitAnim( pkvExitList, false );
		}

		// Do we have an exit section?
		pkvExitList = modelKeyValues->FindKey("vehicle_escape_exit");
		if ( pkvExitList )
		{
			ParseExitAnim( pkvExitList, true );
		}

		// Parse the NPC vehicle roles as well
		KeyValues *pkvPassengerList = modelKeyValues->FindKey( "vehicle_npc_passengers" );
		if ( pkvPassengerList  )
		{
			ParseNPCRoles( pkvPassengerList );
		}
	}

	modelKeyValues->deleteThis();

	// Determine the entry and exit points for the 
	CacheEntryExitPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::HandlePassengerEntry( CBaseCombatCharacter *pPassenger, bool bAllowEntryOutsideZone )
{
	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		// Find out which hitbox the player's eyepoint is within
		int iEntryAnim = GetEntryAnimForPoint( pPlayer->EyePosition() );

		// Get this interface for animation queries
		CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
		if ( !pAnimating )
			return;

		// Are we in an entrypoint zone? 
		if ( iEntryAnim == ACTIVITY_NOT_AVAILABLE )
		{
			// Normal get in refuses to allow entry
			if ( !bAllowEntryOutsideZone )
				return;

			// We failed to find a valid entry anim, but we've got to get back in because the player's
			// got stuck exiting the vehicle. For now, just use the first get in anim
			// UNDONE: We need a better solution for this.
			iEntryAnim = pAnimating->LookupSequence( m_EntryAnimations[0].szAnimName );
		}

		// Check to see if this vehicle can be controlled or if it's locked
		if ( GetDrivableVehicle()->CanEnterVehicle( pPlayer ) )
		{
			// Make sure the passenger can get in as well
			if ( pPlayer->CanEnterVehicle( this, VEHICLE_ROLE_DRIVER ) )
			{
				// Setup the "enter" vehicle sequence and skip the animation if it isn't present.
				pAnimating->SetCycle( 0 );
				pAnimating->m_flAnimTime = gpGlobals->curtime;
				pAnimating->ResetSequence( iEntryAnim );
				pAnimating->ResetClientsideFrame();
				pAnimating->InvalidateBoneCache();	// This is necessary because we need to query attachment points this frame for blending!
				GetDrivableVehicle()->SetVehicleEntryAnim( true );

				pPlayer->GetInVehicle( this, VEHICLE_ROLE_DRIVER );
			}
		}
	}
	else
	{
		// NPCs handle transitioning themselves, they should NOT call this function
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::HandlePassengerExit( CBaseCombatCharacter *pPassenger )
{
	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		// Clear hud hints
		UTIL_HudHintText( pPlayer, "" );

		vbs_sound_update_t params;
		InitSoundParams(params);
		params.bExitVehicle = true;
		SoundState_Update( params );

		// Find the right exit anim to use based on available exit points.
		Vector vecExitPoint;
		bool bAllPointsBlocked;
		int iSequence = GetExitAnimToUse( vecExitPoint, bAllPointsBlocked );

		// If all exit points were blocked and this vehicle doesn't allow exiting in
		// these cases, bail.
		Vector vecNewPos = pPlayer->GetAbsOrigin();
		QAngle angNewAngles = pPlayer->GetAbsAngles();

		int nRole = GetPassengerRole( pPlayer );
		if ( ( bAllPointsBlocked ) || ( iSequence == ACTIVITY_NOT_AVAILABLE ) )
		{
			// Animation-driven exit points are all blocked, or we have none. Fall back to the more simple static exit points.
			if ( !GetPassengerExitPoint( nRole, &vecNewPos, &angNewAngles ) && !GetDrivableVehicle()->AllowBlockedExit( pPlayer, nRole ) )
				return false;

			// At this point, the player has exited the vehicle but did so without playing an animation.  We need to give the vehicle a
			// chance to do any post-animation clean-up it may need to perform.
			HandleEntryExitFinish( false, true );
		}

		// Now we either have an exit sequence to play, a valid static exit position, or we don't care
		// whether we're blocked or not. We're getting out, one way or another.
		GetDrivableVehicle()->PreExitVehicle( pPlayer, nRole );

		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
			if ( pAnimating )
			{
				pAnimating->SetCycle( 0 );
				pAnimating->m_flAnimTime = gpGlobals->curtime;
				pAnimating->ResetSequence( iSequence );
				pAnimating->ResetClientsideFrame();
				GetDrivableVehicle()->SetVehicleExitAnim( true, vecExitPoint );

				// Re-deploy our weapon
				if ( pPlayer && pPlayer->IsAlive() )
				{
					if ( pPlayer->GetActiveWeapon() )
					{
						pPlayer->GetActiveWeapon()->Deploy();
						pPlayer->ShowCrosshair( true );
					}
				}

				// To prevent anything moving into the volume the player's going to occupy at the end of the exit
				// NOTE: Set the player as the blocker's owner so the player is allowed to intersect it
				Vector vecExitFeetPoint = vecExitPoint - VEC_VIEW;
				m_hExitBlocker = CEntityBlocker::Create( vecExitFeetPoint, VEC_HULL_MIN, VEC_HULL_MAX, pPlayer, true );

				// We may as well stand where we're going to get out at and stop being parented
				pPlayer->SetAbsOrigin( vecExitFeetPoint );
				pPlayer->SetParent( NULL );

				return true;
			}
		}

		// Couldn't find an animation, so exit immediately
		pPlayer->LeaveVehicle( vecNewPos, angNewAngles );
		return true;
	}
	else
	{
		// NPCs handle transitioning themselves, they should NOT call this function
		Assert( 0 );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseServerVehicle::GetEntryAnimForPoint( const Vector &vecEyePoint )
{
	// Parse the vehicle animations the first time they get in the vehicle
	if ( !m_bParsedAnimations )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	// No entry anims? Vehicles with no entry anims are always enterable.
	if ( !m_EntryAnimations.Count() )
		return 0;

	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( !pAnimating )
		return 0;

	CStudioHdr *pStudioHdr = pAnimating->GetModelPtr();
	if (!pStudioHdr)
		return 0;
	int iHitboxSet = FindHitboxSetByName( pStudioHdr, "entryboxes" );
	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( iHitboxSet );
	if ( !set || !set->numhitboxes )
		return 0;

	// Loop through the hitboxes and find out which one we're in
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox( i );

		Vector vecPosition;
		QAngle vecAngles;
		pAnimating->GetBonePosition( pbox->bone, vecPosition, vecAngles );

		// Build a rotation matrix from orientation
		matrix3x4_t fRotateMatrix;
		AngleMatrix( vecAngles, vecPosition, fRotateMatrix);

		Vector localEyePoint;
		VectorITransform( vecEyePoint, fRotateMatrix, localEyePoint );
		if ( IsPointInBox( localEyePoint, pbox->bbmin, pbox->bbmax ) )
		{
			// Find the entry animation for this hitbox
			int iCount = m_EntryAnimations.Count();
			for ( int entry = 0; entry < iCount; entry++ )
			{
				if ( m_EntryAnimations[entry].iHitboxGroup == pbox->group )
				{
					// Get the sequence for the animation
					return pAnimating->LookupSequence( m_EntryAnimations[entry].szAnimName );
				}
			}
		}
	}

	// Fail
	return ACTIVITY_NOT_AVAILABLE;
}

//-----------------------------------------------------------------------------
// Purpose: Find an exit animation that'll get the player to a valid position
// Input  : vecEyeExitEndpoint - Returns with the final eye position after exiting.
//			bAllPointsBlocked - Returns whether all exit points were found to be blocked.
// Output : 
//-----------------------------------------------------------------------------
int CBaseServerVehicle::GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked )
{
	bAllPointsBlocked = false;
	
	// Parse the vehicle animations the first time they get in the vehicle
	if ( !m_bParsedAnimations )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	// No exit anims? 
	if ( !m_ExitAnimations.Count() )
		return ACTIVITY_NOT_AVAILABLE;

	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(m_pVehicle);
	if ( !pAnimating )
		return ACTIVITY_NOT_AVAILABLE;

	CStudioHdr *pStudioHdr = pAnimating->GetModelPtr();
	if (!pStudioHdr)
		return ACTIVITY_NOT_AVAILABLE;

	bool bUpright = IsVehicleUpright();

	// Loop through the exit animations and find one that ends in a clear position
	// Also attempt to choose the animation which brings you closest to your view direction.
	CBasePlayer *pPlayer = ToBasePlayer( GetDriver() );
	if ( pPlayer == NULL )
		return ACTIVITY_NOT_AVAILABLE;

	int nRole = GetPassengerRole( pPlayer );

	int nBestExitAnim = -1;
	bool bBestExitIsEscapePoint = true;
	Vector vecViewDirection, vecViewOrigin, vecBestExitPoint( 0, 0, 0 );
	vecViewOrigin = pPlayer->EyePosition();
	pPlayer->EyeVectors( &vecViewDirection );
	vecViewDirection.z = 0.0f;
	VectorNormalize( vecViewDirection );

	float flMaxCosAngleDelta = -2.0f;

	int iCount = m_ExitAnimations.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( m_ExitAnimations[i].bUpright != bUpright )
			continue;

		// Don't use an escape point if we found a non-escape point already
		if ( !bBestExitIsEscapePoint && m_ExitAnimations[i].bEscapeExit )
			continue;
		
		Vector vehicleExitOrigin;
		QAngle vehicleExitAngles;

		// NOTE: HL2 and Ep1 used a method that relied on the animators to place attachment points in the model which marked where
		//		 the player would exit to.  This was rendered unnecessary in Ep2, but the choreo vehicles of these older products
		//		 did not have proper exit animations and relied on the exact queries that were happening before.  For choreo vehicles,
		//		 we now just allow them to perform those older queries to keep those products happy.  - jdw

		// Get the position we think we're going to end up at
		if ( m_bUseLegacyExitChecks )
		{
			pAnimating->GetAttachment( m_ExitAnimations[i].szAnimName, vehicleExitOrigin, vehicleExitAngles );
		}
		else
		{
			vehicleExitOrigin = m_ExitAnimations[i].vecExitPointLocal;
			vehicleExitAngles = m_ExitAnimations[i].vecExitAnglesLocal;
			UTIL_ParentToWorldSpace( pAnimating, vehicleExitOrigin, vehicleExitAngles );
		}

		// Don't bother checking points which are farther from our view direction.
		Vector vecDelta;
		VectorSubtract( vehicleExitOrigin, vecViewOrigin, vecDelta );
		vecDelta.z = 0.0f;
		VectorNormalize( vecDelta );
		float flCosAngleDelta = DotProduct( vecDelta, vecViewDirection );

		// But always check non-escape exits if our current best exit is an escape exit.
		if ( !bBestExitIsEscapePoint || m_ExitAnimations[i].bEscapeExit )
		{
			if ( flCosAngleDelta < flMaxCosAngleDelta )
				continue;
		}

		// The attachment points are where the driver's eyes will end up, so we subtract the view offset
		// to get the actual exit position.
		vehicleExitOrigin -= VEC_VIEW;

		Vector vecMove(0,0,64);
		Vector vecStart = vehicleExitOrigin + vecMove;
		Vector vecEnd = vehicleExitOrigin - vecMove;

		// Starting at the exit point, trace a flat plane down until we hit ground
		// NOTE: The hull has no vertical span because we want to test the lateral constraints against the ground, not height (yet)
		trace_t tr;
		UTIL_TraceHull( vecStart, vecEnd, VEC_HULL_MIN, Vector( VEC_HULL_MAX.x, VEC_HULL_MAX.y, VEC_HULL_MIN.z ), MASK_PLAYERSOLID, NULL, COLLISION_GROUP_NONE, &tr );
		
		if ( g_debug_vehicleexit.GetBool() )
		{
			NDebugOverlay::SweptBox( vecStart, vecEnd, VEC_HULL_MIN, Vector( VEC_HULL_MAX.x, VEC_HULL_MAX.y, VEC_HULL_MIN.y ), vec3_angle, 255, 255, 255, 8.0f, 20.0f );
		}
		
		if ( tr.fraction < 1.0f )
		{
			// If we hit the ground, try to now "stand up" at that point to see if we'll fit
			UTIL_TraceHull( tr.endpos, tr.endpos, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_NONE, &tr );
			
			// See if we're unable to stand at this space
			if ( tr.startsolid )
			{
				if ( g_debug_vehicleexit.GetBool() )
				{
					NDebugOverlay::Box( tr.endpos, VEC_HULL_MIN, VEC_HULL_MAX, 255, 0, 0, 8.0f, 20.0f );
				}
				continue;
			}

			if ( g_debug_vehicleexit.GetBool() )
			{
				NDebugOverlay::Box( tr.endpos, VEC_HULL_MIN, VEC_HULL_MAX, 0, 255, 0, 8.0f, 20.0f );
			}
		}
		else if ( tr.allsolid || ( ( tr.fraction == 1.0 ) && !GetDrivableVehicle()->AllowMidairExit( pPlayer, nRole ) ) )
		{
			if ( g_debug_vehicleexit.GetBool() )
			{
				NDebugOverlay::Box( tr.endpos, VEC_HULL_MIN, VEC_HULL_MAX, 255,0,0, 64, 10 );
			}
			continue;
		}
		
		// Calculate the exit endpoint & viewpoint
		Vector vecExitEndPoint = tr.endpos;

		// Make sure we can trace to the center of the exit point
		UTIL_TraceLine( vecViewOrigin, vecExitEndPoint, MASK_PLAYERSOLID, pAnimating, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
		{
#ifdef HL2_EPISODIC
			if ( ShouldVehicleIgnoreEntity( GetVehicleEnt(), tr.m_pEnt ) == false )
#endif //HL2_EPISODIC
			{
				if ( g_debug_vehicleexit.GetBool() )
				{
					NDebugOverlay::Line( vecViewOrigin, vecExitEndPoint, 255,0,0, true, 10 );
				}
				continue;
			}
		}

		bBestExitIsEscapePoint = m_ExitAnimations[i].bEscapeExit;
		vecBestExitPoint = vecExitEndPoint;
		nBestExitAnim = i;
		flMaxCosAngleDelta = flCosAngleDelta;
	}

	if ( nBestExitAnim >= 0 )
	{
		m_vecCurrentExitEndPoint = vecBestExitPoint;

		if ( g_debug_vehicleexit.GetBool() )
		{
			NDebugOverlay::Cross3D( m_vecCurrentExitEndPoint, 16, 0, 255, 0, true, 10 );
			NDebugOverlay::Box( m_vecCurrentExitEndPoint, VEC_HULL_MIN, VEC_HULL_MAX, 255,255,255, 8, 10 );
		}

		vecEyeExitEndpoint = vecBestExitPoint + VEC_VIEW;
		m_iCurrentExitAnim = nBestExitAnim;
		return pAnimating->LookupSequence( m_ExitAnimations[m_iCurrentExitAnim].szAnimName );
	}

	// Fail, all exit points were blocked.
	bAllPointsBlocked = true;
	return ACTIVITY_NOT_AVAILABLE;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::HandleEntryExitFinish( bool bExitAnimOn, bool bResetAnim )
{
	// Parse the vehicle animations. This is needed because they may have 
	// saved, and loaded during exit anim, which would clear the exit anim.
	if ( !m_bParsedAnimations )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	// Figure out which entrypoint hitbox the player is in
	CBaseAnimating *pAnimating = m_pVehicle->GetBaseAnimating();
	if ( !pAnimating )
		return;
		
	// Did the entry anim just finish?
	if ( bExitAnimOn )
	{
		// The exit animation just finished
		CBasePlayer *pPlayer = ToBasePlayer( GetDriver() );
		if ( pPlayer != NULL )
		{
			Vector vecEyes;
			QAngle vecEyeAng;
			if ( m_iCurrentExitAnim >= 0 && m_iCurrentExitAnim < m_ExitAnimations.Count() )
			{
				// Convert our offset points to worldspace ones
				vecEyes = m_ExitAnimations[m_iCurrentExitAnim].vecExitPointLocal;
				vecEyeAng = m_ExitAnimations[m_iCurrentExitAnim].vecExitAnglesLocal;
				UTIL_ParentToWorldSpace( pAnimating, vecEyes, vecEyeAng );

				// Use the endpoint we figured out when we exited
				vecEyes = m_vecCurrentExitEndPoint;
			}
			else
			{
				pAnimating->GetAttachment( "vehicle_driver_eyes", vecEyes, vecEyeAng );
			}

			if ( g_debug_vehicleexit.GetBool() )
			{
				NDebugOverlay::Box( vecEyes, -Vector(2,2,2), Vector(2,2,2), 255,0,0, 64, 10.0 );
			}

			// If the end point isn't clear, get back in the vehicle
			/*
			trace_t tr;
			UTIL_TraceHull( vecEyes, vecEyes, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid && tr.fraction < 1.0 )
			{
				pPlayer->LeaveVehicle( vecEyes, vecEyeAng );
				m_pVehicle->Use( pPlayer, pPlayer, USE_TOGGLE, 1 );
				return;
			}
			*/

			pPlayer->LeaveVehicle( vecEyes, vecEyeAng );
		}
	}

	// Only reset the animation if we're told to
	if ( bResetAnim )
	{
		// Start the vehicle idling again
		int iSequence = pAnimating->SelectWeightedSequence( ACT_IDLE );
		if ( iSequence > ACTIVITY_NOT_AVAILABLE )
		{
			pAnimating->SetCycle( 0 );
			pAnimating->m_flAnimTime = gpGlobals->curtime;
			pAnimating->ResetSequence( iSequence );
			pAnimating->ResetClientsideFrame();
		}
	}

	GetDrivableVehicle()->SetVehicleEntryAnim( false );
	GetDrivableVehicle()->SetVehicleExitAnim( false, vec3_origin );
}

//-----------------------------------------------------------------------------
// Purpose: Where does the passenger see from?
//-----------------------------------------------------------------------------
void CBaseServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*= NULL*/ )
{
	Assert( nRole == VEHICLE_ROLE_DRIVER );
	CBaseCombatCharacter *pPassenger = GetPassenger( VEHICLE_ROLE_DRIVER );
	Assert( pPassenger );

	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		// Call through the player to resolve the actual position (if available)
		if ( pAbsOrigin != NULL )
		{
			*pAbsOrigin = pPlayer->EyePosition();
		}
		
		if ( pAbsAngles != NULL )
		{
			*pAbsAngles = pPlayer->EyeAngles();
		}

		if ( pFOV )
		{
			*pFOV = pPlayer->GetFOV();
		}
	}
	else
	{
		// NPCs are not supported
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	GetDrivableVehicle()->SetupMove( player, ucmd, pHelper, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData )
{
	GetDrivableVehicle()->ProcessMovement( pPlayer, pMoveData );

	trace_t	tr;
	UTIL_TraceLine( pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin() - Vector( 0, 0, 256 ), MASK_PLAYERSOLID, GetVehicleEnt(), COLLISION_GROUP_NONE, &tr );

	// If our gamematerial has changed, tell any player surface triggers that are watching
	IPhysicsSurfaceProps *physprops = MoveHelper()->GetSurfaceProps();
	const surfacedata_t *pSurfaceProp = physprops->GetSurfaceData( tr.surface.surfaceProps );
	char cCurrGameMaterial = pSurfaceProp->game.material;

	// Changed?
	if ( m_chPreviousTextureType != cCurrGameMaterial )
	{
		CEnvPlayerSurfaceTrigger::SetPlayerSurface( pPlayer, cCurrGameMaterial );
	}

	m_chPreviousTextureType = cCurrGameMaterial;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	GetDrivableVehicle()->FinishMove( player, ucmd, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::ItemPostFrame( CBasePlayer *player )
{
	Assert( player == GetDriver() );

	GetDrivableVehicle()->ItemPostFrame( player );

	if ( player->m_afButtonPressed & IN_USE )
	{
		if ( GetDrivableVehicle()->CanExitVehicle(player) )
		{
			if ( !HandlePassengerExit( player ) && ( player != NULL ) )
			{
				player->PlayUseDenySound();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_ThrottleForward( void )
{
	m_nNPCButtons |= IN_FORWARD;
	m_nNPCButtons &= ~IN_BACK;
	m_nNPCButtons &= ~IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_ThrottleReverse( void )
{
	m_nNPCButtons |= IN_BACK;
	m_nNPCButtons &= ~IN_FORWARD;
	m_nNPCButtons &= ~IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_ThrottleCenter( void )
{
	m_nNPCButtons &= ~IN_FORWARD;
	m_nNPCButtons &= ~IN_BACK;
	m_nNPCButtons &= ~IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_Brake( void )
{
	m_nNPCButtons &= ~IN_FORWARD;
	m_nNPCButtons &= ~IN_BACK;
	m_nNPCButtons |= IN_JUMP;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_TurnLeft( float flDegrees )
{
	m_nNPCButtons |= IN_MOVELEFT;
	m_nNPCButtons &= ~IN_MOVERIGHT;
	m_flTurnDegrees = -flDegrees;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_TurnRight( float flDegrees )
{
	m_nNPCButtons |= IN_MOVERIGHT;
	m_nNPCButtons &= ~IN_MOVELEFT;
	m_flTurnDegrees = flDegrees;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_TurnCenter( void )
{
	m_nNPCButtons &= ~IN_MOVERIGHT;
	m_nNPCButtons &= ~IN_MOVELEFT;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_PrimaryFire( void )
{
	m_nNPCButtons |= IN_ATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::NPC_SecondaryFire( void )
{
	m_nNPCButtons |= IN_ATTACK2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = 64;
	*flMaxRange = 1024;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange )
{
	*flMinRange = 64;
	*flMaxRange = 1024;
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's primary weapon can fire again
//-----------------------------------------------------------------------------
float CBaseServerVehicle::Weapon_PrimaryCanFireAt( void )
{
	return gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Return the time at which this vehicle's secondary weapon can fire again
//-----------------------------------------------------------------------------
float CBaseServerVehicle::Weapon_SecondaryCanFireAt( void )
{
	return gpGlobals->curtime;
}

const char *pSoundStateNames[] =
{
	"SS_NONE",
	"SS_SHUTDOWN",
	"SS_SHUTDOWN_WATER",
	"SS_START_WATER",
	"SS_START_IDLE",
	"SS_IDLE",
	"SS_GEAR_0",
	"SS_GEAR_1",
	"SS_GEAR_2",
	"SS_GEAR_3",
	"SS_GEAR_4",
	"SS_SLOWDOWN",
	"SS_SLOWDOWN_HIGHSPEED",
	"SS_GEAR_0_RESUME",
	"SS_GEAR_1_RESUME",
	"SS_GEAR_2_RESUME",
	"SS_GEAR_3_RESUME",
	"SS_GEAR_4_RESUME",
	"SS_TURBO",
	"SS_REVERSE",
};


static int SoundStateIndexFromName( const char *pName )
{
	for ( int i = 0; i < SS_NUM_STATES; i++ )
	{
		Assert( i < ARRAYSIZE(pSoundStateNames) );
		if ( !strcmpi( pSoundStateNames[i], pName ) )
			return i;
	}
	return -1;
}

static const char *SoundStateNameFromIndex( int index )
{
	index = clamp(index, 0, SS_NUM_STATES-1 );
	return pSoundStateNames[index];
}

void CBaseServerVehicle::PlaySound( const char *pSound )
{
	if ( !pSound || !pSound[0] )
		return;

	if ( g_debug_vehiclesound.GetInt() )
	{
		Msg("Playing non-looping vehicle sound: %s\n", pSound );
	}
	m_pVehicle->EmitSound( pSound );
}

void CBaseServerVehicle::StopLoopingSound( float fadeTime )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pStateSoundFade )
	{
		controller.SoundDestroy( m_pStateSoundFade );
		m_pStateSoundFade = NULL;
	}
	if ( m_pStateSound )
	{
		m_pStateSoundFade = m_pStateSound;
		m_pStateSound = NULL;
		controller.SoundFadeOut( m_pStateSoundFade, fadeTime, false );
	}
}

void CBaseServerVehicle::PlayLoopingSound( const char *pSoundName )
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter( m_pVehicle );
	CSoundPatch *pNewSound = NULL;
	if ( pSoundName && pSoundName[0] )
	{
		pNewSound = controller.SoundCreate( filter, m_pVehicle->entindex(), CHAN_STATIC, pSoundName, ATTN_NORM );
	}

	if ( m_pStateSound && pNewSound && controller.SoundGetName( pNewSound ) == controller.SoundGetName( m_pStateSound ) )
	{
		// if the sound is the same, don't play this, just re-use the old one
		controller.SoundDestroy( pNewSound );
		pNewSound = m_pStateSound;
		controller.SoundChangeVolume( pNewSound, 1.0f, 0.0f );
		m_pStateSound = NULL;
	}
	else if ( g_debug_vehiclesound.GetInt() )
	{
		const char *pStopSound = m_pStateSound ?  controller.SoundGetName( m_pStateSound ).ToCStr() : "NULL";
		const char *pStartSound = pNewSound ?  controller.SoundGetName( pNewSound ).ToCStr() : "NULL";
		Msg("Stop %s, start %s\n", pStopSound, pStartSound );
	}

	StopLoopingSound();
	m_pStateSound = pNewSound;
	if ( m_pStateSound )
	{
		controller.Play( m_pStateSound, 1.0f, 100 );
	}
}

static sound_states MapGearToState( vbs_sound_update_t &params, int gear )
{
	switch( gear )
	{
	case 0: return params.bReverse ? SS_REVERSE : SS_GEAR_0;
	case 1: return SS_GEAR_1;
	case 2: return SS_GEAR_2;
	case 3: return SS_GEAR_3;
	default:case 4: return SS_GEAR_4;
	}
}

static sound_states MapGearToMidState( vbs_sound_update_t &params, int gear )
{
	switch( gear )
	{
	case 0: return params.bReverse ? SS_REVERSE : SS_GEAR_0_RESUME;
	case 1: return SS_GEAR_1_RESUME;
	case 2: return SS_GEAR_2_RESUME;
	case 3: return SS_GEAR_3_RESUME;
	default:case 4: return SS_GEAR_4_RESUME;
	}
}

bool CBaseServerVehicle::PlayCrashSound( float speed )
{
	int i;
	float delta = 0;
	float absSpeed = fabs(speed);
	float absLastSpeed = fabs(m_lastSpeed);
	if ( absLastSpeed > absSpeed )
	{
		delta = fabs(m_lastSpeed - speed);
	}
	
	float rumble = delta / 8.0f;

	if( rumble > 60.0f )
		rumble = 60.0f;

	if( rumble > 5.0f )
	{
		if ( GetDriver() )
		{
			UTIL_ScreenShake( GetDriver()->GetAbsOrigin(), rumble, 150.0f, 1.0f, 240.0f, SHAKE_START_RUMBLEONLY, true );
		}
	}

	for ( i = 0; i < m_vehicleSounds.crashSounds.Count(); i++ )
	{
		const vehicle_crashsound_t &crash = m_vehicleSounds.crashSounds[i];
		if ( !crash.gearLimit )
			continue;

		if ( m_iSoundGear <= crash.gearLimit )
		{
			if ( delta > crash.flMinDeltaSpeed && absLastSpeed > crash.flMinSpeed )
			{
				PlaySound( crash.iszCrashSound.ToCStr() );
				return true;
			}
		}
	}

	for ( i = m_vehicleSounds.crashSounds.Count()-1; i >= 0; --i )
	{
		const vehicle_crashsound_t &crash = m_vehicleSounds.crashSounds[i];
		if ( delta > crash.flMinDeltaSpeed && absLastSpeed > crash.flMinSpeed )
		{
			PlaySound( crash.iszCrashSound.ToCStr() );
			return true;
		}
	}
	return false;
}


bool CBaseServerVehicle::CheckCrash( vbs_sound_update_t &params )
{
	if ( params.bVehicleInWater )
		return false;

	bool bCrashed = PlayCrashSound( params.flWorldSpaceSpeed );
	if ( bCrashed )
	{
		if ( g_debug_vehiclesound.GetInt() )
		{
			Msg("Crashed!: speed %.2f, lastSpeed %.2f\n", params.flWorldSpaceSpeed, m_lastSpeed );
		}
	}
	m_lastSpeed = params.flWorldSpaceSpeed;
	return bCrashed;
}


sound_states CBaseServerVehicle::SoundState_ChooseState( vbs_sound_update_t &params )
{
	float timeInState = gpGlobals->curtime - m_soundStateStartTime;
	bool bInStateForMinTime = timeInState > m_vehicleSounds.minStateTime[m_soundState] ? true : false;

	sound_states stateOut = m_soundState;

	// exit overrides everything else
	if ( params.bExitVehicle )
	{
		switch ( m_soundState )
		{
		case SS_NONE:
		case SS_SHUTDOWN:
		case SS_SHUTDOWN_WATER:
			return m_soundState;
		}
		return SS_SHUTDOWN;
	}

	// check global state in states that don't mask them
	switch( m_soundState )
	{
	// global states masked for these states.
	case SS_NONE:
	case SS_START_IDLE:
	case SS_SHUTDOWN:
		break;
	case SS_START_WATER:
	case SS_SHUTDOWN_WATER:
		if ( !params.bVehicleInWater )
			return SS_START_IDLE;
		break;

	case SS_TURBO:
		if ( params.bVehicleInWater )
			return SS_SHUTDOWN_WATER;
		if ( CheckCrash(params) )
			return SS_IDLE;
		break;
	
	case SS_IDLE:
		if ( params.bVehicleInWater )
			return SS_SHUTDOWN_WATER;
		break;

	case SS_REVERSE:
	case SS_GEAR_0:
	case SS_GEAR_1:
	case SS_GEAR_2:
	case SS_GEAR_3:
	case SS_GEAR_4:
	case SS_SLOWDOWN:
	case SS_SLOWDOWN_HIGHSPEED:
	case SS_GEAR_0_RESUME:
	case SS_GEAR_1_RESUME:
	case SS_GEAR_2_RESUME:
	case SS_GEAR_3_RESUME:
	case SS_GEAR_4_RESUME:
		if ( params.bVehicleInWater )
		{
			return SS_SHUTDOWN_WATER;
		}
		if ( params.bTurbo )
		{
			return SS_TURBO;
		}
		if ( CheckCrash(params) )
			return SS_IDLE;
		break;
	}

	switch( m_soundState )
	{
	case SS_START_IDLE:
		if ( bInStateForMinTime || params.bThrottleDown )
			return SS_IDLE;
		break;
	case SS_IDLE:
		if ( bInStateForMinTime && params.bThrottleDown )
		{
			if ( params.bTurbo )
				return SS_TURBO;
			return params.bReverse ? SS_REVERSE : SS_GEAR_0;
		}
		break;

	case SS_GEAR_0_RESUME:
	case SS_GEAR_0:
		if ( (bInStateForMinTime && !params.bThrottleDown) || params.bReverse )
		{
			return SS_IDLE;
		}
		if ( m_iSoundGear > 0 )
		{
			return SS_GEAR_1;
		}
		break;
	case SS_GEAR_1_RESUME:
	case SS_GEAR_1:
		if ( bInStateForMinTime )
		{
			if ( !params.bThrottleDown )
				return SS_SLOWDOWN;
		}
		if ( m_iSoundGear != 1 )
			return MapGearToState( params, m_iSoundGear);
		break;

	case SS_GEAR_2_RESUME:
	case SS_GEAR_2:
		if ( bInStateForMinTime )
		{
			if ( !params.bThrottleDown )
				return SS_SLOWDOWN;
			else if ( m_iSoundGear != 2 )
				return MapGearToState(params, m_iSoundGear);
		}
		break;

	case SS_GEAR_3_RESUME:
	case SS_GEAR_3:
		if ( bInStateForMinTime )
		{
			if ( !params.bThrottleDown )
				return SS_SLOWDOWN;
			else if ( m_iSoundGear != 3 )
				return MapGearToState(params, m_iSoundGear);
		}
		break;

	case SS_GEAR_4_RESUME:
	case SS_GEAR_4:
		if ( bInStateForMinTime && !params.bThrottleDown )
		{
			return SS_SLOWDOWN;
		}
		if ( m_iSoundGear != 4 )
		{
			return MapGearToMidState(params, m_iSoundGear);
		}
		break;
	case SS_REVERSE:
		if ( bInStateForMinTime && !params.bReverse )
		{
			return SS_SLOWDOWN;
		}
		break;

	case SS_SLOWDOWN_HIGHSPEED:
	case SS_SLOWDOWN:
		if ( params.bThrottleDown )
		{
			// map gears
			return MapGearToMidState(params, m_iSoundGear);
		}
		if ( m_iSoundGear == 0 )
		{
			return SS_IDLE;
		}
		break;

	case SS_NONE:
		stateOut = params.bVehicleInWater ? SS_START_WATER : SS_START_IDLE;
		break;
	case SS_TURBO:
		if ( bInStateForMinTime && !params.bTurbo )
		{
			return MapGearToMidState(params, m_iSoundGear);
		}
		break;
	default:
		break;
	}

	return stateOut;
}

const char *CBaseServerVehicle::StateSoundName( sound_states state )
{
	return m_vehicleSounds.iszStateSounds[state].ToCStr();
}

void CBaseServerVehicle::SoundState_OnNewState( sound_states lastState )
{
	if ( g_debug_vehiclesound.GetInt() )
	{
		int index = m_soundState;
		Msg("Switched to state: %d (%s)\n", m_soundState, SoundStateNameFromIndex(index) );
	}

	switch ( m_soundState )
	{
	case SS_SHUTDOWN:
	case SS_SHUTDOWN_WATER:
	case SS_START_WATER:
		StopLoopingSound();
		PlaySound( StateSoundName(m_soundState) );
		break;
	case SS_IDLE:
		m_lastSpeed = -1;
		PlayLoopingSound( StateSoundName(m_soundState) );
		break;
	case SS_START_IDLE:
	case SS_REVERSE:
	case SS_GEAR_0:
	case SS_GEAR_0_RESUME:
	case SS_GEAR_1:
	case SS_GEAR_1_RESUME:
	case SS_GEAR_2:
	case SS_GEAR_2_RESUME:
	case SS_GEAR_3:
	case SS_GEAR_3_RESUME:
	case SS_GEAR_4:
	case SS_GEAR_4_RESUME:
	case SS_TURBO:
		PlayLoopingSound( StateSoundName(m_soundState) );
		break;

	case SS_SLOWDOWN_HIGHSPEED:
	case SS_SLOWDOWN:
		if ( m_iSoundGear < 2 )
		{
			PlayLoopingSound( StateSoundName( SS_SLOWDOWN ) );
		}
		else
		{
			PlayLoopingSound( StateSoundName( SS_SLOWDOWN_HIGHSPEED ) );
		}
		break;
		
	default:break;
	}

	m_soundStateStartTime = gpGlobals->curtime;
}


void CBaseServerVehicle::SoundState_Update( vbs_sound_update_t &params )
{
	sound_states newState = SoundState_ChooseState( params );
	if ( newState != m_soundState )
	{
		sound_states lastState = m_soundState;
		m_soundState = newState;
		SoundState_OnNewState( lastState );
	}

	switch( m_soundState )
	{
	case SS_SHUTDOWN:
	case SS_SHUTDOWN_WATER:
	case SS_START_WATER:
	case SS_START_IDLE:
	case SS_IDLE:
	case SS_REVERSE:
	case SS_GEAR_0:
	case SS_GEAR_4:
	case SS_SLOWDOWN_HIGHSPEED:
	case SS_SLOWDOWN:
	case SS_GEAR_0_RESUME:
	case SS_GEAR_4_RESUME:
		break;
	default:break;
	}
}

void CBaseServerVehicle::InitSoundParams( vbs_sound_update_t &params )
{
	params.Defaults();
	params.bVehicleInWater = IsVehicleBodyInWater();
}

//-----------------------------------------------------------------------------
// Purpose: Vehicle Sound Start
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SoundStart()
{
	StartEngineRumble();

	m_soundState = SS_NONE;
	vbs_sound_update_t params;
	InitSoundParams(params);

	SoundState_Update( params );
}

// vehicle is starting up disabled, but in some cases you still want to play a sound
// HACK: handle those here.
void CBaseServerVehicle::SoundStartDisabled()
{
	m_soundState = SS_NONE;
	vbs_sound_update_t params;
	InitSoundParams(params);
	sound_states newState = SoundState_ChooseState( params );

	switch( newState )
	{
	case SS_START_WATER:
		PlaySound( StateSoundName(newState) );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SoundShutdown( float flFadeTime )
{
	StopEngineRumble();

	// Stop any looping sounds that may be running, as the following stop sound may not exist
	// and thus leave a looping sound playing after the user gets out.
	for ( int i = 0; i < NUM_SOUNDS_TO_STOP_ON_EXIT; i++ )
	{
		StopSound( g_iSoundsToStopOnExit[i] );
	}

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pStateSoundFade )
	{
		controller.SoundFadeOut( m_pStateSoundFade, flFadeTime, true );
		m_pStateSoundFade = NULL;
	}
	if ( m_pStateSound )
	{
		controller.SoundFadeOut( m_pStateSound, flFadeTime, true );
		m_pStateSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseServerVehicle::SoundUpdate( vbs_sound_update_t &params )
{
	if ( g_debug_vehiclesound.GetInt() > 1 )
	{
		Msg("Throttle: %s, Reverse: %s\n", params.bThrottleDown?"on":"off", params.bReverse?"on":"off" );
	}

	float flCurrentSpeed = params.flCurrentSpeedFraction;
	if ( g_debug_vehiclesound.GetInt() > 1 )
	{
		Msg("CurrentSpeed: %.3f  ", flCurrentSpeed );
	}

	// Figure out our speed for the purposes of sound playing.
	// We slow the transition down a little to make the gear changes slower.
	if ( m_vehicleSounds.pGears.Count() > 0 )
	{
		if ( flCurrentSpeed > m_flSpeedPercentage )
		{
			// don't accelerate when the throttle isn't down
			if ( !params.bThrottleDown )
			{
				flCurrentSpeed = m_flSpeedPercentage;
			}
			flCurrentSpeed = Approach( flCurrentSpeed, m_flSpeedPercentage, params.flFrameTime * m_vehicleSounds.pGears[m_iSoundGear].flSpeedApproachFactor );
		}
	}
	m_flSpeedPercentage = clamp( flCurrentSpeed, 0.0f, 1.0f );

	if ( g_debug_vehiclesound.GetInt() > 1 )
	{
		Msg("Sound Speed: %.3f\n", m_flSpeedPercentage );
	}

	// Only do gear changes when the throttle's down
	RecalculateSoundGear( params );

	SoundState_Update( params );
}

//-----------------------------------------------------------------------------
// Purpose: Play a non-gear based vehicle sound
//-----------------------------------------------------------------------------
void CBaseServerVehicle::PlaySound( vehiclesound iSound )
{
	if ( m_vehicleSounds.iszSound[iSound] != NULL_STRING )
	{
		CPASAttenuationFilter filter( m_pVehicle );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_VOICE;
		ep.m_pSoundName = STRING(m_vehicleSounds.iszSound[iSound]);
		ep.m_flVolume = m_flVehicleVolume;
		ep.m_SoundLevel = SNDLVL_NORM;

		CBaseEntity::EmitSound( filter, m_pVehicle->entindex(), ep );
		if ( g_debug_vehiclesound.GetInt() )
		{
			Msg("Playing vehicle sound: %s\n", ep.m_pSoundName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop a non-gear based vehicle sound
//-----------------------------------------------------------------------------
void CBaseServerVehicle::StopSound( vehiclesound iSound )
{
	if ( m_vehicleSounds.iszSound[iSound] != NULL_STRING )
	{
		CBaseEntity::StopSound( m_pVehicle->entindex(), CHAN_VOICE, STRING(m_vehicleSounds.iszSound[iSound]) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the gear we should be in based upon the vehicle's current speed
//-----------------------------------------------------------------------------
void CBaseServerVehicle::RecalculateSoundGear( vbs_sound_update_t &params )
{
	int iNumGears = m_vehicleSounds.pGears.Count();
	for ( int i = (iNumGears-1); i >= 0; i-- )
	{
		if ( m_flSpeedPercentage > m_vehicleSounds.pGears[i].flMinSpeed )
		{
			m_iSoundGear = i;
			break;
		}
	}

	// If we're going in reverse, we want to stay in first gear
	if ( params.bReverse )
	{
		m_iSoundGear = 0;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseServerVehicle::StartEngineRumble()
{
	return;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBaseServerVehicle::StopEngineRumble()
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Find the passenger in the given seat of the vehicle
// Input  : nSeatID - seat ID to check
// Output : CBaseCombatCharacter - character in the seat
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CBaseServerVehicle::NPC_GetPassengerInSeat( int nRoleID, int nSeatID )
{
	// Search all passengers in the vehicle
	for ( int i = 0; i < m_PassengerInfo.Count(); i++ )
	{
		// If the seat ID matches, return the entity in that seat
		if ( m_PassengerInfo[i].GetSeat() == nSeatID && m_PassengerInfo[i].GetRole() == nRoleID )
			return m_PassengerInfo[i].m_hPassenger;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find the first available seat (ranked by priority)
// Input  : nRoleID - Role index
// Output : int - Seat by index
//-----------------------------------------------------------------------------
int CBaseServerVehicle::NPC_GetAvailableSeat_Any( CBaseCombatCharacter *pPassenger, int nRoleID )
{
	// Look through all available seats
	for ( int i = 0; i < m_PassengerRoles[nRoleID].m_PassengerSeats.Count(); i++ )
	{
		// See if anyone is already in this seat
		CBaseCombatCharacter *pCurrentPassenger = NPC_GetPassengerInSeat( nRoleID, i );
		if ( pCurrentPassenger != NULL && pCurrentPassenger != pPassenger )
			continue;

		// This seat is open
		return i;
	}

	// Nothing found
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Find the seat with the nearest entry point to the querier
// Input  : *pPassenger - Terget to be nearest to
//			nRoleID - Role index
// Output : int - Seat by index
//-----------------------------------------------------------------------------
int CBaseServerVehicle::NPC_GetAvailableSeat_Nearest( CBaseCombatCharacter *pPassenger, int nRoleID )
{
	// Not yet implemented
	Assert( 0 );
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Get a seat in the vehicle based on our role and criteria
// Input  : *pPassenger - Entity attempting to find a seat
//			strRoleName - Role the seat must serve
//			nQueryType -  Method for choosing the best seat (if multiple)
// Output : int - Seat by unique ID
//-----------------------------------------------------------------------------
int CBaseServerVehicle::NPC_GetAvailableSeat( CBaseCombatCharacter *pPassenger, string_t strRoleName, VehicleSeatQuery_e nQueryType )
{
	// Parse the vehicle animations the first time they get in the vehicle
	if ( m_bParsedAnimations == false )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	// Get the role index
	int nRole = FindRoleIndexByName( strRoleName );
	if ( m_PassengerRoles.IsValidIndex( nRole ) == false )
		return -1;

	switch( nQueryType )
	{
	case VEHICLE_SEAT_ANY:
		return NPC_GetAvailableSeat_Any( pPassenger, nRole );
		break;

	case VEHICLE_SEAT_NEAREST:
		return NPC_GetAvailableSeat_Nearest( pPassenger, nRole );
		break;

	default:
		Assert( 0 );
		break;
	};

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if there's an available seat of a given role name
// Input  : strRoleName - name of the role
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::NPC_HasAvailableSeat( string_t strRoleName )
{
	return ( NPC_GetAvailableSeat( NULL, strRoleName, VEHICLE_SEAT_ANY ) != -1 );
}

//-----------------------------------------------------------------------------
// Purpose: Find a role index by name
// Input  : strRoleName - name of the role
//-----------------------------------------------------------------------------
int CBaseServerVehicle::FindRoleIndexByName( string_t strRoleName )
{
	// Search through all our known roles
	for ( int i = 0; i < m_PassengerRoles.Count(); i++ )
	{
		// Return the index if the name matches
		if ( FStrEq( STRING( m_PassengerRoles[i].GetName() ), STRING( strRoleName ) ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Find a seat index by its name
// Input  : strSeatName - name of the seat
//-----------------------------------------------------------------------------
int CBaseServerVehicle::FindSeatIndexByName( int nRoleIndex, string_t strSeatName )
{
	// Role must be valid
	if ( m_PassengerRoles.IsValidIndex( nRoleIndex ) == false )
		return -1;

	// Used for attachment polling
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating *>(GetVehicleEnt());
	if ( pAnimating == NULL )
		return -1;

	// Get the index of the named attachment in the model
	int nAttachmentID = pAnimating->LookupAttachment( STRING( strSeatName ) );

	// Look through the roles for this seat attachment ID
	for ( int i = 0; i < m_PassengerRoles[nRoleIndex].m_PassengerSeats.Count(); i++ )
	{
		// Return that index if found
		if ( m_PassengerRoles[nRoleIndex].m_PassengerSeats[i].GetAttachmentID() == nAttachmentID )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Called after loading a saved game
//-----------------------------------------------------------------------------
void CBaseServerVehicle::RestorePassengerInfo( void )
{
	// If there is passenger information, then we have passengers in the vehicle
	if ( m_PassengerInfo.Count() != 0 && m_bParsedAnimations == false )
	{
		// Load the entry/exit animations from the vehicle
		ParseEntryExitAnims();
		m_bParsedAnimations = true;
	}

	// FIXME: If a passenger cannot fix-up its indices, it must be removed from the vehicle!

	// Fix-up every passenger with updated indices
	for ( int i = 0; i < m_PassengerInfo.Count(); i++ )
	{
		// Fix up the role first
		int nRoleIndex = FindRoleIndexByName( m_PassengerInfo[i].m_strRoleName );
		if ( m_PassengerRoles.IsValidIndex( nRoleIndex ) )
		{
			// New role index
			m_PassengerInfo[i].m_nRole = nRoleIndex;
			
			// Then fix up the seat via attachment name
			int nSeatIndex = FindSeatIndexByName( nRoleIndex, m_PassengerInfo[i].m_strSeatName );
			if ( m_PassengerRoles[nRoleIndex].m_PassengerSeats.IsValidIndex( nSeatIndex ) )
			{
				// New seat index
				m_PassengerInfo[i].m_nSeat = nSeatIndex;
			}
			else
			{
				// The seat attachment was not found.  This most likely means that the seat attachment name has changed
				// in the target vehicle and the NPC passenger is now stranded!
				Assert( 0 );
			}
		}
		else
		{
			// The role was not found.  This most likely means that the role names have changed
			// in the target vehicle and the NPC passenger is now stranded!
			Assert( 0 );
		}
	}
}

void CBaseServerVehicle::ReloadScript()
{
	if ( m_pDrivableVehicle )
	{
		string_t script = m_pDrivableVehicle->GetVehicleScriptName();
		IPhysicsVehicleController *pController = GetVehicleController();
		vehicleparams_t *pVehicleParams = pController ? &(pController->GetVehicleParamsForChange()) : NULL;
		PhysFindOrAddVehicleScript( script.ToCStr(), pVehicleParams, &m_vehicleSounds );
		if ( pController )
		{
			pController->VehicleDataReload();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Passes this call down into the server vehicle where the tests are done
//-----------------------------------------------------------------------------
bool CBaseServerVehicle::PassengerShouldReceiveDamage( CTakeDamageInfo &info )
{ 
	if ( GetDrivableVehicle() )
		return GetDrivableVehicle()->PassengerShouldReceiveDamage( info );

	return true;
}

//===========================================================================================================
// Vehicle Sounds
//===========================================================================================================

// These are sounds that are to be automatically stopped whenever the vehicle's driver leaves it
vehiclesound g_iSoundsToStopOnExit[] =
{
	VS_ENGINE2_START,
	VS_ENGINE2_STOP,
};

const char *vehiclesound_parsenames[VS_NUM_SOUNDS] =
{
	"skid_lowfriction",
	"skid_normalfriction",
	"skid_highfriction",
	"engine2_start",
	"engine2_stop",
	"misc1",
	"misc2",
	"misc3",
	"misc4",
};

CVehicleSoundsParser::CVehicleSoundsParser( void )
{
	// UNDONE: Revisit this pattern - move sub-block processing ideas into the parser architecture
	m_iCurrentGear = -1;
	m_iCurrentState = -1;
	m_iCurrentCrashSound = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleSoundsParser::ParseKeyValue( void *pData, const char *pKey, const char *pValue )
{
	vehiclesounds_t *pSounds = (vehiclesounds_t *)pData;
	// New gear?
	if ( !strcmpi( pKey, "gear" ) )
	{
		// Create, initialize, and add a new gear to our list
		int iNewGear = pSounds->pGears.AddToTail();
		pSounds->pGears[iNewGear].flMaxSpeed = 0;
		pSounds->pGears[iNewGear].flSpeedApproachFactor = 1.0;

		// Set our min speed to the previous gear's max
		if ( iNewGear == 0 )
		{
			// First gear, so our minspeed is 0
			pSounds->pGears[iNewGear].flMinSpeed = 0;
		}
		else
		{
			pSounds->pGears[iNewGear].flMinSpeed = pSounds->pGears[iNewGear-1].flMaxSpeed;
		}

		// Remember which gear we're reading data from
		m_iCurrentGear = iNewGear;
	}
	else if ( !strcmpi( pKey, "state" ) )
	{
		m_iCurrentState = 0;
	}
	else if ( !strcmpi( pKey, "crashsound" ) )
	{
		m_iCurrentCrashSound = pSounds->crashSounds.AddToTail();
		pSounds->crashSounds[m_iCurrentCrashSound].flMinSpeed = 0;
		pSounds->crashSounds[m_iCurrentCrashSound].flMinDeltaSpeed = 0;
		pSounds->crashSounds[m_iCurrentCrashSound].iszCrashSound = NULL_STRING;
	}
	else
	{
		int i;

		// Are we currently in a gear block?
		if ( m_iCurrentGear >= 0 )
		{
			Assert( m_iCurrentGear < pSounds->pGears.Count() );

			// Check gear keys
			if ( !strcmpi( pKey, "max_speed" ) )
			{
				pSounds->pGears[m_iCurrentGear].flMaxSpeed = atof(pValue);
				return;
			}
			if ( !strcmpi( pKey, "speed_approach_factor" ) )
			{
				pSounds->pGears[m_iCurrentGear].flSpeedApproachFactor = atof(pValue);
				return;
			}
		}
		// We're done reading a gear, so stop checking them.
		m_iCurrentGear = -1;

		if ( m_iCurrentState >= 0 )
		{
			if ( !strcmpi( pKey, "name" ) )
			{
				m_iCurrentState = SoundStateIndexFromName( pValue );
				pSounds->iszStateSounds[m_iCurrentState] = NULL_STRING;
				pSounds->minStateTime[m_iCurrentState] = 0.0f;
				return;
			}
			else if ( !strcmpi( pKey, "sound" ) )
			{
				pSounds->iszStateSounds[m_iCurrentState] = AllocPooledString(pValue);
				return;
			}
			else if ( !strcmpi( pKey, "min_time" ) )
			{
				pSounds->minStateTime[m_iCurrentState] = atof(pValue);
				return;
			}
		}
		// 
		m_iCurrentState = -1;

		if ( m_iCurrentCrashSound >= 0 )
		{
			if ( !strcmpi( pKey, "min_speed" ) )
			{
				pSounds->crashSounds[m_iCurrentCrashSound].flMinSpeed = atof(pValue);
				return;
			}
			else if ( !strcmpi( pKey, "sound" ) )
			{
				pSounds->crashSounds[m_iCurrentCrashSound].iszCrashSound = AllocPooledString(pValue);
				return;
			}
			else if ( !strcmpi( pKey, "min_speed_change" ) )
			{
				pSounds->crashSounds[m_iCurrentCrashSound].flMinDeltaSpeed = atof(pValue);
				return;
			}
			else if ( !strcmpi( pKey, "gear_limit" ) )
			{
				pSounds->crashSounds[m_iCurrentCrashSound].gearLimit = atoi(pValue);
				return;
			}
		}
		m_iCurrentCrashSound = -1;

		for ( i = 0; i < VS_NUM_SOUNDS; i++ )
		{
			if ( !strcmpi( pKey, vehiclesound_parsenames[i] ) )
			{
				pSounds->iszSound[i] = AllocPooledString(pValue);
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CVehicleSoundsParser::SetDefaults( void *pData ) 
{
	vehiclesounds_t *pSounds = (vehiclesounds_t *)pData;
	pSounds->Init();
}

