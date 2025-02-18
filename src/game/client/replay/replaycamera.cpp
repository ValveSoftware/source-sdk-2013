//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#include "replay/ireplaymanager.h"
#include "replay/replay.h"
#include "replay/replaycamera.h"
#include "cdll_client_int.h"
#include "util_shared.h"
#include "prediction.h"
#include "movevars_shared.h"
#include "in_buttons.h"
#include "text_message.h"
#include "vgui_controls/Controls.h"
#include "vgui/ILocalize.h"
#include "vguicenterprint.h"
#include "game/client/iviewport.h"
#include "vgui/IInput.h"
#include <KeyValues.h>
#include "iinput.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "vgui/IInput.h"
#include "mathlib/noise.h"

#ifdef CSTRIKE_DLL
	#include "c_cs_player.h"
#endif

ConVar replay_editor_camera_length( "replay_editor_camera_length", "15", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "This is the camera length used to simulate camera shake in the replay editor.  The larger this number, the more the actual position will change.  It can also be set to negative values." );

//ConVar spec_autodirector( "spec_autodirector", "1", FCVAR_CLIENTDLL | FCVAR_CLIENTCMD_CAN_EXECUTE, "Auto-director chooses best view modes while spectating" );
extern ConVar spec_autodirector;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CHASE_CAM_DISTANCE_MAX	96.0f
#define WALL_OFFSET				6.0f
#define DEFAULT_ROAMING_ACCEL	5.0f
#define DEFAULT_ROAMING_SPEED	3.0f

static Vector WALL_MIN(-WALL_OFFSET,-WALL_OFFSET,-WALL_OFFSET);
static Vector WALL_MAX(WALL_OFFSET,WALL_OFFSET,WALL_OFFSET);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
static wchar_t* ConvertCRtoNL( wchar_t *str )
{
	for ( wchar_t *ch = str; *ch != 0; ch++ )
		if ( *ch == L'\r' )
			*ch = L'\n';
	return str;
}

static C_ReplayCamera s_ReplayCamera;

C_ReplayCamera *ReplayCamera()
{
	return &s_ReplayCamera;
}

C_ReplayCamera::C_ReplayCamera()
{
	Reset();

	m_nNumSpectators = 0;
	m_szTitleText[0] = 0;
}

C_ReplayCamera::~C_ReplayCamera()
{

}

void C_ReplayCamera::Init()
{
	ListenForGameEvent( "game_newmap" );
		
	Reset();

	m_nNumSpectators = 0;
	m_szTitleText[0] = 0;
}

void C_ReplayCamera::Reset()
{
	m_nCameraMode = OBS_MODE_FIXED;
	m_iTarget1 = m_iTarget2 = 0;
	m_flFOV = 90.0f;
	m_flDistance = m_flLastDistance = CHASE_CAM_DISTANCE_MAX;
	m_flInertia = 3.0f;
	m_flPhi = 0;
	m_flTheta = 0;
	m_flOffset = 0;
	m_bEntityPacketReceived = false;
	m_bOverrideView = false;
	m_flOldTime = 0.0f;
	m_bInputEnabled = true;

	m_flRoamingAccel = DEFAULT_ROAMING_ACCEL;
	m_flRoamingSpeed = DEFAULT_ROAMING_SPEED;
	m_flRoamingFov[0] = m_flRoamingFov[1] = 90.0f;
	m_flRoamingRotFilterFactor = 10.0f;
	m_flRoamingShakeAmount = 0.0f;
	m_flRoamingShakeSpeed = 0.0f;
	m_flNoiseSample = 0.0f;
	m_flRoamingShakeDir = Lerp( 0.5f, FREE_CAM_SHAKE_DIR_MIN, FREE_CAM_SHAKE_DIR_MAX );
	
	m_vCamOrigin.Init();
	m_aCamAngle.Init();
	m_aSmoothedRoamingAngles.Init();

	m_OverrideViewData.origin.Init();
	m_OverrideViewData.angles.Init();
	m_OverrideViewData.fov = 90;

	m_LastCmd.Reset();
	m_vecVelocity.Init();

	InitRoamingKeys();
}

