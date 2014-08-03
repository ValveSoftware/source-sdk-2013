//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"		
#include <vgui_controls/Controls.h>
#include <Color.h>
#include "c_vehicle_crane.h"
#include "view.h"
#include "vehicle_viewblend_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int ScreenTransform( const Vector& point, Vector& screen );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PropCannon : public C_BaseAnimating, public IClientVehicle
{

	DECLARE_CLASS( C_PropCannon, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	C_PropCannon();
	
	void PreDataUpdate( DataUpdateType_t updateType );

public:

	// IClientVehicle overrides.
	virtual void GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );
	virtual void GetVehicleFOV( float &flFOV ) { flFOV = 0.0f; }
	virtual void DrawHudElements();
	virtual bool IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd ) {}
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
	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );

private:

	CHandle<C_BasePlayer>	m_hPlayer;
	CHandle<C_BasePlayer>	m_hPrevPlayer;

	bool					m_bEnterAnimOn;
	bool					m_bExitAnimOn;
	Vector					m_vecEyeExitEndpoint;

	Vector					m_vecOldShadowDir;

	ViewSmoothingData_t			m_ViewSmoothingData;
};


IMPLEMENT_CLIENTCLASS_DT(C_PropCannon, DT_PropCannon, CPropCannon)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropBool( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropBool( RECVINFO( m_bExitAnimOn ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropCannon )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()


#define ROLL_CURVE_ZERO		5		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	45		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropCannon::C_PropCannon( void )
{
	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );
	m_ViewSmoothingData.pVehicle = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropCannon::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropCannon::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropCannon::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropCannon::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*=NULL*/ )
{
	SharedVehicleViewSmoothing( m_hPlayer, 
								pAbsOrigin, pAbsAngles, 
								m_bEnterAnimOn, m_bExitAnimOn, 
								m_vecEyeExitEndpoint, 
								&m_ViewSmoothingData, 
								pFOV );
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropCannon::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
void C_PropCannon::DrawHudElements( )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : theMins - 
//			theMaxs - 
//-----------------------------------------------------------------------------
void C_PropCannon::GetRenderBounds( Vector &theMins, Vector &theMaxs )
{
	// This is kind of hacky:( Add 660.0 to the y coordinate of the bounding box to
	// allow for the full extension of the crane arm.
	BaseClass::GetRenderBounds( theMins, theMaxs );
	theMaxs.y += 660.0f;
}

