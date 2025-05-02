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

		// Server-side charge turn capping
		if ( pTFPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			// Calculate yaw delta between current view and the previous charge view angle.
			float flYawDelta = AngleDiff( ucmd->viewangles[YAW], pTFPlayer->m_qPreviousChargeEyeAngle[YAW] );
			
			// Clamp the yaw change using the unified function.
			float flCappedYawDelta = pTFPlayer->m_Shared.CapChargeTurnRate( flYawDelta );

			// Only clamp if the yaw difference exceeds the cap by more than the tolerance.
			if ( fabs(flYawDelta) > tf_charge_turn_tolerance.GetFloat() * fabs(flCappedYawDelta) )
			{
				// Adjust the view angle based on the capped delta.
				ucmd->viewangles[YAW] = pTFPlayer->m_qPreviousChargeEyeAngle[YAW] + flCappedYawDelta;
				pTFPlayer->SnapEyeAngles( ucmd->viewangles );
			}
			
			// Store the current view angle for the next tick.
			pTFPlayer->m_qPreviousChargeEyeAngle = ucmd->viewangles;
		}
		else
		{
			pTFPlayer->m_qPreviousChargeEyeAngle = pTFPlayer->EyeAngles();
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