void C_ReplayCamera::InitRoamingKeys()
{
	m_aMovementButtons[DIR_FWD  ] = KEY_W;
	m_aMovementButtons[DIR_BACK ] = KEY_S;
	m_aMovementButtons[DIR_LEFT ] = KEY_A;
	m_aMovementButtons[DIR_RIGHT] = KEY_D;
	m_aMovementButtons[DIR_DOWN ] = KEY_X;
	m_aMovementButtons[DIR_UP   ] = KEY_Z;
}

bool C_ReplayCamera::ShouldUseDefaultRoamingSettings() const
{
	return vgui::input()->IsKeyDown( KEY_LSHIFT );
}

void C_ReplayCamera::CalcChaseCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta )
{
	bool bManual = true;
	
 	Vector targetOrigin1, targetOrigin2, cameraOrigin, forward;

 	if ( m_iTarget1 == 0 )
		return;

	// get primary target, also translates to ragdoll
	C_BaseEntity *target1 = GetPrimaryTarget();

 	if ( !target1 ) 
		return;
	
	if ( target1->IsAlive() && target1->IsDormant() )
		return;

	targetOrigin1 = target1->GetRenderOrigin();

	if ( !target1->IsAlive() )
	{
		targetOrigin1 += VEC_DEAD_VIEWHEIGHT;
	}
	else if ( target1->GetFlags() & FL_DUCKING )
	{
		targetOrigin1 += VEC_DUCK_VIEW;
	}
	else
	{
		targetOrigin1 += VEC_VIEW;
	}

	// get secondary target if set
	C_BaseEntity *target2 = NULL;

	if ( m_iTarget2 > 0 && (m_iTarget2 != m_iTarget1) && !bManual )
	{
		target2 = ClientEntityList().GetBaseEntity( m_iTarget2 );

		// if target is out PVS and not dead, it's not valid
		if ( target2 && target2->IsDormant() && target2->IsAlive() )
			target2 = NULL;

		if ( target2 )
		{
			targetOrigin2 = target2->GetRenderOrigin();

			if ( !target2->IsAlive() )
			{
				targetOrigin2 += VEC_DEAD_VIEWHEIGHT;
			}
			else if ( target2->GetFlags() & FL_DUCKING )
			{
				targetOrigin2 += VEC_DUCK_VIEW;
			}
			else
			{
				targetOrigin2 += VEC_VIEW;
			}
		}
	}

		// apply angle offset & smoothing
	QAngle angleOffset(  m_flPhi, m_flTheta, 0 );
	QAngle cameraAngles = m_aCamAngle;

	if ( bManual )
	{
		// let spectator choose the view angles
 		engine->GetViewAngles( cameraAngles );
	}
	else if ( target2 )
	{
		// look into direction of second target
 		forward = targetOrigin2 - targetOrigin1;
        VectorAngles( forward, cameraAngles );
        cameraAngles.z = 0; // no ROLL
	}
	else if ( m_iTarget2 == 0 || m_iTarget2 == m_iTarget1)
	{
		// look into direction where primary target is looking
		cameraAngles = target1->EyeAngles();
		cameraAngles.x = 0; // no PITCH
		cameraAngles.z = 0; // no ROLL
	}
	else
	{
		// target2 is missing, just keep angelsm, reset offset
		angleOffset.Init();
	}

	if ( !bManual )
	{
		if ( !target1->IsAlive() )
		{
			angleOffset.x = 15;
		}

		cameraAngles += angleOffset;
	}

	AngleVectors( cameraAngles, &forward );

	VectorNormalize( forward );

	// calc optimal camera position
	VectorMA(targetOrigin1, -m_flDistance, forward, cameraOrigin );

 	targetOrigin1.z += m_flOffset; // add offset

	// clip against walls
  	trace_t trace;
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull( targetOrigin1, cameraOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, target1, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

  	float dist = VectorLength( trace.endpos -  targetOrigin1 );

	// grow distance by 32 unit a second
  	m_flLastDistance += flDelta * 32.0f; 

  	if ( dist > m_flLastDistance )
	{
		VectorMA(targetOrigin1, -m_flLastDistance, forward, cameraOrigin );
	}
 	else
	{
		cameraOrigin = trace.endpos;
		m_flLastDistance = dist;
	}
	
  	if ( target2 )
	{
		// if we have 2 targets look at point between them
		forward = (targetOrigin1+targetOrigin2)/2 - cameraOrigin;
 		QAngle angle;
		VectorAngles( forward, angle );
		cameraAngles.y = angle.y;
		
		NormalizeAngles( cameraAngles );
		cameraAngles.x = clamp( cameraAngles.x, -60.f, 60.f );

		SmoothCameraAngle( cameraAngles );
	}
	else
	{
		SetCameraAngle( cameraAngles );
	}
 	
	VectorCopy( cameraOrigin, m_vCamOrigin );
	VectorCopy( m_aCamAngle, eyeAngles );
	VectorCopy( m_vCamOrigin, eyeOrigin );

	fov = m_flFOV;
}

int C_ReplayCamera::GetMode()
{
	return m_nCameraMode;	
}

C_BaseEntity* C_ReplayCamera::GetPrimaryTarget()
{
	if ( m_iTarget1 <= 0 )
		return NULL;

	C_BaseEntity* target = ClientEntityList().GetEnt( m_iTarget1 );

	return target;
}

void C_ReplayCamera::CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta )
{
	C_BasePlayer *pPlayer = UTIL_PlayerByIndex( m_iTarget1 );

	if ( !pPlayer )
		return;

	if ( !pPlayer->IsAlive() )
	{
		// if dead, show from 3rd person
		CalcChaseCamView( eyeOrigin, eyeAngles, fov, flDelta );
		return;
	}

	m_aCamAngle	= pPlayer->EyeAngles();
	m_vCamOrigin = pPlayer->GetAbsOrigin();
	m_flFOV = pPlayer->GetFOV();

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		m_vCamOrigin += VEC_DUCK_VIEW;
	}
	else
	{
		m_vCamOrigin += VEC_VIEW;
	}

	eyeOrigin = m_vCamOrigin;
	eyeAngles = m_aCamAngle;
	fov = m_flFOV;

	pPlayer->CalcViewModelView( eyeOrigin, eyeAngles);

	C_BaseViewModel *pViewModel = pPlayer->GetViewModel( 0 );

	if ( pViewModel )
	{
		Assert( pViewModel->GetOwner() == pPlayer );
		pViewModel->UpdateVisibility();
	}

	// This fixes the bug where going from third or first person to free cam defaults to some arbitrary angle,
	// because free cam uses engine->GetViewAngles().
	engine->SetViewAngles( m_aCamAngle );
}

