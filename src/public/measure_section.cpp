//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include <string.h>
#include <stdlib.h>
#include "basetypes.h"
#include "measure_section.h"
#include "convar.h"


// Static members
CMeasureSection *CMeasureSection::s_pSections = 0;
int	CMeasureSection::s_nCount = 0;
double CMeasureSection::m_dNextResort = 0.0;

ConVar	measure_resort( "measure_resort", "1.0", 0, "How often to re-sort profiling sections\n" );
ConVar	game_speeds( "game_speeds","0" );
extern ConVar host_speeds;

//-----------------------------------------------------------------------------
// Purpose: Creates a profiling section
// Input  : *name - name of the section ( allocated on stack hopefully )
//-----------------------------------------------------------------------------
CMeasureSection::CMeasureSection( const char *name )
{
	// Just point at name since it's static
	m_pszName		= name;
	
	// Clear accumulators
	Reset();
	SortReset();
	m_dMaxTime.Init();

	// Link into master list
	m_pNext			= s_pSections;
	s_pSections		= this;
	// Update count
	s_nCount++;
}

//-----------------------------------------------------------------------------
// Purpose: Destroys the object
//-----------------------------------------------------------------------------
CMeasureSection::~CMeasureSection( void )
{
	m_pNext = NULL;
	s_nCount--;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMeasureSection::UpdateMax( void )
{
	if (m_dMaxTime.IsLessThan(m_dAccumulatedTime))
	{
		m_dMaxTime.Init();
		CCycleCount::Add(m_dMaxTime,m_dAccumulatedTime,m_dMaxTime);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMeasureSection::Reset( void )
{
	m_dAccumulatedTime.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMeasureSection::SortReset( void )
{
	m_dTotalTime.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CMeasureSection::GetName( void )
{
	return m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CCycleCount const& CMeasureSection::GetTotalTime( void )
{
	return m_dTotalTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CCycleCount const& CMeasureSection::GetTime( void )
{
	return m_dAccumulatedTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
CCycleCount const& CMeasureSection::GetMaxTime( void )
{
	return m_dMaxTime;
}

//-----------------------------------------------------------------------------
// Purpose: Accumulates a timeslice
// Input  : time - 
//-----------------------------------------------------------------------------
void CMeasureSection::AddTime( CCycleCount const &rCount )
{
	CCycleCount::Add(m_dAccumulatedTime, rCount, m_dAccumulatedTime);
	CCycleCount::Add(m_dTotalTime, rCount, m_dTotalTime);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CMeasureSection
//-----------------------------------------------------------------------------
CMeasureSection *CMeasureSection::GetNext( void )
{
	return m_pNext;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CMeasureSection
//-----------------------------------------------------------------------------
CMeasureSection *CMeasureSection::GetList( void )
{
	return s_pSections;
}

//-----------------------------------------------------------------------------
// Purpose: Compares accumulated time for two sections
// Input  : ppms1 - 
//			ppms2 - 
// Output : static int
//-----------------------------------------------------------------------------
static int SectionCompare( const void* ppms1,const void* ppms2 )
{
	CMeasureSection* pms1 = *(CMeasureSection**)ppms1;
	CMeasureSection* pms2 = *(CMeasureSection**)ppms2;

	if ( pms1->GetTotalTime().IsLessThan(pms2->GetTotalTime()) )
		return 1;
	else
		return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Sorts sections by time usage
//-----------------------------------------------------------------------------
void CMeasureSection::SortSections( void )
{
	// Not enough to be sortable
	if ( s_nCount <= 1 )
		return;

	CMeasureSection *sortarray[ 128 ];
	CMeasureSection *ms;

	memset(sortarray,sizeof(CMeasureSection*)*128,0);
	
	ms = GetList();
	int i;
	int c = 0;
	while ( ms )
	{
		sortarray[ c++ ] = ms;
		ms = ms->GetNext();
	}

	// Sort the array alphabetically
	qsort( sortarray, c , sizeof( CMeasureSection * ), SectionCompare );

	// Fix next pointers
	for ( i = 0; i < c-1; i++ )
	{
		sortarray[ i ]->m_pNext = sortarray[ i + 1 ];
	}
	sortarray[i]->m_pNext = NULL;

	// Point head of list at it
	s_pSections = sortarray[ 0 ];
}

//-----------------------------------------------------------------------------
// Purpose: Creates an instance for timing a section
// Input  : *ms - 
//-----------------------------------------------------------------------------
CMeasureSectionInstance::CMeasureSectionInstance( CMeasureSection *ms )
{
	// Remember where to put accumulated time
	m_pMS		= ms;

	if ( host_speeds.GetInt() < 3 && !game_speeds.GetInt())
		return;
	
	// Get initial timestamp
	m_Timer.Start();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CMeasureSectionInstance::~CMeasureSectionInstance( void )
{
	if ( host_speeds.GetInt() < 3 && !game_speeds.GetInt())
		return;

	// Get final timestamp
	m_Timer.End();

	// Add time to section
	m_pMS->AddTime( m_Timer.GetDuration() );
}

//-----------------------------------------------------------------------------
// Purpose: Re-sort all data and determine whether sort keys should be reset too ( after
//  re-doing sort if needed ).
//-----------------------------------------------------------------------------
void ResetTimeMeasurements( void )
{
#if defined( _DEBUG ) || defined( FORCE_MEASURE )
	bool sort_reset = false;

	// Time to redo sort?
	if ( measure_resort.GetFloat() > 0.0 &&
		GetRealTime() >= CMeasureSection::m_dNextResort )
	{
		// Redo it
		CMeasureSection::SortSections();
		// Set next time
		CMeasureSection::m_dNextResort = GetRealTime() + measure_resort.GetFloat();
		// Flag to reset sort accumulator, too
		sort_reset = true;
	}

	// Iterate through the sections now
	CMeasureSection *p = CMeasureSection::GetList();
	while ( p )
	{
		// Reset regular accum.
		p->Reset();
		// Reset sort accum less often
		if ( sort_reset )
		{
			p->SortReset();
		}
		p = p->GetNext();
	}
#endif
}