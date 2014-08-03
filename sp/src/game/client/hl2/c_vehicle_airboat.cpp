//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side implementation of the airboat.
//
//			- Dampens motion of driver's view to reduce nausea.
//			- Autocenters driver's view after a period of inactivity.
//			- Controls headlights.
//			- Controls curve parameters for pitch/roll blending.
//
//=============================================================================//

#include "cbase.h"
#include "c_prop_vehicle.h"
#include "datacache/imdlcache.h"
#include "flashlighteffect.h"
#include "movevars_shared.h"
#include "ammodef.h"
#include "SpriteTrail.h"
#include "beamdraw.h"
#include "enginesprite.h"
#include "fx_quad.h"
#include "fx.h"
#include "fx_water.h"
#include "engine/ivdebugoverlay.h"
#include "view.h"
#include "clienteffectprecachesystem.h"
#include "c_basehlplayer.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar r_AirboatViewBlendTo( "r_AirboatViewBlendTo", "1", FCVAR_CHEAT );
ConVar r_AirboatViewBlendToScale( "r_AirboatViewBlendToScale", "0.03", FCVAR_CHEAT );
ConVar r_AirboatViewBlendToTime( "r_AirboatViewBlendToTime", "1.5", FCVAR_CHEAT );

ConVar cl_draw_airboat_wake( "cl_draw_airboat_wake", "1", FCVAR_CHEAT );

// Curve parameters for pitch/roll blending.
// NOTE: Must restart (or create a new airboat) after changing these cvars!
ConVar r_AirboatRollCurveZero( "r_AirboatRollCurveZero", "90.0", FCVAR_CHEAT );			// Roll less than this is clamped to zero.
ConVar r_AirboatRollCurveLinear( "r_AirboatRollCurveLinear", "120.0", FCVAR_CHEAT );	// Roll greater than this is mapped directly.
																						// Spline in between.

ConVar r_AirboatPitchCurveZero( "r_AirboatPitchCurveZero", "25.0", FCVAR_CHEAT );		// Pitch less than this is clamped to zero.
ConVar r_AirboatPitchCurveLinear( "r_AirboatPitchCurveLinear", "60.0", FCVAR_CHEAT );	// Pitch greater than this is mapped directly.
																						// Spline in between.

ConVar airboat_joy_response_move( "airboat_joy_response_move", "1" );					// Quadratic steering response
																						

#define AIRBOAT_DELTA_LENGTH_MAX	12.0f			// 1 foot
#define AIRBOAT_FRAMETIME_MIN		1e-6

#define HEADLIGHT_DISTANCE		1000

#define	MAX_WAKE_POINTS	16
#define	WAKE_POINT_MASK (MAX_WAKE_POINTS-1)

#define	WAKE_LIFETIME	0.5f

//=============================================================================
//
// Client-side Airboat Class
//
class C_PropAirboat : public C_PropVehicleDriveable
{
	DECLARE_CLASS( C_PropAirboat, C_PropVehicleDriveable );

public:

	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();
 	DECLARE_DATADESC();

	C_PropAirboat();
	~C_PropAirboat();

public:

	// C_BaseEntity
	virtual void Simulate();

	// IClientVehicle
	virtual void UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd );
	virtual void OnEnteredVehicle( C_BasePlayer *pPlayer );
	virtual int GetPrimaryAmmoType() const;
	virtual int GetPrimaryAmmoClip() const;
	virtual bool PrimaryAmmoUsesClips() const;
	virtual int GetPrimaryAmmoCount() const;
	virtual int GetJoystickResponseCurve() const;

	int		DrawModel( int flags );

	// Draws crosshair in the forward direction of the boat
	void DrawHudElements( );

private:

	void DrawPropWake( Vector origin, float speed );
	void DrawPontoonSplash( Vector position, Vector direction, float speed );
	void DrawPontoonWake( Vector startPos, Vector wakeDir, float wakeLength, float speed);

	void DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles );
	void DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime );
	void ComputePDControllerCoefficients( float *pCoefficientsOut, float flFrequency, float flDampening, float flDeltaTime );

	void UpdateHeadlight( void );
	void UpdateWake( void );
	int	 DrawWake( void );
	void DrawSegment( const BeamSeg_t &beamSeg, const Vector &vNormal );

	TrailPoint_t *GetTrailPoint( int n )
	{
		int nIndex = (n + m_nFirstStep) & WAKE_POINT_MASK;
		return &m_vecSteps[nIndex];
	}