void C_ReplayCamera::Accelerate( Vector& wishdir, float wishspeed, float accel, float flDelta )
{
	float addspeed, accelspeed, currentspeed;

	// See if we are changing direction a bit
	currentspeed =m_vecVelocity.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of acceleration.
	accelspeed = accel * flDelta * wishspeed;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (int i=0 ; i<3 ; i++)
	{
		m_vecVelocity[i] += accelspeed * wishdir[i];	
	}
}

bool C_ReplayCamera::ShouldOverrideView( Vector& origin, QAngle& angles, float& fov )
{
	if ( !m_bOverrideView )
		return false;

	origin = m_OverrideViewData.origin;
	angles = m_OverrideViewData.angles;
	fov = m_OverrideViewData.fov;

	return true;
}

// movement code is a copy of CGameMovement::FullNoClipMove()
void C_ReplayCamera::CalcRoamingView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta)
{
	// only if PVS isn't locked by auto-director
	if ( !IsPVSLocked() )
	{
		Vector wishvel;
		Vector forward, right, up;
		Vector wishdir;
		float wishspeed;
		float factor = ShouldUseDefaultRoamingSettings() ? DEFAULT_ROAMING_SPEED : m_flRoamingSpeed;
		float maxspeed = sv_maxspeed.GetFloat() * factor;

		AngleVectors ( m_aCamAngle, &forward, &right, &up );  // Determine movement angles

		if ( m_LastCmd.buttons & IN_SPEED )
		{
			factor /= 2.0f;
		}

		// Check for movement
		float fmove = 0.0f;
		float smove = 0.0f;
		float vmove = 0.0f;
		if ( !enginevgui->IsGameUIVisible() && m_bInputEnabled )
		{
			// Forward/backward movement
			if ( vgui::input()->IsKeyDown( m_aMovementButtons[DIR_FWD] ) )
			{
				fmove =  factor * maxspeed;
			}
			else if ( vgui::input()->IsKeyDown( m_aMovementButtons[DIR_BACK] ) )
			{
				fmove = -factor * maxspeed;
			}

			// Lateral movement
			if ( vgui::input()->IsKeyDown( m_aMovementButtons[DIR_LEFT] ) )
			{
				smove = -factor * maxspeed;
			}
			else if ( vgui::input()->IsKeyDown( m_aMovementButtons[DIR_RIGHT] ) )
			{
				smove =  factor * maxspeed;
			}

			// Vertical movement
			if ( vgui::input()->IsKeyDown( m_aMovementButtons[DIR_UP] ) )
			{
				vmove =  factor * maxspeed;
			}
			else if ( vgui::input()->IsKeyDown( m_aMovementButtons[DIR_DOWN] ) )
			{
				vmove = -factor * maxspeed;
			}
		}

		// Normalize remainder of vectors
		VectorNormalize(forward);
		VectorNormalize(right);
		VectorNormalize(up);

		for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
			wishvel[i] = forward[i]*fmove + right[i]*smove + up[i]*vmove;
		wishvel[2] += m_LastCmd.upmove * factor;

		VectorCopy (wishvel, wishdir);   // Determine magnitude of speed of move
		wishspeed = VectorNormalize(wishdir);

		//
		// Clamp to server defined max speed
		//
		if (wishspeed > maxspeed )
		{
			VectorScale (wishvel, maxspeed/wishspeed, wishvel);
			wishspeed = maxspeed;
		}

		const float flRoamingAccel = ShouldUseDefaultRoamingSettings() ?
			DEFAULT_ROAMING_ACCEL : m_flRoamingAccel;

		if ( flRoamingAccel > 0.0 )
		{
			// Set move velocity
			Accelerate ( wishdir, wishspeed, flRoamingAccel, flDelta );

			float spd = VectorLength( m_vecVelocity );
			if ( CloseEnough( spd, 0.0f ) ) 
			{
				m_vecVelocity.Init();
			}
			else
			{
				// Bleed off some speed, but if we have less than the bleed
				//  threshold, bleed the threshold amount.
				float control = spd;

				float friction = sv_friction.GetFloat();

				// Add the amount to the drop amount.
				float drop = control * friction * flDelta;

				// scale the velocity
				float newspeed = spd - drop;
				if (newspeed < 0)
					newspeed = 0;

				// Determine proportion of old speed we are using.
				newspeed /= spd;
				VectorScale( m_vecVelocity, newspeed, m_vecVelocity );
			}
		}
		else
		{
			VectorCopy( wishvel, m_vecVelocity );
		}

		// Just move ( don't clip or anything )
		VectorMA( m_vCamOrigin, flDelta, m_vecVelocity, m_vCamOrigin );
		
		// get camera angle directly from engine
		engine->GetViewAngles( m_aCamAngle );

		// Zero out velocity if in noaccel mode
		if ( sv_specaccelerate.GetFloat() < 0.0f )
		{
			m_vecVelocity.Init();
		}
	}

	// Smooth the angles
	float flPercent = clamp( flDelta * m_flRoamingRotFilterFactor, 0.0f, 1.0f );
	m_aSmoothedRoamingAngles = Lerp( flPercent, m_aSmoothedRoamingAngles, m_aCamAngle );

	Vector vCameraShakeOffset;
	vCameraShakeOffset.Init();

	// Add in camera shake
	if ( !ShouldUseDefaultRoamingSettings() && m_flRoamingShakeAmount > 0.0f )
	{
		QAngle angShake( 0.0f, 0.0f, 0.0f );

		m_flNoiseSample += m_flRoamingShakeSpeed * flDelta;

		float flNoiseX = Lerp( FractalNoise( Vector( m_flNoiseSample, 0.0f, 0.0f ), 1 ), -1.0f, 1.0f );
		float flNoiseY = Lerp( FractalNoise( Vector( 0.0f, 1000 + m_flNoiseSample, 0.0f ), 1 ), -1.0f, 1.0f );

		// Vertical shake
		const float flAmplitudeX = m_flRoamingShakeAmount * ( m_flRoamingShakeDir < 0.0f ? ( 1.0f + m_flRoamingShakeDir ) : 1.0f );
		angShake.x = flAmplitudeX * flNoiseX;

		// Lateral shake
		const float flAmplitudeY = m_flRoamingShakeAmount * ( m_flRoamingShakeDir > 0.0f ? ( 1.0f - m_flRoamingShakeDir ) : 1.0f );
		angShake.y = flAmplitudeY * flNoiseY;

		// The math below simulates a camera with length "replay_editor_camera_length," so that the camera position bounces around
		// as if it were on someone's shoulder.  If we were to just use angShake at this point with no translation, we would get a
		// camera that looks around but is anchored and doesn't feel quite right.  With the code below, the camera will translate,
		// but be centered around the same point as when camera shake is off completely, rather than actually offsetting that point
		// by the camera length.

		// Get the forward vector from the shake transform/angles
		Vector vShakeForward;
		AngleVectors( angShake, &vShakeForward );

		// Calculate an offset, simulating a camera length
		Vector vCameraOffset = vShakeForward * replay_editor_camera_length.GetFloat();

		// Get the global matrix without any shake
		matrix3x4_t mGlobal;
		AngleMatrix( m_aSmoothedRoamingAngles, m_vCamOrigin, mGlobal );

		// Convert local shake angles and offset to a matrix
		matrix3x4_t mShake;
		AngleMatrix( angShake, mShake );

		// Setup a translation matrix using the offset
		matrix3x4_t mOffset;
		SetIdentityMatrix( mOffset );
		PositionMatrix( vCameraOffset, mOffset );

		matrix3x4_t mOffsetInv;
		MatrixInvert( mOffset, mOffsetInv );

		// The meat
		matrix3x4_t mFinal = mGlobal;
		MatrixMultiply( mFinal, mOffsetInv, mFinal );
		MatrixMultiply( mFinal, mShake, mFinal );
		MatrixMultiply( mFinal, mOffset, mFinal );

		// Convert back to Vector / QAngle
		MatrixAngles( mFinal, eyeAngles, eyeOrigin );
	}
	else
	{
		// No shake
		eyeOrigin = m_vCamOrigin;
		eyeAngles = m_aSmoothedRoamingAngles;
	}

	fov = m_flRoamingFov[0];
}

