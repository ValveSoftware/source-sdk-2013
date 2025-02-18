//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "hl2mp_bot.h"
#include "hl2mp_bot_squad.h"

//----------------------------------------------------------------------
CHL2MPBotSquad::CHL2MPBotSquad( void )
{
	m_leader = NULL;
	m_formationSize = -1.0f;
	m_bShouldPreserveSquad = false;
}


//----------------------------------------------------------------------
void CHL2MPBotSquad::Join( CHL2MPBot *bot )
{
	// first member is the leader
	if ( m_roster.Count() == 0 )
	{
		m_leader = bot;
	}

	m_roster.AddToTail( bot );
}


//----------------------------------------------------------------------
void CHL2MPBotSquad::Leave( CHL2MPBot *bot )
{
	m_roster.FindAndRemove( bot );

	if ( bot == m_leader.Get() )
	{
		m_leader = NULL;

		// pick the next living leader that's left in the squad
		if ( m_bShouldPreserveSquad )
		{
			CUtlVector< CHL2MPBot* > members;
			CollectMembers( &members );
			if ( members.Count() )
			{
				m_leader = members[0];
			}
		}
	}
	
	if ( GetMemberCount() == 0 )
	{
		DisbandAndDeleteSquad();
	}
}


//----------------------------------------------------------------------
INextBotEventResponder *CHL2MPBotSquad::FirstContainedResponder( void ) const
{
	return m_roster.Count() ? m_roster[0] : NULL;
}


//----------------------------------------------------------------------
INextBotEventResponder *CHL2MPBotSquad::NextContainedResponder( INextBotEventResponder *current ) const
{
	CHL2MPBot *currentBot = (CHL2MPBot *)current;

	int i = m_roster.Find( currentBot );

	if ( i == m_roster.InvalidIndex() )
		return NULL;

	if ( ++i >= m_roster.Count() )
		return NULL;

	return (CHL2MPBot *)m_roster[i];
}


//----------------------------------------------------------------------
CHL2MPBot *CHL2MPBotSquad::GetLeader( void ) const
{
	return m_leader;
}


//----------------------------------------------------------------------
void CHL2MPBotSquad::CollectMembers( CUtlVector< CHL2MPBot * > *memberVector ) const
{
	for( int i=0; i<m_roster.Count(); ++i )
	{
		if ( m_roster[i] != NULL && m_roster[i]->IsAlive() )
		{
			memberVector->AddToTail( m_roster[i] );
		}
	}
}


//----------------------------------------------------------------------
CHL2MPBotSquad::Iterator CHL2MPBotSquad::GetFirstMember( void ) const
{
	// find first non-NULL member
	for( int i=0; i<m_roster.Count(); ++i )
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
			return Iterator( m_roster[i], i );

	return InvalidIterator();
}


//----------------------------------------------------------------------
CHL2MPBotSquad::Iterator CHL2MPBotSquad::GetNextMember( const Iterator &it ) const
{
	// find next non-NULL member
	for( int i=it.m_index+1; i<m_roster.Count(); ++i )
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
			return Iterator( m_roster[i], i );

	return InvalidIterator();
}


//----------------------------------------------------------------------
int CHL2MPBotSquad::GetMemberCount( void ) const
{
	// count the non-NULL members
	int count = 0;
	for( int i=0; i<m_roster.Count(); ++i )
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
			++count;

	return count;
}


//----------------------------------------------------------------------
// Return the speed of the slowest member of the squad
float CHL2MPBotSquad::GetSlowestMemberSpeed( bool includeLeader ) const
{
	float speed = FLT_MAX;

	int i = includeLeader ? 0 : 1;

	for( ; i<m_roster.Count(); ++i )
	{
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			float memberSpeed = m_roster[i]->MaxSpeed();
			if ( memberSpeed < speed )
			{
				speed = memberSpeed;
			}
		}
	}

	return speed;
}


//----------------------------------------------------------------------
// Return the speed of the slowest member of the squad, 
// considering their ideal class speed.
float CHL2MPBotSquad::GetSlowestMemberIdealSpeed( bool includeLeader ) const
{
	float speed = FLT_MAX;

	int i = includeLeader ? 0 : 1;

	for( ; i<m_roster.Count(); ++i )
	{
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			// TODO(misyl): One could make this consider all the members of the roster's
			// aux power, and see if they are allowed to sprint here.
			//
			// I am not planning on using squads, just pointing it out if someone
			// else wants to use this code.
			float memberSpeed = hl2_normspeed.GetFloat();
			if ( memberSpeed < speed )
			{
				speed = memberSpeed;
			}
		}
	}

	return speed;
}


//----------------------------------------------------------------------
// Return the maximum formation error of the squad's memebers.
float CHL2MPBotSquad::GetMaxSquadFormationError( void ) const
{
	float maxError = 0.0f;

	// skip the leader since he's what the formation forms around
	for( int i=1; i<m_roster.Count(); ++i )
	{
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			float error = m_roster[i]->GetSquadFormationError();
			if ( error > maxError )
			{
				maxError = error;
			}
		}
	}

	return maxError;
}


//----------------------------------------------------------------------
// Return true if the squad leader needs to wait for members to catch up, ignoring those who have broken ranks
bool CHL2MPBotSquad::ShouldSquadLeaderWaitForFormation( void ) const
{
	// skip the leader since he's what the formation forms around
	for( int i=1; i<m_roster.Count(); ++i )
	{
		// the squad leader should wait if any member is out of position, but not yet broken ranks
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			if ( m_roster[i]->GetSquadFormationError() >= 1.0f && 
				 !m_roster[i]->HasBrokenFormation() && 
				 !m_roster[i]->GetLocomotionInterface()->IsStuck() )
			{
				// wait for me!
				return true;
			}
		}
	}

	return false;
}


//----------------------------------------------------------------------
// Return true if the squad is in formation (everyone is in or nearly in their desired positions)
bool CHL2MPBotSquad::IsInFormation( void ) const
{
	// skip the leader since he's what the formation forms around
	for( int i=1; i<m_roster.Count(); ++i )
	{
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			if ( m_roster[i]->HasBrokenFormation() ||
				 m_roster[i]->GetLocomotionInterface()->IsStuck() )
			{
				// I'm not "in formation"
				continue;
			}

			if ( m_roster[i]->GetSquadFormationError() > 0.75f )
			{
				// I'm not in position yet
				return false;
			}
		}
	}

	return true;
}

//----------------------------------------------------------------------
// Tell all members to leave the squad and then delete itself
void CHL2MPBotSquad::DisbandAndDeleteSquad( void )
{
	// Tell each member of the squad to remove this reference
	for( int i=0; i < m_roster.Count(); ++i )
	{
		if ( m_roster[i].Get() != NULL )
		{
			m_roster[i]->DeleteSquad();
		}
	}

	delete this;
}