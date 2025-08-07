//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF2 specific input handling
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "kbutton.h"
#include "input.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "inetchannelinfo.h"

#include "c_tf_player.h"
#include "cam_thirdperson.h"
#include "tf_gamerules.h"

extern ConVar 		tf_charge_turn_debug_client;
extern ConVar		thirdperson_platformer;
extern ConVar		cam_idealyaw;
extern ConVar		cl_yawspeed;
extern ConVar		tf_charge_turn_rate; // Base cap for charge turning
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
// Purpose: Client-side charge state tracking for immediate turn capping
//-----------------------------------------------------------------------------
static bool s_bClientChargeActive = false;
static float s_flClientChargeStartTime = 0.0f;
static float s_flPredictiveClampDuration = 0.0f; // dynamically set from latency
static float s_flLastChargeInputTime = 0.0f;
static int s_nPreviousButtons = 0;
static int  s_nLastClampFrame = -1;
static bool s_bClampCached   = false;

//-----------------------------------------------------------------------------
// Purpose: Helper function to determine if we should apply charge turn capping
// This includes both confirmed server state and immediate client-side input detection
//-----------------------------------------------------------------------------
static bool ShouldApplyChargeClamping( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	int currFrame = gpGlobals->framecount;
	if ( currFrame == s_nLastClampFrame )
		return s_bClampCached;

	
	// Debug: Log function calls
	static float s_flLastDebugTime = 0.0f;
	
	// Cancel predictive clamp immediately if the player swings primary (melee),
	// matching server-side EndClassSpecialSkill()
	if ( pPlayer->m_nButtons & IN_ATTACK )
	{
		s_bClientChargeActive = false;
		s_nLastClampFrame = currFrame;
		s_bClampCached = false;
		return false;
	}
	if ( tf_charge_turn_debug_client.GetBool() && (gpGlobals->curtime - s_flLastDebugTime) > 0.1f )
	{
		DevMsg("[ShouldApplyChargeClamping] Called - server charge: %s, client active: %s\n", 
			pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) ? "YES" : "NO",
			s_bClientChargeActive ? "YES" : "NO");
		s_flLastDebugTime = gpGlobals->curtime;
	}
	

	// Always clamp if server has confirmed we're charging
	if ( pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
	{
		// Server confirmed charge, make sure our client state is active
		if ( !s_bClientChargeActive )
		{
			s_bClientChargeActive = true;
			s_flClientChargeStartTime = gpGlobals->curtime;
			if ( tf_charge_turn_debug_client.GetBool() )
			{
				DevMsg("[ShouldApplyChargeClamping] Server confirmed charge - activating client state\n");
			}
		}
		s_nLastClampFrame = currFrame;
		s_bClampCached = true;
		return true;
	}
	
	// Check if we have a demo shield equipped
	if ( !pPlayer->m_Shared.HasDemoShieldEquipped() )
	{
		s_bClientChargeActive = false;
		s_nLastClampFrame = currFrame;
		s_bClampCached = false;
		return false;
	}
	
	// Check if charge input was recently pressed (immediate client-side detection)
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && pLocalPlayer == pPlayer )
	{
		// Debug: Log button state changes
		if ( tf_charge_turn_debug_client.GetBool() )
		{
			if ( (pPlayer->m_nButtons & IN_ATTACK2) != (s_nPreviousButtons & IN_ATTACK2) )
			{
				DevMsg("[BUTTON DEBUG] IN_ATTACK2 changed: %s -> %s (class: %d)\n",
					(s_nPreviousButtons & IN_ATTACK2) ? "PRESSED" : "RELEASED",
					(pPlayer->m_nButtons & IN_ATTACK2) ? "PRESSED" : "RELEASED",
					pPlayer->GetPlayerClass()->GetClassIndex());
			}
		}
		
		// Check if IN_ATTACK2 is currently pressed (charge input for demoman)
		if ( (pPlayer->m_nButtons & IN_ATTACK2) && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_DEMOMAN )
		{
			// Check if this is a new press (not held from previous frame)
			if ( !(s_nPreviousButtons & IN_ATTACK2) )
			{
				// Only begin predictive clamping if shield has a full charge available
				if ( pPlayer->m_Shared.GetDemomanChargeMeter() >= 100.f )
				{
					// Fresh charge input detected - start immediate clamping
					s_bClientChargeActive = true;
					s_flClientChargeStartTime = gpGlobals->curtime;
					s_flLastChargeInputTime = gpGlobals->curtime;
					
					// Determine clamp duration based on current latency (round-trip) + 50ms safety buffer
					INetChannelInfo *nci = engine->GetNetChannelInfo();
					float flLatency = 0.0f;
					if ( nci )
					{
						flLatency = nci->GetAvgLatency( FLOW_OUTGOING ) + nci->GetAvgLatency( FLOW_INCOMING );
					}
					s_flPredictiveClampDuration = flLatency + 0.05f; // add 50ms
				}
				else if ( tf_charge_turn_debug_client.GetBool() )
				{
					DevMsg("[CLIENT IMMEDIATE] Charge input detected but meter not full (%.1f) – no predictive clamp\n", pPlayer->m_Shared.GetDemomanChargeMeter());
				}
				
				if ( tf_charge_turn_debug_client.GetBool() )
				{
					DevMsg("[CLIENT IMMEDIATE] Charge input detected - starting immediate turn clamping\n");
				}
			}
		}

		
		// Always update previous buttons for next frame
		s_nPreviousButtons = pPlayer->m_nButtons;
	}
	
	// If we have active client-side charge state, keep clamping for a short duration
	// This handles the gap between input and server confirmation
	if ( s_bClientChargeActive )
	{
		float flTimeSinceStart = gpGlobals->curtime - s_flClientChargeStartTime;
		
		// Keep clamping for the latency-adjusted duration
		if ( flTimeSinceStart < s_flPredictiveClampDuration )
		{
			s_nLastClampFrame = currFrame;
		s_bClampCached = true;
		return true;
		}
		else
		{
			// Timeout - server didn't confirm charge, stop client-side clamping
			s_bClientChargeActive = false;
			if ( tf_charge_turn_debug_client.GetBool() )
			{
				DevMsg("[CLIENT IMMEDIATE] Charge clamping timeout - stopping client prediction\n");
			}
		}
	}
	
	s_nLastClampFrame = currFrame;
		s_bClampCached = false;
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: Predictive turn-rate cap used before the server confirms the charge
//-----------------------------------------------------------------------------
static float PredictiveCapChargeTurnRate( CTFPlayer *pPlayer, float flYawDelta )
{
	// Use the same base convar as the real cap function
	float flBaseCap = tf_charge_turn_rate.GetFloat();

	// NOTE: We purposely skip the attribute hook here; it will be applied after
	// the server confirms the charge.  This keeps the client code simple while
	// still preventing players from over-turning in the early frames.
	float flMaxYawDelta = flBaseCap * gpGlobals->frametime / TICK_INTERVAL;

	if ( tf_charge_turn_debug_client.GetBool() )
	{
		DevMsg("[CLIENT PREDICT] yaw delta %.2f, max allowed %.2f (cap %.2f, frametime %.4f)\n",
			flYawDelta, flMaxYawDelta, flBaseCap, gpGlobals->frametime );
	}

	if ( flYawDelta > flMaxYawDelta )
		return flMaxYawDelta;
	else if ( flYawDelta < -flMaxYawDelta )
		return -flMaxYawDelta;

	return flYawDelta;
}

