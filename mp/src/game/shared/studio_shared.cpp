//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "studio.h"
#include "engine/ivmodelinfo.h"
#include "utlsymbol.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

////////////////////////////////////////////////////////////////////////
const studiohdr_t *studiohdr_t::FindModel( void **cache, char const *modelname ) const
{
	return modelinfo->FindModel( this, cache, modelname );
}

virtualmodel_t *studiohdr_t::GetVirtualModel( void ) const
{
	if ( numincludemodels == 0 )
		return NULL;
	return modelinfo->GetVirtualModel( this );
}

const studiohdr_t *virtualgroup_t::GetStudioHdr( ) const
{
	return modelinfo->FindModel( this->cache );
}


byte *studiohdr_t::GetAnimBlock( int iBlock ) const
{
	return modelinfo->GetAnimBlock( this, iBlock );
}

int	studiohdr_t::GetAutoplayList( unsigned short **pOut ) const
{
	return modelinfo->GetAutoplayList( this, pOut );
}
