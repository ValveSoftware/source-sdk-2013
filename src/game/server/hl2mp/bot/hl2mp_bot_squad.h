//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_SQUAD_H
#define HL2MP_BOT_SQUAD_H

#include "NextBot/NextBotEventResponderInterface.h"

class CHL2MPBot;

class CHL2MPBotSquad : public INextBotEventResponder
{
public:
	CHL2MPBotSquad( void );
	virtual ~CHL2MPBotSquad() { }		

	// EventResponder ------
	virtual INextBotEventResponder *FirstContainedResponder( void ) const;
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const;
	//----------------------

	bool IsMember( CHL2MPBot *bot ) const;		// is the given bot in this squad?
	bool IsLeader( CHL2MPBot *bot ) const;		// is the given bot the leader of this squad?

// 	CHL2MPBot *GetMember( int i );
 	int GetMemberCount( void ) const;

	CHL2MPBot *GetLeader( void ) const;

	class Iterator
	{
	public:
		Iterator( void )
		{
			m_bot = NULL;
			m_index = -1;
		}

		Iterator( CHL2MPBot *bot, int index )
		{
			m_bot = bot;
			m_index = index;
		}

		CHL2MPBot *operator() ( void )
		{
			return m_bot;
		}

		bool operator==( const Iterator &it ) const	{ return m_bot == it.m_bot && m_index == it.m_index; }
		bool operator!=( const Iterator &it ) const	{ return m_bot != it.m_bot || m_index != it.m_index; }

		CHL2MPBot *m_bot;
		int m_index;
	};

	Iterator GetFirstMember( void ) const;
	Iterator GetNextMember( const Iterator &it ) const;
	Iterator InvalidIterator() const;

	void CollectMembers( CUtlVector< CHL2MPBot * > *memberVector ) const;

	#define EXCLUDE_LEADER false
	float GetSlowestMemberSpeed( bool includeLeader = true ) const;
	float GetSlowestMemberIdealSpeed( bool includeLeader = true ) const;
	float GetMaxSquadFormationError( void ) const;

	bool ShouldSquadLeaderWaitForFormation( void ) const;		// return true if the squad leader needs to wait for members to catch up, ignoring those who have broken ranks
	bool IsInFormation( void ) const;						// return true if the squad is in formation (everyone is in or nearly in their desired positions)

	float GetFormationSize( void ) const;
	void SetFormationSize( float size );

	void DisbandAndDeleteSquad( void );

	void SetShouldPreserveSquad( bool bShouldPreserveSquad ) { m_bShouldPreserveSquad = bShouldPreserveSquad; }
	bool ShouldPreserveSquad() const { return m_bShouldPreserveSquad; }

private:
	friend class CHL2MPBot;

	void Join( CHL2MPBot *bot );
	void Leave( CHL2MPBot *bot );

	CUtlVector< CHandle< CHL2MPBot > > m_roster;
	CHandle< CHL2MPBot > m_leader;

	float m_formationSize;
	bool m_bShouldPreserveSquad;
};

inline bool CHL2MPBotSquad::IsMember( CHL2MPBot *bot ) const
{
	return m_roster.HasElement( bot );
}

inline bool CHL2MPBotSquad::IsLeader( CHL2MPBot *bot ) const
{
	return m_leader == bot;
}

inline CHL2MPBotSquad::Iterator CHL2MPBotSquad::InvalidIterator() const
{
	return Iterator( NULL, -1 );
}

inline float CHL2MPBotSquad::GetFormationSize( void ) const
{
	return m_formationSize;
}

inline void CHL2MPBotSquad::SetFormationSize( float size )
{
	m_formationSize = size;
}


#endif // HL2MP_BOT_SQUAD_H

