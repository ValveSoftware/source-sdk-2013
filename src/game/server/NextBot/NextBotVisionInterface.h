// NextBotVisionInterface.h
// Visual information query interface for bots
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_VISION_INTERFACE_H_
#define _NEXT_BOT_VISION_INTERFACE_H_

#include "NextBotComponentInterface.h"
#include "NextBotKnownEntity.h"

class IBody;
class INextBotEntityFilter;
class IKnownEntity;

inline HSCRIPT ToHScript(const CKnownEntity *pKnownEntity)
{
	if (!pKnownEntity)
		return NULL;
	
	// Return the underlying entity instead of the knowledge wrapper
	return ToHScript(pKnownEntity->GetEntity());
}

inline CKnownEntity *ToKnownEntity( HSCRIPT hScript )
{
	return NULL;
}

//----------------------------------------------------------------------------------------------------------------
/**
 * The interface for HOW the bot sees (near sighted? night vision? etc)
 */
class IVision : public INextBotComponent
{
public:
	IVision( INextBot *bot );
	virtual ~IVision() { }

	virtual void Reset( void );									// reset to initial state
	virtual void Update( void );								// update internal state

	//-- attention/short term memory interface follows ------------------------------------------

	//
	// WARNING: Do not keep CKnownEntity pointers returned by these methods, as they can be invalidated/freed 
	//

	/**
	 * Iterate each interesting entity we are aware of.
	 * If functor returns false, stop iterating and return false.
	 * NOTE: known.GetEntity() is guaranteed to be non-NULL
	 */
	class IForEachKnownEntity
	{
	public:
		virtual bool Inspect( const CKnownEntity &known ) = 0;
	};
	virtual bool ForEachKnownEntity( IForEachKnownEntity &func );

	virtual void CollectKnownEntities( CUtlVector< CKnownEntity > *knownVector );	// populate given vector with all currently known entities

	virtual const CKnownEntity *GetPrimaryKnownThreat( bool onlyVisibleThreats = false ) const;	// return the biggest threat to ourselves that we are aware of
	HSCRIPT ScriptGetPrimaryKnownThreat( bool onlyVisibleThreats = false ) const { return ToHScript( this->GetPrimaryKnownThreat( onlyVisibleThreats ) ); }
	
	virtual float GetTimeSinceVisible( int team ) const;				// return time since we saw any member of the given team

	virtual const CKnownEntity *GetClosestKnown( int team = TEAM_ANY ) const;	// return the closest known entity
	HSCRIPT ScriptGetClosestKnown( int team = TEAM_ANY ) const { return ToHScript( this->GetClosestKnown( team ) ); }
	
	virtual int GetKnownCount( int team, bool onlyVisible = false, float rangeLimit = -1.0f ) const;		// return the number of entities on the given team known to us closer than rangeLimit

	virtual const CKnownEntity *GetClosestKnown( const INextBotEntityFilter &filter ) const;	// return the closest known entity that passes the given filter

	virtual const CKnownEntity *GetKnown( const CBaseEntity *entity ) const;		// given an entity, return our known version of it (or NULL if we don't know of it)
	HSCRIPT ScriptGetKnown( HSCRIPT hEntity ) const { return ToHScript( this->GetKnown( ToEnt( hEntity ) ) ); }
	
	// Introduce a known entity into the system. Its position is assumed to be known
	// and will be updated, and it is assumed to not yet have been seen by us, allowing for learning
	// of known entities by being told about them, hearing them, etc.
	virtual void AddKnownEntity( CBaseEntity *entity );
	void ScriptAddKnownEntity( HSCRIPT hEntity ) { this->AddKnownEntity( ToEnt( hEntity ) ); }

	virtual void ForgetEntity( CBaseEntity *forgetMe );			// remove the given entity from our awareness (whether we know if it or not)
	void ScriptForgetEntity( HSCRIPT hEntity ) { this->ForgetEntity( ToEnt( hEntity ) ); }

	virtual void ForgetAllKnownEntities( void );

	//-- physical vision interface follows ------------------------------------------------------

