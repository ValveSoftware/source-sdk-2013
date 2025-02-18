//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_squad.h
// Small groups of TFBot, managed as a unit
// Michael Booth, November 2009

#ifndef TF_BOT_SQUAD_H
#define TF_BOT_SQUAD_H

#include "NextBot/NextBotEventResponderInterface.h"

class CTFBot;

class CTFBotSquad : public INextBotEventResponder
{
public:
	CTFBotSquad( void );
	virtual ~CTFBotSquad() { }		

	// EventResponder ------
	virtual INextBotEventResponder *FirstContainedResponder( void ) const;
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const;
	//----------------------

	bool IsMember( CTFBot *bot ) const;		// is the given bot in this squad?
	bool IsLeader( CTFBot *bot ) const;		// is the given bot the leader of this squad?

// 	CTFBot *GetMember( int i );
 	int GetMemberCount( void ) const;

	CTFBot *GetLeader( void ) const;

	class Iterator
	{
	public:
		Iterator( void )
		{
			m_bot = NULL;
			m_index = -1;
		}

		Iterator( CTFBot *bot, int index )
		{
			m_bot = bot;
			m_index = index;
		}

		CTFBot *operator() ( void )
		{
			return m_bot;
		}

		bool operator==( const Iterator &it ) const	{ return m_bot == it.m_bot && m_index == it.m_index; }
		bool operator!=( const Iterator &it ) const	{ return m_bot != it.m_bot || m_index != it.m_index; }

		CTFBot *m_bot;
		int m_index;
	};

	Iterator GetFirstMember( void ) const;
	Iterator GetNextMember( const Iterator &it ) const;
	Iterator InvalidIterator() const;

	void CollectMembers( CUtlVector< CTFBot * > *memberVector ) const;

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
	friend class CTFBot;

	void Join( CTFBot *bot );
	void Leave( CTFBot *bot );

	CUtlVector< CHandle< CTFBot > > m_roster;
	CHandle< CTFBot > m_leader;

	float m_formationSize;
	bool m_bShouldPreserveSquad;
};

inline bool CTFBotSquad::IsMember( CTFBot *bot ) const
{
	return m_roster.HasElement( bot );
}

inline bool CTFBotSquad::IsLeader( CTFBot *bot ) const
{
	return m_leader == bot;
}

inline CTFBotSquad::Iterator CTFBotSquad::InvalidIterator() const
{
	return Iterator( NULL, -1 );
}

inline float CTFBotSquad::GetFormationSize( void ) const
{
	return m_formationSize;
}

inline void CTFBotSquad::SetFormationSize( float size )
{
	m_formationSize = size;
}


#endif // TF_BOT_SQUAD_H

