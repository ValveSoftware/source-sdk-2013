//========= Copyright © 1996-2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: Core types for the response rules -- criteria, responses, rules, and matchers.
//
// $NoKeywords: $
//=============================================================================//

#include "rrbase.h"
#ifdef MAPBASE
#include "convar.h"
#include "mapbase_matchers_base.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace ResponseRules;

#ifdef MAPBASE
ConVar rr_bucket_name_who( "rr_bucket_name_who", "Classname", FCVAR_NONE, "The name of the criteria to use for the 'Who' bucket." ); // Default changed to "Classname" for HL2
ConVar rr_bucket_name_concept( "rr_bucket_name_concept", "Concept", FCVAR_NONE, "The name of the criteria to use for the 'Concept' bucket." );
ConVar rr_bucket_name_subject( "rr_bucket_name_subject", "Subject", FCVAR_NONE, "The name of the criteria to use for the 'Subject' bucket." );
#endif




ResponseRulePartition::ResponseRulePartition()
{
	Assert(true);
	COMPILE_TIME_ASSERT( kIDX_ELEM_MASK < (1 << 16) );
	COMPILE_TIME_ASSERT( (kIDX_ELEM_MASK & (kIDX_ELEM_MASK + 1)) == 0 ); /// assert is power of two minus one
}

ResponseRulePartition::~ResponseRulePartition()
{
	RemoveAll();
}

ResponseRulePartition::tIndex ResponseRulePartition::IndexFromDictElem( tRuleDict* pDict, int elem )
{
	Assert( pDict );
	// If this fails, you've tried to build an index for a rule that's not stored
	// in this partition
	Assert( pDict >= m_RuleParts && pDict < m_RuleParts + N_RESPONSE_PARTITIONS );
	AssertMsg1( elem <= kIDX_ELEM_MASK, "A rule dictionary has %d elements; this exceeds the 255 that can be packed into an index.\n", elem );

	int bucket = pDict - m_RuleParts;
	return ( bucket << 16 ) | ( elem & kIDX_ELEM_MASK ); // this is a native op on PPC
}


char const *ResponseRulePartition::GetElementName( const tIndex &i ) const
{
	Assert( IsValid(i) );
	return m_RuleParts[ BucketFromIdx(i) ].GetElementName( PartFromIdx(i) );
}


Rule *ResponseRulePartition::FindByName( char const *name ) const
{
	int count;

	for ( int bukkit = 0 ; bukkit < N_RESPONSE_PARTITIONS ; ++bukkit )
	{
		count = m_RuleParts[bukkit].Count();
		for ( int i = 0; i < count; ++i )
		{
			if (V_strncmp( m_RuleParts[bukkit].GetElementName(i), name, CRR_Response::MAX_RULE_NAME ) == 0)
				return m_RuleParts[bukkit][i];
		}
	}

	return NULL;
}


int ResponseRulePartition::Count( void )
{
	int count = 0 ;
	for ( int bukkit = 0 ; bukkit < N_RESPONSE_PARTITIONS ; ++bukkit )
	{
		count += m_RuleParts[bukkit].Count();
	}

	return count;
}

void ResponseRulePartition::RemoveAll( void )
{
	for ( int bukkit = 0 ; bukkit < N_RESPONSE_PARTITIONS ; ++bukkit )
	{
		for ( int i = m_RuleParts[bukkit].FirstInorder(); i != m_RuleParts[bukkit].InvalidIndex(); i = m_RuleParts[bukkit].NextInorder( i ) )
		{
			delete m_RuleParts[bukkit][ i ];
		}
		m_RuleParts[bukkit].RemoveAll();
	}
}

#ifdef MAPBASE
void ResponseRulePartition::PurgeAndDeleteElements()
{
	for ( int bukkit = 0 ; bukkit < N_RESPONSE_PARTITIONS ; ++bukkit )
	{
		for ( int i = m_RuleParts[bukkit].FirstInorder(); i != m_RuleParts[bukkit].InvalidIndex(); i = m_RuleParts[bukkit].NextInorder( i ) )
		{
			delete m_RuleParts[bukkit][ i ];
		}
		m_RuleParts[bukkit].Purge();
	}
}
#endif

// don't bucket "subject" criteria that prefix with operators, since stripping all that out again would
// be a big pain, and the most important rules that need subjects are tlk_remarks anyway. 
static inline bool CanBucketBySubject( const char * RESTRICT pszSubject )
{
	return  pszSubject && 
		( ( pszSubject[0] >= 'A' && pszSubject[0] <= 'Z' ) ||
		  ( pszSubject[0] >= 'a' && pszSubject[0] <= 'z' ) )
#ifdef MAPBASE
		&& !Matcher_ContainsWildcard( pszSubject )
#endif
		;
}

ResponseRulePartition::tRuleDict &ResponseRulePartition::GetDictForRule( CResponseSystem *pSystem, Rule *pRule )
{
#ifdef MAPBASE
	const static CUtlSymbol kWHO = CriteriaSet::ComputeCriteriaSymbol( rr_bucket_name_who.GetString() );
	const static CUtlSymbol kCONCEPT = CriteriaSet::ComputeCriteriaSymbol( rr_bucket_name_concept.GetString() );
	const static CUtlSymbol kSUBJECT = CriteriaSet::ComputeCriteriaSymbol( rr_bucket_name_subject.GetString() );
#else
	const static CUtlSymbol kWHO = CriteriaSet::ComputeCriteriaSymbol("Who");
	const static CUtlSymbol kCONCEPT = CriteriaSet::ComputeCriteriaSymbol("Concept");
	const static CUtlSymbol kSUBJECT = CriteriaSet::ComputeCriteriaSymbol("Subject");
#endif

	const char *pszSpeaker = pRule->GetValueForRuleCriterionByName( pSystem, kWHO );
	const char *pszConcept = pRule->GetValueForRuleCriterionByName( pSystem, kCONCEPT );
	const Criteria *pSubjCrit = pRule->GetPointerForRuleCriterionByName( pSystem, kSUBJECT );

	return m_RuleParts[ 
		GetBucketForSpeakerAndConcept( pszSpeaker, pszConcept, 
			( pSubjCrit && pSubjCrit->required && CanBucketBySubject(pSubjCrit->value) ) ? 
			pSubjCrit->value : 
		NULL ) 
	];
}


void ResponseRulePartition::GetDictsForCriteria( CUtlVectorFixed< ResponseRulePartition::tRuleDict *, 2 > *pResult, const CriteriaSet &criteria )
{
	pResult->RemoveAll();
	pResult->EnsureCapacity( 2 );

	// get the values for Who and Concept, which are what we bucket on
	int speakerIdx = criteria.FindCriterionIndex( "Who" );
	const char *pszSpeaker = speakerIdx != -1 ? criteria.GetValue( speakerIdx ) : NULL ;

	int conceptIdx = criteria.FindCriterionIndex( "Concept" );
	const char *pszConcept = conceptIdx != -1 ? criteria.GetValue( conceptIdx ) : NULL ;

	int subjectIdx = criteria.FindCriterionIndex( "Subject" );
	const char *pszSubject = subjectIdx != -1 ? criteria.GetValue( subjectIdx ) : NULL ;

	pResult->AddToTail( &m_RuleParts[ GetBucketForSpeakerAndConcept(pszSpeaker, pszConcept, pszSubject) ] );
	// also try the rules not specifying subject
	pResult->AddToTail( &m_RuleParts[ GetBucketForSpeakerAndConcept(pszSpeaker, pszConcept, NULL) ] );

}