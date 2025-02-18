//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef VEHICLE_BASESERVER_H
#define VEHICLE_BASESERVER_H
#ifdef _WIN32
#pragma once
#endif

#include "vehicle_sounds.h"
#include "entityblocker.h"

class CSoundPatch;

struct vbs_sound_update_t
{
	float	flFrameTime;
	float	flCurrentSpeedFraction;
	float	flWorldSpaceSpeed;
	bool	bThrottleDown;
	bool	bReverse;
	bool	bTurbo;
	bool	bVehicleInWater;
	bool	bExitVehicle;

	void Defaults()
	{
		flFrameTime = gpGlobals->frametime;
		flCurrentSpeedFraction = 0;
		flWorldSpaceSpeed = 0;
		bThrottleDown = false;
		bReverse = false;
		bTurbo = false;
		bVehicleInWater = false;
		bExitVehicle = false;
	}
};

// -----------------------------------------
//  Information about the passenger in the car 
// -----------------------------------------
class CPassengerInfo
{
public:
	CPassengerInfo( void ) : m_nRole( -1 ), m_nSeat( -1 ), m_strRoleName( NULL_STRING ), m_strSeatName( NULL_STRING ) {}

	DECLARE_SIMPLE_DATADESC();

	int GetSeat( void ) const { return m_nSeat; }
	int	GetRole( void ) const { return m_nRole; }
	CBaseCombatCharacter *GetPassenger( void ) const { return m_hPassenger; }

private:
	int									m_nRole;		// Role (by index)
	int									m_nSeat;		// Seat (by index)
	string_t							m_strRoleName;	// Used in restoration for fix-up
	string_t							m_strSeatName;	// Used in restoration for fix-up
	CHandle<CBaseCombatCharacter>		m_hPassenger;	// Actual passenger

	friend class CBaseServerVehicle;
};

// -----------------------------------------
//  Seat transition information (animation and priority)
// -----------------------------------------

class CPassengerSeatTransition
{
public:
	CPassengerSeatTransition( void ) : m_strAnimationName( NULL_STRING ), m_nPriority( -1 ) {};

	string_t GetAnimationName( void ) const { return m_strAnimationName; }
	int		 GetPriority( void ) const { return m_nPriority; }

private:
	string_t	m_strAnimationName;	// Name of animation to play
	int			m_nPriority;		// Priority of the transition

	friend class CBaseServerVehicle;
};

// -----------------------------------------
//  Seat in a vehicle (attachment and a collection of animations to reach it)
// -----------------------------------------
class CPassengerSeat
{
public:
	CPassengerSeat( void ) : m_nAttachmentID( -1 ) {};
	int GetAttachmentID( void ) const { return m_nAttachmentID; }

private:
	string_t								m_strSeatName;			// Used for save/load fixup
	int										m_nAttachmentID;		// Goal attachment
	CUtlVector<CPassengerSeatTransition>	m_EntryTransitions;		// Entry information
	CUtlVector<CPassengerSeatTransition>	m_ExitTransitions;		// Exit information

	friend class CBaseServerVehicle;
};

// -----------------------------------------
//  Passenger role information
// -----------------------------------------
class CPassengerRole
{
public:
	CPassengerRole( void ) : m_strName( NULL_STRING ) {};
	string_t GetName( void ) const { return m_strName; }

private:
	string_t						m_strName;			// Name of the set
	CUtlVector<CPassengerSeat>		m_PassengerSeats;	// Passenger info

	friend class CBaseServerVehicle;
};

//-----------------------------------------------------------------------------
// Purpose: Base class for drivable vehicle handling. Contain it in your 
//			drivable vehicle.
//-----------------------------------------------------------------------------
class CBaseServerVehicle : public IServerVehicle
{
public:
	DECLARE_SIMPLE_DATADESC();
	DECLARE_CLASS_NOBASE( CBaseServerVehicle );

	CBaseServerVehicle( void );
	~CBaseServerVehicle( void );

	virtual void			Precache( void );

// IVehicle
public:
	virtual CBaseCombatCharacter *GetPassenger( int nRole = VEHICLE_ROLE_DRIVER );

	virtual int				GetPassengerRole( CBaseCombatCharacter *pPassenger );
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );
	virtual bool			IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual void			SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void			ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData );
	virtual void			FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
	virtual void			ItemPostFrame( CBasePlayer *pPlayer );

