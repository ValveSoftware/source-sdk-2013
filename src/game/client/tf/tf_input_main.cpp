//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2 specific input handling
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "kbutton.h"
#include "input.h"

#include "c_tf_player.h"
#include "cam_thirdperson.h"

extern ConVar		thirdperson_platformer;
extern ConVar		cam_idealyaw;
extern ConVar		cl_yawspeed;
extern kbutton_t	in_left;
extern kbutton_t	in_right;
extern CThirdPersonManager g_ThirdPersonManager;

//-----------------------------------------------------------------------------
// Purpose: TF Input interface
//-----------------------------------------------------------------------------
class CTFInput : public CInput
{
public:
	CTFInput()
		: m_angThirdPersonOffset( 0.f, 0.f, 0.f )
	{}
	virtual		float		CAM_CapYaw( float fVal ) const OVERRIDE;
	virtual		float		CAM_CapPitch( float fVal ) const OVERRIDE;
	virtual		void		AdjustYaw( float speed, QAngle& viewangles );
	virtual		float		JoyStickAdjustYaw( float flSpeed ) OVERRIDE;
	virtual void ApplyMouse( QAngle& viewangles, CUserCmd *cmd, float mouse_x, float mouse_y ) OVERRIDE;
private:

	QAngle m_angThirdPersonOffset;
};

static CTFInput g_Input;

// Expose this interface
IInput *input = ( IInput * )&g_Input;

//-----------------------------------------------------------------------------
// Purpose: Cap yaw movement
//-----------------------------------------------------------------------------
float CTFInput::CAM_CapYaw( float fVal ) const
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return fVal;

	if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		// Use the unified charge turn rate limiter
		return pPlayer->m_Shared.CapChargeTurnRate( fVal );
	}

	return fVal;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFInput::CAM_CapPitch( float fVal ) const
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return fVal;

	return fVal;
}


//-----------------------------------------------------------------------------
// Purpose: Unified yaw adjustment for all input sources
//-----------------------------------------------------------------------------
void CTFInput::AdjustYaw( float speed, QAngle& viewangles )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	
	// Store previous viewangles for comparison
	QAngle prevViewangles = viewangles;
	float totalYawChange = 0.0f;
	
	// Handle keyboard turning if not strafing
	if ( !( in_strafe.state & 1 ) )
	{
		// Calculate keyboard input turn amount
		float frameTime = gpGlobals->frametime;
		float yaw_right = speed*cl_yawspeed.GetFloat() * KeyState( &in_right ) * frameTime * ( 1.0f / TICK_INTERVAL );
		float yaw_left = speed*cl_yawspeed.GetFloat() * KeyState( &in_left ) * frameTime * ( 1.0f / TICK_INTERVAL );
		
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			// Cap the keyboard turning using unified function
			yaw_right = pPlayer->m_Shared.CapChargeTurnRate( yaw_right );
			yaw_left = pPlayer->m_Shared.CapChargeTurnRate( yaw_left );
		}
		
		// Apply keyboard turn
		viewangles[YAW] -= yaw_right;
		viewangles[YAW] += yaw_left;
		
		// Calculate how much we've turned from keyboard
		totalYawChange = AngleDiff( viewangles[YAW], prevViewangles[YAW] );
	}
	
	// For mouse movement, the engine has already applied the changes to viewangles
	// We need to check if the mouse change + keyboard change exceeds the cap
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		// Calculate the total angle change from previous frame
		float mouseYawChange = AngleDiff(viewangles[YAW], prevViewangles[YAW]) - totalYawChange;
		
		// Cap mouse movement
		float cappedMouseYaw = pPlayer->m_Shared.CapChargeTurnRate(mouseYawChange);
		
		// Apply capped mouse movement
		if (cappedMouseYaw != mouseYawChange)
		{
			viewangles[YAW] = prevViewangles[YAW] + cappedMouseYaw + totalYawChange;
		}
	}

	// Handle third person camera adjustments
	if ( CAM_IsThirdPerson() )
	{
		if ( thirdperson_platformer.GetInt() )
		{
			float side = KeyState(&in_moveleft) - KeyState(&in_moveright);
			float forward = KeyState(&in_forward) - KeyState(&in_back);

			if ( side || forward )
			{
				viewangles[YAW] = RAD2DEG(atan2(side, forward)) + g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ];
			}
			if ( side || forward || KeyState (&in_right) || KeyState (&in_left) )
			{
				cam_idealyaw.SetValue( g_ThirdPersonManager.GetCameraOffsetAngles()[ YAW ] - viewangles[ YAW ] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Joystick yaw adjustment
//-----------------------------------------------------------------------------
float CTFInput::JoyStickAdjustYaw( float flSpeed )
{
	// Make sure we're not strafing
	if ( flSpeed && !(in_strafe.state & 1) )
	{
		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			// Use the unified charge turn rate limiter
			flSpeed = pPlayer->m_Shared.CapChargeTurnRate( flSpeed );
		}
	}

	return flSpeed;
}

ConVar tf_halloween_kart_cam_follow( "tf_halloween_kart_cam_follow", "0.3f", FCVAR_CHEAT );
void CTFInput::ApplyMouse( QAngle& viewangles, CUserCmd *cmd, float mouse_x, float mouse_y )
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		// Make the camera drift a little behind the car
		float flDelta = pPlayer->GetTauntYaw() - m_angThirdPersonOffset[YAW];
		float flSign = Sign( flDelta );
		flDelta = Max( 2.f , (float)fabs(flDelta) ) * flSign;
		float flSpeed = gpGlobals->frametime * flDelta * flDelta * tf_halloween_kart_cam_follow.GetFloat();
		m_angThirdPersonOffset[YAW] = Approach( pPlayer->GetTauntYaw(), m_angThirdPersonOffset[YAW], flSpeed );
		viewangles[YAW] = m_angThirdPersonOffset[YAW];
	}
	else
	{
		CInput::ApplyMouse( viewangles, cmd, mouse_x, mouse_y );
	}
}