void C_ReplayCamera::CalcFixedView(Vector& eyeOrigin, QAngle& eyeAngles, float& fov, float flDelta )
{
	eyeOrigin = m_vCamOrigin;
	eyeAngles = m_aCamAngle;
	fov = m_flFOV;

	if ( m_iTarget1 == 0 )
		return;

 	C_BaseEntity * target = ClientEntityList().GetBaseEntity( m_iTarget1 );
	
	if ( target && target->IsAlive() )
	{
		// if we're chasing a target, change viewangles
		QAngle angle;
		VectorAngles( (target->GetAbsOrigin()+VEC_VIEW) - m_vCamOrigin, angle );
		SmoothCameraAngle( angle );
	}
}

void C_ReplayCamera::PostEntityPacketReceived()
{
	m_bEntityPacketReceived = true;
}

void C_ReplayCamera::SmoothFov( float flDelta )
{
	m_flRoamingFov[0] = clamp(
		Lerp( 7 * flDelta, m_flRoamingFov[0], m_flRoamingFov[1] ),
//		Approach( m_flRoamingFov[1], m_flRoamingFov[0], 40 * m_flFrameTime ),
		FREE_CAM_FOV_MIN,
		FREE_CAM_FOV_MAX
	);
}

void C_ReplayCamera::FixupMovmentParents()
{
	// Find resource zone
	
	for (	ClientEntityHandle_t e = ClientEntityList().FirstHandle();
			e != ClientEntityList().InvalidHandle(); e = ClientEntityList().NextHandle( e ) )
	{
		C_BaseEntity *ent = C_BaseEntity::Instance( e );

		if ( !ent )
			continue;

		ent->HierarchyUpdateMoveParent();
	}
}

