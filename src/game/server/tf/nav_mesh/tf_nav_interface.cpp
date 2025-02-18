//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements nav interface entity.  Used by maps to do various things
//			with the nav mesh
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CPointNavInterface : public CPointEntity
{
	DECLARE_CLASS( CPointNavInterface, CPointEntity );

public:

	// Input handlers
	void RecomputeBlockers(inputdata_t &inputdata);
	
	DECLARE_DATADESC();
};

BEGIN_DATADESC( CPointNavInterface )

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "RecomputeBlockers", RecomputeBlockers ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_point_nav_interface, CPointNavInterface );


void CPointNavInterface::RecomputeBlockers( inputdata_t &inputdata )
{
	CTFNavMesh* pTFNavMesh = dynamic_cast<CTFNavMesh*>( TheNavMesh );
	Assert( pTFNavMesh );
	if( pTFNavMesh )
	{
		pTFNavMesh->ScheduleRecomputationOfInternalData( CTFNavMesh::MAP_LOGIC );
	}
}
