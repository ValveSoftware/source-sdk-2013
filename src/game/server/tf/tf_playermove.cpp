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

		// targe Exploit fix. Clients sending higher view angle changes then allowed
		// Clamp their YAW Movement
		if ( pTFPlayer->m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		{
			// Get the view deltas and clamp them if they are too high, give a high tolerance (lag)
			float flCap = pTFPlayer->m_Shared.CalculateChargeCap();
			flCap *= 2.5f;
			QAngle qAngle = pTFPlayer->m_qPreviousChargeEyeAngle;
			float flDiff = abs( qAngle[YAW] ) - abs( ucmd->viewangles[YAW] );
			if ( flDiff > flCap )
			{
				//float flReportedPitchDelta = qAngle[YAW] - ucmd->viewangles[YAW];
				if ( ucmd->viewangles[YAW] > qAngle[YAW] )
				{
					ucmd->viewangles[YAW] = qAngle[YAW] + flCap;
					pTFPlayer->SnapEyeAngles( ucmd->viewangles );
				}
				else // smaller values
				{
					ucmd->viewangles[YAW] = qAngle[YAW] - flCap;
					pTFPlayer->SnapEyeAngles( ucmd->viewangles );
				}
			}
			
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
