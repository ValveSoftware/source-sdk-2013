//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "SelectionTool.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SERVER_USES_VGUI

using namespace vgui;


//--------------------------------------------------------------------------------------------------------
SelectionToolPanel::SelectionToolPanel( vgui::Panel *parent, const char *toolName ) : CNavUIToolPanel( parent, toolName )
{
	LoadControlSettings( "Resource/UI/NavTools/SelectionTool.res" );
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::Init( void )
{
	m_dragType = DRAG_NONE;
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::Shutdown( void )
{
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::PerformLayout( void )
{
	Panel *parent = GetParent();
	if ( parent )
	{
		int w, h;
		parent->GetSize( w, h );
		SetBounds( 0, 0, w, h );
	}

	BaseClass::PerformLayout();
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::OnCommand( const char *command )
{
	if ( FStrEq( "FloodSelect", command ) )
	{
		TheNavUI()->SetLeftClickAction( "Selection::Flood", "Flood Select" );
	}

	BaseClass::OnCommand( command );
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::OnCursorMoved( int x, int y )
{
	CNavArea *area = TheNavMesh->GetSelectedArea();
	if ( area )
	{
		bool selected = TheNavMesh->IsInSelectedSet( area );

		if ( selected && m_dragType == DRAG_UNSELECT )
		{
			TheNavMesh->RemoveFromSelectedSet( area );
			TheNavUI()->PlaySound( "EDIT_END_AREA.Creating" );
		}
		else if ( !selected && m_dragType == DRAG_SELECT )
		{
			TheNavMesh->AddToSelectedSet( area );
			TheNavUI()->PlaySound( "EDIT_END_AREA.Creating" );
		}
	}
	
	BaseClass::OnCursorMoved( x, y );
}


//--------------------------------------------------------------------------------------------------------
class FloodSelectionCollector
{
public:
	FloodSelectionCollector( SelectionToolPanel *panel )
	{
		m_count = 0;
		m_panel = panel;
	}

	bool operator() ( CNavArea *area )
	{
		// already selected areas terminate flood select
		if ( TheNavMesh->IsInSelectedSet( area ) )
			return false;

		if ( !m_panel->IsFloodSelectable( area ) )
			return false;

		TheNavMesh->AddToSelectedSet( area );
		++m_count;

		return true;
	}

	int m_count;

private:
	SelectionToolPanel *m_panel;
};


//--------------------------------------------------------------------------------------------------------
bool SelectionToolPanel::IsFloodSelectable( CNavArea *area )
{
	if ( IsCheckButtonChecked( "Place" ) )
	{
		if ( m_floodStartArea->GetPlace() != area->GetPlace() )
		{
			return false;
		}
	}

	if ( IsCheckButtonChecked( "Jump" ) )
	{
		if ( (m_floodStartArea->GetAttributes() & NAV_MESH_JUMP) != (area->GetAttributes() & NAV_MESH_JUMP) )
		{
			return false;
		}
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::FloodSelect( void )
{
	m_floodStartArea = TheNavMesh->GetSelectedArea();
	if ( m_floodStartArea )
	{
		TheNavUI()->PlaySound( "EDIT_DELETE" );

		int connections = INCLUDE_BLOCKED_AREAS;

		if ( IsCheckButtonChecked( "Incoming" ) )
		{
			connections = connections | INCLUDE_INCOMING_CONNECTIONS;
		}

		if ( !IsCheckButtonChecked( "Outgoing" ) )
		{
			connections = connections | EXCLUDE_OUTGOING_CONNECTIONS;
		}

		// collect all areas connected to this area
		FloodSelectionCollector collector( this );
		SearchSurroundingAreas( m_floodStartArea, m_floodStartArea->GetCenter(), collector, -1, connections );

		Msg( "Selected %d areas.\n", collector.m_count );
	}
	m_floodStartArea = NULL;

	TheNavMesh->SetMarkedArea( NULL );			// unmark the mark area
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::StartLeftClickAction( const char *actionName )
{
	if ( FStrEq( actionName, "Selection::Flood" ) )
	{
		TheNavUI()->SetLeftClickAction( "", "" );
		FloodSelect();
	}
	else if ( FStrEq( actionName, "Selection::Select" ) )
	{
		CNavArea *area = TheNavMesh->GetSelectedArea();
		if ( area )
		{
			if ( TheNavMesh->IsInSelectedSet( area ) )
			{
				TheNavMesh->RemoveFromSelectedSet( area );
				m_dragType = DRAG_UNSELECT;
			}
			else
			{
				TheNavMesh->AddToSelectedSet( area );
				m_dragType = DRAG_SELECT;
			}
			TheNavUI()->PlaySound( "EDIT_END_AREA.Creating" );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::FinishLeftClickAction( const char *actionName )
{
	m_dragType = DRAG_NONE;
}


//--------------------------------------------------------------------------------------------------------
void SelectionToolPanel::StartRightClickAction( const char *actionName )
{
	if ( m_dragType != DRAG_NONE )
	{
		TheNavUI()->PlaySound( "EDIT_END_AREA.Creating" );
		m_dragType = DRAG_NONE;
		return;
	}

	if ( FStrEq( actionName, "Selection::ClearSelection" ) )
	{
		if ( FStrEq( TheNavUI()->GetLeftClickAction(), "Selection::Select" ) )
		{
			if ( TheNavMesh->GetSelecteSetSize() > 0 )
			{
				TheNavUI()->PlaySound( "EDIT_END_AREA.Creating" );
			}
			TheNavMesh->ClearSelectedSet();
		}
		else
		{
			TheNavUI()->SetLeftClickAction( "", "" );
		}
	}
}

#endif // SERVER_USES_VGUI


//--------------------------------------------------------------------------------------------------------
