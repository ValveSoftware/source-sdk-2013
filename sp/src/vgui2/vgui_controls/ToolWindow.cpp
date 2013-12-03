//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include "vgui/IInput.h"
#include "vgui/MouseCode.h"
#include "vgui/ISurface.h"

#include <vgui_controls/ToolWindow.h>
#include <vgui_controls/PropertySheet.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

CUtlVector< ToolWindow * > ToolWindow::s_ToolWindows;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : int
//-----------------------------------------------------------------------------
int ToolWindow::GetToolWindowCount()
{
	return s_ToolWindows.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : PropertySheet
//-----------------------------------------------------------------------------
ToolWindow *ToolWindow::GetToolWindow( int index )
{
	return s_ToolWindows[ index ];
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ToolWindow::ToolWindow(
	Panel *parent, 
	bool contextlabel,
	IToolWindowFactory *factory /*= 0*/, 
	Panel *page /*= NULL*/, 
	char const *title /*= NULL */,
	bool contextMenu /*=false*/,
	bool inGlobalList /*= true*/ ) : BaseClass( parent, "ToolWindow" ),
	m_pFactory( factory )
{
	if ( inGlobalList )
	{
		s_ToolWindows.AddToTail( this );
	}

	// create the property sheet
	m_pPropertySheet = new PropertySheet(this, "ToolWindowSheet", true );
	m_pPropertySheet->ShowContextButtons( contextlabel );
	m_pPropertySheet->AddPage( page, title, 0, contextMenu );
	m_pPropertySheet->AddActionSignalTarget(this);
	m_pPropertySheet->SetSmallTabs( true );
	m_pPropertySheet->SetKBNavigationEnabled( false );

	SetSmallCaption( true );

	SetMenuButtonResponsive(false);
	SetMinimizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetMoveable( true );
	SetSizeable(true);

	SetClipToParent( false );
	SetVisible( true );

	SetDeleteSelfOnClose( true );

	SetTitle( "", false );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ToolWindow::~ToolWindow()
{
	// These don't actually kill the children of the property sheet
	m_pPropertySheet->RemoveAllPages();

	s_ToolWindows.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Pass through to sheet
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ToolWindow::IsDraggableTabContainer() const
{
	return m_pPropertySheet->IsDraggableTab();
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the PropertySheet this dialog encapsulates
// Output : PropertySheet *
//-----------------------------------------------------------------------------
PropertySheet *ToolWindow::GetPropertySheet()
{
	return m_pPropertySheet;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a pointer to the currently active page.
// Output : Panel
//-----------------------------------------------------------------------------
Panel *ToolWindow::GetActivePage()
{
	return m_pPropertySheet->GetActivePage();
}

void ToolWindow::SetActivePage( Panel *page )
{
	m_pPropertySheet->SetActivePage( page );
}

//-----------------------------------------------------------------------------
// Purpose: Wrapped function
//-----------------------------------------------------------------------------
void ToolWindow::AddPage(Panel *page, const char *title, bool contextMenu)
{
	m_pPropertySheet->AddPage(page, title, 0, contextMenu );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *page - 
//-----------------------------------------------------------------------------
void ToolWindow::RemovePage( Panel *page )
{
	m_pPropertySheet->RemovePage( page );
	if ( m_pPropertySheet->GetNumPages() == 0 )
	{
		MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the sheet
//-----------------------------------------------------------------------------
void ToolWindow::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	m_pPropertySheet->SetBounds(x, y, wide, tall);
	m_pPropertySheet->InvalidateLayout(); // tell the propertysheet to redraw!
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Overrides build mode so it edits the sub panel
//-----------------------------------------------------------------------------
void ToolWindow::ActivateBuildMode()
{
	// no subpanel, no build mode
	EditablePanel *panel = dynamic_cast<EditablePanel *>(GetActivePage());
	if (!panel)
		return;

	panel->ActivateBuildMode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ToolWindow::RequestFocus(int direction)
{
    m_pPropertySheet->RequestFocus(direction);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *factory - 
//-----------------------------------------------------------------------------
void ToolWindow::SetToolWindowFactory( IToolWindowFactory *factory )
{
	m_pFactory = factory;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IToolWindowFactory
//-----------------------------------------------------------------------------
IToolWindowFactory *ToolWindow::GetToolWindowFactory()
{
	return m_pFactory;
}

//-----------------------------------------------------------------------------
// Purpose: To fill the space left by other tool windows
// Input  :  edge: 0=all, 1=top, 2=right, 3=bottom, 4=left
// Output : 
//-----------------------------------------------------------------------------

void ToolWindow::Grow( int edge, int from_x, int from_y )
{
	int status_h = 24;
	int menubar_h = 27;

	int sw, sh;
	surface()->GetScreenSize( sw, sh );

	int old_x, old_y, old_w, old_h;
	GetBounds( old_x, old_y, old_w, old_h );

	int new_x, new_y, new_w, new_h;
	new_x = old_x;
	new_y = old_y;
	new_w = old_w;
	new_h = old_h;

	int c = GetToolWindowCount();

	// grow up
	if ( ( edge == 0 ) || ( edge == 1 ) )
	{
		// first shrink the edge back to the grow point
		if ( from_y >= 0 )
		{
			old_h = old_h - ( from_y - old_y );
			old_y = from_y;
		}

		// now grow the edge as far as it can go
		new_h = old_h + ( old_y - menubar_h );
		new_y = menubar_h;

		for ( int i = 0 ; i < c; ++i )
		{
			ToolWindow *tw = GetToolWindow( i );
			Assert( tw );
			if ( ( !tw ) || ( tw == this ) )
				continue;

			// Get panel bounds
			int x, y, w, h;
			tw->GetBounds( x, y, w, h );

			// grow it
			if ( ( ( ( old_x > x ) && ( old_x < x + w ) )
				|| ( ( old_x + old_w > x ) && ( old_x + old_w < x + w ) )
				|| ( ( old_x <= x ) && old_x + old_w >= x + w ))
				&& ( ( old_y >= y + h ) && ( new_y < y + h ) ) )
			{
				new_h = old_h + ( old_y - ( y + h ) );
				new_y = y + h;
			}
		}
		old_h = new_h;
		old_y = new_y;
	}

	// grow right
	if ( ( edge == 0 ) || ( edge == 2 ) )
	{
		// first shrink the edge back to the grow point
		if ( from_x >= 0 )
		{
			old_w = from_x - old_x;
		}

		// now grow the edge as far as it can go
		new_w = sw - old_x;

		for ( int i = 0 ; i < c; ++i )
		{
			ToolWindow *tw = GetToolWindow( i );
			Assert( tw );
			if ( ( !tw ) || ( tw == this ) )
				continue;

			// Get panel bounds
			int x, y, w, h;
			tw->GetBounds( x, y, w, h );

			// grow it
			if ( ( ( ( old_y > y ) && ( old_y < y + h ) )
				|| ( ( old_y + old_h > y ) && ( old_y + old_h < y + h ) )
				|| ( ( old_y <= y ) && old_y + old_h >= y + h ))
				&& ( ( old_x + old_w <= x ) && ( new_w > x - old_x ) ) )
			{
				new_w = x - old_x;
			}
		}
		old_w = new_w;
	}

	// grow down
	if ( ( edge == 0 ) || ( edge == 3 ) )
	{
		// first shrink the edge back to the grow point
		if ( from_y >= 0 )
		{
			old_h = from_y - old_y;
		}

		// now grow the edge as far as it can go
		new_h = sh - old_y - status_h;

		for ( int i = 0 ; i < c; ++i )
		{
			ToolWindow *tw = GetToolWindow( i );
			Assert( tw );
			if ( ( !tw ) || ( tw == this ) )
				continue;

			// Get panel bounds
			int x, y, w, h;
			tw->GetBounds( x, y, w, h );

			// grow it
			if ( ( ( ( old_x > x ) && ( old_x < x + w ) )
				|| ( ( old_x + old_w > x ) && ( old_x + old_w < x + w ) )
				|| ( ( old_x <= x ) && old_x + old_w >= x + w ))
				&& ( ( old_y + old_h <= y ) && ( new_h > y - old_y ) ) )
			{
				new_h = y - old_y;
			}
		}
		old_h = new_h;
	}

	// grow left
	if ( ( edge == 0 ) || ( edge == 4 ) )
	{
		// first shrink the edge back to the grow point
		if ( from_x >= 0 )
		{
			old_w = old_w - ( from_x - old_x );
			old_x = from_x;
		}

		// now grow the edge as far as it can go
		new_w = old_w + old_x;
		new_x = 0;

		for ( int i = 0 ; i < c; ++i )
		{
			ToolWindow *tw = GetToolWindow( i );
			Assert( tw );
			if ( ( !tw ) || ( tw == this ) )
				continue;

			// Get panel bounds
			int x, y, w, h;
			tw->GetBounds( x, y, w, h );

			// grow it
			if ( ( ( ( old_y > y ) && ( old_y < y + h ) )
				|| ( ( old_y + old_h > y ) && ( old_y + old_h < y + h ) )
				|| ( ( old_y <= y ) && old_y + old_h >= y + h ))
				&& ( ( old_x >= x + w ) && ( new_x < x + w ) ) )
			{
				new_w = old_w + ( old_x - ( x + w ) );
				new_x = x + w;
			}
		}
		old_w = new_w;
		old_x = new_x;
	}

	// Set panel bounds
	SetBounds( new_x, new_y, new_w, new_h );

}

//-----------------------------------------------------------------------------
// Purpose: Calls Grow based on where the mouse is.
//          over titlebar: grows all edges ( from mouse pos )
//          over edge grab area: grows just that edge
//          over corner grab area: grows the two adjacent edges
// Input  : 
// Output : 
//-----------------------------------------------------------------------------

void ToolWindow::GrowFromClick()
{
	int mx, my;
	input()->GetCursorPos( mx, my );

	int esz, csz, brsz, ch;
	esz = GetDraggerSize();
	csz = GetCornerSize();
	brsz = GetBottomRightSize();
	ch = GetCaptionHeight();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	// upper right
	if ( ( mx > x+w-csz-1 ) && ( my < y+csz ) )
	{
		Grow(1);
		Grow(2);
	}
	// lower right (the big one)
	else if ( ( mx > x+w-brsz-1 ) && ( my > y+h-brsz-1 ) )
	{
		Grow(2);
		Grow(3);
	}
	// lower left
	else if ( ( mx < x+csz ) && ( my > y+h-csz-1 ) )
	{
		Grow(3);
		Grow(4);
	}
	// upper left
	else if ( ( mx < x+csz ) && ( my < y+csz ) )
	{
		Grow(4);
		Grow(1);
	}
	// top edge
	else if ( my < y+esz )
	{
		Grow(1);
	}
	// right edge
	else if ( mx > x+w-esz-1 )
	{
		Grow(2);
	}
	// bottom edge
	else if ( my > y+h-esz-1 )
	{
		Grow(3);
	}
	// left edge
	else if ( mx < x+esz )
	{
		Grow(4);
	}
	// otherwise (if over the grab bar), grow all edges (from the clicked point)
	else if ( my < y + ch )
	{
		Grow(0, mx, my);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : 
//-----------------------------------------------------------------------------

void ToolWindow::OnMouseDoublePressed( MouseCode code )
{
	GrowFromClick();
}

void ToolWindow::OnMousePressed( MouseCode code )
{
	switch ( code )
	{
	case MOUSE_MIDDLE:
		GrowFromClick();
		break;
	default:
		BaseClass::OnMousePressed( code );
	}
}
