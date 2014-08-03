//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "player_command.h"
#include "player.h"
#include "igamemovement.h"
#include "hl_movedata.h"
#include "ipredictionsystem.h"
#include "iservervehicle.h"
#include "hl2_player.h"
#include "vehicle_base.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CHLPlayerMove : public CPlayerMove
{
	DECLARE_CLASS( CHLPlayerMove, CPlayerMove );
public:
	CHLPlayerMove() :
		m_bWasInVehicle( false ),
		m_bVehicleFlipped( false ),
		m_bInGodMode( false ),
		m_bInNoClip( false )
	{
		m_vecSaveOrigin.Init();
	}

	void SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	void FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );

private:
	Vector m_vecSaveOrigin;
	bool m_bWasInVehicle;
	bool m_bVehicleFlipped;
	bool m_bInGodMode;
	bool m_bInNoClip;
};

//
//
// PlayerMove Interface
static CHLPlayerMove g_PlayerMove;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}

//

static CHLMoveData g_HLMoveData;
CMoveData *g_pMoveData = &g_HLMoveData;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;

void CHLPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// Call the default SetupMove code.
	BaseClass::SetupMove( player, ucmd, pHelper, move );

	// Convert to HL2 data.
	CHL2_Player *pHLPlayer = static_cast<CHL2_Player*>( player );
	Assert( pHLPlayer );

	CHLMoveData *pHLMove = static_cast<CHLMoveData*>( move );
	Assert( pHLMove );

	player->m_flForwardMove = ucmd->forwardmove;
	player->m_flSideMove = ucmd->sidemove;

	pHLMove->m_bIsSprinting = pHLPlayer->IsSprinting();

	if ( gpGlobals->frametime != 0 )
	{
		IServerVehicle *pVehicle = player->GetVehicle();

		if ( pVehicle )
		{
			pVehicle->SetupMove( player, ucmd, pHelper, move ); 

			if ( !m_bWasInVehicle )
			{
				m_bWasInVehicle = true;
				m_vecSaveOrigin.Init();
			}
		}
		else
		{
			m_vecSaveOrigin = player->GetAbsOrigin();
			if ( m_bWasInVehicle )
			{
				m_bWasInVehicle = false;
			}
		}
	}
}


void CHLPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );
	if ( gpGlobals->frametime != 0 )
	{		
		float distance = 0.0f;
		IServerVehicle *pVehicle = player->GetVehicle();
		if ( pVehicle )
		{
			pVehicle->FinishMove( player, ucmd, move );
			IPhysicsObject *obj = player->GetVehicleEntity()->VPhysicsGetObject();
			if ( obj )
			{
				Vector newPos;
				obj->GetPosition( &newPos, NULL );
				distance = VectorLength( newPos - m_vecSaveOrigin );
				if ( m_vecSaveOrigin == vec3_origin || distance > 100.0f )
					distance = 0.0f;
				m_vecSaveOrigin = newPos;
			}
			
			CPropVehicleDriveable *driveable = dynamic_cast< CPropVehicleDriveable * >( player->GetVehicleEntity() );
			if ( driveable )
			{
				// Overturned and at rest (if still moving it can fix itself)
				bool bFlipped = driveable->IsOverturned() && ( distance < 0.5f );
				if ( m_bVehicleFlipped != bFlipped )
				{
					if ( bFlipped )
					{
						gamestats->Event_FlippedVehicle( player, driveable );
					}
					m_bVehicleFlipped = bFlipped;
				}
			}
			else
			{
				m_bVehicleFlipped = false;
			}
		}
		else
		{
			m_bVehicleFlipped = false;
			distance = VectorLength( player->GetAbsOrigin() - m_vecSaveOrigin );
		}
		if ( distance > 0 )
		{
			gamestats->Event_PlayerTraveled( player, distance, pVehicle ? true : false, !pVehicle && static_cast< CHL2_Player * >( player )->IsSprinting() );
		}
	}

	bool bGodMode = ( player->GetFlags() & FL_GODMODE ) ? true : false;
	if ( m_bInGodMode != bGodMode )
	{
		m_bInGodMode = bGodMode;
		if ( bGodMode )
		{
			gamestats->Event_PlayerEnteredGodMode( player );
		}
	}
	bool bNoClip = ( player->GetMoveType() == MOVETYPE_NOCLIP );
	if ( m_bInNoClip != bNoClip )
	{
		m_bInNoClip = bNoClip;
		if ( bNoClip )
		{
			gamestats->Event_PlayerEnteredNoClip( player );
		}
	}
}
