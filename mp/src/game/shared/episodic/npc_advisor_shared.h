//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared data between client and server side npc_advisor classes.
//
// Catchphrase: "It's advising us!!!"
//
//=============================================================================//

#ifndef NPC_ADVISOR_SHARED_H
#define NPC_ADVISOR_SHARED_H
#ifdef _WIN32
#pragma once
#endif


// Set this to 0 to disable the advisor's special AI behavior (all that object chucking), 
// which we did in Ep2 to make him a scripted creature.
#define NPC_ADVISOR_HAS_BEHAVIOR 0

#if NPC_ADVISOR_HAS_BEHAVIOR
// Message ID constants used for communciation between client and server.
enum 
{
	ADVISOR_MSG_START_BEAM = 10,
	ADVISOR_MSG_STOP_BEAM,
	ADVISOR_MSG_STOP_ALL_BEAMS,
	ADVISOR_MSG_START_ELIGHT,
	ADVISOR_MSG_STOP_ELIGHT,
};
#endif

#endif // NPC_ADVISOR_SHARED_H
