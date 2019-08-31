//-----------------------------------------------------------------------------
// class CPointEntityFinder
//
// Purpose: Finds an entity using a specified heuristic and outputs it as !caller
//			with the OnFoundEntity output.
//-----------------------------------------------------------------------------

#include "cbase.h"
#include "filters.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


enum EntFinderMethod_t
{
	ENT_FIND_METHOD_NEAREST = 0,
	ENT_FIND_METHOD_FARTHEST,
	ENT_FIND_METHOD_RANDOM,
};

class CPointEntityFinder : public CBaseEntity
{
	void Activate( void );

	DECLARE_CLASS( CPointEntityFinder, CBaseEntity );

private:

	EHANDLE						m_hEntity;
	string_t					m_iFilterName;
	CHandle<class CBaseFilter>	m_hFilter;
	string_t					m_iRefName;
	EHANDLE						m_hReference;

	EntFinderMethod_t			m_FindMethod;

	void FindEntity( void );
	void FindByDistance( void );
	void FindByRandom( void );

	// Input handlers
	void InputFindEntity( inputdata_t &inputdata );

	// Output handlers
	COutputEvent m_OnFoundEntity;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( point_entity_finder, CPointEntityFinder );

BEGIN_DATADESC( CPointEntityFinder )

	DEFINE_KEYFIELD(	m_FindMethod,	FIELD_INTEGER,	"method" ),
	DEFINE_KEYFIELD(	m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_FIELD(		m_hFilter,		FIELD_EHANDLE ),
	DEFINE_KEYFIELD(	m_iRefName,		FIELD_STRING,	"referencename" ),
	DEFINE_FIELD(		m_hReference,	FIELD_EHANDLE ),

	DEFINE_OUTPUT( m_OnFoundEntity, "OnFoundEntity" ),

	//---------------------------------

	DEFINE_INPUTFUNC( FIELD_VOID, "FindEntity", InputFindEntity ),

END_DATADESC()


void CPointEntityFinder::Activate( void )
{
	// Get the filter, if it exists.
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}


void CPointEntityFinder::FindEntity( void )
{
	// Get the reference entity, if it exists.
	if (m_iRefName != NULL_STRING)
	{
		m_hReference = gEntList.FindEntityByName( NULL, m_iRefName );
	}

	switch ( m_FindMethod )
	{

	case ( ENT_FIND_METHOD_NEAREST ):
		FindByDistance();
		break;
	case ( ENT_FIND_METHOD_FARTHEST ):
		FindByDistance();
		break;
	case ( ENT_FIND_METHOD_RANDOM ):
		FindByRandom();
		break;
	}
}

void CPointEntityFinder::FindByDistance( void )
{
	m_hEntity = NULL;
	CBaseFilter *pFilter = m_hFilter.Get();

// go through each entity and determine whether it's closer or farther from the current entity.  Pick according to Method selected.

	float flBestDist = 0;
	CBaseEntity *pEntity = gEntList.FirstEnt();
	while ( pEntity )
	{
		if ( FStrEq( STRING( pEntity->m_iClassname ), "worldspawn" ) 
			|| FStrEq( STRING( pEntity->m_iClassname ), "soundent" ) 
			|| FStrEq( STRING( pEntity->m_iClassname ), "player_manager" ) 
			|| FStrEq( STRING( pEntity->m_iClassname ), "bodyque" ) 
			|| FStrEq( STRING( pEntity->m_iClassname ), "ai_network" ) 
			|| pEntity == this
			|| ( pFilter && !( pFilter->PassesFilter( this, pEntity ) ) ) )	   
		{
			pEntity = gEntList.NextEnt( pEntity );
			continue;
		}

		// if we have a reference entity, use that, otherwise, check against 'this'
		Vector vecStart;
		if ( m_hReference )
		{
			vecStart = m_hReference->GetAbsOrigin();
		}
		else
		{
			vecStart = GetAbsOrigin();
		}

		// init m_hEntity with a valid entity.
		if (m_hEntity == NULL )
		{
			m_hEntity = pEntity;
			flBestDist = ( pEntity->GetAbsOrigin() - vecStart ).LengthSqr();
		}

		float flNewDist = ( pEntity->GetAbsOrigin() - vecStart ).LengthSqr();

		switch ( m_FindMethod )
		{

		case ( ENT_FIND_METHOD_NEAREST ):
			if ( flNewDist < flBestDist )
			{
				m_hEntity = pEntity;
				flBestDist = flNewDist;
			}
			break;

		case ( ENT_FIND_METHOD_FARTHEST ):
			if ( flNewDist > flBestDist )
			{
				m_hEntity = pEntity;
				flBestDist = flNewDist;
			}
			break;

		default:
			Assert( false );
			break;
		}

		pEntity = gEntList.NextEnt( pEntity );
	}
}

void CPointEntityFinder::FindByRandom( void )
{
	// TODO: optimize the case where there is no filter
	m_hEntity = NULL;
	CBaseFilter *pFilter = m_hFilter.Get();
	CUtlVector<CBaseEntity *> ValidEnts;

	CBaseEntity *pEntity = gEntList.FirstEnt();
	do	   // note all valid entities.
	{
		if ( pFilter &&  pFilter->PassesFilter( this, pEntity ) )
		{
			ValidEnts.AddToTail( pEntity );
		}

		pEntity = gEntList.NextEnt( pEntity );

	} while ( pEntity );

	// pick one at random
	if ( ValidEnts.Count() != 0 )
	{
		m_hEntity = ValidEnts[ RandomInt( 0, ValidEnts.Count() - 1 )];
	}
}

void CPointEntityFinder::InputFindEntity( inputdata_t &inputdata )
{
	FindEntity();

	m_OnFoundEntity.FireOutput( inputdata.pActivator, m_hEntity );
}