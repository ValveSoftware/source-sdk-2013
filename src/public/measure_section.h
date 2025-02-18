//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#if !defined( MEASURE_SECTION_H )
#define MEASURE_SECTION_H
#ifdef _WIN32
#pragma once
#endif


#include "tier0/fasttimer.h"
#include "convar.h"


// This is the macro to use in your code to measure until the code goes
//  out of scope
#if defined( _DEBUG ) || defined( FORCE_MEASURE )
#define MEASURECODE( description ) \
	static CMeasureSection	_xxx_ms( description ); \
	CMeasureSectionInstance _xxx_ms_inst( &_xxx_ms );
#else
#define MEASURECODE( description )
#endif


// ------------------------------------------------------------------------------------ //
// These things must exist in the executable for the CMeasureSection code to work.
// ------------------------------------------------------------------------------------ //
float GetRealTime();	// Get the clock's time.

extern ConVar	game_speeds;
extern ConVar	measure_resort;
// ------------------------------------------------------------------------------------ //



// Called once per frame to allow any necessary measurements to latch
void ResetTimeMeasurements( void );

//-----------------------------------------------------------------------------
// Purpose: Accumulates time for the named section
//-----------------------------------------------------------------------------
class CMeasureSection
{
public:
	// Allows for measuring named section
						CMeasureSection( const char *name );
	virtual				~CMeasureSection( void );


	// Update max value hit
	void				UpdateMax( void );
	// Reset totals
	void				Reset( void );
	// Reset sortable totals
	void				SortReset( void );
	// Get static name of section
	const char			*GetName( void );
	
	// Get accumulated time
	CCycleCount const&	GetTotalTime( void );

	CCycleCount const&	GetTime();

	CCycleCount const&	GetMaxTime();
	
	// Add in some time
	void				AddTime( CCycleCount const &rCount );

	// Get next section in chain
	CMeasureSection		*GetNext( void );

	// Get head of list of all sections
	static CMeasureSection *GetList( void );
	// Sort all sections by most time consuming
	static void			SortSections( void );

public:
	// Time when list should be sorted again
	static double		m_dNextResort;

private:
	// Accumulated time for section
	CCycleCount			m_dAccumulatedTime;
	
	// Max time for section
	CCycleCount			m_dMaxTime;

	// Elapsed time for section
	CCycleCount			m_dTotalTime;
	
	// Name of section
	const char			*m_pszName;
	// Next section in chain
	CMeasureSection		*m_pNext;
	// Head of section list
	static CMeasureSection *s_pSections;
	// Quick total for doing sorts faster
	static int			s_nCount;
};

//-----------------------------------------------------------------------------
// Purpose: On construction marks time and on destruction adds time to 
//  parent CMeasureSection object
//-----------------------------------------------------------------------------
class CMeasureSectionInstance
{
public:
	// Constructor:  Points to object to accumulate time into
						CMeasureSectionInstance( CMeasureSection *ms );
	// Destructor:  Latches accumulated time
	virtual				~CMeasureSectionInstance( void );

private:
	// Time of construction
	CFastTimer			m_Timer;

	// Where to place elapsed time
	CMeasureSection		*m_pMS;
};

#endif // MEASURE_SECTION_H
