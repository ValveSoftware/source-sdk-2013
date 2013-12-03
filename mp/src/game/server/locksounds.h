//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a structure common to buttons and doors for playing locked
//			and unlocked sounds.
//
// $NoKeywords: $
//=============================================================================//

#ifndef LOCKSOUNDS_H
#define LOCKSOUNDS_H
#ifdef _WIN32
#pragma once
#endif

#include "datamap.h"


struct locksound_t					// sounds that doors and buttons make when locked/unlocked
{
	DECLARE_DATADESC();

	string_t	sLockedSound;		// sound a door makes when it's locked
	string_t	sLockedSentence;	// sentence group played when door is locked
	string_t	sUnlockedSound;		// sound a door makes when it's unlocked
	string_t	sUnlockedSentence;	// sentence group played when door is unlocked

	int		iLockedSentence;		// which sentence in sentence group to play next
	int		iUnlockedSentence;		// which sentence in sentence group to play next

	float	flwaitSound;			// time delay between playing consecutive 'locked/unlocked' sounds
	float	flwaitSentence;			// time delay between playing consecutive sentences
	byte	bEOFLocked;				// true if hit end of list of locked sentences
	byte	bEOFUnlocked;			// true if hit end of list of unlocked sentences
};


#endif // LOCKSOUNDS_H
