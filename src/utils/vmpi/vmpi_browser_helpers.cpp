//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "tier1/strtools.h"
#include "vmpi_browser_helpers.h"



void FormatTimeString( unsigned long nInputSeconds, char *timeStr, int outLen )
{
	// Make a string to say how long the thing has been running.
	unsigned long minutes = nInputSeconds / 60;
	unsigned long nSeconds = nInputSeconds - minutes * 60;

	unsigned long hours = minutes / 60;
	minutes -= hours * 60;

	unsigned long days = hours / 24;
	hours -= days * 24;

	if ( days && hours )
	{
		Q_snprintf( timeStr, outLen, "%dd %dh %dm", days, hours, minutes );
	}
	else if ( hours )
	{
		Q_snprintf( timeStr, outLen, "%dh %dm", hours, minutes );
	}
	else if ( minutes )
	{
		Q_snprintf( timeStr, outLen, "%dm %ds", minutes, nSeconds );
	}
	else
	{
		Q_snprintf( timeStr, outLen, "%d seconds", nSeconds );
	}		
}