	/**
	 * Populate "potentiallyVisible" with the set of all entities we could potentially see. 
	 * Entities in this set will be tested for visibility/recognition in IVision::Update()
	 */
	virtual void CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity * > *potentiallyVisible );

	virtual float GetMaxVisionRange( void ) const;				// return maximum distance vision can reach
	virtual float GetMinRecognizeTime( void ) const;			// return VISUAL reaction time

	/**
	 * IsAbleToSee() returns true if the viewer can ACTUALLY SEE the subject or position,
	 * taking into account blindness, smoke effects, invisibility, etc.
	 * If 'visibleSpot' is non-NULL, the highest priority spot on the subject that is visible is returned.
	 */ 
	enum FieldOfViewCheckType { USE_FOV, DISREGARD_FOV };
	virtual bool IsAbleToSee( CBaseEntity *subject, FieldOfViewCheckType checkFOV, Vector *visibleSpot = NULL ) const;
	
	virtual bool IsAbleToSee( const Vector &pos, FieldOfViewCheckType checkFOV ) const;
	bool ScriptIsAbleToSee( const Vector &pos, bool checkFOV ) const 
	{ 
		return this->IsAbleToSee( pos, checkFOV ? USE_FOV : DISREGARD_FOV ); 
	}
	
	virtual bool IsIgnored( CBaseEntity *subject ) const;		// return true to completely ignore this entity (may not be in sight when this is called)
	bool ScriptIsIgnored( HSCRIPT hEntity ) const { return this->IsIgnored( ToEnt( hEntity ) ); }
	
	virtual bool IsVisibleEntityNoticed( CBaseEntity *subject ) const;		// return true if we 'notice' the subject, even though we have LOS to it
	bool ScriptIsVisibleEntityNoticed( HSCRIPT hEntity ) const { return this->IsVisibleEntityNoticed( ToEnt( hEntity ) ); }
	/**
	 * Check if 'subject' is within the viewer's field of view
	 */
	virtual bool IsInFieldOfView( const Vector &pos ) const;
	bool ScriptIsInFieldOfView( const Vector &pos ) const { return this->IsInFieldOfView( pos ); }
	
	virtual bool IsInFieldOfView( CBaseEntity *subject ) const;
	bool ScriptIsEntityInFieldOfView( HSCRIPT hEntity ) const { return this->IsInFieldOfView( ToEnt( hEntity ) ); }
	
	virtual float GetDefaultFieldOfView( void ) const;			// return default FOV in degrees
	virtual float GetFieldOfView( void ) const;					// return FOV in degrees
	virtual void SetFieldOfView( float horizAngle );			// angle given in degrees

	virtual bool IsLineOfSightClear( const Vector &pos ) const;	// return true if the ray to the given point is unobstructed

	/**
	 * Returns true if the ray between the position and the subject is unobstructed.
	 * A visible spot on the subject is returned in 'visibleSpot'.
	 */
	virtual bool IsLineOfSightClearToEntity( const CBaseEntity *subject, Vector *visibleSpot = NULL ) const;

	/// @todo: Implement LookAt system
	virtual bool IsLookingAt( const Vector &pos, float cosTolerance = 0.95f ) const;					// are we looking at the given position
	bool ScriptIsLookingAt( const Vector &pos, float cosTolerance = 0.95f ) const { return this->IsLookingAt( pos, cosTolerance ); }
	
	virtual bool IsLookingAt( const CBaseCombatCharacter *actor, float cosTolerance = 0.95f ) const;	// are we looking at the given actor

	//- Script access to vision functions ------------------------------------------------------------------
	DECLARE_ENT_SCRIPTDESC();

private:
	CountdownTimer m_scanTimer;			// for throttling update rate
	
	float m_FOV;						// current FOV in degrees
	float m_cosHalfFOV;					// the cosine of FOV/2
	
	CUtlVector< CKnownEntity > m_knownEntityVector;		// the set of enemies/friends we are aware of
	void UpdateKnownEntities( void );
	bool IsAwareOf( const CKnownEntity &known ) const;	// return true if our reaction time has passed for this entity
	mutable CHandle< CBaseEntity > m_primaryThreat;

	float m_lastVisionUpdateTimestamp;
	IntervalTimer m_notVisibleTimer[ MAX_TEAMS ];		// for tracking interval since last saw a member of the given team
};

inline void IVision::CollectKnownEntities( CUtlVector< CKnownEntity > *knownVector )
{
	if ( knownVector )
	{
		knownVector->RemoveAll();

		for( int i=0; i<m_knownEntityVector.Count(); ++i )
		{
			if ( !m_knownEntityVector[i].IsObsolete() )
			{
				knownVector->AddToTail( m_knownEntityVector[i] );
			}
		}
	}
}

inline float IVision::GetDefaultFieldOfView( void ) const
{
	return 90.0f;
}

inline float IVision::GetFieldOfView( void ) const
{
	return m_FOV;
}


inline float IVision::GetTimeSinceVisible( int team ) const
{
	if ( team == TEAM_ANY )
	{
		// return minimum time	
		float time = 9999999999.9f;
		for( int i=0; i<MAX_TEAMS; ++i )
		{
			if ( m_notVisibleTimer[i].HasStarted() )
			{
				if ( time > m_notVisibleTimer[i].GetElapsedTime() )
				{
					team = m_notVisibleTimer[i].GetElapsedTime();
				}
			}
		}
		return time;
	}
	
	if ( team >= 0 && team < MAX_TEAMS )
	{
		return m_notVisibleTimer[ team ].GetElapsedTime();
	}
	
	return 0.0f;
}


inline bool IVision::IsAwareOf( const CKnownEntity &known ) const
{
	return known.GetTimeSinceBecameKnown() >= GetMinRecognizeTime();
}


inline bool IVision::ForEachKnownEntity( IVision::IForEachKnownEntity &func )
{
	for( int i=0; i<m_knownEntityVector.Count(); ++i )
	{
		const CKnownEntity &known = m_knownEntityVector[i];

		if ( !known.IsObsolete() && IsAwareOf( known ) )
		{
			if ( func.Inspect( known ) == false )
			{
				return false;
			}
		}
	}
	
	return true;
}

inline bool IVision::IsVisibleEntityNoticed( CBaseEntity *subject ) const
{
	return true;
}

inline bool IVision::IsIgnored( CBaseEntity *subject ) const
{
	return false;
}

inline float IVision::GetMaxVisionRange( void ) const
{
	return 2000.0f;
}

inline float IVision::GetMinRecognizeTime( void ) const
{
	return 0.0f;
}


#endif // _NEXT_BOT_VISION_INTERFACE_H_
