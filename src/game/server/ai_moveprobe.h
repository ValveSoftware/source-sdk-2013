//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_MOVEPROBE_H
#define AI_MOVEPROBE_H

#include "ai_component.h"
#include "ai_navtype.h"
#include "ai_movetypes.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: Set of basic tools for probing box movements through space.
//			No moves actually take place
//-----------------------------------------------------------------------------

enum AI_TestGroundMoveFlags_t
{
	AITGM_DEFAULT					= 0,
	AITGM_IGNORE_FLOOR				= 0x01,
	AITGM_IGNORE_INITIAL_STAND_POS	= 0x02,
	AITGM_2D						= 0x04,
	AITGM_DRAW_RESULTS				= 0x08,
};

enum AI_MoveLimitFlags_t
{
	AIMLF_DEFAULT	= 0,
	AIMLF_2D		= 0x01,
	AIMLF_DRAW_RESULTS = 0x02,
	AIMLF_IGNORE_TRANSIENTS = 0x04,
	AIMLF_QUICK_REJECT = 0x08,
};

class CAI_MoveProbe : public CAI_Component
{
public:

	CAI_MoveProbe( CAI_BaseNPC *pOuter );
	~CAI_MoveProbe();
	
	// ----------------------------------------------------
	// Queries & probes
	// ----------------------------------------------------
	bool				MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMove = NULL );
	bool				MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, float pctToCheckStandPositions, AIMoveTrace_t* pMove = NULL );
	bool				MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, float pctToCheckStandPositions, unsigned flags, AIMoveTrace_t* pMove = NULL );
	
	bool				CheckStandPosition( const Vector &vecStart, unsigned int collisionMask ) const;
	bool				FloorPoint( const Vector &vecStart, unsigned int collisionMask, float flStartZ, float flEndZ, Vector *pVecResult ) const;

	// --------------------------------
	// Tracing tools
	// --------------------------------
	void				TraceLine( const Vector &vecStart, const Vector &vecEnd, unsigned int mask, 
								   bool bUseCollisionGroup, trace_t *pResult ) const;

	void 				TraceHull( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
								   const Vector &hullMax, unsigned int mask, 
								   trace_t *ptr ) const;
	
	void 				TraceHull( const Vector &vecStart, const Vector &vecEnd, unsigned int mask, 
								   trace_t *ptr ) const;

	// --------------------------------
	// Checks a ground-based movement
	// --------------------------------
	bool				TestGroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, 
										unsigned int collisionMask, unsigned flags, AIMoveTrace_t *pMoveTrace ) const;

	bool				TestGroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, 
										unsigned int collisionMask, float pctToCheckStandPositions, unsigned flags, AIMoveTrace_t *pMoveTrace ) const;

	bool				ShouldBrushBeIgnored( CBaseEntity *pEntity );

	void				ClearBlockingEntity()	{ m_hLastBlockingEnt = NULL; }
	CBaseEntity *		GetBlockingEntity()	{ return m_hLastBlockingEnt; }

private:
	struct CheckStepArgs_t
	{
		Vector				vecStart;
		Vector				vecStepDir;
		float				stepSize;
		float				stepHeight;
		float				stepDownMultiplier;
		float				minStepLanding;
		unsigned			collisionMask;
		StepGroundTest_t	groundTest;
	};

	struct CheckStepResult_t
	{
		Vector			endPoint;
		Vector			hitNormal;
		bool			fStartSolid;
		CBaseEntity *	pBlocker;
	};

	
	bool				CheckStep( const CheckStepArgs_t &args, CheckStepResult_t *pResult ) const;
	void				SetupCheckStepTraceListData( const CheckStepArgs_t &args ) const;
	void				ResetTraceListData() const	{ if ( m_pTraceListData ) const_cast<CAI_MoveProbe *>(this)->m_pTraceListData->Reset(); }
	bool				OldCheckStandPosition( const Vector &vecStart, unsigned int collisionMask ) const;

	// these check connections between positions in space, regardless of routes
	void				GroundMoveLimit( const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, unsigned testGroundMoveFlags, float pctToCheckStandPositions, AIMoveTrace_t* pMoveTrace ) const;
	void				FlyMoveLimit( const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMoveTrace) const;
	void				JumpMoveLimit( const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMoveTrace) const;
	void				ClimbMoveLimit( const Vector &vecStart, const Vector &vecEnd, const CBaseEntity *pTarget, AIMoveTrace_t* pMoveTrace) const;

	// A floorPoint that is useful only in the contect of iterative movement
	bool				IterativeFloorPoint( const Vector &vecStart, unsigned int collisionMask, Vector *pVecResult ) const;
	bool				IterativeFloorPoint( const Vector &vecStart, unsigned int collisionMask, float flAddedStep, Vector *pVecResult ) const;
	bool 				IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;

public:
	Vector 				CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float gravity, float *pminHeight, float maxHorzVelocity, Vector *vecApex ) const;
private:

	// Common services provided by CAI_BaseNPC, Convenience methods to simplify code
	float				StepHeight() const;
	bool				CanStandOn( CBaseEntity *pSurface ) const;

	bool				m_bIgnoreTransientEntities;

	CTraceListData *	m_pTraceListData;

	EHANDLE				m_hLastBlockingEnt;

	DECLARE_SIMPLE_DATADESC();
};

// ----------------------------------------------------------------------------

inline bool CAI_MoveProbe::MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, float pctToCheckStandPositions, AIMoveTrace_t* pMove)
{
	return MoveLimit( navType, vecStart, vecEnd, collisionMask, pTarget, pctToCheckStandPositions, AIMLF_DEFAULT, pMove);
}

// ------------------------------------

inline bool CAI_MoveProbe::MoveLimit( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, AIMoveTrace_t* pMove)
{
	return MoveLimit( navType, vecStart, vecEnd, collisionMask, pTarget, 100.0f, AIMLF_DEFAULT, pMove);
}

// ------------------------------------

inline bool CAI_MoveProbe::TestGroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, unsigned int collisionMask, unsigned flags, AIMoveTrace_t *pMoveTrace ) const
{
	return TestGroundMove( vecActualStart, vecDesiredEnd, collisionMask, 100, flags, pMoveTrace ); // floor ignore flag will override 100%
}

#endif // AI_MOVEPROBE_H
