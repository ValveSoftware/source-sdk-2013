//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_xp_source.h"
#include "gcsdk/enumutils.h"
#include "schemainitutils.h"
// memdbgon must be the last include file in a .cpp file!!!

#include "tier0/memdbgon.h"

using namespace GCSDK;

CXPSource::CXPSource()
{
	Obj().set_account_id( 0 );
	Obj().set_amount( 0 );
	Obj().set_match_id( 0 );
	Obj().set_match_group( ETFMatchGroup::k_eTFMatchGroup_Invalid );
	// Explicitly not setting type()
}


