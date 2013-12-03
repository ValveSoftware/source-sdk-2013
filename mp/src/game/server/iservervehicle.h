//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERVEHICLE_H
#define ISERVERVEHICLE_H

#ifdef _WIN32
#pragma once
#endif

#include "IVehicle.h"
#include "vphysics/vehicles.h"

class CBaseEntity;
class CBasePlayer;
class CBaseCombatCharacter;
class CNPC_VehicleDriver;
enum VehicleSeatQuery_e;

// This is used by the player to access vehicles. It's an interface so the
// vehicles are not restricted in what they can derive from.
abstract_class IServerVehicle : public IVehicle
{
public:
	// Get the entity associated with the vehicle.
	virtual CBaseEntity*	GetVehicleEnt() = 0;

	// Get and set the current driver. Use PassengerRole_t enum in shareddefs.h for adding passengers
	virtual void			SetPassenger( int nRole, CBaseCombatCharacter *pPassenger ) = 0;
	
	// Is the player visible while in the vehicle? (this is a constant the vehicle)
	virtual bool			IsPassengerVisible( int nRole = VEHICLE_ROLE_DRIVER ) = 0;

	// Can a given passenger take damage?
	virtual bool			IsPassengerDamagable( int nRole  = VEHICLE_ROLE_DRIVER ) = 0;
	virtual bool			PassengerShouldReceiveDamage( CTakeDamageInfo &info ) = 0;

	// Is the vehicle upright?
	virtual bool			IsVehicleUpright( void ) = 0;

	// Whether or not we're in a transitional phase
	virtual bool			IsPassengerEntering( void ) = 0;
	virtual bool			IsPassengerExiting( void ) = 0;

	// Get a position in *world space* inside the vehicle for the player to start at
	virtual void			GetPassengerSeatPoint( int nRole, Vector *pPoint, QAngle *pAngles ) = 0;

	virtual void			HandlePassengerEntry( CBaseCombatCharacter *pPassenger, bool bAllowEntryOutsideZone = false ) = 0;
	virtual bool			HandlePassengerExit( CBaseCombatCharacter *pPassenger ) = 0;

	// Get a point in *world space* to leave the vehicle from (may be in solid)
	virtual bool			GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles ) = 0;
	virtual int				GetEntryAnimForPoint( const Vector &vecPoint ) = 0;
	virtual int				GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked ) = 0;
	virtual void			HandleEntryExitFinish( bool bExitAnimOn, bool bResetAnim ) = 0;

	virtual Class_T			ClassifyPassenger( CBaseCombatCharacter *pPassenger, Class_T defaultClassification ) = 0;
	virtual float			PassengerDamageModifier( const CTakeDamageInfo &info ) = 0;

	// Get me the parameters for this vehicle
	virtual const vehicleparams_t	*GetVehicleParams( void ) = 0;
	// If I'm a physics vehicle, get the controller
	virtual IPhysicsVehicleController *GetVehicleController() = 0;

	virtual int				NPC_GetAvailableSeat( CBaseCombatCharacter *pPassenger, string_t strRoleName, VehicleSeatQuery_e nQueryType ) = 0;
	virtual bool			NPC_AddPassenger( CBaseCombatCharacter *pPassenger, string_t strRoleName, int nSeat ) = 0;
	virtual bool			NPC_RemovePassenger( CBaseCombatCharacter *pPassenger ) = 0;
	virtual bool			NPC_GetPassengerSeatPosition( CBaseCombatCharacter *pPassenger, Vector *vecResultPos, QAngle *vecResultAngle ) = 0;
	virtual bool			NPC_GetPassengerSeatPositionLocal( CBaseCombatCharacter *pPassenger, Vector *vecResultPos, QAngle *vecResultAngle ) = 0;
	virtual int				NPC_GetPassengerSeatAttachment( CBaseCombatCharacter *pPassenger ) = 0;
	virtual bool			NPC_HasAvailableSeat( string_t strRoleName ) = 0;
	
	virtual const PassengerSeatAnims_t	*NPC_GetPassengerSeatAnims( CBaseCombatCharacter *pPassenger, PassengerSeatAnimType_t nType ) = 0;
	virtual CBaseCombatCharacter		*NPC_GetPassengerInSeat( int nRoleID, int nSeatID ) = 0;

	virtual void			RestorePassengerInfo( void ) = 0;

	// NPC Driving
	virtual bool			NPC_CanDrive( void ) = 0;
	virtual void			NPC_SetDriver( CNPC_VehicleDriver *pDriver ) = 0;
  	virtual void			NPC_DriveVehicle( void ) = 0;
	virtual void			NPC_ThrottleCenter( void ) = 0;
	virtual void			NPC_ThrottleReverse( void ) = 0;
	virtual void			NPC_ThrottleForward( void ) = 0;
	virtual void			NPC_Brake( void ) = 0;
	virtual void			NPC_TurnLeft( float flDegrees ) = 0;
	virtual void			NPC_TurnRight( float flDegrees ) = 0;
	virtual void			NPC_TurnCenter( void ) = 0;
	virtual void			NPC_PrimaryFire( void ) = 0;
	virtual void			NPC_SecondaryFire( void ) = 0;
	virtual bool			NPC_HasPrimaryWeapon( void ) = 0;
	virtual bool			NPC_HasSecondaryWeapon( void ) = 0;
	virtual void			NPC_AimPrimaryWeapon( Vector vecTarget ) = 0;
	virtual void			NPC_AimSecondaryWeapon( Vector vecTarget ) = 0;

	// Weapon handling
	virtual void			Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange ) = 0;	
	virtual void			Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange ) = 0;	
	virtual float			Weapon_PrimaryCanFireAt( void ) = 0;	// Return the time at which this vehicle's primary weapon can fire again
	virtual float			Weapon_SecondaryCanFireAt( void ) = 0;	// Return the time at which this vehicle's secondary weapon can fire again

	// debugging, script file flushed
	virtual void			ReloadScript() = 0;
};

// This is an interface to derive from if your class contains an IServerVehicle 
// handler (i.e. something derived CBaseServerVehicle.
abstract_class IDrivableVehicle
{
public:
	virtual CBaseEntity		*GetDriver( void ) = 0;

	// Process movement
	virtual void			ItemPostFrame( CBasePlayer *pPlayer ) = 0;
	virtual void			SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) = 0;
	virtual void			ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) = 0;
	virtual void			FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) = 0;

	// Entering / Exiting
	virtual bool			CanEnterVehicle( CBaseEntity *pEntity ) = 0;
	virtual bool			CanExitVehicle( CBaseEntity *pEntity ) = 0;
	virtual void			SetVehicleEntryAnim( bool bOn ) = 0;
	virtual void			SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) = 0;
	virtual void			EnterVehicle( CBaseCombatCharacter *pPassenger ) = 0;

	virtual void			PreExitVehicle( CBaseCombatCharacter *pPassenger, int nRole ) = 0;
	virtual void			ExitVehicle( int nRole ) = 0;
	virtual bool			AllowBlockedExit( CBaseCombatCharacter *pPassenger, int nRole ) = 0;
	virtual bool			AllowMidairExit( CBaseCombatCharacter *pPassenger, int nRole ) = 0;
	virtual string_t		GetVehicleScriptName() = 0;

	virtual bool			PassengerShouldReceiveDamage( CTakeDamageInfo &info ) = 0;
};

#endif // IVEHICLE_H
