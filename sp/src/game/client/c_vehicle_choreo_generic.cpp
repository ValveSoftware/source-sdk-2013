//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"		
#include "c_props.h"
#include "iclientvehicle.h"
#include <vgui_controls/Controls.h>
#include <Color.h>
#include "vehicle_choreo_generic_shared.h"
#include "vehicle_viewblend_shared.h"
// NVNT haptic utils
#include "haptics/haptic_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern float RemapAngleRange( float startInterval, float endInterval, float value );


#define ROLL_CURVE_ZERO		5		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	45		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PropVehicleChoreoGeneric : public C_DynamicProp, public IClientVehicle
{
	DECLARE_CLASS( C_PropVehicleChoreoGeneric, C_DynamicProp );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	C_PropVehicleChoreoGeneric();
	
	void PreDataUpdate( DataUpdateType_t updateType );
	void PostDataUpdate( DataUpdateType_t updateType );

public:

	// IClientVehicle overrides.
	virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );
	virtual void GetVehicleFOV( float &flFOV )
	{
		flFOV = m_flFOV;
	}
	virtual void DrawHudElements();
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	virtual C_BaseCombatCharacter *GetPassenger( int nRole );
	virtual int	GetPassengerRole( C_BaseCombatCharacter *pPassenger );
	virtual void GetVehicleClipPlanes( float &flZNear, float &flZFar ) const;
	virtual int GetPrimaryAmmoType() const { return -1; }
	virtual int GetPrimaryAmmoCount() const { return -1; }
	virtual int GetPrimaryAmmoClip() const  { return -1; }
	virtual bool PrimaryAmmoUsesClips() const { return false; }
	virtual int GetJoystickResponseCurve() const { return 0; }

public:

	// C_BaseEntity overrides.
	virtual IClientVehicle*	GetClientVehicle() { return this; }
	virtual C_BaseEntity	*GetVehicleEnt() { return this; }
	virtual void SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) {}
	virtual void ProcessMovement( C_BasePlayer *pPlayer, CMoveData *pMoveData ) {}
	virtual void FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move ) {}
	virtual bool IsPredicted() const { return false; }
	virtual void ItemPostFrame( C_BasePlayer *pPlayer ) {}
	virtual bool IsSelfAnimating() { return false; };

private:

	CHandle<C_BasePlayer>	m_hPlayer;
	CHandle<C_BasePlayer>	m_hPrevPlayer;

	bool					m_bEnterAnimOn;
	bool					m_bExitAnimOn;
	Vector					m_vecEyeExitEndpoint;
	float					m_flFOV;				// The current FOV (changes during entry/exit anims).

	ViewSmoothingData_t		m_ViewSmoothingData;

	vehicleview_t m_vehicleView;
};

IMPLEMENT_CLIENTCLASS_DT(C_PropVehicleChoreoGeneric, DT_PropVehicleChoreoGeneric, CPropVehicleChoreoGeneric)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropBool( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropBool( RECVINFO( m_bExitAnimOn ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
	RecvPropBool( RECVINFO( m_vehicleView.bClampEyeAngles ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flPitchCurveZero ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flPitchCurveLinear ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flRollCurveZero ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flRollCurveLinear ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flFOV ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flYawMin ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flYawMax ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flPitchMin ) ),
	RecvPropFloat( RECVINFO( m_vehicleView.flPitchMax ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropVehicleChoreoGeneric )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropVehicleChoreoGeneric::C_PropVehicleChoreoGeneric( void )
{
	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );

	m_ViewSmoothingData.pVehicle = this;
	m_ViewSmoothingData.flPitchCurveZero = PITCH_CURVE_ZERO;
	m_ViewSmoothingData.flPitchCurveLinear = PITCH_CURVE_LINEAR;
	m_ViewSmoothingData.flRollCurveZero = ROLL_CURVE_ZERO;
	m_ViewSmoothingData.flRollCurveLinear = ROLL_CURVE_LINEAR;
	m_flFOV = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropVehicleChoreoGeneric::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleChoreoGeneric::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	// NVNT if we have entered this vehicle notify the haptics system
	if ( m_hPlayer && !m_hPrevPlayer )
	{
#if defined( WIN32 ) && !defined( _X360 )
		//They have just entered the vehicle.
		HapticsEnteredVehicle(this,m_hPlayer);
#endif
	}

	if ( !m_hPlayer && m_hPrevPlayer )
	{
#if defined( WIN32 ) && !defined( _X360 )
		// NVNT we have just exited this vehicle so we notify the haptics system
		HapticsExitedVehicle(this,m_hPrevPlayer);
#endif
		// They have just exited the vehicle.
		// Sometimes we never reach the end of our exit anim, such as if the
		// animation doesn't have fadeout 0 specified in the QC, so we fail to
		// catch it in VehicleViewSmoothing. Catch it here instead.
		m_ViewSmoothingData.bWasRunningAnim = false;

		//There's no need to "smooth" the view when leaving the vehicle so just set this here so the stair code doesn't get confused.
		m_hPrevPlayer->SetOldPlayerZ( m_hPrevPlayer->GetLocalOrigin().z );
	}

	m_ViewSmoothingData.bClampEyeAngles = m_vehicleView.bClampEyeAngles;
	m_ViewSmoothingData.flFOV = m_vehicleView.flFOV;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropVehicleChoreoGeneric::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}


//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropVehicleChoreoGeneric::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropVehicleChoreoGeneric::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*=NULL*/ )
{
	SharedVehicleViewSmoothing( m_hPlayer, 
								pAbsOrigin, pAbsAngles, 
								m_bEnterAnimOn, m_bExitAnimOn, 
								m_vecEyeExitEndpoint, 
								&m_ViewSmoothingData, 
								pFOV );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pLocalPlayer - 
//			pCmd - 
//-----------------------------------------------------------------------------
void C_PropVehicleChoreoGeneric::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

	// Limit the yaw.
	float flAngleDiff = AngleDiff( pCmd->viewangles.y, vehicleEyeAngles.y );
	flAngleDiff = clamp( flAngleDiff, (float) m_vehicleView.flYawMin, (float) m_vehicleView.flYawMax );
	pCmd->viewangles.y = vehicleEyeAngles.y + flAngleDiff;

	// Limit the pitch -- don't let them look down into the empty pod!
	flAngleDiff = AngleDiff( pCmd->viewangles.x, vehicleEyeAngles.x );
	flAngleDiff = clamp( flAngleDiff, (float) m_vehicleView.flPitchMin, (float) m_vehicleView.flPitchMax );
	pCmd->viewangles.x = vehicleEyeAngles.x + flAngleDiff;
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropVehicleChoreoGeneric::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// Pod doesn't need to adjust the clip planes.
	//flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
void C_PropVehicleChoreoGeneric::DrawHudElements( )
{
}


