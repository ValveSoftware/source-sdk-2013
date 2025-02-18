//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// func_passtime_goal - based on func_capture_zone
#include "cbase.h"
#include "c_func_passtime_goal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_FuncPasstimeGoal, DT_FuncPasstimeGoal, CFuncPasstimeGoal )
	RecvPropBool( RECVINFO( m_bTriggerDisabled ) ),
	RecvPropInt( RECVINFO( m_iGoalType ) ),
END_RECV_TABLE()
