//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Used to create a path that can be followed by NPCs and trains.
//
//=============================================================================//

#include "cbase.h"
#include "trains.h"
#include "entitylist.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CPathCorner : public CPointEntity
{
	DECLARE_CLASS( CPathCorner, CPointEntity );
public:

	void	Spawn( );
	float	GetDelay( void ) { return m_flWait; }
	int		DrawDebugTextOverlays(void);
	void	DrawDebugGeometryOverlays(void);

	// Input handlers	
	void InputSetNextPathCorner( inputdata_t &inputdata );
	void InputInPass( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	float			m_flWait;
	COutputEvent	m_OnPass;
};

LINK_ENTITY_TO_CLASS( path_corner, CPathCorner );


class CPathCornerCrash : public CPathCorner
{
	DECLARE_CLASS( CPathCornerCrash, CPathCorner );
};

LINK_ENTITY_TO_CLASS( path_corner_crash, CPathCornerCrash );


BEGIN_DATADESC( CPathCorner )

	DEFINE_KEYFIELD( m_flWait, FIELD_FLOAT, "wait" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetNextPathCorner", InputSetNextPathCorner),

	// Internal inputs - not exposed in the FGD
	DEFINE_INPUTFUNC( FIELD_VOID, "InPass", InputInPass ),

	// Outputs
	DEFINE_OUTPUT( m_OnPass, "OnPass"),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPathCorner::Spawn( void )
{
	ASSERTSZ(GetEntityName() != NULL_STRING, "path_corner without a targetname");
}


//------------------------------------------------------------------------------
// Purpose: Sets the next path corner by name.
// Input  : String ID name of next path corner.
//-----------------------------------------------------------------------------
void CPathCorner::InputSetNextPathCorner( inputdata_t &inputdata )
{
	m_target = inputdata.value.StringID();
}


//-----------------------------------------------------------------------------
// Purpose: Fired by path followers as they pass the path corner.
//-----------------------------------------------------------------------------
void CPathCorner::InputInPass( inputdata_t &inputdata )
{
	m_OnPass.FireOutput( inputdata.pActivator, inputdata.pCaller, 0);
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPathCorner::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		// --------------
		// Print Target
		// --------------
		char tempstr[255];
		if (m_target!=NULL_STRING) 
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Target: %s",STRING(m_target));
		}
		else
		{
			Q_strncpy(tempstr,"Target:   -  ",sizeof(tempstr));
		}
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of paths
//-----------------------------------------------------------------------------
void CPathCorner::DrawDebugGeometryOverlays(void) 
{
	// ----------------------------------------------
	// Draw line to next target is bbox is selected
	// ----------------------------------------------
	if (m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_ABSBOX_BIT))
	{
		NDebugOverlay::Box(GetAbsOrigin(), Vector(-10,-10,-10), Vector(10,10,10), 255, 100, 100, 0 ,0);

		if (m_target != NULL_STRING)
		{
			CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_target );
			if (pTarget)
			{
				NDebugOverlay::Line(GetAbsOrigin(),pTarget->GetAbsOrigin(),255,100,100,true,0.0);
			}
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}
