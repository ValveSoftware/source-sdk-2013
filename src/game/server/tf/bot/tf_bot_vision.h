//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_vision.h
// Team Fortress NextBot vision interface
// Michael Booth, May 2009

#ifndef TF_BOT_VISION_H
#define TF_BOT_VISION_H

#include "NextBotVisionInterface.h"

//----------------------------------------------------------------------------
class CTFBotVision : public IVision
{
public:
	CTFBotVision( INextBot *bot ) : IVision( bot )
	{
	}

	virtual ~CTFBotVision() { }

	virtual void Update( void );								// update internal state

	/**
	 * Populate "potentiallyVisible" with the set of all entities we could potentially see. 
	 * Entities in this set will be tested for visibility/recognition in IVision::Update()
	 */
	virtual void CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity * > *potentiallyVisible );

	virtual bool IsIgnored( CBaseEntity *subject ) const;		// return true to completely ignore this entity (may not be in sight when this is called)
	virtual bool IsVisibleEntityNoticed( CBaseEntity *subject ) const;		// return true if we 'notice' the subject, even though we have LOS to it

	virtual float GetMaxVisionRange( void ) const;				// return maximum distance vision can reach
	virtual float GetMinRecognizeTime( void ) const;			// return VISUAL reaction time

private:
	CUtlVector< CHandle< CBaseCombatCharacter > > m_potentiallyVisibleNPCVector;
	CountdownTimer m_potentiallyVisibleUpdateTimer;
	void UpdatePotentiallyVisibleNPCVector( void );

	CountdownTimer m_scanTimer;
};


#endif // TF_BOT_VISION_H
