//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a bastardization of the vehicle code for the choreography
//			group who want to have smooth view lerping code out of a keyframed
//			controlled player viewpoint.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vehicle_base.h"
#include "hl2_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPropVehicleViewController : public CPropVehicleDriveable
{
	DECLARE_CLASS( CPropVehicleViewController, CPropVehicleDriveable );
public:
	DECLARE_DATADESC();

	// CBaseEntity
	void			Spawn( void ); 
	void			Think(void);

	// CPropVehicle
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	EnterVehicle( CBasePlayer *pPlayer );
	virtual void	ExitVehicle( int nRole );

	// Inputs to force the player in/out of the vehicle
	void			InputForcePlayerIn( inputdata_t &inputdata );
	void			InputForcePlayerOut( inputdata_t &inputdata );
};

BEGIN_DATADESC( CPropVehicleViewController )
	DEFINE_INPUTFUNC( FIELD_STRING, "ForcePlayerIn", InputForcePlayerIn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForcePlayerOut", InputForcePlayerOut ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( vehicle_viewcontroller, CPropVehicleViewController );

//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropVehicleViewController::Spawn( void )
{
	BaseClass::Spawn();
	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleViewController::Think(void)
{
	BaseClass::Think();

	SetSimulationTime( gpGlobals->curtime );
	SetNextThink( gpGlobals->curtime );
	SetAnimatedEveryTick( true );

	StudioFrameAdvance();

	// If the exit anim has finished, move the player to the right spot and stop animating
	if ( IsSequenceFinished() && (m_bExitAnimOn || m_bEnterAnimOn) )
	{
		// If we're exiting and have had the tau cannon removed, we don't want to reset the animation
		GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn, false );
		m_bExitAnimOn = false;
		m_bEnterAnimOn = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleViewController::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// Does nothing, because this vehicle doesn't drive
}

//-----------------------------------------------------------------------------
// Purpose: 
//    NOTE: Doesn't call the base call enter vehicle on purpose!
//-----------------------------------------------------------------------------
void CPropVehicleViewController::EnterVehicle( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	m_hPlayer = pPlayer;

	pPlayer->SetViewOffset( vec3_origin );
	pPlayer->ShowCrosshair( false );
	m_playerOn.FireOutput( pPlayer, this, 0 );

	// Start Thinking
	SetNextThink( gpGlobals->curtime );

	m_VehiclePhysics.GetVehicle()->OnVehicleEnter();

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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleViewController::ExitVehicle( int nRole )
{
	BaseClass::ExitVehicle( nRole );
	m_bEnterAnimOn = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleViewController::InputForcePlayerIn( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
	if ( !pPlayer )
		return;

	ResetUseKey( pPlayer );

	// Get the entry animation from the input
	int iEntryAnim = ACTIVITY_NOT_AVAILABLE;
	if ( inputdata.value.StringID() != NULL_STRING )
	{
		iEntryAnim = LookupSequence( inputdata.value.String() );
		if ( iEntryAnim == ACTIVITY_NOT_AVAILABLE )
		{
			Warning("vehicle_viewcontroller %s could not find specified entry animation %s\n", STRING(GetEntityName()), inputdata.value.String() );
			return;
		}
	}

	// Make sure we successfully got in the vehicle
	if ( pPlayer->GetInVehicle( GetServerVehicle(), VEHICLE_ROLE_DRIVER ) == false )
	{
		// The player was unable to enter the vehicle and the output has failed
		Assert( 0 );
		return;
	}

	// Setup the "enter" vehicle sequence
	SetCycle( 0 );
	m_flAnimTime = gpGlobals->curtime;
	ResetSequence( iEntryAnim );
	ResetClientsideFrame();
	m_bEnterAnimOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleViewController::InputForcePlayerOut( inputdata_t &inputdata )
{
	if ( !GetDriver() )
		return;

	GetServerVehicle()->HandlePassengerExit( m_hPlayer );
}
