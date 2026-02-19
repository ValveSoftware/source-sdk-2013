//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player_command.h"
#include "igamemovement.h"
#include "in_buttons.h"
#include "ipredictionsystem.h"
#include "tf_player.h"

extern ConVar tf_charge_turn_debug;

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;


//-----------------------------------------------------------------------------
// Sets up the move data for TF2
//-----------------------------------------------------------------------------
class CTFPlayerMove : public CPlayerMove
{
DECLARE_CLASS( CTFPlayerMove, CPlayerMove );

public:
	virtual void	StartCommand( CBasePlayer *player, CUserCmd *cmd );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );
};

// PlayerMove Interface
static CTFPlayerMove g_PlayerMove;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}

//-----------------------------------------------------------------------------
// Main setup, finish
//-----------------------------------------------------------------------------

void CTFPlayerMove::StartCommand( CBasePlayer *player, CUserCmd *cmd )
{
	BaseClass::StartCommand( player, cmd );
}

// Define a tolerance factor (e.g., 1.25 means a 25% buffer before clamping kicks in).
ConVar tf_charge_turn_tolerance( "tf_charge_turn_tolerance", "1.25", FCVAR_REPLICATED | FCVAR_CHEAT, "Tolerance factor for charge turn clamping.", true, 1.0f, true, 2.0f );

//-----------------------------------------------------------------------------
// Purpose: This is called pre player movement and copies all the data necessary
//          from the player for movement. (Server-side, the client-side version
//          of this code can be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( player );
	if ( pTFPlayer )
	{
		// Check to see if we are a crouched, heavy, firing his weapons and zero out movement.
		if ( pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			if ( pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) )
			{
				if ( pTFPlayer->GetFlags() & FL_DUCKING )
				{
					ucmd->forwardmove = 0.0f;
					ucmd->sidemove = 0.0f;
				}

				// Don't allow jumping while firing (unless the design changes)
				ucmd->buttons &= ~IN_JUMP;
			}
		}

		// Server-side charge turn capping with proper lag compensation
		if ( pTFPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			// Cache the original (unmodified) view angles for history tracking
			QAngle qOriginalViewAngles = ucmd->viewangles;
			// Calculate yaw delta between current raw view and the previous *raw* charge view angle.
			float flYawDelta = AngleDiff( qOriginalViewAngles[YAW], pTFPlayer->m_qPreviousChargeEyeAngle[YAW] );
			
			// Calculate the actual time difference between user commands for proper lag compensation
			float flTimeDelta = TICK_INTERVAL;
			int nCommandDiff = 1;
			if ( pTFPlayer->m_nPreviousChargeCommandNumber > 0 )
			{
				nCommandDiff = ucmd->command_number - pTFPlayer->m_nPreviousChargeCommandNumber;
				if ( nCommandDiff > 0 && nCommandDiff < 64 ) // Sanity check to prevent overflow issues
				{
					flTimeDelta = nCommandDiff * TICK_INTERVAL;
				}
				else
				{
					nCommandDiff = 1; // Fallback to single tick
				}
			}
			
			if ( tf_charge_turn_debug.GetInt() >= 1 )
			{
				DevMsg("[PLAYERMOVE] Player charging: cmd %d->%d (diff %d), yaw %.2f->%.2f (delta %.2f), timeDelta %.4f\n",
					pTFPlayer->m_nPreviousChargeCommandNumber, ucmd->command_number, nCommandDiff,
					pTFPlayer->m_qPreviousChargeEyeAngle[YAW], ucmd->viewangles[YAW], flYawDelta, flTimeDelta);
			}
			
			// Clamp the yaw change using the unified function with proper time delta.
			float flCappedYawDelta = pTFPlayer->m_Shared.CapChargeTurnRate( flYawDelta, flTimeDelta );

			// Only clamp if the yaw difference exceeds the cap by more than the tolerance.
			if ( fabs(flYawDelta) > tf_charge_turn_tolerance.GetFloat() * fabs(flCappedYawDelta) )
			{
				if ( tf_charge_turn_debug.GetInt() >= 1 )
				{
					DevMsg("[PLAYERMOVE] APPLYING SERVER CLAMP: yaw %.2f -> %.2f (tolerance %.2f exceeded)\n",
						ucmd->viewangles[YAW], pTFPlayer->m_qPreviousChargeEyeAngle[YAW] + flCappedYawDelta, tf_charge_turn_tolerance.GetFloat());
				}
				// Adjust the view angle based on the capped delta.
				ucmd->viewangles[YAW] = pTFPlayer->m_qPreviousChargeEyeAngle[YAW] + flCappedYawDelta;
				pTFPlayer->SnapEyeAngles( ucmd->viewangles );
			}
			else if ( tf_charge_turn_debug.GetInt() >= 2 )
			{
				DevMsg("[PLAYERMOVE] No server clamp needed: delta %.2f within tolerance of capped %.2f\n", flYawDelta, flCappedYawDelta);
			}
			
			// Store the **final applied** eye angle (post-clamp) so that next tick's
			// comparison reflects what the player actually saw on the server. This
			// avoids small opposite-direction deltas that appear when we kept the
			// unclamped value.
			pTFPlayer->m_qPreviousChargeEyeAngle = ucmd->viewangles;
			pTFPlayer->m_nPreviousChargeCommandNumber = ucmd->command_number;
		}
		else
		{
			pTFPlayer->m_qPreviousChargeEyeAngle = pTFPlayer->EyeAngles();
			pTFPlayer->m_nPreviousChargeCommandNumber = 0; // Reset for next charge
		}
	}

	BaseClass::SetupMove( player, ucmd, pHelper, move );
}


//-----------------------------------------------------------------------------
// Purpose: This is called post player movement to copy back all data that
//          movement could have modified and that is necessary for future
//          movement. (Server-side, the client-side version of this code can 
//          be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );
}
