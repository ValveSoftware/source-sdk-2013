//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>

#include "studio.h"
#include "utlrbtree.h"

extern studiohdr_t *FindOrLoadGroupFile( char const *modelname );

virtualmodel_t *studiohdr_t::GetVirtualModel( void ) const
{
	if (numincludemodels == 0)
	{
		return NULL;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)virtualModel;

	if (pVModel == NULL)
	{
		pVModel = new virtualmodel_t;

		// !!! Set cache handle?  Set pointer to local virtual model??
		virtualModel = (void *)pVModel;

		int group = pVModel->m_group.AddToTail( );
		pVModel->m_group[ group ].cache = (void *)this;
		pVModel->AppendModels( 0, this );
	}

	return pVModel;
}


const studiohdr_t *studiohdr_t::FindModel( void **cache, char const *modelname ) const
{
	studiohdr_t *hdr = (studiohdr_t *)(*cache);

	if (hdr)
	{
		return hdr;
	}

	hdr = FindOrLoadGroupFile( modelname );

	*cache = (void *)hdr;

	return hdr;
}

const studiohdr_t *virtualgroup_t::GetStudioHdr( void ) const
{
	return (studiohdr_t *)cache;
}


byte *studiohdr_t::GetAnimBlock( int i ) const
{
	byte *hdr = (byte *)animblockModel;

	if (!hdr)
	{
		hdr = (byte *)FindOrLoadGroupFile( pszAnimBlockName() );
		animblockModel = hdr;
	}

	return hdr + pAnimBlock( i )->datastart;
}

//-----------------------------------------------------------------------------
// Purpose: Builds up a dictionary of autoplay indices by studiohdr_t *
// NOTE:  This list never gets freed even if the model gets unloaded, but we're in a tool so we can probably live with that
//-----------------------------------------------------------------------------
struct AutoPlayGeneric_t
{
public:

	AutoPlayGeneric_t() :
	  hdr( 0 )
	{
	}

	// Implement copy constructor
	AutoPlayGeneric_t( const AutoPlayGeneric_t& src )
	{
		hdr = src.hdr;
		autoplaylist.EnsureCount( src.autoplaylist.Count() );
		autoplaylist.CopyArray( src.autoplaylist.Base(), src.autoplaylist.Count() );
	}

	static bool AutoPlayGenericLessFunc( const AutoPlayGeneric_t& lhs, const AutoPlayGeneric_t& rhs )
	{
		return lhs.hdr < rhs.hdr;
	}

public:
	// Data
	const studiohdr_t	*hdr;
	CUtlVector< unsigned short >	autoplaylist;
};

// A global array to track this data
static CUtlRBTree< AutoPlayGeneric_t, int >	g_AutoPlayGeneric( 0, 0, AutoPlayGeneric_t::AutoPlayGenericLessFunc );

int	studiohdr_t::GetAutoplayList( unsigned short **pAutoplayList ) const
{
	virtualmodel_t *pVirtualModel = GetVirtualModel();
	if ( pVirtualModel )
	{
		if ( pAutoplayList && pVirtualModel->m_autoplaySequences.Count() )
		{
			*pAutoplayList = pVirtualModel->m_autoplaySequences.Base();
		}
		return pVirtualModel->m_autoplaySequences.Count();
	}

	AutoPlayGeneric_t *pData = NULL;

	// Search for this studiohdr_t ptr in the global list
	AutoPlayGeneric_t search;
	search.hdr = this;
	int index = g_AutoPlayGeneric.Find( search );
	if ( index == g_AutoPlayGeneric.InvalidIndex() )
	{
		// Not there, so add it
		index = g_AutoPlayGeneric.Insert( search );
		pData = &g_AutoPlayGeneric[ index ];
		// And compute the autoplay info this one time
		int autoPlayCount = CountAutoplaySequences();
		pData->autoplaylist.EnsureCount( autoPlayCount );
		CopyAutoplaySequences( pData->autoplaylist.Base(), autoPlayCount );
	}
	else
	{
		// Refer to existing data
		pData = &g_AutoPlayGeneric[ index ];
	}

	// Oops!!!
	if ( !pData )
	{
		return 0;
	}

	// Give back data if it's being requested
	if ( pAutoplayList )
	{
		*pAutoplayList = pData->autoplaylist.Base();
	}
	return pData->autoplaylist.Count();
}
