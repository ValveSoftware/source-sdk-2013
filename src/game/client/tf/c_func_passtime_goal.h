//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// func_passtime_goal - based on func_capture_zone
#ifndef C_FUNC_PASSTIME_GOAL_H
#define C_FUNC_PASSTIME_GOAL_H
#ifdef _WIN32
#pragma once
#endif

#include "util_shared.h"
#include "c_baseentity.h"

//-----------------------------------------------------------------------------
class C_FuncPasstimeGoal : public C_BaseEntity, public TAutoList<C_FuncPasstimeGoal>
{
public:
	DECLARE_CLASS( C_FuncPasstimeGoal, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	bool BGoalTriggerDisabled() const { return m_bTriggerDisabled; }
	int GetGoalType() const { return m_iGoalType;  }

	enum GoalType
	{
		TYPE_HOOP,
		TYPE_ENDZONE,
		TYPE_TOWER,
	};

private:
	CNetworkVar( bool, m_bTriggerDisabled );
	CNetworkVar( int, m_iGoalType );
};

#endif // C_FUNC_PASSTIME_GOAL_H  