void C_ReplayCamera::EnableInput( bool bEnable )
{
	m_bInputEnabled = bEnable;
}

void C_ReplayCamera::ClearOverrideView()
{
	if ( m_bOverrideView )
	{
		m_vCamOrigin = m_OverrideViewData.origin;
		m_aCamAngle = m_aSmoothedRoamingAngles = m_OverrideViewData.angles;
		m_flRoamingFov[0] = m_flRoamingFov[1] = m_OverrideViewData.fov;
	}

	m_bOverrideView = false;

	// Set view angles in engine so that CalcRoamingView() won't pop to some stupid angle
	engine->SetViewAngles( m_aCamAngle );
}

void C_ReplayCamera::OverrideView( const Vector *pOrigin, const QAngle *pAngles, float flFov )
{
	m_bOverrideView = true;

	m_OverrideViewData.origin = *pOrigin;
	m_OverrideViewData.angles = *pAngles;
	m_OverrideViewData.fov = flFov;
}

void C_ReplayCamera::CalcView(Vector &origin, QAngle &angles, float &fov )
{
	// NOTE ABOUT CLOCKS:  'realtime' is used, because otherwise we can't move the camera round while
	// the game is paused.

	// Calculate elapsed time since last call to CalcView()
	if ( m_flOldTime == 0.0f )
	{
		m_flOldTime = gpGlobals->realtime;
	}
	const float flDelta = gpGlobals->realtime - m_flOldTime;
	m_flOldTime = gpGlobals->realtime;

	if ( m_bEntityPacketReceived )
	{
		// try to fixup movment parents
		FixupMovmentParents();
		m_bEntityPacketReceived = false;
	}

	// Completely override?
	if ( ShouldOverrideView( origin, angles, fov ) )
		return;

	switch ( m_nCameraMode )
	{
		case OBS_MODE_ROAMING	:	CalcRoamingView( origin, angles, fov, flDelta );
									break;

		case OBS_MODE_FIXED		:	CalcFixedView( origin, angles, fov, flDelta );
									break;

		case OBS_MODE_IN_EYE	:	CalcInEyeCamView( origin, angles, fov, flDelta );
									break;

		case OBS_MODE_CHASE		:	CalcChaseCamView( origin, angles, fov, flDelta  );
									break;
	}

	// Cache in case we want to access this data later in the frame
	m_CachedView.origin = origin;
	m_CachedView.angles = angles;
	m_CachedView.fov = fov;
}