//-----------------------------------------------------------------------------
// Purpose: Cap yaw movement
//-----------------------------------------------------------------------------
float CTFInput::CAM_CapYaw( float fVal ) const
{
	CTFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	
	if ( ShouldApplyChargeClamping( pPlayer ) )
	{
		float flClamped;
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			flClamped = pPlayer->m_Shared.CapChargeTurnRate( fVal );
		}
		else
		{
			flClamped = PredictiveCapChargeTurnRate( pPlayer, fVal );
		}
		if ( tf_charge_turn_debug_client.GetBool() && fabs(flClamped - fVal) > 0.01f )
		{
			DevMsg("[CAM_CapYaw] Clamped %.3f -> %.3f\n", fVal, flClamped);
		}
		return flClamped;
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
		
		if ( ShouldApplyChargeClamping( pPlayer ) )
		{
			if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
			{
				yaw_right = pPlayer->m_Shared.CapChargeTurnRate( yaw_right );
				yaw_left  = pPlayer->m_Shared.CapChargeTurnRate( yaw_left );
			}
			else
			{
				yaw_right = PredictiveCapChargeTurnRate( pPlayer, yaw_right );
				yaw_left  = PredictiveCapChargeTurnRate( pPlayer, yaw_left );
			}
		}
		
		// Apply keyboard turn
		viewangles[YAW] -= yaw_right;
		viewangles[YAW] += yaw_left;
		
		// Calculate how much we've turned from keyboard
		totalYawChange = AngleDiff( viewangles[YAW], prevViewangles[YAW] );
	}
	
	// For mouse movement, the engine has already applied the changes to viewangles
	// We need to check if the mouse change + keyboard change exceeds the cap
	if ( ShouldApplyChargeClamping( pPlayer ) )
	{
		// Calculate the total angle change from previous frame
		float mouseYawChange = AngleDiff(viewangles[YAW], prevViewangles[YAW]) - totalYawChange;
		
		float cappedMouseYaw;
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			cappedMouseYaw = pPlayer->m_Shared.CapChargeTurnRate( mouseYawChange );
		}
		else
		{
			cappedMouseYaw = PredictiveCapChargeTurnRate( pPlayer, mouseYawChange );
		}
		
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
		if ( ShouldApplyChargeClamping( pPlayer ) )
		{
			if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
			{
				flSpeed = pPlayer->m_Shared.CapChargeTurnRate( flSpeed );
			}
			else
			{
				flSpeed = PredictiveCapChargeTurnRate( pPlayer, flSpeed );
			}
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