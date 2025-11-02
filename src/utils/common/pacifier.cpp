//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>
#include "basetypes.h"
#include "pacifier.h"
#include "tier0/dbg.h"


static int g_LastPacifierDrawn = -1;
static bool g_bPacifierSuppressed = false;
static float g_start;
static float g_end;

#define clamp(a,b,c) ( (a) > (c) ? (c) : ( (a) < (b) ? (b) : (a) ) )


void StartPacifier( char const *pPrefix )
{
	g_start = Plat_FloatTime();
	Msg( "%s", pPrefix );
	g_LastPacifierDrawn = -1;
	UpdatePacifier( 0.001f );
}


void UpdatePacifier( float flPercent )
{
	int iCur = (int)(flPercent * 40.0f);
	iCur = clamp( iCur, g_LastPacifierDrawn, 40 );
	
	if( iCur != g_LastPacifierDrawn && !g_bPacifierSuppressed )
	{
		for( int i=g_LastPacifierDrawn+1; i <= iCur; i++ )
		{
			if ( !( i % 4 ) )
			{
				Msg("%d", i/4);
			}
			else
			{
				if( i != 40 )
				{
					Msg(".");
				}
			}
		}
		
		g_LastPacifierDrawn = iCur;
	}
}


void EndPacifier( bool bCarriageReturn )
{
	UpdatePacifier(1);
	g_end = Plat_FloatTime();
	if (bCarriageReturn && !g_bPacifierSuppressed)
		Msg(" (%.1fs)\n", g_end - g_start);
}


void SuppressPacifier( bool bSuppress )
{
	g_bPacifierSuppressed = bSuppress;
}
