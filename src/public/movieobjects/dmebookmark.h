//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMEBOOKMARK_H
#define DMEBOOKMARK_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "movieobjects/timeutils.h"

class CDmeBookmark : public CDmElement
{
	DEFINE_ELEMENT( CDmeBookmark, CDmElement );

public:
	const char *GetNote() const { return m_Note; }
	DmeTime_t GetTime() const { return DmeTime_t( m_Time ); }
	DmeTime_t GetDuration() const { return DmeTime_t( m_Duration ); }

	void SetNote( const char *pNote ) { m_Note = pNote; }
	void SetTime( DmeTime_t time ) { m_Time = time.GetTenthsOfMS(); }
	void SetDuration( DmeTime_t duration ) { m_Duration = duration.GetTenthsOfMS(); }

private:
	CDmaString m_Note;
	CDmaVar< int > m_Time;
	CDmaVar< int > m_Duration;
};

#endif // DMEBOOKMARK_H
