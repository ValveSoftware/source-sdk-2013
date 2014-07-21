//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPCEVENT_H
#define NPCEVENT_H
#ifdef _WIN32
#pragma once
#endif

class CBaseAnimating;

struct animevent_t
{
	int				event;
	const char		*options;
	float			cycle;
	float			eventtime;
	int				type;
	CBaseAnimating	*pSource;
};
#define EVENT_SPECIFIC			0
#define EVENT_SCRIPTED			1000		// see scriptevent.h
#define EVENT_SHARED			2000
#define EVENT_WEAPON			3000
#define EVENT_CLIENT			5000

#define NPC_EVENT_BODYDROP_LIGHT	2001
#define NPC_EVENT_BODYDROP_HEAVY	2002

#define NPC_EVENT_SWISHSOUND		2010

#define NPC_EVENT_180TURN			2020

#define NPC_EVENT_ITEM_PICKUP					2040
#define NPC_EVENT_WEAPON_DROP					2041
#define NPC_EVENT_WEAPON_SET_SEQUENCE_NAME		2042
#define NPC_EVENT_WEAPON_SET_SEQUENCE_NUMBER	2043
#define NPC_EVENT_WEAPON_SET_ACTIVITY			2044

#define	NPC_EVENT_LEFTFOOT			2050
#define NPC_EVENT_RIGHTFOOT			2051

#define NPC_EVENT_OPEN_DOOR			2060

// !! DON'T CHANGE TO ORDER OF THESE.  THEY ARE HARD CODED IN THE WEAPON QC FILES (YUCK!) !!
#define EVENT_WEAPON_MELEE_HIT			3001
#define EVENT_WEAPON_SMG1				3002
#define EVENT_WEAPON_MELEE_SWISH		3003
#define EVENT_WEAPON_SHOTGUN_FIRE		3004
#define EVENT_WEAPON_THROW				3005
#define EVENT_WEAPON_AR1				3006
#define EVENT_WEAPON_AR2				3007
#define EVENT_WEAPON_HMG1				3008
#define EVENT_WEAPON_SMG2				3009
#define EVENT_WEAPON_MISSILE_FIRE		3010
#define EVENT_WEAPON_SNIPER_RIFLE_FIRE	3011
#define EVENT_WEAPON_AR2_GRENADE		3012
#define EVENT_WEAPON_THROW2				3013
#define	EVENT_WEAPON_PISTOL_FIRE		3014
#define EVENT_WEAPON_RELOAD				3015
#define EVENT_WEAPON_THROW3				3016
#define EVENT_WEAPON_RELOAD_SOUND		3017		// Use this + EVENT_WEAPON_RELOAD_FILL_CLIP to prevent shooting during the reload animation 
#define EVENT_WEAPON_RELOAD_FILL_CLIP	3018
#define EVENT_WEAPON_SMG1_BURST1		3101		// first round in a 3-round burst
#define EVENT_WEAPON_SMG1_BURSTN		3102		// 2, 3 rounds
#define EVENT_WEAPON_AR2_ALTFIRE		3103

#define EVENT_WEAPON_SEQUENCE_FINISHED	3900

// NOTE: MUST BE THE LAST WEAPON EVENT -- ONLY WEAPON EVENTS BETWEEN EVENT_WEAPON AND THIS
#define EVENT_WEAPON_LAST				3999

#endif // NPCEVENT_H
