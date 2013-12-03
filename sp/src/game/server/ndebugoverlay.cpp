//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Namespace for functions dealing with Debug Overlays
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "mathlib/mathlib.h"
#include "player.h"
#include "ndebugoverlay.h"
#include "wcedit.h"

#ifdef POSIX
#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_networkmanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define		NUM_DEBUG_OVERLAY_LINES		20

int				m_nDebugOverlayIndex	= -1;
OverlayLine_t*	m_debugOverlayLine[NUM_DEBUG_OVERLAY_LINES];

//-----------------------------------------------------------------------------
// Purpose: Return the old overlay line from the overlay pool
//-----------------------------------------------------------------------------
OverlayLine_t* GetDebugOverlayLine(void)
{
	// Make casing pool if not initialized
	if (m_nDebugOverlayIndex == -1)
	{
		for (int i=0;i<NUM_DEBUG_OVERLAY_LINES;i++)
		{
			m_debugOverlayLine[i]				= new OverlayLine_t;
			m_debugOverlayLine[i]->noDepthTest  = true;
			m_debugOverlayLine[i]->draw			= false;
		}
		m_nDebugOverlayIndex = 0;
	}

	int id;

	m_nDebugOverlayIndex++;
	if (m_nDebugOverlayIndex == NUM_DEBUG_OVERLAY_LINES)
	{
		m_nDebugOverlayIndex = 0;
		id			  = NUM_DEBUG_OVERLAY_LINES-1;

	}
	else
	{
		id = m_nDebugOverlayIndex-1;
	}
	return m_debugOverlayLine[id];
}

//-----------------------------------------------------------------------------
// Purpose: Adds a debug line to be drawn on the screen
// Input  : If testLOS is true, color is based on line of sight test
// Output : 
//-----------------------------------------------------------------------------
void UTIL_AddDebugLine(const Vector &startPos, const Vector &endPos, bool noDepthTest, bool testLOS) 
{
	OverlayLine_t* debugLine = GetDebugOverlayLine();

	debugLine->origin		= startPos;
	debugLine->dest			= endPos;
	debugLine->noDepthTest	= noDepthTest;
	debugLine->draw = true;

	if (testLOS)
	{
		trace_t tr;
		UTIL_TraceLine ( debugLine->origin, debugLine->dest, MASK_BLOCKLOS, NULL, COLLISION_GROUP_NONE, &tr );
		if (tr.startsolid || tr.fraction < 1.0)
		{
			debugLine->r = 255;
			debugLine->g = 0;
			debugLine->b = 0;
			return;
		}
	}

	debugLine->r = 255;
	debugLine->g = 255;
	debugLine->b = 255;
}

//-----------------------------------------------------------------------------
// Purpose: Returns z value of floor below given point (up to 384 inches below)
//-----------------------------------------------------------------------------
float GetLongFloorZ(const Vector &origin) 
{
	// trace to the ground, then pop up 8 units and place node there to make it
	// easier for them to connect (think stairs, chairs, and bumps in the floor).
	// After the routing is done, push them back down.
	//
	trace_t	tr;
	UTIL_TraceLine ( origin,
					 origin - Vector ( 0, 0, 2048 ),
					 MASK_NPCSOLID_BRUSHONLY,
					 NULL,
					 COLLISION_GROUP_NONE,
					 &tr );

	// This trace is ONLY used if we hit an entity flagged with FL_WORLDBRUSH
	trace_t	trEnt;
	UTIL_TraceLine ( origin,
					 origin - Vector ( 0, 0, 2048 ),
					 MASK_NPCSOLID,
					 NULL,
					 COLLISION_GROUP_NONE,
					 &trEnt );

	
	// Did we hit something closer than the floor?
	if ( trEnt.fraction < tr.fraction )
	{
		// If it was a world brush entity, copy the node location
		if ( trEnt.m_pEnt )
		{
			CBaseEntity *e = trEnt.m_pEnt;
			if ( e && (e->GetFlags() & FL_WORLDBRUSH) )
			{
				tr.endpos = trEnt.endpos;
			}
		}
	}

	return tr.endpos.z;
}

//------------------------------------------------------------------------------
// Purpose : Draws a large crosshair at flCrossDistance from the debug player
//			 with tick marks
//------------------------------------------------------------------------------
void UTIL_DrawPositioningOverlay( float flCrossDistance )
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(CBaseEntity::m_nDebugPlayer);

	if (!pPlayer) 
	{
		return;
	}

	Vector pRight;
	pPlayer->EyeVectors( NULL, &pRight, NULL );

