//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"		
#include "c_physicsprop.h"		
#include "iclientvehicle.h"
#include <vgui_controls/Controls.h>
#include <Color.h>
#include "vehicle_viewblend_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern float RemapAngleRange( float startInterval, float endInterval, float value );


#define ROLL_CURVE_ZERO		5		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	45		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between

#define POD_VIEW_FOV		90
#define POD_VIEW_YAW_MIN	-60
#define POD_VIEW_YAW_MAX	60
#define POD_VIEW_PITCH_MIN	-90
#define POD_VIEW_PITCH_MAX	38		// Don't let players look down and see that the pod is empty


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PropVehiclePrisonerPod : public C_PhysicsProp, public IClientVehicle
{
	DECLARE_CLASS( C_PropVehiclePrisonerPod, C_PhysicsProp );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	C_PropVehiclePrisonerPod();
	
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
	virtual C_BaseCombatCharacter* GetPassenger( int nRole );
	virtual int	GetPassengerRole( C_BaseCombatCharacter *pEnt );
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
};


IMPLEMENT_CLIENTCLASS_DT(C_PropVehiclePrisonerPod, DT_PropVehiclePrisonerPod, CPropVehiclePrisonerPod)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropBool( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropBool( RECVINFO( m_bExitAnimOn ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropVehiclePrisonerPod )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropVehiclePrisonerPod::C_PropVehiclePrisonerPod( void )
{
	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );

	m_ViewSmoothingData.pVehicle = this;
	m_ViewSmoothingData.bClampEyeAngles = true;
	m_ViewSmoothingData.flPitchCurveZero = PITCH_CURVE_ZERO;
	m_ViewSmoothingData.flPitchCurveLinear = PITCH_CURVE_LINEAR;
	m_ViewSmoothingData.flRollCurveZero = ROLL_CURVE_ZERO;
	m_ViewSmoothingData.flRollCurveLinear = ROLL_CURVE_LINEAR;
	m_ViewSmoothingData.flFOV = POD_VIEW_FOV;

	m_flFOV = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropVehiclePrisonerPod::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehiclePrisonerPod::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( !m_hPlayer && m_hPrevPlayer )
	{
		// They have just exited the vehicle.
		// Sometimes we never reach the end of our exit anim, such as if the
		// animation doesn't have fadeout 0 specified in the QC, so we fail to
		// catch it in VehicleViewSmoothing. Catch it here instead.
		m_ViewSmoothingData.bWasRunningAnim = false;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropVehiclePrisonerPod::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}


//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropVehiclePrisonerPod::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropVehiclePrisonerPod::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*=NULL*/ )
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
void C_PropVehiclePrisonerPod::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

	// Limit the yaw.
	float flAngleDiff = AngleDiff( pCmd->viewangles.y, vehicleEyeAngles.y );
	flAngleDiff = clamp( flAngleDiff, POD_VIEW_YAW_MIN, POD_VIEW_YAW_MAX );
	pCmd->viewangles.y = vehicleEyeAngles.y + flAngleDiff;

	// Limit the pitch -- don't let them look down into the empty pod!
	flAngleDiff = AngleDiff( pCmd->viewangles.x, vehicleEyeAngles.x );
	flAngleDiff = clamp( flAngleDiff, POD_VIEW_PITCH_MIN, POD_VIEW_PITCH_MAX );
	pCmd->viewangles.x = vehicleEyeAngles.x + flAngleDiff;
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropVehiclePrisonerPod::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// Pod doesn't need to adjust the clip planes.
	//flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
void C_PropVehiclePrisonerPod::DrawHudElements( )
{
}