// IServerVehicle
public:
	virtual CBaseEntity		*GetVehicleEnt( void ) { return m_pVehicle; }
	virtual void			SetPassenger( int nRole, CBaseCombatCharacter *pPassenger );
	virtual bool			IsPassengerVisible( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual bool			IsPassengerDamagable( int nRole  = VEHICLE_ROLE_DRIVER ) { return true; }
	virtual bool			PassengerShouldReceiveDamage( CTakeDamageInfo &info );

	virtual bool			IsVehicleUpright( void ) { return true; }
	virtual bool			IsPassengerEntering( void ) { Assert( 0 ); return false; }
	virtual bool			IsPassengerExiting( void ) { Assert( 0 ); return false; }
	
	virtual void			HandlePassengerEntry( CBaseCombatCharacter *pPassenger, bool bAllowEntryOutsideZone = false );
	virtual bool			HandlePassengerExit( CBaseCombatCharacter *pPassenger );

	virtual void			GetPassengerSeatPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual bool			GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual Class_T			ClassifyPassenger( CBaseCombatCharacter *pPassenger, Class_T defaultClassification ) { return defaultClassification; }
	virtual float			PassengerDamageModifier( const CTakeDamageInfo &info ) { return 1.0; }
	virtual const vehicleparams_t	*GetVehicleParams( void ) { return NULL; }
	virtual bool			IsVehicleBodyInWater( void ) { return false; }
	virtual IPhysicsVehicleController *GetVehicleController() { return NULL; }

	// NPC Driving
	virtual bool			NPC_CanDrive( void ) { return true; }
	virtual void			NPC_SetDriver( CNPC_VehicleDriver *pDriver ) { return; }
	virtual void			NPC_DriveVehicle( void ) { return; }
	virtual void			NPC_ThrottleCenter( void );
	virtual void			NPC_ThrottleReverse( void );
	virtual void			NPC_ThrottleForward( void );
	virtual void			NPC_Brake( void );
	virtual void			NPC_TurnLeft( float flDegrees );
	virtual void			NPC_TurnRight( float flDegrees );
	virtual void			NPC_TurnCenter( void );
	virtual void			NPC_PrimaryFire( void );
	virtual void			NPC_SecondaryFire( void );
	virtual bool			NPC_HasPrimaryWeapon( void ) { return false; }
	virtual bool			NPC_HasSecondaryWeapon( void ) { return false; }
	virtual void			NPC_AimPrimaryWeapon( Vector vecTarget ) { return; }
	virtual void			NPC_AimSecondaryWeapon( Vector vecTarget ) { return; }

	// Weapon handling
	virtual void			Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange );
	virtual void			Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange );	
	virtual float			Weapon_PrimaryCanFireAt( void );		// Return the time at which this vehicle's primary weapon can fire again
	virtual float			Weapon_SecondaryCanFireAt( void );		// Return the time at which this vehicle's secondary weapon can fire again

	// ----------------------------------------------------------------------------
	// NPC passenger data

public:

	bool			NPC_AddPassenger( CBaseCombatCharacter *pPassenger, string_t strRoleName, int nSeat );
	bool			NPC_RemovePassenger( CBaseCombatCharacter *pPassenger );
	virtual bool	NPC_GetPassengerSeatPosition( CBaseCombatCharacter *pPassenger, Vector *vecResultPos, QAngle *vecResultAngle );
	virtual bool	NPC_GetPassengerSeatPositionLocal( CBaseCombatCharacter *pPassenger, Vector *vecResultPos, QAngle *vecResultAngles );
	virtual int		NPC_GetPassengerSeatAttachment( CBaseCombatCharacter *pPassenger );
	virtual int		NPC_GetAvailableSeat( CBaseCombatCharacter *pPassenger, string_t strRoleName, VehicleSeatQuery_e nQueryType );
	bool			NPC_HasAvailableSeat( string_t strRoleName );


	virtual const PassengerSeatAnims_t	*NPC_GetPassengerSeatAnims( CBaseCombatCharacter *pPassenger, PassengerSeatAnimType_t nType );
	virtual CBaseCombatCharacter		*NPC_GetPassengerInSeat( int nRoleID, int nSeatID );

	Vector	GetSavedViewOffset( void ) { return m_savedViewOffset; }

private:

	// Vehicle entering/exiting
	void	ParseNPCRoles( KeyValues *pModelKeyValues );
	void	ParseNPCPassengerSeat( KeyValues *pSetKeyValues, CPassengerSeat *pSeat );
	void	ParseNPCSeatTransition( KeyValues *pTransitionKeyValues, CPassengerSeatTransition *pTransition );

protected:
	
	int		FindRoleIndexByName( string_t strRoleName );
	int		FindSeatIndexByName( int nRoleIndex, string_t strSeatName );
	int		NPC_GetAvailableSeat_Any( CBaseCombatCharacter *pPassenger, int nRoleID );
	int		NPC_GetAvailableSeat_Nearest( CBaseCombatCharacter *pPassenger, int nRoleID );

	CPassengerRole *FindOrCreatePassengerRole( string_t strName, int *nIndex = NULL );

	CUtlVector< CPassengerInfo >	m_PassengerInfo;
	CUtlVector< CPassengerRole >	m_PassengerRoles;	// Not save/restored

	// ----------------------------------------------------------------------------
	void	ReloadScript();	// debug/tuning
