//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================
#ifndef TF_ACHIEVEMENT_DATA_H
#define TF_ACHIEVEMENT_DATA_H
#pragma once

#include "UtlSortVector.h"

#define MAX_ACHIEVEMENT_HISTORY_SLOTS	4
#define MAX_ACHIEVEMENT_DAMAGE_HISTORY_SLOTS	128

//=============================================================================
// Custom class to manage lists of history events. Maintains a prioritized list of 
// events, but includes two extra features: 
//		- Maximum size of the number of entries in the queue.
//		- Ensures each associated-entity in the entries appears only once in the queue.
template <class T, class LessFunc, int maxSize>
class CHistoryVector : public CUtlSortVector<T, LessFunc>
{
public:
	CHistoryVector()
	{
	}

	void InsertHistory( T const &element )
	{
		LessFunc less;

		// Make sure it's not in the list already
		for ( int i = 0; i < this->Count(); i++ )
		{
			if ( less.HistoryMatch( this->Element(i), element ) )
			{
				this->Remove( i );
				break;
			}
		}

		CUtlSortVector<T, LessFunc>::Insert( element );

		// Remove the oldest entry if we're over max size
		if ( this->Count() > maxSize )
		{
			this->Remove( this->Count()-1 );
		}
	}
};

//=============================================================================
// Data stored in players for achievement handling
struct EntityHistory_t
{
	EHANDLE hEntity;
	EHANDLE hObject;
	float	flTimeDamage;
};

struct EntityDamageHistory_t : public EntityHistory_t
{
	int	nDamageAmount;
};


class CEntityHistoryLess
{
public:
	bool Less( const EntityHistory_t &dmg1, const EntityHistory_t &dmg2, void *pCtx )
	{
		return (dmg1.flTimeDamage > dmg2.flTimeDamage);
	}
	bool HistoryMatch( const EntityHistory_t &dmg1, const EntityHistory_t &dmg2 )
	{
		return (dmg1.hEntity == dmg2.hEntity);
	}
};

// Allow duplicate (source) entries with this type; HistoryMatch always returns false
class CEntityDamageHistoryLess
{
public:
	bool Less( const EntityDamageHistory_t &dmg1, const EntityDamageHistory_t &dmg2, void *pCtx )
	{
		return ( dmg2.flTimeDamage < dmg1.flTimeDamage );
	}
	bool HistoryMatch( const EntityDamageHistory_t &dmg1, const EntityDamageHistory_t &dmg2 )
	{
		return false;
	}
};

// Achievement Tracking container
class CAchievementData
{
public:
	void ClearHistories( void )
	{
		aDamagers.RemoveAll();
		aDamageEvents.RemoveAll();
		aTargets.RemoveAll();
		aSentryDamagers.RemoveAll();
		aPushers.RemoveAll();
	}

	void				AddDamagerToHistory( EHANDLE hDamager );
	EntityHistory_t		*GetDamagerHistory( int i ) { if (i >= aDamagers.Count()) return NULL; return &aDamagers[i]; }
	int					CountDamagersWithinTime( float flTime );
	bool				IsDamagerInHistory( CBaseEntity *pTarget, float flTimeWindow );
	void				DumpDamagers( void );

	// Capture the last 64 damage events - duplicates allowed
	void				AddDamageEventToHistory( EHANDLE hAttacker, float flDmgAmount = 0.f );
	EntityDamageHistory_t *GetDamageEventHistory( int i ) { if ( i >= aDamageEvents.Count() ) return NULL; return &aDamageEvents[i]; }
	int					GetDamageEventHistoryCount( void ) { return aDamageEvents.Count(); }
	bool				IsEntityInDamageEventHistory( CBaseEntity *pEntity, float flTimeWindow );
	int					GetAmountForDamagerInEventHistory( CBaseEntity *pEntity, float flTimeWindow );
	float				GetFirstEntryTimeForDamagerInHistory( CBaseEntity *pEntity );

	void				AddTargetToHistory( EHANDLE hTarget );
	bool				IsTargetInHistory( CBaseEntity *pTarget, float flTimeWindow );
	EntityHistory_t		*GetTargetHistory( int i ) { if (i >= aTargets.Count()) return NULL; return &aTargets[i]; }
	int					CountTargetsWithinTime( float flTime );

	void                AddSentryDamager( EHANDLE hDamager, EHANDLE hObject );
	EntityHistory_t     *IsSentryDamagerInHistory( CBaseEntity *pDamager, float flTimeWindow );

	void				AddPusherToHistory( EHANDLE hPlayer );
	bool				IsPusherInHistory( CBaseEntity *pPlayer, float flTimeWindow );

private:
	CHistoryVector< EntityHistory_t, CEntityHistoryLess, MAX_ACHIEVEMENT_HISTORY_SLOTS > aDamagers;
	CHistoryVector< EntityDamageHistory_t, CEntityDamageHistoryLess, MAX_ACHIEVEMENT_DAMAGE_HISTORY_SLOTS > aDamageEvents;	// Duplicates allowed
	CHistoryVector< EntityHistory_t, CEntityHistoryLess, MAX_ACHIEVEMENT_HISTORY_SLOTS > aTargets;
	CHistoryVector< EntityHistory_t, CEntityHistoryLess, MAX_ACHIEVEMENT_HISTORY_SLOTS > aSentryDamagers;
	CHistoryVector< EntityHistory_t, CEntityHistoryLess, MAX_ACHIEVEMENT_HISTORY_SLOTS > aPushers;
};

#endif
