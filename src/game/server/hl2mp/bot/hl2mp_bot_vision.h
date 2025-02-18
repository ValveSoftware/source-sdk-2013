//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_VISION_H
#define HL2MP_BOT_VISION_H

#include "NextBotVisionInterface.h"

//----------------------------------------------------------------------------
class CHL2MPBotVision : public IVision
{
public:
	CHL2MPBotVision( INextBot *bot ) : IVision( bot )
	{
	}

	virtual ~CHL2MPBotVision() { }

	/**
	 * Populate "potentiallyVisible" with the set of all entities we could potentially see. 
	 * Entities in this set will be tested for visibility/recognition in IVision::Update()
	 */
	virtual void CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity * > *potentiallyVisible );

	virtual bool IsIgnored( CBaseEntity *subject ) const;		// return true to completely ignore this entity (may not be in sight when this is called)

	virtual float GetMaxVisionRange( void ) const;				// return maximum distance vision can reach
	virtual float GetMinRecognizeTime( void ) const;			// return VISUAL reaction time

private:
	CUtlVector< CHandle< CBaseCombatCharacter > > m_potentiallyVisibleNPCVector;
	CountdownTimer m_potentiallyVisibleUpdateTimer;
	void UpdatePotentiallyVisibleNPCVector( void );

	CountdownTimer m_scanTimer;
};


#endif // HL2MP_BOT_VISION_H