#ifdef _WIN32
	Vector topPos		= NWCEdit::AirNodePlacementPosition();
#else
        Vector pForward;
        pPlayer->EyeVectors( &pForward );

        Vector  floorVec = pForward;
        floorVec.z = 0;
        VectorNormalize( floorVec );
        VectorNormalize( pForward );

        float cosAngle = DotProduct(floorVec,pForward);

        float lookDist = g_pAINetworkManager->GetEditOps()->m_flAirEditDistance/cosAngle;
        Vector topPos = pPlayer->EyePosition()+pForward * lookDist;
#endif

	Vector bottomPos	= topPos;
	bottomPos.z			= GetLongFloorZ(bottomPos);

	// Make sure we can see the target pos
	trace_t tr;
	Vector	endPos;
	UTIL_TraceLine(pPlayer->EyePosition(), topPos, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction == 1.0)
	{
		Vector rightTrace = topPos + pRight*400;
		float  traceLen	  = (topPos - rightTrace).Length();
		UTIL_TraceLine(topPos, rightTrace, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
		endPos = topPos+(pRight*traceLen*tr.fraction);
		NDebugOverlay::DrawTickMarkedLine(topPos, endPos, 24.0, 5, 255,0,0,false,0);

		Vector leftTrace	= topPos - pRight*400;
		traceLen			= (topPos - leftTrace).Length();
		UTIL_TraceLine(topPos, leftTrace, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
		endPos				= topPos-(pRight*traceLen*tr.fraction);
		NDebugOverlay::DrawTickMarkedLine(topPos, endPos, 24.0, 5, 255,0,0,false,0);

		Vector upTrace		= topPos + Vector(0,0,1)*400;
		traceLen			= (topPos - upTrace).Length();
		UTIL_TraceLine(topPos, upTrace, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
		endPos				= topPos+(Vector(0,0,1)*traceLen*tr.fraction);
		NDebugOverlay::DrawTickMarkedLine(bottomPos, endPos, 24.0, 5, 255,0,0,false,0);

		// Draw white cross at center
		NDebugOverlay::Cross3D(topPos, Vector(-2,-2,-2), Vector(2,2,2), 255,255,255, true, 0);
	}
	else
	{
		// Draw warning  cross at center
		NDebugOverlay::Cross3D(topPos, Vector(-2,-2,-2), Vector(2,2,2), 255,100,100, true, 0 );
	}
	/*  Don't show distance for now.
	char text[25];
	Q_snprintf(text,sizeof(text),"%3.0f",flCrossDistance/12.0);
	Vector textPos = topPos - pRight*16 + Vector(0,0,10);
	NDebugOverlay::Text( textPos, text, true, 0 );
	*/
}

//------------------------------------------------------------------------------
// Purpose : Draw all overlay lines in the list
//------------------------------------------------------------------------------
void UTIL_DrawOverlayLines(void) 
{
	if (m_nDebugOverlayIndex != -1)
	{
		for (int i=0;i<NUM_DEBUG_OVERLAY_LINES;i++)
		{
			if (m_debugOverlayLine[i]->draw)
			{
				NDebugOverlay::Line(m_debugOverlayLine[i]->origin,
									 m_debugOverlayLine[i]->dest,
									 m_debugOverlayLine[i]->r,
									 m_debugOverlayLine[i]->g,
									 m_debugOverlayLine[i]->b,
									 m_debugOverlayLine[i]->noDepthTest,0);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add an overlay line with padding on the start and end
//-----------------------------------------------------------------------------
void DebugDrawLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration )
{
	NDebugOverlay::Line( vecAbsStart + Vector( 0,0,0.1), vecAbsEnd + Vector( 0,0,0.1), r,g,b, test, duration );
}

//-----------------------------------------------------------------------------
// Purpose: Allow all debug overlays to be cleared at once
//-----------------------------------------------------------------------------
CON_COMMAND( clear_debug_overlays, "clears debug overlays" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBaseEntity *pEntity = gEntList.FirstEnt();
	
	// Clear all entities of their debug overlays
	while ( pEntity )
	{
		pEntity->m_debugOverlays = 0;
		// UNDONE: Clear out / expire timed overlays?
		pEntity = gEntList.NextEnt( pEntity );
	}
	
	// Clear all engine overlays
	if ( debugoverlay )
	{
		debugoverlay->ClearAllOverlays();
	}
}