void C_ReplayCamera::GetCachedView( Vector &origin, QAngle &angles, float &fov )
{
	origin = m_CachedView.origin;
	angles = m_CachedView.angles;
	fov = m_CachedView.fov;
}

void C_ReplayCamera::SetMode(int iMode)
{
	if ( m_nCameraMode == iMode )
		return;

    Assert( iMode > OBS_MODE_NONE && iMode <= LAST_PLAYER_OBSERVERMODE );

	m_nCameraMode = iMode;

	if ( m_nCameraMode != OBS_MODE_ROAMING && m_nCameraMode != OBS_MODE_CHASE )
	{
		ClearOverrideView();
	}
}

void C_ReplayCamera::SetPrimaryTarget( int nEntity ) 
{
 	if ( m_iTarget1 == nEntity )
		return;

	m_iTarget1 = nEntity;

	if ( GetMode() == OBS_MODE_ROAMING )
	{
		Vector vOrigin;
		QAngle aAngles;
		float flFov;

		CalcChaseCamView( vOrigin,  aAngles, flFov, 0.015f );
	}
	else if ( GetMode() == OBS_MODE_CHASE )
	{
		C_BaseEntity* target = ClientEntityList().GetEnt( m_iTarget1 );
		if ( target )
		{
			QAngle eyeAngle = target->EyeAngles();
			prediction->SetViewAngles( eyeAngle );
		}
	}

	m_flLastDistance = m_flDistance;
	m_flLastAngleUpdateTime = -1;
}

