//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: motd: Handles a list of message of the day entries
//
//=============================================================================

#include "cbase.h"
#include "motd.h"
#include "schemainitutils.h"
#include "rtime.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace GCSDK;

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
CMOTDEntryDefinition::CMOTDEntryDefinition( void )
{
	m_pKVMOTD = NULL;
	m_PostTime = 0;
	m_ChangedTime = 0;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CMOTDEntryDefinition::BInitFromKV( KeyValues *pKVMOTD, CUtlVector<CUtlString> *pVecErrors )
{
	m_pKVMOTD = pKVMOTD->MakeCopy();

	const char *pszTime = m_pKVMOTD->GetString( "post_time", NULL );
	m_PostTime = (pszTime && pszTime[0]) ? CRTime::RTime32FromString(pszTime) : 0;

	pszTime = m_pKVMOTD->GetString( "last_changed_time", NULL );
	m_ChangedTime = (pszTime && pszTime[0]) ? CRTime::RTime32FromString(pszTime) : 0;

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *CMOTDEntryDefinition::GetTitle( ELanguage eLang ) 
{ 
	if ( m_pKVMOTD )
	{
		// See if we have a localised block for the specified language.
		const char *pszLanguage = GetLanguageShortName( eLang );
		if ( pszLanguage && pszLanguage[0] )
		{
			const char *pszText = m_pKVMOTD->GetString( CFmtStr( "title_%s", pszLanguage ), NULL );
			if ( pszText && pszText[0] )
				return pszText;
		}

		// Fall back to english
		return m_pKVMOTD->GetString( "title_english", "No Title" );
	}

	return "No Title";
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *CMOTDEntryDefinition::GetText( ELanguage eLang ) 
{ 
	if ( m_pKVMOTD )
	{
		// See if we have a localised block for the specified language.
		const char *pszLanguage = GetLanguageShortName( eLang );
		if ( pszLanguage && pszLanguage[0] )
		{
			const char *pszText = m_pKVMOTD->GetString( CFmtStr( "text_%s", pszLanguage ), NULL );
			if ( pszText && pszText[0] )
				return pszText;
		}

		// Fall back to english
		return m_pKVMOTD->GetString( "text_english", "No text" );
	}

	return "No text";
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
const char *CMOTDEntryDefinition::GetHeaderTitle( ELanguage eLang )
{ 
	if ( m_pKVMOTD )
	{
		// See if we have a localised block for the specified language.
		const char *pszLanguage = GetLanguageShortName( eLang );
		if ( pszLanguage && pszLanguage[0] )
		{
			const char *pszText = m_pKVMOTD->GetString( CFmtStr( "header_%s", pszLanguage ), NULL );
			if ( pszText && pszText[0] )
				return pszText;
		}

		// Fall back to english
		return m_pKVMOTD->GetString( "header_english", "News" );
	}

	return "News";
}

// Sort by ID
int	MOTDEntriesListLess( const CMOTDEntryDefinition *pLhs, const CMOTDEntryDefinition *pRhs )
{
	// This is stupid, sort by the KeyID instead
	return ( pLhs->GetNameInt() > pRhs->GetNameInt() );
}

//-----------------------------------------------------------------------------
// Purpose:	Initializes the loot lists section of the schema
//-----------------------------------------------------------------------------
bool CMOTDManager::BInitMOTDEntries( KeyValues *pKVMOTDEntries, CUtlVector<CUtlString> *pVecErrors )
{
	m_vecMOTDEntries.RemoveAll();

	RTime32 iPrevTime = 0;

	if ( NULL != pKVMOTDEntries )
	{
		FOR_EACH_TRUE_SUBKEY( pKVMOTDEntries, pKVEntry )
		{
			const char *listName = pKVEntry->GetName();

			SCHEMA_INIT_CHECK( listName != NULL, "All MOTD entries must have titles." );

			int idx = m_vecMOTDEntries.AddToTail();
			SCHEMA_INIT_SUBSTEP( m_vecMOTDEntries[idx].BInitFromKV( pKVEntry, pVecErrors ) );

			// Make sure the dates all move forward
			SCHEMA_INIT_CHECK( m_vecMOTDEntries[idx].GetPostTime() > iPrevTime , "MOTD entry '%s' occurs prior to the previous entry.", m_vecMOTDEntries[idx].GetName() );
			iPrevTime = m_vecMOTDEntries[idx].GetPostTime();
		}
	}

	// Then sort all the MOTDs in order of their changed times, so we can easily send them
	m_vecMOTDEntries.Sort( MOTDEntriesListLess );

	return SCHEMA_INIT_SUCCESS();
}

//-----------------------------------------------------------------------------
// Purpose:	Returns the number of MOTD entries we've got after the specified time
//-----------------------------------------------------------------------------
int CMOTDManager::GetNumMOTDAfter( RTime32 iTime )
{
	FOR_EACH_VEC( m_vecMOTDEntries, i )
	{
		if ( m_vecMOTDEntries[i].GetChangedTime() > iTime )
		{
			// We've hit the first MOTD entry after this time. All following posts are assumed after.
			return (m_vecMOTDEntries.Count() - i);
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Remove all unused MOTD: Save memory and whatever
//-----------------------------------------------------------------------------
void CMOTDManager::PurgeUnusedMOTDEntries( KeyValues *pKVMOTDEntries ) 
{
	// Find the latest entry name and remove all others
	int iLargest = -1;
	FOR_EACH_VEC_BACK( m_vecMOTDEntries, i )
	{
		int iMOTDindex = m_vecMOTDEntries[i].GetNameInt();
		if ( iMOTDindex > iLargest )
		{
			iLargest = iMOTDindex;
		}
	}

	FOR_EACH_VEC_BACK( m_vecMOTDEntries, i )
	{
		int iMOTDindex = m_vecMOTDEntries[i].GetNameInt();
		if ( iMOTDindex < iLargest )
		{
			if ( pKVMOTDEntries )
			{
				KeyValues *pKey = pKVMOTDEntries->FindKey( m_vecMOTDEntries[i].GetName() );
				if ( pKey )
				{
					pKVMOTDEntries->RemoveSubKey( pKey );
				}
			}
			m_vecMOTDEntries.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:	Returns the definition for the next blog post after the specified time
//-----------------------------------------------------------------------------
CMOTDEntryDefinition *CMOTDManager::GetNextMOTDAfter( RTime32 iTime )
{
	FOR_EACH_VEC( m_vecMOTDEntries, i )
	{
		if ( m_vecMOTDEntries[i].GetChangedTime() > iTime )
			return &m_vecMOTDEntries[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	
//-----------------------------------------------------------------------------
CMOTDEntryDefinition *CMOTDManager::GetMOTDByIndex( int iIndex )
{
	if ( iIndex < 0 || iIndex > m_vecMOTDEntries.Count() )
		return NULL;
	return &m_vecMOTDEntries[iIndex];
}


