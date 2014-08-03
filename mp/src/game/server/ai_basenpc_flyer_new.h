//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_BASENPC_FLYER_NEW_H
#define AI_BASENPC_FLYER_NEW_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_condition.h"


enum BaseNPCFlyerConditions_t 
{
	COND_FLYER_MOVE_BLOCKED = LAST_SHARED_CONDITION,
	COND_FLYER_MOVE_IMPOSSIBLE,

	// ======================================
	// IMPORTANT: This must be the last enum
	// ======================================
	LAST_FLYER_SHARED_CONDITION
};


//-----------------------------------------------------------------------------
// The combot.
//-----------------------------------------------------------------------------
class CAI_BaseNPCFlyerNew : public CAI_BaseNPC
{
	DECLARE_CLASS( CAI_BaseNPCFlyerNew, CAI_BaseNPC );
public:
//	DEFINE_CUSTOM_AI;

	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );

	virtual float	GetIdealSpeed( ) const;
	virtual float	MinGroundDist(void);

	CAI_BaseNPCFlyerNew();

protected:
	// Call this to set up a flyer
	void SpawnFlyer();

	// Yarg! Must be chained down from leaf classes...
	void ClearFlyerConditions(void);

	// Override this when we had to abort movement
	virtual void AbortedMovement( void ) {}
};

#endif // AI_BASENPC_FLYER_NEW_H