void C_ReplayCamera::SpecNextPlayer( bool bInverse )
{
	int start = 1;

	if ( m_iTarget1 > 0 && m_iTarget1 <= gpGlobals->maxClients )
		start = m_iTarget1;

	int index = start;

	while ( true )
	{	
		// got next/prev player
		if ( bInverse )
			index--;
		else
			index++;

		// check bounds
		if ( index < 1 )
			index = gpGlobals->maxClients;
		else if ( index > gpGlobals->maxClients )
			index = 1;

		if ( index == start )
			break; // couldn't find a new valid player

		C_BasePlayer *pPlayer =	UTIL_PlayerByIndex( index );

		if ( !pPlayer )
			continue;

		// only follow living players 
		if ( pPlayer->IsObserver() )
			continue;

		break; // found a new player
	}

	SetPrimaryTarget( index );

	// turn off auto director once user tried to change view settings
	SetAutoDirector( false );
}

void C_ReplayCamera::SpecPlayerByPredicate( const char *szSearch )
{
	CBasePlayer *pPlayer = UTIL_PlayerByCommandArg( szSearch );
	if ( !pPlayer )
		return;

	// only follow living players or dedicated spectators
	if ( pPlayer->IsObserver() && pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		return;

	SetPrimaryTarget( pPlayer->entindex() );
	return;
}

void C_ReplayCamera::FireGameEvent( IGameEvent * event)
{
	if ( !g_pEngineClientReplay->IsPlayingReplayDemo() )
		return;	// not in Replay mode

	const char *type = event->GetName();

	if ( Q_strcmp( "game_newmap", type ) == 0 )
	{
		// Do not reset the camera, since we reload the map when "rewinding"
		// and want to keep our camera settings intact. 
		// Reset();	// reset all camera settings

		// show spectator UI
		if ( !gViewPortInterface )
			return;

		if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
        {
			SetMode( OBS_MODE_IN_EYE );

			CReplay *pReplay = g_pReplayManager->GetPlayingReplay();
			SetPrimaryTarget( ( pReplay && pReplay->m_nPlayerSlot >= 0 ) ? pReplay->m_nPlayerSlot : 0 );
		}
		else
		{
			// during live broadcast only show black bars
			gViewPortInterface->ShowPanel( PANEL_SPECMENU, true );
		}

		return;
	}

	// after this only auto-director commands follow
	// don't execute them is autodirector is off and PVS is unlocked
	if ( !spec_autodirector.GetBool() && !IsPVSLocked() )
		return;
}

// this is a cheap version of FullNoClipMove():
void C_ReplayCamera::CreateMove( CUserCmd *cmd)
{
	if ( cmd )
	{
		m_LastCmd = *cmd;
	}
}

void C_ReplayCamera::SetCameraAngle( QAngle& targetAngle )
{
	m_aCamAngle	= targetAngle;
 	NormalizeAngles( m_aCamAngle );
	m_flLastAngleUpdateTime = gpGlobals->realtime;
}

void C_ReplayCamera::SmoothCameraAngle( QAngle& targetAngle )
{
	if ( m_flLastAngleUpdateTime > 0 )
	{
		float deltaTime = gpGlobals->realtime - m_flLastAngleUpdateTime;

		deltaTime = clamp( deltaTime*m_flInertia, 0.01f, 1.f);

		InterpolateAngles( m_aCamAngle, targetAngle, m_aCamAngle, deltaTime );
	}
	else
	{
		m_aCamAngle = targetAngle;
	}

	m_flLastAngleUpdateTime = gpGlobals->realtime;
}

bool C_ReplayCamera::IsPVSLocked()
{
	return false;
}

void C_ReplayCamera::SetAutoDirector( bool bActive )
{
	spec_autodirector.SetValue( bActive?1:0 );
}

#endif
