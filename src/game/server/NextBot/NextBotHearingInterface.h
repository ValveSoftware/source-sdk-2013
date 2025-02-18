// NextBotHearingInterface.h
// Interface for auditory queries of a bot
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_HEARING_INTERFACE_H_
#define _NEXT_BOT_HEARING_INTERFACE_H_

#include "NextBotComponentInterface.h"

//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for hearing sounds
 */
class IHearing : public INextBotComponent
{
public:
	IHearing( INextBot *bot ) : INextBotComponent( bot ) { }
	virtual ~IHearing() { }

	virtual void Reset( void );									// reset to initial state
	virtual void Update( void );								// update internal state

	virtual float GetTimeSinceHeard( int team ) const;			// return time since we heard any member of the given team

	virtual CBaseEntity *GetClosestRecognized( int team = TEAM_ANY ) const;	// return the closest recognized entity
	virtual int GetRecognizedCount( int team, float rangeLimit = -1.0f ) const;	// return the number of actors on the given team visible to us closer than rangeLimit

	virtual float GetMaxHearingRange( void ) const;				// return maximum distance we can hear
	virtual float GetMinRecognizeTime( void ) const;			// return HEARING reaction time
};


#endif // _NEXT_BOT_HEARING_INTERFACE_H_