private:

	Vector		m_vecLastEyePos;
	Vector		m_vecLastEyeTarget;
	Vector		m_vecEyeSpeed;
	Vector		m_vecTargetSpeed;

	float		m_flViewAngleDeltaTime;

	bool		m_bHeadlightIsOn;
	int			m_nAmmoCount;
	CHeadlightEffect *m_pHeadlight;

	int				m_nExactWaterLevel;
	
	TrailPoint_t	m_vecSteps[MAX_WAKE_POINTS];
	int				m_nFirstStep;
	int				m_nStepCount;
	float			m_flUpdateTime;

	TimedEvent		m_SplashTime;
	CMeshBuilder	m_Mesh;

	Vector			m_vecPhysVelocity;
};

IMPLEMENT_CLIENTCLASS_DT( C_PropAirboat, DT_PropAirboat, CPropAirboat )
	RecvPropBool( RECVINFO( m_bHeadlightIsOn ) ),
	RecvPropInt( RECVINFO( m_nAmmoCount ) ),
	RecvPropInt( RECVINFO( m_nExactWaterLevel ) ),
	RecvPropInt( RECVINFO( m_nWaterLevel ) ),
	RecvPropVector( RECVINFO( m_vecPhysVelocity ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropAirboat )
	DEFINE_FIELD( m_vecLastEyePos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecLastEyeTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecEyeSpeed, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecTargetSpeed, FIELD_VECTOR ),
	//DEFINE_FIELD( m_flViewAngleDeltaTime, FIELD_FLOAT ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_PropAirboat::C_PropAirboat()
{
	m_vecEyeSpeed.Init();
	m_flViewAngleDeltaTime = 0.0f;
	m_pHeadlight = NULL;

	m_ViewSmoothingData.flPitchCurveZero = r_AirboatPitchCurveZero.GetFloat();
	m_ViewSmoothingData.flPitchCurveLinear = r_AirboatPitchCurveLinear.GetFloat();
	m_ViewSmoothingData.flRollCurveZero = r_AirboatRollCurveZero.GetFloat();
	m_ViewSmoothingData.flRollCurveLinear = r_AirboatRollCurveLinear.GetFloat();

	m_ViewSmoothingData.rollLockData.flLockInterval = 1.5;
	m_ViewSmoothingData.rollLockData.flUnlockBlendInterval = 1.0;

	m_ViewSmoothingData.pitchLockData.flLockInterval = 1.5;
	m_ViewSmoothingData.pitchLockData.flUnlockBlendInterval = 1.0;

	m_nFirstStep = 0;
	m_nStepCount = 0;
	m_SplashTime.Init( 60 );
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
C_PropAirboat::~C_PropAirboat()
{
	if (m_pHeadlight)
	{
		delete m_pHeadlight;
	}
}


//-----------------------------------------------------------------------------
// Draws the ammo for the airboat
//-----------------------------------------------------------------------------
int C_PropAirboat::GetPrimaryAmmoType() const
{
	if ( m_nAmmoCount < 0 )
		return -1;

	int nAmmoType = GetAmmoDef()->Index( "AirboatGun" );
	return nAmmoType; 
}

int C_PropAirboat::GetPrimaryAmmoCount() const
{ 
	return m_nAmmoCount; 
}

bool C_PropAirboat::PrimaryAmmoUsesClips() const
{ 
	return false; 
}

int C_PropAirboat::GetPrimaryAmmoClip() const
{ 
	return -1; 
}

//-----------------------------------------------------------------------------
// The airboat prefers a more peppy response curve for joystick control.
//-----------------------------------------------------------------------------
int C_PropAirboat::GetJoystickResponseCurve() const
{
	return airboat_joy_response_move.GetInt();
}

//-----------------------------------------------------------------------------
// Draws crosshair in the forward direction of the boat
//-----------------------------------------------------------------------------
void C_PropAirboat::DrawHudElements( )
{
	BaseClass::DrawHudElements();

	MDLCACHE_CRITICAL_SECTION();

	CHudTexture *pIcon = gHUD.GetIcon( IsX360() ? "crosshair_default" : "plushair" );
	if ( pIcon != NULL )
	{
		float x, y;
		Vector screen;

		int vx, vy, vw, vh;
		vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );
		float screenWidth = vw;
		float screenHeight = vh;
		
		x = screenWidth/2;
		y = screenHeight/2;

		int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
		Vector vehicleEyeOrigin;
		QAngle vehicleEyeAngles;
		GetAttachment( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

		// Only worry about yaw.
		vehicleEyeAngles.x = vehicleEyeAngles.z = 0.0f;

		Vector vecForward;
		AngleVectors( vehicleEyeAngles, &vecForward );
		VectorMA( vehicleEyeOrigin, 100.0f, vecForward, vehicleEyeOrigin );

		ScreenTransform( vehicleEyeOrigin, screen );
		x += 0.5 * screen[0] * screenWidth + 0.5;
		y -= 0.5 * screen[1] * screenHeight + 0.5;

		x -= pIcon->Width() / 2; 
		y -= pIcon->Height() / 2; 
		
		pIcon->DrawSelf( x, y, gHUD.m_clrNormal );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Blend view angles.
//-----------------------------------------------------------------------------
void C_PropAirboat::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( r_AirboatViewBlendTo.GetInt() )
	{
		//
		// Autocenter the view after a period of no mouse movement while throttling.
		//
		bool bResetViewAngleTime = false;

		if ( ( pCmd->mousedx != 0 || pCmd->mousedy != 0 ) || ( fabsf( m_flThrottle ) < 0.01f ) )
		{
			if ( IsX360() )
			{
				// Only reset this if there isn't an autoaim target!
				C_BaseHLPlayer *pLocalHLPlayer = (C_BaseHLPlayer *)pLocalPlayer;
				if ( pLocalHLPlayer )
				{
					// Get the autoaim target.
					CBaseEntity *pTarget = pLocalHLPlayer->m_HL2Local.m_hAutoAimTarget.Get();

					if( !pTarget )
					{
						bResetViewAngleTime = true;
					}
				}
			}
			else
			{
				bResetViewAngleTime = true;
			}
		}

		if( bResetViewAngleTime )
		{
			m_flViewAngleDeltaTime = 0.0f;
		}
		else
		{
			m_flViewAngleDeltaTime += gpGlobals->frametime;
		}

		if ( m_flViewAngleDeltaTime > r_AirboatViewBlendToTime.GetFloat() )
		{
			// Blend the view angles.
			int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
			Vector vehicleEyeOrigin;
			QAngle vehicleEyeAngles;
			GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );
			
			QAngle outAngles;
			InterpolateAngles( pCmd->viewangles, vehicleEyeAngles, outAngles, r_AirboatViewBlendToScale.GetFloat() );
			pCmd->viewangles = outAngles;
		}
	}

	BaseClass::UpdateViewAngles( pLocalPlayer, pCmd );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropAirboat::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
	// Get the frametime. (Check to see if enough time has passed to warrent dampening).
	float flFrameTime = gpGlobals->frametime;
	if ( flFrameTime < AIRBOAT_FRAMETIME_MIN )
	{
		vecVehicleEyePos = m_vecLastEyePos;
		DampenUpMotion( vecVehicleEyePos, vecVehicleEyeAngles, 0.0f );
		return;
	}

	// Keep static the sideways motion.

	// Dampen forward/backward motion.
	DampenForwardMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );

	// Blend up/down motion.
	DampenUpMotion( vecVehicleEyePos, vecVehicleEyeAngles, flFrameTime );
}


//-----------------------------------------------------------------------------
// Use the controller as follows:
// speed += ( pCoefficientsOut[0] * ( targetPos - currentPos ) + pCoefficientsOut[1] * ( targetSpeed - currentSpeed ) ) * flDeltaTime;
//-----------------------------------------------------------------------------
void C_PropAirboat::ComputePDControllerCoefficients( float *pCoefficientsOut,
												  float flFrequency, float flDampening,
												  float flDeltaTime )
{
	float flKs = 9.0f * flFrequency * flFrequency;
	float flKd = 4.5f * flFrequency * flDampening;

	float flScale = 1.0f / ( 1.0f + flKd * flDeltaTime + flKs * flDeltaTime * flDeltaTime );

	pCoefficientsOut[0] = flKs * flScale;
	pCoefficientsOut[1] = ( flKd + flKs * flDeltaTime ) * flScale;
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropAirboat::DampenForwardMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// vecVehicleEyePos = real eye position this frame

	// m_vecLastEyePos = eye position last frame
	// m_vecEyeSpeed = eye speed last frame
	// vecPredEyePos = predicted eye position this frame (assuming no acceleration - it will get that from the pd controller).
	// vecPredEyeSpeed = predicted eye speed
	Vector vecPredEyePos = m_vecLastEyePos + m_vecEyeSpeed * flFrameTime;
	Vector vecPredEyeSpeed = m_vecEyeSpeed;

	// m_vecLastEyeTarget = real eye position last frame (used for speed calculation).
	// Calculate the approximate speed based on the current vehicle eye position and the eye position last frame.
	Vector vecVehicleEyeSpeed = ( vecVehicleEyePos - m_vecLastEyeTarget ) / flFrameTime;
	m_vecLastEyeTarget = vecVehicleEyePos;
	if (vecVehicleEyeSpeed.Length() == 0.0)
	{
		return;
	}

	// Calculate the delta between the predicted eye position and speed and the current eye position and speed.
	Vector vecDeltaSpeed = vecVehicleEyeSpeed - vecPredEyeSpeed;
	Vector vecDeltaPos = vecVehicleEyePos - vecPredEyePos;

	// Forward vector.
	Vector vecForward;
	AngleVectors( vecVehicleEyeAngles, &vecForward );

	float flDeltaLength = vecDeltaPos.Length();
	if ( flDeltaLength > AIRBOAT_DELTA_LENGTH_MAX )
	{
		// Clamp.
		float flDelta = flDeltaLength - AIRBOAT_DELTA_LENGTH_MAX;
		if ( flDelta > 40.0f )
		{
			// This part is a bit of a hack to get rid of large deltas (at level load, etc.).
			m_vecLastEyePos = vecVehicleEyePos;
			m_vecEyeSpeed = vecVehicleEyeSpeed;
		}
		else
		{
			// Position clamp.
			float flRatio = AIRBOAT_DELTA_LENGTH_MAX / flDeltaLength;
			vecDeltaPos *= flRatio;
			Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
			vecVehicleEyePos -= vecForwardOffset;
			m_vecLastEyePos = vecVehicleEyePos;

			// Speed clamp.
			vecDeltaSpeed *= flRatio;
			float flCoefficients[2];
			ComputePDControllerCoefficients( flCoefficients, r_AirboatViewDampenFreq.GetFloat(), r_AirboatViewDampenDamp.GetFloat(), flFrameTime );
			m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );
		}
	}
	else
	{
		// Generate an updated (dampening) speed for use in next frames position prediction.
		float flCoefficients[2];
		ComputePDControllerCoefficients( flCoefficients, r_AirboatViewDampenFreq.GetFloat(), r_AirboatViewDampenDamp.GetFloat(), flFrameTime );
		m_vecEyeSpeed += ( ( flCoefficients[0] * vecDeltaPos + flCoefficients[1] * vecDeltaSpeed ) * flFrameTime );
		
		// Save off data for next frame.
		m_vecLastEyePos = vecPredEyePos;
		
		// Move eye forward/backward.
		Vector vecForwardOffset = vecForward * ( vecForward.Dot( vecDeltaPos ) );
		vecVehicleEyePos -= vecForwardOffset;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropAirboat::DampenUpMotion( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles, float flFrameTime )
{
	// Get up vector.
	Vector vecUp;
	AngleVectors( vecVehicleEyeAngles, NULL, NULL, &vecUp );
	vecUp.z = clamp( vecUp.z, 0.0f, vecUp.z );
	vecVehicleEyePos.z += r_AirboatViewZHeight.GetFloat() * vecUp.z;

	// NOTE: Should probably use some damped equation here.
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropAirboat::OnEnteredVehicle( C_BasePlayer *pPlayer )
{
	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachment( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

	m_vecLastEyeTarget = vehicleEyeOrigin;
	m_vecLastEyePos = vehicleEyeOrigin;
	m_vecEyeSpeed = vec3_origin;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropAirboat::Simulate()
{
	UpdateHeadlight();
	UpdateWake();

	BaseClass::Simulate();
}


//-----------------------------------------------------------------------------
// Purpose: Creates, destroys, and updates the headlight effect as needed.
//-----------------------------------------------------------------------------
void C_PropAirboat::UpdateHeadlight()
{
	if (m_bHeadlightIsOn)
	{
		if (!m_pHeadlight)
		{
			// Turned on the headlight; create it.
			m_pHeadlight = new CHeadlightEffect();

			if (!m_pHeadlight)
				return;

			m_pHeadlight->TurnOn();
		}

		// The headlight is emitted from an attachment point so that it can move
		// as we turn the handlebars.
		int nHeadlightIndex = LookupAttachment( "vehicle_headlight" );

		Vector vecLightPos;
		QAngle angLightDir;
		GetAttachment(nHeadlightIndex, vecLightPos, angLightDir);

		Vector vecLightDir, vecLightRight, vecLightUp;
		AngleVectors( angLightDir, &vecLightDir, &vecLightRight, &vecLightUp );

		// Update the light with the new position and direction.		
		m_pHeadlight->UpdateLight( vecLightPos, vecLightDir, vecLightRight, vecLightUp, HEADLIGHT_DISTANCE );
	}
	else if (m_pHeadlight)
	{
		// Turned off the headlight; delete it.
		delete m_pHeadlight;
		m_pHeadlight = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropAirboat::UpdateWake( void )
{
	if ( gpGlobals->frametime <= 0.0f )
		return;

	// Can't update too quickly
	if ( m_flUpdateTime > gpGlobals->curtime )
		return;

	Vector	screenPos = GetRenderOrigin();
	screenPos.z = m_nExactWaterLevel;

	TrailPoint_t *pLast = m_nStepCount ? GetTrailPoint( m_nStepCount-1 ) : NULL;
	if ( ( pLast == NULL ) || ( pLast->m_vecScreenPos.DistToSqr( screenPos ) > 4.0f ) )
	{
		// If we're over our limit, steal the last point and put it up front
		if ( m_nStepCount >= MAX_WAKE_POINTS )
		{
			--m_nStepCount;
			++m_nFirstStep;
		}

		// Save off its screen position, not its world position
		TrailPoint_t *pNewPoint = GetTrailPoint( m_nStepCount );
		pNewPoint->m_vecScreenPos = screenPos + Vector( 0, 0, 2 );
		pNewPoint->m_flDieTime	= gpGlobals->curtime + WAKE_LIFETIME;
		pNewPoint->m_flWidthVariance = random->RandomFloat( -16, 16 );
		
		if ( pLast )
		{
			pNewPoint->m_flTexCoord	= pLast->m_flTexCoord + pLast->m_vecScreenPos.DistTo( screenPos );
			pNewPoint->m_flTexCoord = fmod( pNewPoint->m_flTexCoord, 1 );
		}
		else
		{
			pNewPoint->m_flTexCoord = 0.0f;
		}

		++m_nStepCount;
	}

	// Don't update again for a bit
	m_flUpdateTime = gpGlobals->curtime + ( 0.5f / (float) MAX_WAKE_POINTS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &beamSeg - 
//-----------------------------------------------------------------------------
void C_PropAirboat::DrawSegment( const BeamSeg_t &beamSeg, const Vector &vNormal )
{
	// Build the endpoints.
	Vector vPoint1, vPoint2;
	VectorMA( beamSeg.m_vPos,  beamSeg.m_flWidth*0.5f, vNormal, vPoint1 );
	VectorMA( beamSeg.m_vPos, -beamSeg.m_flWidth*0.5f, vNormal, vPoint2 );

	// Specify the points.
	m_Mesh.Position3fv( vPoint1.Base() );
	m_Mesh.Color4f( VectorExpand( beamSeg.m_vColor ), beamSeg.m_flAlpha );
	m_Mesh.TexCoord2f( 0, 0, beamSeg.m_flTexCoord );
	m_Mesh.TexCoord2f( 1, 0, beamSeg.m_flTexCoord );
	m_Mesh.AdvanceVertex();

	m_Mesh.Position3fv( vPoint2.Base() );
	m_Mesh.Color4f( VectorExpand( beamSeg.m_vColor ), beamSeg.m_flAlpha );
	m_Mesh.TexCoord2f( 0, 1, beamSeg.m_flTexCoord );
	m_Mesh.TexCoord2f( 1, 1, beamSeg.m_flTexCoord );
	m_Mesh.AdvanceVertex();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//-----------------------------------------------------------------------------
void C_PropAirboat::DrawPontoonSplash( Vector origin, Vector direction, float speed )
{
	Vector	offset;
	
	CSmartPtr<CSplashParticle> pSimple = CSplashParticle::Create( "splish" );
	pSimple->SetSortOrigin( origin );

	SimpleParticle	*pParticle;

	Vector	color = Vector( 0.8f, 0.8f, 0.75f );
	float	colorRamp;

	float	flScale = RemapVal( speed, 64, 256, 0.75f, 1.0f );

	PMaterialHandle	hMaterial;
	
	float tempDelta = gpGlobals->frametime;

	while( m_SplashTime.NextEvent( tempDelta ) )
	{
		if ( random->RandomInt( 0, 1 ) )
		{
			hMaterial = ParticleMgr()->GetPMaterial( "effects/splash1" );
		}
		else
		{
			hMaterial = ParticleMgr()->GetPMaterial( "effects/splash2" );
		}

		offset = RandomVector( -8.0f * flScale, 8.0f * flScale );
		offset[2] = 0.0f;
		offset += origin;

		pParticle = (SimpleParticle *) pSimple->AddParticle( sizeof( SimpleParticle ), hMaterial, offset );

		if ( pParticle == NULL )
			continue;
		
		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime	= 0.25f;

		pParticle->m_vecVelocity.Random( -0.4f, 0.4f );
		pParticle->m_vecVelocity += (direction*5.0f+Vector(0,0,1));
		
		VectorNormalize( pParticle->m_vecVelocity );

		pParticle->m_vecVelocity *= speed + random->RandomFloat( -128.0f, 128.0f );
		
		colorRamp = random->RandomFloat( 0.75f, 1.25f );

		pParticle->m_uchColor[0]	= MIN( 1.0f, color[0] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[1]	= MIN( 1.0f, color[1] * colorRamp ) * 255.0f;
		pParticle->m_uchColor[2]	= MIN( 1.0f, color[2] * colorRamp ) * 255.0f;
		
		pParticle->m_uchStartSize	= random->RandomFloat( 8, 16 ) * flScale;
		pParticle->m_uchEndSize		= pParticle->m_uchStartSize * 2;
		
		pParticle->m_uchStartAlpha	= 255;
		pParticle->m_uchEndAlpha	= 0;
		
		pParticle->m_flRoll			= random->RandomInt( 0, 360 );
		pParticle->m_flRollDelta	= random->RandomFloat( -4.0f, 4.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Vector	startPos - 
//			wakeDir - 
//			wakeLength - 
//-----------------------------------------------------------------------------
void C_PropAirboat::DrawPontoonWake( Vector	startPos, Vector wakeDir, float wakeLength, float speed )
{
#define	WAKE_STEPS	6

	Vector	wakeStep = wakeDir * ( wakeLength / (float) WAKE_STEPS );
	Vector	origin;
	float	scale;

	IMaterial *pMaterial = materials->FindMaterial( "effects/splashwake1", NULL, false );
	CMatRenderContextPtr pRenderContext( materials );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( 0, 0, 0, pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, WAKE_STEPS );

	for ( int i = 0; i < WAKE_STEPS; i++ )
	{
		origin = startPos + ( wakeStep * i );
		origin[0] += random->RandomFloat( -4.0f, 4.0f );
		origin[1] += random->RandomFloat( -4.0f, 4.0f );
		origin[2] = m_nExactWaterLevel + 2.0f;

		float scaleRange = RemapVal( i, 0, WAKE_STEPS-1, 32, 64 );
		scale = scaleRange + ( 8.0f * sin( gpGlobals->curtime * 5 * i ) );
		
		float alpha = RemapValClamped( speed, 128, 600, 0.05f, 0.25f );
		float color[4] = { 1.0f, 1.0f, 1.0f, alpha };
		
		// Needs to be time based so it'll freeze when the game is frozen
		float yaw = random->RandomFloat( 0, 360 );

		Vector rRight = ( Vector(1,0,0) * cos( DEG2RAD( yaw ) ) ) - ( Vector(0,1,0) * sin( DEG2RAD( yaw ) ) );
		Vector rUp = ( Vector(1,0,0) * cos( DEG2RAD( yaw+90.0f ) ) ) - ( Vector(0,1,0) * sin( DEG2RAD( yaw+90.0f ) ) );

		Vector point;
		meshBuilder.Color4fv (color);
		meshBuilder.TexCoord2f (0, 0, 1);
		VectorMA (origin, -scale, rRight, point);
		VectorMA (point, -scale, rUp, point);
		meshBuilder.Position3fv (point.Base());
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4fv (color);
		meshBuilder.TexCoord2f (0, 0, 0);
		VectorMA (origin, scale, rRight, point);
		VectorMA (point, -scale, rUp, point);
		meshBuilder.Position3fv (point.Base());
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4fv (color);
		meshBuilder.TexCoord2f (0, 1, 0);
		VectorMA (origin, scale, rRight, point);
		VectorMA (point, scale, rUp, point);
		meshBuilder.Position3fv (point.Base());
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4fv (color);
		meshBuilder.TexCoord2f (0, 1, 1);
		VectorMA (origin, -scale, rRight, point);
		VectorMA (point, scale, rUp, point);
		meshBuilder.Position3fv (point.Base());
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_PropAirboat::DrawWake( void )
{
	if ( cl_draw_airboat_wake.GetBool() == false )
		return 0;

	// Make sure we're in water...
	if ( GetWaterLevel() == 0 )
		return 0;

	//FIXME: For now, we don't draw slime this way
	if ( GetWaterLevel() == 2 )
		return 0;

	bool bDriven = ( GetPassenger( VEHICLE_ROLE_DRIVER ) != NULL );

	Vector vehicleDir = m_vecPhysVelocity;
	float vehicleSpeed = VectorNormalize( vehicleDir );

	Vector vecPontoonFrontLeft;
	Vector vecPontoonFrontRight;
	Vector vecPontoonRearLeft;
	Vector vecPontoonRearRight;
	Vector vecSplashPoint;

	QAngle fooAngles;

	//FIXME: This lookup should be cached off
	// Get all attachments
	GetAttachment( LookupAttachment( "raytrace_fl" ), vecPontoonFrontLeft, fooAngles );
	GetAttachment( LookupAttachment( "raytrace_fr" ), vecPontoonFrontRight, fooAngles );
	GetAttachment( LookupAttachment( "raytrace_rl" ), vecPontoonRearLeft, fooAngles );
	GetAttachment( LookupAttachment( "raytrace_rr" ), vecPontoonRearRight, fooAngles );
	GetAttachment( LookupAttachment( "splash_pt" ), vecSplashPoint, fooAngles );

	// Find the direction of the pontoons
	Vector vecLeftWakeDir = ( vecPontoonRearLeft - vecPontoonFrontLeft );
	Vector vecRightWakeDir = ( vecPontoonRearRight - vecPontoonFrontRight );

	// Find the pontoon's size
	float flWakeLeftLength = VectorNormalize( vecLeftWakeDir );
	float flWakeRightLength = VectorNormalize( vecRightWakeDir );

	vecPontoonFrontLeft.z = m_nExactWaterLevel;
	vecPontoonFrontRight.z = m_nExactWaterLevel;

	if ( bDriven && vehicleSpeed > 128.0f )
	{
		DrawPontoonWake( vecPontoonFrontLeft, vecLeftWakeDir, flWakeLeftLength, vehicleSpeed );
		DrawPontoonWake( vecPontoonFrontRight, vecRightWakeDir, flWakeRightLength, vehicleSpeed );

		Vector vecSplashDir;
		Vector vForward;
		GetVectors( &vForward, NULL, NULL );

		if ( m_vecPhysVelocity.x < -64.0f )
		{
			vecSplashDir = vecLeftWakeDir - vForward;
			VectorNormalize( vecSplashDir );

			// Don't do this if we're going backwards
			if ( m_vecPhysVelocity.y > 0.0f )
			{
				DrawPontoonSplash( vecPontoonFrontLeft + ( vecLeftWakeDir * 1.0f ), vecSplashDir, m_vecPhysVelocity.y );
			}
		}
		else if ( m_vecPhysVelocity.x > 64.0f )
		{
			vecSplashDir = vecRightWakeDir + vForward;
			VectorNormalize( vecSplashDir );

			// Don't do this if we're going backwards
			if ( m_vecPhysVelocity.y > 0.0f )
			{
				DrawPontoonSplash( vecPontoonFrontRight + ( vecRightWakeDir * 1.0f ), vecSplashDir, m_vecPhysVelocity.y );
			}
		}
	}

	// Must have at least one point
	if ( m_nStepCount <= 1 )
		return 1;

	IMaterial *pMaterial = materials->FindMaterial( "effects/splashwake4", 0);
		
	//Bind the material
	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, pMaterial );
	
	m_Mesh.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, (m_nStepCount-1) * 2 );

	TrailPoint_t *pLast = GetTrailPoint( m_nStepCount - 1 );
	
	TrailPoint_t currentPoint;
	currentPoint.m_flDieTime = gpGlobals->curtime + 0.5f;
	currentPoint.m_vecScreenPos = GetAbsOrigin();
	currentPoint.m_vecScreenPos[2] = m_nExactWaterLevel + 16;
	currentPoint.m_flTexCoord = pLast->m_flTexCoord + currentPoint.m_vecScreenPos.DistTo(pLast->m_vecScreenPos);
	currentPoint.m_flTexCoord = fmod( currentPoint.m_flTexCoord, 1 );
	currentPoint.m_flWidthVariance = 0.0f;

	TrailPoint_t *pPrevPoint = NULL;
	
	Vector segDir, normal;

	for ( int i = 0; i <= m_nStepCount; ++i )
	{
		// This makes it so that we're always drawing to the current location
		TrailPoint_t *pPoint = (i != m_nStepCount) ? GetTrailPoint(i) : &currentPoint;

		float flLifePerc = RemapValClamped( ( pPoint->m_flDieTime - gpGlobals->curtime ), 0, WAKE_LIFETIME, 0.0f, 1.0f );

		BeamSeg_t curSeg;
		curSeg.m_vColor.x = curSeg.m_vColor.y = curSeg.m_vColor.z = 1.0f;

		float flAlphaFade = flLifePerc;
		float alpha = RemapValClamped( fabs( m_vecPhysVelocity.y ), 128, 600, 0.0f, 1.0f );

		curSeg.m_flAlpha = 0.25f;
		curSeg.m_flAlpha *= flAlphaFade * alpha;

		curSeg.m_vPos = pPoint->m_vecScreenPos;
		
		float widthBase = SimpleSplineRemapVal( fabs( m_vecPhysVelocity.y ), 128, 600, 32, 48 );

		curSeg.m_flWidth = Lerp( flLifePerc, widthBase*6, widthBase );
		curSeg.m_flWidth += pPoint->m_flWidthVariance;

		if ( curSeg.m_flWidth < 0.0f )
		{
			curSeg.m_flWidth = 0.0f;
		}

		curSeg.m_flTexCoord = pPoint->m_flTexCoord;

		if ( pPrevPoint != NULL )
		{
			segDir = ( pPrevPoint->m_vecScreenPos - pPoint->m_vecScreenPos );
			VectorNormalize( segDir );

			normal = CrossProduct( segDir, Vector( 0, 0, -1 ) );

			DrawSegment( curSeg, normal );
		}

		pPrevPoint = pPoint;
	}

	m_Mesh.End();
	pMesh->Draw();

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flags - 
// Output : int
//-----------------------------------------------------------------------------
int C_PropAirboat::DrawModel( int flags )
{
	if ( BaseClass::DrawModel( flags ) == false )
		return 0;
	
	if ( !m_bReadyToDraw )
		return 0;

	return DrawWake();
}
