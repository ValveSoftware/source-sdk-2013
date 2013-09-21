//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ====//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "vehicle_base.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"
#include "soundenvelope.h"
#include "soundent.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"
#include "vcollide_parse.h"
#include "ndebugoverlay.h"
#include "hl2_player.h"
#include "props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	VEHICLE_HITBOX_DRIVER		1

#ifdef SecobMod__ENABLE_FAKE_PASSENGER_SEATS

//SecobMod__Information: Only allow passenger seats if the AI patch code is enabled.
#ifdef SecobMod__Enable_Fixed_Multiplayer_AI

//
// Anim events.
//
enum
{
	AE_POD_OPEN = 1,	// The pod is now open and can be entered or exited.
	AE_POD_CLOSE = 2,	// The pod is now closed and cannot be entered or exited.
};


extern ConVar g_debug_vehicledriver;


class CPropVehiclePassengerSeat;


// Pod bones that have physics followers
static const char *pPodFollowerBoneNames[] =
{
	"base",
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPassengerSeatServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;

// IServerVehicle
public:
	void GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV = NULL );
	virtual void ItemPostFrame( CBasePlayer *pPlayer );

	virtual bool	IsPassengerEntering( void ) { return false; }	// NOTE: This mimics the scenario HL2 would have seen
	virtual bool	IsPassengerExiting( void ) { return false; }

protected:

	CPropVehiclePassengerSeat *GetPod( void );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPropVehiclePassengerSeat : public CPhysicsProp, public IDrivableVehicle
{
	DECLARE_CLASS( CPropVehiclePassengerSeat, CPhysicsProp );

public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CPropVehiclePassengerSeat( void )
	{
		m_ServerVehicle.SetVehicle( this );
	}

	~CPropVehiclePassengerSeat( void )
	{
	}

	// CBaseEntity
	virtual void	Precache( void );
	void			Spawn( void );
	void			Think(void);
	virtual int		ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void	DrawDebugGeometryOverlays( void );

	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );

	//void			PlayerControlInit( CBasePlayer *pPlayer );
	//void			PlayerControlShutdown( void );
	void			ResetUseKey( CBasePlayer *pPlayer );

	virtual bool OverridePropdata() { return true; }

	void			GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const;

	bool ShouldForceExit() { return m_bForcedExit; }
	void ClearForcedExit() { m_bForcedExit = false; }

	// CBaseAnimating
	void HandleAnimEvent( animevent_t *pEvent );

	// Inputs
	void InputEnterVehicleImmediate( inputdata_t &inputdata );
	void InputEnterVehicle( inputdata_t &inputdata );
	void InputExitVehicle( inputdata_t &inputdata );
	void InputLock( inputdata_t &inputdata );
	void InputUnlock( inputdata_t &inputdata );
	void InputOpen( inputdata_t &inputdata );
	void InputClose( inputdata_t &inputdata );

	CNetworkHandle( CBasePlayer, m_hPlayer );

// IDrivableVehicle
public:

	virtual bool PassengerShouldReceiveDamage( CTakeDamageInfo &info ) 
	{ 
			return true;
	}

	virtual CBaseEntity *GetDriver( void );
	virtual void ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMoveData ) { return; }
	virtual void FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move ) { return; }
	virtual bool CanEnterVehicle( CBaseEntity *pEntity );
	virtual bool CanExitVehicle( CBaseEntity *pEntity );
	virtual void SetVehicleEntryAnim( bool bOn );
	virtual void SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = bOn; if ( bOn ) m_vecEyeExitEndpoint = vecEyeExitEndpoint; }
	virtual void EnterVehicle( CBaseCombatCharacter *pPassenger );

	virtual bool AllowBlockedExit( CBaseCombatCharacter *pPassenger, int nRole ) { return true; }
	virtual bool AllowMidairExit( CBaseCombatCharacter *pPassenger, int nRole ) { return true; }
	virtual void PreExitVehicle( CBaseCombatCharacter *pPassenger, int nRole ) {}
	virtual void ExitVehicle( int nRole );

	virtual void ItemPostFrame( CBasePlayer *pPlayer ) {}
	virtual void SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual string_t GetVehicleScriptName() { return m_vehicleScript; }
	
	// If this is a vehicle, returns the vehicle interface
	virtual IServerVehicle *GetServerVehicle() { return &m_ServerVehicle; }

