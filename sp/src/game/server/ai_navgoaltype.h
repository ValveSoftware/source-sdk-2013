//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_NAVGOALTYPE_H
#define AI_NAVGOALTYPE_H

#if defined( _WIN32 )
#pragma once
#endif

// =======================================
//  Movement goals 
//		Used both to store the current
//		movment goal in m_routeGoalType
//		and to or/and with route
// =======================================
enum GoalType_t 
{
	GOALTYPE_NONE,
	GOALTYPE_TARGETENT,
	GOALTYPE_ENEMY,
	GOALTYPE_PATHCORNER,
	GOALTYPE_LOCATION,
	GOALTYPE_LOCATION_NEAREST_NODE,
	GOALTYPE_FLANK,
	GOALTYPE_COVER,
	
	GOALTYPE_INVALID
};

#endif // AI_NAVGOALTYPE_H
