// NextBotManager.h
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_MANAGER_H_
#define _NEXT_BOT_MANAGER_H_

#include "NextBotInterface.h"

class CTerrorPlayer;

//----------------------------------------------------------------------------------------------------------------
/**
 * The NextBotManager manager 
 */
class NextBotManager
{
public:
	NextBotManager( void );
	virtual ~NextBotManager();

	void Reset( void );								// reset to initial state
	virtual void Update( void );

	bool ShouldUpdate( INextBot *bot );
	void NotifyBeginUpdate( INextBot *bot );
	void NotifyEndUpdate( INextBot *bot );

	int GetNextBotCount( void ) const;				// How many nextbots are alive right now?


	/**
	 * Populate given vector with all bots in the system
	 */
	void CollectAllBots( CUtlVector< INextBot * > *botVector );


	/**
	 * DEPRECATED: Use CollectAllBots().
	 * Execute functor for each NextBot in the system.
	 * If a functor returns false, stop iteration early
	 * and return false.
	 */	
	template < typename Functor >
	bool ForEachBot( Functor &func )
	{
		for( int i=m_botList.Head(); i != m_botList.InvalidIndex(); i = m_botList.Next( i ) )
		{
			if ( !func( m_botList[i] ) )
			{
				return false;
			}
		}

		return true;
	}

	/**
	 * DEPRECATED: Use CollectAllBots().
	 * Execute functor for each NextBot in the system as 
	 * a CBaseCombatCharacter.
	 * If a functor returns false, stop iteration early
	 * and return false.
	 */	
	template < typename Functor >
	bool ForEachCombatCharacter( Functor &func )
	{
		for( int i=m_botList.Head(); i != m_botList.InvalidIndex(); i = m_botList.Next( i ) )
		{
			if ( !func( m_botList[i]->GetEntity() ) )
			{

				return false;
			}
		}

		return true;
	}

	/**
	 * Return closest bot to given point that passes the given filter
	 */	
	template < typename Filter >
	INextBot *GetClosestBot( const Vector &pos, Filter &filter )
	{
		INextBot *close = NULL;
		float closeRangeSq = FLT_MAX;

		for( int i=m_botList.Head(); i != m_botList.InvalidIndex(); i = m_botList.Next( i ) )
		{
			float rangeSq = ( m_botList[i]->GetEntity()->GetAbsOrigin() - pos ).LengthSqr();
			if ( rangeSq < closeRangeSq && filter( m_botList[i] ) )
			{
				closeRangeSq = rangeSq;
				close = m_botList[i];
			}
		}

		return close;
	}

	/**
	 * Event propagators
	 */
	virtual void OnMapLoaded( void );						// when the server has changed maps
	virtual void OnRoundRestart( void );					// when the scenario restarts
	virtual void OnBeginChangeLevel( void );				// when the server is about to change maps
	virtual void OnKilled( CBaseCombatCharacter *victim, const CTakeDamageInfo &info );	// when an actor is killed
	virtual void OnSound( CBaseEntity *source, const Vector &pos, KeyValues *keys );				// when an entity emits a sound
	virtual void OnSpokeConcept( CBaseCombatCharacter *who, AIConcept_t concept, AI_Response *response );	// when an Actor speaks a concept
	virtual void OnWeaponFired( CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon );		// when someone fires a weapon

	/**
	 * Debugging
	 */
	bool IsDebugging( unsigned int type ) const;	// return true if debugging system is on for the given type(s)
	void SetDebugTypes( NextBotDebugType type );	// start displaying debug info of the given type(s)

	void DebugFilterAdd( int index );				// add given entindex to the debug filter
	void DebugFilterAdd( const char *name );		// add given name to the debug filter
	void DebugFilterRemove( int index );			// remove given entindex from the debug filter
	void DebugFilterRemove( const char *name );		// remove given name from the debug filter
	void DebugFilterClear( void );					// clear the debug filter (remove all entries)
	bool IsDebugFilterMatch( const INextBot *bot ) const;	// return true if the given bot matches the debug filter

	void Select( INextBot *bot );					// mark bot as selected for further operations
	void DeselectAll( void );
	INextBot *GetSelected( void ) const;

	INextBot *GetBotUnderCrosshair( CBasePlayer *picker );	// Get the bot under the given player's crosshair

	//
	// Put these in a derived class
	//
	void OnSurvivorVomitedUpon( CTerrorPlayer *victim );	// when a Survivor has been hit by Boomer Vomit

	static void SetInstance( NextBotManager *pInstance ) { sInstance = pInstance; };
	static NextBotManager* GetInstance() { return sInstance; }

protected:
	static NextBotManager* sInstance;

	friend class INextBot;

	int Register( INextBot *bot );
	void UnRegister( INextBot *bot );

	CUtlLinkedList< INextBot * > m_botList;				// list of all active NextBots

	int m_iUpdateTickrate;
	double m_CurUpdateStartTime;
	double m_SumFrameTime;

	unsigned int m_debugType;						// debug flags

	struct DebugFilter
	{
		int index;			// entindex
		enum { MAX_DEBUG_NAME_SIZE = 128 };
		char name[ MAX_DEBUG_NAME_SIZE ];
	};
	CUtlVector< DebugFilter > m_debugFilterList;

	INextBot *m_selectedBot;						// selected bot for further debug operations
};

inline int NextBotManager::GetNextBotCount( void ) const
{
	return m_botList.Count();
}

inline bool NextBotManager::IsDebugging( unsigned int type ) const
{
	if ( type & m_debugType )
	{
		return true;
	}

	return false;
}


inline void NextBotManager::SetDebugTypes( NextBotDebugType type )
{
	m_debugType = (unsigned int)type;
}


inline void NextBotManager::Select( INextBot *bot )
{
	m_selectedBot = bot;
}

inline void NextBotManager::DeselectAll( void )
{
	m_selectedBot = NULL;
}

inline INextBot *NextBotManager::GetSelected( void ) const
{
	return m_selectedBot;
}



// singleton accessor
extern NextBotManager &TheNextBots( void );


#endif // _NEXT_BOT_MANAGER_H_