protected:

	// Contained IServerVehicle
	CPassengerSeatServerVehicle m_ServerVehicle;

private:

	// Entering / Exiting
	bool				m_bLocked;
	CNetworkVar( bool,	m_bEnterAnimOn );
	CNetworkVar( bool,	m_bExitAnimOn );
	CNetworkVector(		m_vecEyeExitEndpoint );
	bool				m_bForcedExit;

	// Vehicle script filename
	string_t			m_vehicleScript;

	COutputEvent		m_playerOn;
	COutputEvent		m_playerOff;
	COutputEvent		m_OnOpen;
	COutputEvent		m_OnClose;
};

#ifdef SecobMod__ENABLE_FAKE_PASSENGER_SEATS
LINK_ENTITY_TO_CLASS( prop_vehicle_ss_passengerseat, CPropVehiclePassengerSeat );
#endif //SecobMod__ENABLE_FAKE_PASSENGER_SEATS


BEGIN_DATADESC( CPropVehiclePassengerSeat )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Lock",	InputLock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Unlock",	InputUnlock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnterVehicle", InputEnterVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnterVehicleImmediate", InputEnterVehicleImmediate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ExitVehicle", InputExitVehicle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Open", InputOpen ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Close", InputClose ),

	// Keys
	DEFINE_EMBEDDED( m_ServerVehicle ),

	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bEnterAnimOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bExitAnimOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bForcedExit, FIELD_BOOLEAN ),
 	DEFINE_FIELD( m_vecEyeExitEndpoint, FIELD_POSITION_VECTOR ),

	DEFINE_KEYFIELD( m_vehicleScript, FIELD_STRING, "vehiclescript" ),
	DEFINE_KEYFIELD( m_bLocked, FIELD_BOOLEAN, "vehiclelocked" ),

	DEFINE_OUTPUT( m_playerOn, "PlayerOn" ),
	DEFINE_OUTPUT( m_playerOff, "PlayerOff" ),
	DEFINE_OUTPUT( m_OnOpen, "OnOpen" ),
	DEFINE_OUTPUT( m_OnClose, "OnClose" ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CPropVehiclePassengerSeat, DT_PropVehiclePassengerSeat)
	SendPropEHandle(SENDINFO(m_hPlayer)),
	SendPropBool(SENDINFO(m_bEnterAnimOn)),
	SendPropBool(SENDINFO(m_bExitAnimOn)),
	SendPropVector(SENDINFO(m_vecEyeExitEndpoint), -1, SPROP_COORD),
END_SEND_TABLE();


//------------------------------------------------
// Precache
//------------------------------------------------
void CPropVehiclePassengerSeat::Precache( void )
{
	BaseClass::Precache();

	//PrecacheScriptSound( "d3_citadel.pod_open" );
	//PrecacheScriptSound( "d3_citadel.pod_close" );
	PrecacheModel("models/ss_passenger_cube.mdl");

	m_ServerVehicle.Initialize( STRING(m_vehicleScript) );
}