public:

	void					UseLegacyExitChecks( bool bState ) { m_bUseLegacyExitChecks = bState; }
	void					RestorePassengerInfo( void );

	virtual CBaseEntity		*GetDriver( void );	// Player Driving
	virtual void			ParseEntryExitAnims( void );
	void					ParseExitAnim( KeyValues *pkvExitList, bool bEscapeExit );
	virtual bool			CheckExitPoint( float yaw, int distance, Vector *pEndPoint );
	virtual int				GetEntryAnimForPoint( const Vector &vecPoint );
	virtual int				GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked );
	virtual void			HandleEntryExitFinish( bool bExitAnimOn, bool bResetAnim );

	virtual void			SetVehicle( CBaseEntity *pVehicle );
	IDrivableVehicle 		*GetDrivableVehicle( void );

	// Sound handling
	bool					Initialize( const char *pScriptName );
	virtual void			SoundStart();
	virtual void			SoundStartDisabled();
	virtual void			SoundShutdown( float flFadeTime = 0.0 );
	virtual void			SoundUpdate( vbs_sound_update_t &params );
	virtual void			PlaySound( vehiclesound iSound );
	virtual void			StopSound( vehiclesound iSound );
	virtual void 			RecalculateSoundGear( vbs_sound_update_t &params );
	void					SetVehicleVolume( float flVolume ) { m_flVehicleVolume = clamp( flVolume, 0.0f, 1.0f ); }

	// Rumble
	virtual void			StartEngineRumble();
	virtual void			StopEngineRumble();

public:
	CBaseEntity			*m_pVehicle;
	IDrivableVehicle 	*m_pDrivableVehicle;

	// NPC Driving
	int								m_nNPCButtons;
	int								m_nPrevNPCButtons;
	float							m_flTurnDegrees;

	// Entry / Exit anims
	struct entryanim_t
	{
		int		iHitboxGroup;
		char	szAnimName[128];
	};
	CUtlVector< entryanim_t >		m_EntryAnimations;

	struct exitanim_t
	{
		bool	bUpright;
		bool	bEscapeExit;
		char	szAnimName[128];
		Vector	vecExitPointLocal;		// Point the animation leaves the player at when finished
		QAngle	vecExitAnglesLocal;
	};

	CUtlVector< exitanim_t >		m_ExitAnimations;
	bool							m_bParsedAnimations;
	bool							m_bUseLegacyExitChecks;	// HACK: Choreo vehicles use non-sensical setups to move the player, we need to poll their attachment point positions
	int								m_iCurrentExitAnim;
	Vector							m_vecCurrentExitEndPoint;
	Vector							m_savedViewOffset;
	CHandle<CEntityBlocker>			m_hExitBlocker;				// Entity to prevent other entities blocking the player's exit point during the exit animation

	char							m_chPreviousTextureType;

// sound state
	vehiclesounds_t					m_vehicleSounds;
private:
	float							m_flVehicleVolume;
	int								m_iSoundGear;			// The sound "gear" that we're currently in
	float							m_flSpeedPercentage;

	CSoundPatch						*m_pStateSound;
	CSoundPatch						*m_pStateSoundFade;
	sound_states					m_soundState;
	float							m_soundStateStartTime;
	float							m_lastSpeed;
	
	void	SoundState_OnNewState( sound_states lastState );
	void	SoundState_Update( vbs_sound_update_t &params );
	sound_states SoundState_ChooseState( vbs_sound_update_t &params );
	void	PlaySound( const char *pSound );
	void	StopLoopingSound( float fadeTime = 0.25f );
	void	PlayLoopingSound( const char *pSoundName );
	bool	PlayCrashSound( float speed );
	bool	CheckCrash( vbs_sound_update_t &params );
	const char *StateSoundName( sound_states state );
	void	InitSoundParams( vbs_sound_update_t &params );
	void	CacheEntryExitPoints( void );
	bool	GetLocalAttachmentAtTime( int nQuerySequence, int nAttachmentIndex, float flCyclePoint, Vector *vecOriginOut, QAngle *vecAnglesOut );
	bool	GetLocalAttachmentAtTime( const char *lpszAnimName, int nAttachmentIndex, float flCyclePoint, Vector *vecOriginOut, QAngle *vecAnglesOut );
};

#endif // VEHICLE_BASESERVER_H
