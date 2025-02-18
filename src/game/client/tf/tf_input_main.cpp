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
// Purpose: 
//-----------------------------------------------------------------------------
float CTFInput::CAM_CapYaw( float fVal ) const
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return fVal;

	if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		float flChargeYawCap = pPlayer->m_Shared.CalculateChargeCap();

		if ( fVal > flChargeYawCap )
			return flChargeYawCap;
		else if ( fVal < -flChargeYawCap )
			return -flChargeYawCap;
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
// Purpose: 
//-----------------------------------------------------------------------------
void CTFInput::AdjustYaw( float speed, QAngle& viewangles )
{
	if ( !(in_strafe.state & 1) )
	{
		float yaw_right = speed*cl_yawspeed.GetFloat() * KeyState (&in_right);
		float yaw_left = speed*cl_yawspeed.GetFloat() * KeyState (&in_left);

		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			float flChargeYawCap = pPlayer->m_Shared.CalculateChargeCap();

			if ( yaw_right > flChargeYawCap )
				yaw_right = flChargeYawCap;
			else if ( yaw_right < -flChargeYawCap )
				yaw_right = -flChargeYawCap;
			if ( yaw_left > flChargeYawCap )
				yaw_left = flChargeYawCap;
			else if ( yaw_left < -flChargeYawCap )
				yaw_left = -flChargeYawCap;
		}

		viewangles[YAW] -= yaw_right;
		viewangles[YAW] += yaw_left;
	}

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
// Purpose: 
//-----------------------------------------------------------------------------
float CTFInput::JoyStickAdjustYaw( float flSpeed )
{
	// Make sure we're not strafing
	if ( flSpeed && !(in_strafe.state & 1) )
	{
		CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			float flChargeYawCap = pPlayer->m_Shared.CalculateChargeCap();
			
			if ( flSpeed > 0.f && flSpeed > flChargeYawCap )
				flSpeed = flChargeYawCap;
			else if ( flSpeed < 0.f && flSpeed < -flChargeYawCap )
				flSpeed = -flChargeYawCap;
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