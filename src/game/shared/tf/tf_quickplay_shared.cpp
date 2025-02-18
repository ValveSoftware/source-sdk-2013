//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Quickplay related code shared between GC and client
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_quickplay_shared.h"

//-----------------------------------------------------------------------------

extern const char k_szQuickplayFAQ_URL[] = "https://support.steampowered.com/kb_article.php?ref=2825-AFGJ-3513";

//
// MvM Missions
//

CMvMMissionSet::CMvMMissionSet() { Clear(); }
CMvMMissionSet::CMvMMissionSet( const CMvMMissionSet &x ) { m_bits = x.m_bits; }
CMvMMissionSet::~CMvMMissionSet() {}
void CMvMMissionSet::operator=( const CMvMMissionSet &x ) { m_bits = x.m_bits; }
void CMvMMissionSet::Clear() { m_bits = 0; }
bool CMvMMissionSet::operator==( const CMvMMissionSet &x ) const { return m_bits == x.m_bits; }

void CMvMMissionSet::SetMissionBySchemaIndex( int idxMission, bool flag )
{
	Assert( idxMission >= 0 && idxMission < GetItemSchema()->GetMvmMissions().Count() );
	uint64 mask = ( (uint64)1 << (unsigned)idxMission );
	if ( flag )
		m_bits |= mask;
	else
		m_bits &= ~mask;
}

bool CMvMMissionSet::GetMissionBySchemaIndex( int idxMission ) const
{
	// Bogus index?
	if ( idxMission == k_iMvmMissionIndex_NotInSchema )
		return false;
	if ( idxMission < 0 || idxMission >= GetItemSchema()->GetMvmMissions().Count() )
	{
		Assert( idxMission >= 0 );
		Assert( idxMission < GetItemSchema()->GetMvmMissions().Count() );
		return false;
	}

	// Check the bit
	uint64 mask = ( (uint64)1 << (unsigned)idxMission );
	return ( m_bits & mask ) != 0;
}

void CMvMMissionSet::Intersect( const CMvMMissionSet &x )
{
	m_bits &= x.m_bits;
}

bool CMvMMissionSet::HasIntersection( const CMvMMissionSet &x ) const
{
	return ( m_bits & x.m_bits ) != 0;
}

bool CMvMMissionSet::IsEmpty() const
{
	return ( m_bits == 0 );
}
