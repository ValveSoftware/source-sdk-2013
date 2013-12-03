//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "tier0/platform.h"
#include "interval.h"
#include "tier1/strtools.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pString - 
// Output : interval_t
//-----------------------------------------------------------------------------
interval_t ReadInterval( const char *pString )
{
	interval_t tmp;
	
	tmp.start = 0;
	tmp.range = 0;

	char tempString[128];
	Q_strncpy( tempString, pString, sizeof(tempString) );
	
	char *token = strtok( tempString, "," );
	if ( token )
	{
		tmp.start = atof( token );
		token = strtok( NULL, "," );
		if ( token )
		{
			tmp.range = atof( token ) - tmp.start;
		}
	}

	return tmp;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &interval - 
// Output : float
//-----------------------------------------------------------------------------
float RandomInterval( const interval_t &interval )
{
	float out = interval.start;
	if ( interval.range != 0 )
	{
		out += RandomFloat( 0, interval.range );
	}

	return out;
}
