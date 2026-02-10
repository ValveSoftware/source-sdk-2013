//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_squad.h
// Small groups of TFBot, managed as a unit
// Michael Booth, November 2009

#include "cbase.h"
#include "tf_bot.h"
#include "tf_bot_squad.h"


//----------------------------------------------------------------------
CTFBotSquad::CTFBotSquad( void )
{
	m_leader = NULL;
	m_formationSize = -1.0f;
	m_bShouldPreserveSquad = false;
}


//----------------------------------------------------------------------
void CTFBotSquad::Join( CTFBot *bot )
{
	// first member is the leader
	if ( m_roster.Count() == 0 )
	{
		m_leader = bot;
	}
	else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		bot->SetFlagTarget( NULL );
	}

	m_roster.AddToTail( bot );
}


//----------------------------------------------------------------------
void CTFBotSquad::Leave( CTFBot *bot )
{
	m_roster.FindAndRemove( bot );

	if ( bot == m_leader.Get() )
	{
		m_leader = NULL;

		// pick the next living leader that's left in the squad
		if ( m_bShouldPreserveSquad )
		{
			CUtlVector< CTFBot* > members;
			CollectMembers( &members );
			if ( members.Count() )
			{
				m_leader = members[0];
			}
		}
	}
	else if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		AssertMsg( !bot->HasFlagTaget(), "Squad member shouldn't have a flag target. Always follow the leader." );
		CCaptureFlag *pFlag = bot->GetFlagToFetch();
		if ( pFlag )
		{
			bot->SetFlagTarget( pFlag );
		}
	}
	
	if ( GetMemberCount() == 0 )
	{
		DisbandAndDeleteSquad();
	}
}


//----------------------------------------------------------------------
INextBotEventResponder *CTFBotSquad::FirstContainedResponder( void ) const
{
	return m_roster.Count() ? m_roster[0] : NULL;
}


//----------------------------------------------------------------------
INextBotEventResponder *CTFBotSquad::NextContainedResponder( INextBotEventResponder *current ) const
{
	CTFBot *currentBot = (CTFBot *)current;

	int i = m_roster.Find( currentBot );

	if ( i == m_roster.InvalidIndex() )
		return NULL;

	if ( ++i >= m_roster.Count() )
		return NULL;

	return (CTFBot *)m_roster[i];
}


//----------------------------------------------------------------------
CTFBot *CTFBotSquad::GetLeader( void ) const
{
	return m_leader;
}


//----------------------------------------------------------------------
void CTFBotSquad::CollectMembers( CUtlVector< CTFBot * > *memberVector ) const
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
CTFBotSquad::Iterator CTFBotSquad::GetFirstMember( void ) const
{
	// find first non-NULL member
	for( int i=0; i<m_roster.Count(); ++i )
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
			return Iterator( m_roster[i], i );

	return InvalidIterator();
}


//----------------------------------------------------------------------
CTFBotSquad::Iterator CTFBotSquad::GetNextMember( const Iterator &it ) const
{
	// find next non-NULL member
	for( int i=it.m_index+1; i<m_roster.Count(); ++i )
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
			return Iterator( m_roster[i], i );

	return InvalidIterator();
}


//----------------------------------------------------------------------
int CTFBotSquad::GetMemberCount( void ) const
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
float CTFBotSquad::GetSlowestMemberSpeed( bool includeLeader ) const
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
float CTFBotSquad::GetSlowestMemberIdealSpeed( bool includeLeader ) const
{
	float speed = FLT_MAX;

	int i = includeLeader ? 0 : 1;

	for( ; i<m_roster.Count(); ++i )
	{
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			float memberSpeed = m_roster[i]->GetPlayerClass()->GetMaxSpeed();
			if ( memberSpeed < speed )
			{
				speed = memberSpeed;
			}
		}
	}

	return speed;
}


//----------------------------------------------------------------------
// Return the maximum formation error of the squad's members.
float CTFBotSquad::GetMaxSquadFormationError( void ) const
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
bool CTFBotSquad::ShouldSquadLeaderWaitForFormation( void ) const
{
	// skip the leader since he's what the formation forms around
	for( int i=1; i<m_roster.Count(); ++i )
	{
		// the squad leader should wait if any member is out of position, but not yet broken ranks
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			if ( m_roster[i]->GetSquadFormationError() >= 1.0f && 
				 !m_roster[i]->HasBrokenFormation() && 
				 !m_roster[i]->GetLocomotionInterface()->IsStuck() &&
				 !m_roster[i]->IsPlayerClass( TF_CLASS_MEDIC ) )		// Medics do their own thing
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
bool CTFBotSquad::IsInFormation( void ) const
{
	// skip the leader since he's what the formation forms around
	for( int i=1; i<m_roster.Count(); ++i )
	{
		if ( m_roster[i].Get() != NULL && m_roster[i]->IsAlive() )
		{
			if ( m_roster[i]->HasBrokenFormation() ||
				 m_roster[i]->GetLocomotionInterface()->IsStuck() ||
				 m_roster[i]->IsPlayerClass( TF_CLASS_MEDIC ) )		// Medics do their own thing
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
void CTFBotSquad::DisbandAndDeleteSquad( void )
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