//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropVehiclePassengerSeat::Spawn( void )
{
	Precache();
	//SetModel( STRING( GetModelName() ) );
	SetModel("models/ss_passenger_cube.mdl");

	BaseClass::Spawn();

	m_takedamage = DAMAGE_EVENTS_ONLY;
	
	//We never want to see our passenger seat.
	AddEffects( EF_NODRAW );
	
	//Allow us to take damage, and use weapons.
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	if ( ptr->hitbox == VEHICLE_HITBOX_DRIVER )
	{
		if ( m_hPlayer != NULL )
		{
			m_hPlayer->TakeDamage( info );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropVehiclePassengerSeat::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// Do scaled up physics damage to the pod
	CTakeDamageInfo info = inputInfo;
	//info.ScaleDamage( 25 );

	// reset the damage
	info.SetDamage( inputInfo.GetDamage() );

	// Check to do damage to prisoner
	if ( m_hPlayer != NULL )
	{
		/*// Take no damage from physics damages
		if ( info.GetDamageType() & DMG_CRUSH )
			return 0;*/

		// Take the damage
		m_hPlayer->TakeDamage( info );
	}

	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CPropVehiclePassengerSeat::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	Vector	shotPos;
	matrix3x4_t	matrix;

	int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
	GetAttachment( eyeAttachmentIndex, matrix );
	MatrixGetColumn( matrix, 3, shotPos );

	if ( bNoisy )
	{
		shotPos[0] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[1] += random->RandomFloat( -8.0f, 8.0f );
		shotPos[2] += random->RandomFloat( -8.0f, 8.0f );
	}

	return shotPos;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::Think(void)
{
	SetNextThink( gpGlobals->curtime + 0.1 );

	if ( GetDriver() )
	{
		BaseClass::Think();
		
	//SecobMod__Information: The game has a tendency to not keep our passenger in his seat, so each tick we make them attached to the attachment point.
	#ifdef SecobMod__ALLOW_PLAYER_MODELS_IN_VEHICLES
	Vector vVector;

	int iAttachment = LookupAttachment( "vehicle_seat" );

	if ( iAttachment != -1 )
	{
	GetAttachment( iAttachment, vVector, NULL );
	m_hPlayer->SetAbsOrigin(vVector);
	}
	#endif //SecobMod__ALLOW_PLAYER_MODELS_IN_VEHICLES
		
		// If the enter or exit animation has finished, tell the server vehicle
		if ( IsSequenceFinished() && (m_bExitAnimOn || m_bEnterAnimOn) )
		{
			GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn, true );
		}
	}
	
/*	CBaseEntity *pParent = this->GetParent();
		while( pParent )
		{
		 int currentPhealth = pParent->GetHealth();
		 
		
		
		
		 int currenthealth = this->GetHealth();
		 int health = pParent->GetHealth();//OnTakeDamage(0) = pParent->OnTakeDamage(int OnTakeDamage);
		 int newhealth = currenthealth/health;
		 const newerhealth = newhealth;
		 this->TakeDamage(newerhealth);
		 //this->SetHealth(health);
		}*/
		
	StudioFrameAdvance();
	DispatchAnimEvents( this );
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPropVehiclePassengerSeat::InputOpen( inputdata_t &inputdata )
{
	int nSequence = LookupSequence( "open" );

	// Set to the desired anim, or default anim if the desired is not present
	if ( nSequence > ACTIVITY_NOT_AVAILABLE )
	{
		SetCycle( 0 );
		m_flAnimTime = gpGlobals->curtime;
		ResetSequence( nSequence );
		ResetClientsideFrame();
		//EmitSound( "d3_citadel.pod_open" );
	}
	else
	{
		// Not available try to get default anim
		//Msg( "Prisoner pod %s: missing open sequence\n", GetDebugName() );
		SetSequence( 0 );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPropVehiclePassengerSeat::InputClose( inputdata_t &inputdata )
{
	// The enter anim closes the pod, so don't do this redundantly!
	if ( m_bLocked || m_bEnterAnimOn )
		return;

	int nSequence = LookupSequence( "close" );

	// Set to the desired anim, or default anim if the desired is not present
	if ( nSequence > ACTIVITY_NOT_AVAILABLE )
	{
		SetCycle( 0 );
		m_flAnimTime = gpGlobals->curtime;
		ResetSequence( nSequence );
		ResetClientsideFrame();
		//EmitSound( "d3_citadel.pod_close" );
	}
	else
	{
		// Not available try to get default anim
		Msg( "Prisoner pod %s: missing close sequence\n", GetDebugName() );
		//SetSequence( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_POD_OPEN )
	{
		m_OnOpen.FireOutput( this, this );
		m_bLocked = false;
	}
	else if ( pEvent->event == AE_POD_CLOSE )
	{
		m_OnClose.FireOutput( this, this );
		m_bLocked = false; //was true.
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	ResetUseKey( pPlayer );

	GetServerVehicle()->HandlePassengerEntry( pPlayer, (value > 0) );
}


//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter / exit the vehicle
//-----------------------------------------------------------------------------
bool CPropVehiclePassengerSeat::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	if ( GetDriver() && GetDriver() != pEntity )
		return false;

	// Prevent entering if the vehicle's locked
	return !m_bLocked;
}


//-----------------------------------------------------------------------------
// Purpose: Return true of the player is allowed to exit the vehicle.
//-----------------------------------------------------------------------------
bool CPropVehiclePassengerSeat::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, rotating, or playing an entry/exit anim.
	return ( !m_bLocked && (GetLocalAngularVelocity() == vec3_angle) && !m_bEnterAnimOn && !m_bExitAnimOn );
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::DrawDebugGeometryOverlays(void) 
{
	// Draw if BBOX is on
	if ( m_debugOverlays & OVERLAY_BBOX_BIT )
	{
	}

	BaseClass::DrawDebugGeometryOverlays();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::EnterVehicle( CBaseCombatCharacter *pPassenger )
{
	if ( pPassenger == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		// Remove any player who may be in the vehicle at the moment
		if ( m_hPlayer )
		{
			//ExitVehicle( VEHICLE_ROLE_DRIVER );
			Assert( 0 );
		}

		m_hPlayer = pPlayer;
		m_playerOn.FireOutput( pPlayer, this, 0 );
		
		m_ServerVehicle.SoundStart();		
	}
	else
	{
		// NPCs are not supported yet - jdw
		Assert( 0 );
	}
	
#ifdef SecobMod__ALLOW_PLAYER_MODELS_IN_VEHICLES
	//SecobMod__Information: Use our exit attachment point.
	Vector vVector;

	int iAttachment = LookupAttachment( "vehicle_seat" );

	if ( iAttachment != -1 )
	{
	GetAttachment( iAttachment, vVector, NULL );
	m_hPlayer->SetAbsOrigin(vVector);
	}
#endif //SecobMod__ALLOW_PLAYER_MODELS_IN_VEHICLES
	
	//SecobMod__Information: Since weapon models dissapear on entering vehicles we need to force a weapon switch to occur to bring them back. The only working method found is to switch to the next best weapon.
	m_hPlayer->SwitchToNextBestWeapon(m_hPlayer->GetActiveWeapon());
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::SetVehicleEntryAnim( bool bOn )
{
	m_bEnterAnimOn = bOn;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::ExitVehicle( int nRole )
{
	CBasePlayer *pPlayer = m_hPlayer;

	if ( !pPlayer )
		return;
		
		
	//SecobMod__Information: Use our exit attachment point.
	Vector vVector;

	int iAttachment = LookupAttachment( "exit8" );

	if ( iAttachment != -1 )
	{
	GetAttachment( iAttachment, vVector, NULL );
	m_hPlayer->SetAbsOrigin(vVector);
	}
		
	//SecobMod__Information: Just in case our exit is bad, call this safety function (newly created in player.cpp/h).
	m_hPlayer->SafeVehicleExit(m_hPlayer);

	m_hPlayer = NULL;
	ResetUseKey( pPlayer );

	m_playerOff.FireOutput( pPlayer, this, 0 );
	m_bEnterAnimOn = false;

	m_ServerVehicle.SoundShutdown( 1.0 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}


//-----------------------------------------------------------------------------
// Purpose: Vehicles are permanently oriented off angle for vphysics.
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pRight ); 
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CPropVehiclePassengerSeat::GetDriver( void ) 
{ 
	return m_hPlayer; 
}

//-----------------------------------------------------------------------------
// Purpose: Prevent the player from entering / exiting the vehicle
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::InputLock( inputdata_t &inputdata )
{
	m_bLocked = true;
}


//-----------------------------------------------------------------------------
// Purpose: Allow the player to enter / exit the vehicle
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::InputUnlock( inputdata_t &inputdata )
{
	m_bLocked = false;
}


//-----------------------------------------------------------------------------
// Purpose: Force the player to enter the vehicle.
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::InputEnterVehicle( inputdata_t &inputdata )
{
	if ( m_bEnterAnimOn )
		return;

	// Try the activator first & use them if they are a player.
	CBaseCombatCharacter *pPassenger = ToBaseCombatCharacter( inputdata.pActivator );
	if ( pPassenger == NULL )
	{
		// Activator was not a player, just grab the nearest player. 
pPassenger = UTIL_GetNearestPlayer(GetAbsOrigin()); 
		if ( pPassenger == NULL )
			return;
	}

	// FIXME: I hate code like this. I should really add a parameter to HandlePassengerEntry
	//		  to allow entry into locked vehicles
	bool bWasLocked = m_bLocked;
	m_bLocked = false;
	GetServerVehicle()->HandlePassengerEntry( pPassenger, true );
	m_bLocked = bWasLocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::InputEnterVehicleImmediate( inputdata_t &inputdata )
{
	if ( m_bEnterAnimOn )
		return;

	// Try the activator first & use them if they are a player.
	CBaseCombatCharacter *pPassenger = ToBaseCombatCharacter( inputdata.pActivator );
	if ( pPassenger == NULL )
	{
		// Activator was not a player, just grab the nearest player. 
pPassenger = UTIL_GetNearestPlayer(GetAbsOrigin()); 
		if ( pPassenger == NULL )
			return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		if ( pPlayer->IsInAVehicle() )
		{
			// Force the player out of whatever vehicle they are in.
			pPlayer->LeaveVehicle();
		}
		
		pPlayer->GetInVehicle( GetServerVehicle(), VEHICLE_ROLE_DRIVER );
	}
	else
	{
		// NPCs are not currently supported - jdw
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Force the player to exit the vehicle.
//-----------------------------------------------------------------------------
void CPropVehiclePassengerSeat::InputExitVehicle( inputdata_t &inputdata )
{
	m_bForcedExit = true;
}


//========================================================================================================================================
// CRANE VEHICLE SERVER VEHICLE
//========================================================================================================================================

CPropVehiclePassengerSeat *CPassengerSeatServerVehicle::GetPod( void )
{
	return (CPropVehiclePassengerSeat *)GetDrivableVehicle();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pPlayer - 
//-----------------------------------------------------------------------------
void CPassengerSeatServerVehicle::ItemPostFrame( CBasePlayer *player )
{
	Assert( player == GetDriver() );

	GetDrivableVehicle()->ItemPostFrame( player );

	if (( player->m_afButtonPressed & IN_USE ) || GetPod()->ShouldForceExit() )
	{
		GetPod()->ClearForcedExit();
		if ( GetDrivableVehicle()->CanExitVehicle(player) )
		{
			// Let the vehicle try to play the exit animation
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
void CPassengerSeatServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*= NULL*/ )
{
	// FIXME: This needs to be reconciled with the other versions of this function!
	Assert( nRole == VEHICLE_ROLE_DRIVER );
	CBasePlayer *pPlayer = ToBasePlayer( GetDrivableVehicle()->GetDriver() );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles(); // yuck. this is an in/out parameter.

	float flPitchFactor = 1.0;
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetPod()->GetAttachment( "vehicle_driver_eyes", vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

	// Compute the relative rotation between the unperterbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	// Now perterb the attachment point
	vehicleEyeAngles.x = RemapAngleRange( PITCH_CURVE_ZERO * flPitchFactor, PITCH_CURVE_LINEAR, vehicleEyeAngles.x );
	vehicleEyeAngles.z = RemapAngleRange( ROLL_CURVE_ZERO * flPitchFactor, ROLL_CURVE_LINEAR, vehicleEyeAngles.z );
	AngleMatrix( vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perterbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );
}
#endif //SecobMod__Enable_Fixed_Multiplayer_AI
#endif //SecobMod__ENABLE_FAKE_PASSENGER_SEATS