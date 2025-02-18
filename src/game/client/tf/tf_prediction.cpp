//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "prediction.h"
#include "c_baseplayer.h"
#include "igamemovement.h"
#include "c_tf_player.h"


static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;


class CTFPrediction : public CPrediction
{
DECLARE_CLASS( CTFPrediction, CPrediction );

public:
	virtual void	SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPrediction::SetupMove( C_BasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, 
	CMoveData *move )
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( player );
	if ( pTFPlayer )
	{
		// Check to see if we are a crouched, heavy, firing his weapons and zero out movement.
		if ( pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			if ( ( pTFPlayer->GetFlags() & FL_DUCKING ) && ( pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) ) )
			{
				ucmd->forwardmove = 0.0f;
				ucmd->sidemove = 0.0f;
			}
		}
	}

	// Call the default SetupMove code.
	BaseClass::SetupMove( player, ucmd, pHelper, move );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPrediction::FinishMove( C_BasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );
}


// Expose interface to engine
// Expose interface to engine
static CTFPrediction g_Prediction;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CTFPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction );

CPrediction *prediction = &g_Prediction;

