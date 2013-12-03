//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL_MOVEDATA_H
#define HL_MOVEDATA_H
#ifdef _WIN32
#pragma once
#endif


#include "igamemovement.h"


// This class contains HL2-specific prediction data.
class CHLMoveData : public CMoveData
{
public:
	bool		m_bIsSprinting;
};

class CFuncLadder;
class CReservePlayerSpot;

//-----------------------------------------------------------------------------
// Purpose: Data related to automatic mounting/dismounting from ladders
//-----------------------------------------------------------------------------
struct LadderMove_t
{
	DECLARE_SIMPLE_DATADESC();

	//	Are we forcing player movement during mount/dismount
	bool		m_bForceLadderMove;
	// Is the forced move getting on or off the ladder
	bool		m_bForceMount;
	
	// Simulation info for forcing the player move
	float		m_flStartTime;
	float		m_flArrivalTime;
	Vector		m_vecGoalPosition;
	Vector		m_vecStartPosition;

	// The ladder entity owning the forced move (for marking us "on" the ladder after automounting it)
	CHandle< CFuncLadder > m_hForceLadder;
	CHandle< CReservePlayerSpot > m_hReservedSpot;
};

#endif // HL_MOVEDATA_H
