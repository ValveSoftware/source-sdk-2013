//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_BASEHUMANOID_H
#define AI_BASEHUMANOID_H

#include "ai_behavior.h"
#include "ai_blended_movement.h"

//-----------------------------------------------------------------------------
// CLASS: CAI_BaseHumanoid
//-----------------------------------------------------------------------------

typedef CAI_BlendingHost< CAI_BehaviorHost<CAI_BaseNPC> > CAI_BaseHumanoidBase;

class CAI_BaseHumanoid : public CAI_BaseHumanoidBase
{
	DECLARE_CLASS( CAI_BaseHumanoid, CAI_BaseHumanoidBase );

public:
	bool HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);

	// Tasks
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );
	virtual void BuildScheduleTestBits( );

	// Navigation
	bool OnMoveBlocked( AIMoveResult_t *pResult );

	// Damage
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	// Various start tasks
	virtual	void StartTaskRangeAttack1( const Task_t *pTask );

	// Various run tasks
	virtual void RunTaskRangeAttack1( const Task_t *pTask );

	// Purpose: check ammo
	virtual void CheckAmmo( void );
};

//-----------------------------------------------------------------------------

#endif
