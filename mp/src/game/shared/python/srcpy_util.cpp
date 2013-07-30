//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Utility code
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_util.h"
#include "srcpy.h"
#include "ipredictionsystem.h"
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Sets the entity size
//-----------------------------------------------------------------------------
static void PySetMinMaxSize (CBaseEntity *pEnt, const Vector& mins, const Vector& maxs )
{
	for ( int i=0 ; i<3 ; i++ )
	{
		if ( mins[i] > maxs[i] )
		{
			char buf[256];
			Q_snprintf(buf, 256, "%s: backwards mins/maxs", ( pEnt ) ? pEnt->GetDebugName() : "<NULL>");
			PyErr_SetString(PyExc_ValueError, buf );
			throw boost::python::error_already_set(); 
			return;
		}
	}

	Assert( pEnt );

	pEnt->SetCollisionBounds( mins, maxs );
}

//-----------------------------------------------------------------------------
// Sets the model size
//-----------------------------------------------------------------------------
void UTIL_PySetSize( CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax )
{
	PySetMinMaxSize (pEnt, vecMin, vecMax);
}

//-----------------------------------------------------------------------------
// Sets the model to be associated with an entity
//-----------------------------------------------------------------------------
void UTIL_PySetModel( CBaseEntity *pEntity, const char *pModelName )
{
	// check to see if model was properly precached
	int i = modelinfo->GetModelIndex( pModelName );
	if ( i < 0 )
	{
		char buf[256];
		Q_snprintf(buf, 256, "%i/%s - %s:  UTIL_SetModel:  not precached: %s\n", pEntity->entindex(),
			STRING( pEntity->GetEntityName() ),
			pEntity->GetClassname(), pModelName );
		PyErr_SetString(PyExc_ValueError, buf );
		throw boost::python::error_already_set(); 
		return;
	}

	pEntity->SetModelIndex( i ) ;
	pEntity->SetModelName( AllocPooledString( pModelName ) );

	// brush model
	const model_t *mod = modelinfo->GetModel( i );
	if ( mod )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( mod, mins, maxs );
		PySetMinMaxSize (pEntity, mins, maxs);
	}
	else
	{
		PySetMinMaxSize (pEntity, vec3_origin, vec3_origin);
	}

	CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
	if ( pAnimating )
	{
		pAnimating->m_nForceBone = 0;
	}
}

#else


#endif // CLIENT_DLL

#if 0 // TODO
int UTIL_GetModuleIndex( const char *module )
{
	return SrcPySystem()->GetModuleIndex(module);
}

const char *UTIL_GetModuleNameFromIndex( int index )
{
	return SrcPySystem()->GetModuleNameFromIndex(index);
}
#endif // 0

boost::python::list UTIL_ListDir( const char *pPath, const char *pPathID, const char *pWildCard )
{
	if( !pPath || !pWildCard )
		return boost::python::list();

	const char *pFileName;
	char wildcard[MAX_PATH];
	FileFindHandle_t fh;
	boost::python::list result;

	// TODO: Do we need to add a slash in case there is no slash?
	Q_snprintf(wildcard, MAX_PATH, "%s%s", pPath, pWildCard);

	pFileName = filesystem->FindFirstEx(wildcard, pPathID, &fh);
	while( pFileName )
	{
		result.append( boost::python::object( pFileName ) );
		pFileName = filesystem->FindNext(fh);
	}
	filesystem->FindClose(fh);
	return result;
}

// Python fixed up versions
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::object UTIL_PyEntitiesInBox( int listMax, const Vector &mins, const Vector &maxs, 
					   int flagMask, int partitionmask )
{
	int i, n;
	CBaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (CBaseEntity **)malloc(listMax*sizeof(CBaseEntity *));
	CFlaggedEntitiesEnum boxEnum( pList, listMax, flagMask );
	partition->EnumerateElementsInBox( partitionmask, mins, maxs, false, &boxEnum );
	n = boxEnum.GetCount();
	for( i=0; i < n; i++)
		pylist.append(*pList[i]);
	free(pList);
	return pylist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::object UTIL_PyEntitiesInSphere( int listMax, const Vector &center, float radius, 
						  int flagMask, int partitionmask )
{
	int i, n;
	CBaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (CBaseEntity **)malloc(listMax*sizeof(CBaseEntity *));

	CFlaggedEntitiesEnum sphereEnum( pList, listMax, flagMask );
	partition->EnumerateElementsInSphere( partitionmask, center, radius, false, &sphereEnum );
	n = sphereEnum.GetCount();
	for( i=0; i < n; i++)
		pylist.append(*pList[i]);
	free(pList);
	return pylist;
} 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::object UTIL_PyEntitiesAlongRay( int listMax, const PyRay_t &ray, int flagMask, int partitionmask )
{
	int i, n;
	CBaseEntity **pList;
	boost::python::list pylist = boost::python::list();

	pList = (CBaseEntity **)malloc(listMax*sizeof(CBaseEntity *));

	CFlaggedEntitiesEnum rayEnum( pList, listMax, flagMask );
#if defined( CLIENT_DLL )
	partition->EnumerateElementsAlongRay( partitionmask, ray.ToRay(), false, &rayEnum );
#else
	partition->EnumerateElementsAlongRay( partitionmask, ray.ToRay(), false, &rayEnum );
#endif
	n = rayEnum.GetCount();

	for( i=0; i < n; i++)
		pylist.append(*pList[i]);

	free(pList);
	return pylist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::object ConvertIHandleEntity( IHandleEntity *pHandleEntity )
{
	boost::python::object ret;

	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if( pEntity )
	{
		ret = pEntity->GetPyHandle();
	}

	return ret;
}

// Prediction
CBaseEntity const *GetSuppressHost()
{
	IPredictionSystem *sys = IPredictionSystem::g_pPredictionSystems;
	if( sys )
		return sys->GetSuppressHost();
	return NULL;
}
