//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "vgui/IVGui.h"

#include <vgui_controls/BuildGroup.h>
#include <vgui_controls/BuildModeDialog.h>
#include <vgui_controls/EditablePanel.h>

// these includes are all for the virtual contruction factory Dialog::CreateControlByName()
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/ToggleButton.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/AnimatingImagePanel.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/URLLabel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/BitmapImagePanel.h>

#include "filesystem.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( EditablePanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
#pragma warning( disable : 4355 )

EditablePanel::EditablePanel(Panel *parent, const char *panelName) : Panel(parent, panelName), m_NavGroup(this)
{
	_buildGroup = new BuildGroup(this, this);
	m_pszConfigName = NULL;
	m_iConfigID = 0;
	m_pDialogVariables = NULL;
	m_bShouldSkipAutoResize = false;

	// add ourselves to the build group
	SetBuildGroup(GetBuildGroup());
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
EditablePanel::EditablePanel(Panel *parent, const char *panelName, HScheme hScheme) : Panel(parent, panelName, hScheme), m_NavGroup(this)
{
	_buildGroup = new BuildGroup(this, this);
	m_pszConfigName = NULL;
	m_iConfigID = 0;
	m_pDialogVariables = NULL;
	m_bShouldSkipAutoResize = false;

	// add ourselves to the build group
	SetBuildGroup(GetBuildGroup());
}

#pragma warning( default : 4355 )

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
EditablePanel::~EditablePanel()
{
	delete [] m_pszConfigName;
	delete _buildGroup;

	if (m_pDialogVariables)
	{
		m_pDialogVariables->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a child is added to the panel.
//-----------------------------------------------------------------------------
void EditablePanel::OnChildAdded(VPANEL child)
{
	BaseClass::OnChildAdded(child);

	// add only if we're in the same module
	Panel *panel = ipanel()->GetPanel(child, GetModuleName());
	if (panel)
	{
		panel->SetBuildGroup(_buildGroup);
		panel->AddActionSignalTarget(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void EditablePanel::OnKeyCodePressed( KeyCode code )
{
	static ConVarRef vgui_nav_lock_default_button( "vgui_nav_lock_default_button" );
	if ( !vgui_nav_lock_default_button.IsValid() || vgui_nav_lock_default_button.GetInt() == 0 )
	{
		ButtonCode_t nButtonCode = GetBaseButtonCode( code );

		// check for a default button
		VPANEL panel = GetFocusNavGroup().GetCurrentDefaultButton();
		if ( panel && !IsConsoleStylePanel() )
		{
			switch ( nButtonCode )
			{
			case KEY_XBUTTON_UP:
			case KEY_XSTICK1_UP:
			case KEY_XSTICK2_UP:
			case KEY_UP:
			case STEAMCONTROLLER_DPAD_UP:
			case KEY_XBUTTON_DOWN:
			case KEY_XSTICK1_DOWN:
			case KEY_XSTICK2_DOWN:
			case KEY_DOWN:
			case STEAMCONTROLLER_DPAD_DOWN:
			case KEY_XBUTTON_LEFT:
			case KEY_XSTICK1_LEFT:
			case KEY_XSTICK2_LEFT:
			case KEY_LEFT:
			case KEY_XBUTTON_RIGHT:
			case KEY_XSTICK1_RIGHT:
			case KEY_XSTICK2_RIGHT:
			case KEY_RIGHT:
			case KEY_XBUTTON_B:
			case STEAMCONTROLLER_B:
				// Navigating menus
				vgui_nav_lock_default_button.SetValue( 1 );
				PostMessage( panel, new KeyValues( "KeyCodePressed", "code", code ) );
				return;
			
			case KEY_XBUTTON_A:
			case STEAMCONTROLLER_A:
			case KEY_ENTER:
				if ( ipanel()->IsVisible( panel ) && ipanel()->IsEnabled( panel ) )
				{
					// Activate the button
					PostMessage( panel, new KeyValues( "Hotkey" ) );
					return;
				}
			}
		}
	}

	if ( !m_PassUnhandledInput )
		return;

	// Nothing to do with the button
	BaseClass::OnKeyCodePressed( code );
}



//-----------------------------------------------------------------------------
// Purpose: Callback for when the panel size has been changed
//-----------------------------------------------------------------------------
void EditablePanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();

	for (int i = 0; i < GetChildCount(); i++)
	{
		// perform auto-layout on the child panel
		Panel *child = GetChild(i);
		if ( !child )
			continue;

		int x, y, w, h;
		child->GetBounds( x, y, w, h );

		int px, py;
		child->GetPinOffset( px, py );

		int ox, oy;
		child->GetResizeOffset( ox, oy );

		int ex;
		int ey;

		AutoResize_e resize = child->GetAutoResize(); 
		bool bResizeHoriz = ( resize == AUTORESIZE_RIGHT || resize == AUTORESIZE_DOWNANDRIGHT );
		bool bResizeVert = ( resize == AUTORESIZE_DOWN || resize == AUTORESIZE_DOWNANDRIGHT );

		// The correct version of this code would say:
		// if ( resize != AUTORESIZE_NO )
		// but we're very close to shipping and this causes artifacts in other vgui panels that now
		// depend on this bug.  So, I've added m_bShouldSkipAutoResize, which defaults to false but can
		// be set using "skip_autoresize" in a .res file
		if ( !m_bShouldSkipAutoResize )
		{
			PinCorner_e pinCorner = child->GetPinCorner();
			if ( pinCorner == PIN_TOPRIGHT || pinCorner == PIN_BOTTOMRIGHT )
			{
				// move along with the right edge
				ex = wide + px;
				x = bResizeHoriz ? ox : ex - w;
			}
			else
			{
				x = px;
				ex = bResizeHoriz ? wide + ox : px + w;
			}

			if ( pinCorner == PIN_BOTTOMLEFT || pinCorner == PIN_BOTTOMRIGHT )
			{
				// move along with the right edge
				ey = tall + py;
				y = bResizeVert ? oy : ey - h;
			}
			else
			{
				y = py;
				ey = bResizeVert ? tall + oy : py + h;
			}

			// Clamp..
			if ( ex < x )
			{
				ex = x;
			}
			if ( ey < y )
			{
				ey = y;
			}

			child->SetBounds( x, y, ex - x, ey - y );
			child->InvalidateLayout();
		}
	}
	Repaint();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void EditablePanel::OnCurrentDefaultButtonSet( VPANEL defaultButton )
{
	m_NavGroup.SetCurrentDefaultButton( defaultButton, false );

	// forward the message up
	if (GetVParent())
	{
		KeyValues *msg = new KeyValues("CurrentDefaultButtonSet");
		msg->SetInt("button", ivgui()->PanelToHandle( defaultButton ) );
		PostMessage(GetVParent(), msg);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void EditablePanel::OnDefaultButtonSet( VPANEL defaultButton )
{
	Panel *panel = ipanel()->GetPanel( defaultButton, GetModuleName() );

	m_NavGroup.SetDefaultButton(panel);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void EditablePanel::OnFindDefaultButton()
{
    if (m_NavGroup.GetDefaultButton())
    {
        m_NavGroup.SetCurrentDefaultButton(m_NavGroup.GetDefaultButton());
    }
    else
    {
        if (GetVParent())
        {
            PostMessage(GetVParent(), new KeyValues("FindDefaultButton"));
        }
    }
}

struct leaf_t
{
	short x, y, wide, tall;
	unsigned char split;	// 0 no split; 1 x-axis, 2 y-axis
	bool filled;	// true if this is already filled
	short splitpos;	// place of split

	leaf_t *left;
	leaf_t *right;
};

leaf_t g_Leaves[256];
int g_iNextLeaf;

inline leaf_t *AllocLeaf()
{
	Assert(g_iNextLeaf < 255);

	return &g_Leaves[g_iNextLeaf++];
}

void AddSolidToTree(leaf_t *leaf, int x, int y, int wide, int tall)
{
	// clip to this leaf
	if (x < leaf->x)
	{
		wide -= (leaf->x - x);
		if (wide < 1)
			return;
		x = leaf->x;
	}
	if (y < leaf->y)
	{
		tall -= (leaf->y - y);
		if (tall < 1)
			return;
		y = leaf->y;
	}
	if (x + wide > leaf->x + leaf->wide)
	{
		wide -= ((x + wide) - (leaf->x + leaf->wide));
		if (wide < 1)
			return;
	}
	if (y + tall > leaf->y + leaf->tall)
	{
		tall -= ((y + tall) - (leaf->y + leaf->tall));
		if (tall < 1)
			return;
	}

	// the rect should now be completely within the leaf
	if (leaf->split == 1)
	{
		// see if it is to the left or the right of the split
		if (x < leaf->splitpos)
		{
			// it's to the left
			AddSolidToTree(leaf->left, x, y, wide, tall);
		}
		else if (x + wide > leaf->splitpos)
		{
			// it's to the right
			AddSolidToTree(leaf->right, x, y, wide, tall);
		}
	}
	else if (leaf->split == 2)
	{
		// check y
		// see if it is to the left (above) or the right (below) of the split
		if (y < leaf->splitpos)
		{
			// it's above
			AddSolidToTree(leaf->left, x, y, wide, tall);
		}
		else if (y + tall > leaf->splitpos)
		{
			// it's below
			AddSolidToTree(leaf->right, x, y, wide, tall);
		}
	}
	else
	{
		// this leaf is unsplit, make the first split against the first edge we find
		if (x > leaf->x)
		{
			// split the left side of the rect
			leaf->split = 1;
			leaf->splitpos = (short)x;

			// create 2 new leaves
			leaf_t *left = AllocLeaf();
			leaf_t *right = AllocLeaf();
			memset(left, 0, sizeof(leaf_t));
			memset(right, 0, sizeof(leaf_t));
			leaf->left = left;
			leaf->right = right;

			left->x = leaf->x;
			left->y = leaf->y;
			left->wide = (short)(leaf->splitpos - leaf->x);
			left->tall = leaf->tall;

			right->x = leaf->splitpos;
			right->y = leaf->y;
			right->wide = (short)(leaf->wide - left->wide);
			right->tall = leaf->tall;

			// split the right leaf by the current rect
			AddSolidToTree(leaf->right, x, y, wide, tall);
		}
		else if (y > leaf->y)
		{
			// split the top edge
			leaf->split = 2;
			leaf->splitpos = (short)y;

			// create 2 new leaves (facing to the east)
			leaf_t *left = AllocLeaf();
			leaf_t *right = AllocLeaf();
			memset(left, 0, sizeof(leaf_t));
			memset(right, 0, sizeof(leaf_t));
			leaf->left = left;
			leaf->right = right;

			left->x = leaf->x;
			left->y = leaf->y;
			left->wide = leaf->wide;
			left->tall = (short)(y - leaf->y);

			right->x = leaf->x;
			right->y = leaf->splitpos;
			right->wide = leaf->wide;
			right->tall = (short)(leaf->tall + leaf->y - right->y);

			// split the right leaf by the current rect
			AddSolidToTree(leaf->right, x, y, wide, tall);
		}
		else if (x + wide < leaf->x + leaf->wide)
		{
			// split the right edge
			leaf->split = 1;
			leaf->splitpos = (short)(x + wide);

			// create 2 new leaves
			leaf_t *left = AllocLeaf();
			leaf_t *right = AllocLeaf();
			memset(left, 0, sizeof(leaf_t));
			memset(right, 0, sizeof(leaf_t));
			leaf->left = left;
			leaf->right = right;

			left->x = leaf->x;
			left->y = leaf->y;
			left->wide = (short)(leaf->splitpos - leaf->x);
			left->tall = leaf->tall;

			right->x = leaf->splitpos;
			right->y = leaf->y;
			right->wide = (short)(leaf->wide - left->wide);
			right->tall = leaf->tall;

			// split the left leaf by the current rect
			AddSolidToTree(leaf->left, x, y, wide, tall);
		}
		else if (y + tall < leaf->y + leaf->tall)
		{
			// split the bottom edge
			leaf->split = 2;
			leaf->splitpos = (short)(y + tall);

			// create 2 new leaves (facing to the east)
			leaf_t *left = AllocLeaf();
			leaf_t *right = AllocLeaf();
			memset(left, 0, sizeof(leaf_t));
			memset(right, 0, sizeof(leaf_t));
			leaf->left = left;
			leaf->right = right;

			left->x = leaf->x;
			left->y = leaf->y;
			left->wide = leaf->wide;
			left->tall = (short)(leaf->splitpos - leaf->y);

			right->x = leaf->x;
			right->y = leaf->splitpos;
			right->wide = leaf->wide;
			right->tall = (short)(leaf->tall - left->tall);

			// split the left leaf by the current rect
			AddSolidToTree(leaf->left, x, y, wide, tall);
		}
		else
		{
			// this is the exact same rect! don't draw this leaf
			leaf->filled = true;
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills the panel background, clipping if possible
//-----------------------------------------------------------------------------
void EditablePanel::PaintBackground()
{
	BaseClass::PaintBackground();
	return;

/*
	test code, using a screenspace bsp tree to reduce overdraw in vgui
	not yet fully functional

//	test: fill background with obnoxious color to show holes
//	surface()->DrawSetColor(Color(255, 0, 0, 255));
//	surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
//	return;

	// reset the leaf memory
	g_iNextLeaf = 0;

	leaf_t *headNode = AllocLeaf();
	memset(headNode, 0, sizeof(leaf_t));

	headNode->wide = (short)GetWide();
	headNode->tall = (short)GetTall();

	// split the leaf by the first child
	for (int i = 0; i < GetChildCount(); i++)
	{
		Panel *child = GetChild(i);
		if (child->IsOpaque())
		{
			int x, y, wide, tall;
			child->GetBounds(x, y, wide, tall);

			// ignore small children
			if (wide + tall < 100)
				continue;

			AddSolidToTree(headNode, x, y, wide, tall);
		}
	}

	// walk the built tree, painting the background
	Color col = GetBgColor();
	surface()->DrawSetColor(col);
	for (i = 0; i < g_iNextLeaf; i++)
	{
		leaf_t *leaf = g_Leaves + i;
		if (leaf->splitpos || leaf->filled)
			continue;
		surface()->DrawFilledRect(leaf->x, leaf->y, leaf->x + leaf->wide, leaf->y + leaf->tall);
	}
*/
}

//-----------------------------------------------------------------------------
// Purpose: Activates the build mode dialog for editing panels.
//-----------------------------------------------------------------------------
void EditablePanel::ActivateBuildMode()
{
	_buildGroup->SetEnabled(true);
}

//-----------------------------------------------------------------------------
// Purpose: Loads panel settings from a resource file.
//-----------------------------------------------------------------------------
void EditablePanel::LoadControlSettings(const char *resourceName, const char *pathID, KeyValues *pKeyValues, KeyValues *pConditions)
{
#if defined( DBGFLAG_ASSERT ) && !defined(OSX) && !defined(LINUX)
	// Since nobody wants to fix this assert, I'm making it a Msg instead:
	//     editablepanel.cpp (535) : Resource file "resource\DebugOptionsPanel.res" not found on disk!
	// AssertMsg( g_pFullFileSystem->FileExists( resourceName ), CFmtStr( "Resource file \"%s\" not found on disk!", resourceName ).Access() );
	if ( !g_pFullFileSystem->FileExists( resourceName ) )
	{
		Msg( "Resource file \"%s\" not found on disk!", resourceName );
	}
#endif
	_buildGroup->LoadControlSettings(resourceName, pathID, pKeyValues, pConditions);
	ForceSubPanelsToUpdateWithNewDialogVariables();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: registers a file in the list of control settings, so the vgui dialog can choose between them to edit
//-----------------------------------------------------------------------------
void EditablePanel::RegisterControlSettingsFile(const char *resourceName, const char *pathID)
{
	_buildGroup->RegisterControlSettingsFile(resourceName, pathID);
}

//-----------------------------------------------------------------------------
// Purpose: sets the name of this dialog so it can be saved in the user config area
//-----------------------------------------------------------------------------
void EditablePanel::LoadUserConfig(const char *configName, int dialogID)
{
	KeyValues *data = system()->GetUserConfigFileData(configName, dialogID);

	delete [] m_pszConfigName;
	int len = Q_strlen(configName) + 1;
	m_pszConfigName = new char[ len ];
	Q_strncpy(m_pszConfigName, configName, len );
	m_iConfigID = dialogID;

	// apply our user config settings (this will recurse through our children)
	if (data)
	{
		ApplyUserConfigSettings(data);
	}
}

//-----------------------------------------------------------------------------
// Purpose: saves all the settings to the document
//-----------------------------------------------------------------------------
void EditablePanel::SaveUserConfig()
{
	if (m_pszConfigName)
	{
		KeyValues *data = system()->GetUserConfigFileData(m_pszConfigName, m_iConfigID);

		// get our user config settings (this will recurse through our children)
		if (data)
		{
			GetUserConfigSettings(data);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: combines both of the above, LoadControlSettings & LoadUserConfig
//-----------------------------------------------------------------------------
void EditablePanel::LoadControlSettingsAndUserConfig(const char *dialogResourceName, int dialogID)
{
	LoadControlSettings(dialogResourceName);
	LoadUserConfig(dialogResourceName, dialogID);
}

//-----------------------------------------------------------------------------
// Purpose: applies the user config settings to all the children
//-----------------------------------------------------------------------------
void EditablePanel::ApplyUserConfigSettings(KeyValues *userConfig)
{
	for (int i = 0; i < GetChildCount(); i++)
	{
		Panel *child = GetChild(i);
		if (child->HasUserConfigSettings())
		{
			const char *name = child->GetName();
			if (name && *name)
			{
				child->ApplyUserConfigSettings(userConfig->FindKey(name, true));
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: gets all the children's user config settings
//-----------------------------------------------------------------------------
void EditablePanel::GetUserConfigSettings(KeyValues *userConfig)
{
	for (int i = 0; i < GetChildCount(); i++)
	{
		Panel *child = GetChild(i);
		if (child->HasUserConfigSettings())
		{
			const char *name = child->GetName();
			if (name && *name)
			{
				child->GetUserConfigSettings(userConfig->FindKey(name, true));
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Save user config settings
//-----------------------------------------------------------------------------
void EditablePanel::OnClose()
{
	SaveUserConfig();
}

//-----------------------------------------------------------------------------
// Purpose: Handle information requests
//-----------------------------------------------------------------------------
bool EditablePanel::RequestInfo(KeyValues *data)
{
	if (!stricmp(data->GetName(), "BuildDialog"))
	{
		// a build dialog is being requested, give it one
		// a bit hacky, but this is a case where vgui.dll needs to reach out
		data->SetPtr("PanelPtr", new BuildModeDialog( (BuildGroup *)data->GetPtr("BuildGroupPtr")));
		return true;
	}
	else if (!stricmp(data->GetName(), "ControlFactory"))
	{
		Panel *newPanel = CreateControlByName(data->GetString("ControlName"));
		if (newPanel)
		{
			data->SetPtr("PanelPtr", newPanel);
			return true;
		}
	}
	return BaseClass::RequestInfo(data);
}

//-----------------------------------------------------------------------------
// Purpose: Return the buildgroup that this panel is part of.
// Input  : 
// Output : BuildGroup
//-----------------------------------------------------------------------------
BuildGroup *EditablePanel::GetBuildGroup()
{
	return _buildGroup;
}

//-----------------------------------------------------------------------------
// Purpose: Return a pointer to the nav group
// Output : FocusNavGroup
//-----------------------------------------------------------------------------
FocusNavGroup &EditablePanel::GetFocusNavGroup()
{
	return m_NavGroup;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool EditablePanel::RequestFocusNext(VPANEL panel)
{
	bool bRet = m_NavGroup.RequestFocusNext(panel);
	if ( IsPC() && !bRet && IsConsoleStylePanel() )
	{
		NavigateDown();
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool EditablePanel::RequestFocusPrev(VPANEL panel)
{
	bool bRet = m_NavGroup.RequestFocusPrev(panel);
	if ( IsPC() && !bRet && IsConsoleStylePanel() )
	{
		NavigateUp();
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: Delegates focus to a sub panel
// Input  : direction - the direction in which focus travelled to arrive at this panel; forward = 1, back = -1
//-----------------------------------------------------------------------------
void EditablePanel::RequestFocus(int direction)
{
	// we must be a sub panel for this to be called
	// delegate focus
	if (direction == 1)
	{
		RequestFocusNext(NULL);
	}
	else if (direction == -1)
	{
		RequestFocusPrev(NULL);
	}
	else
	{
		BaseClass::RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pass the focus down onto the last used panel
//-----------------------------------------------------------------------------
void EditablePanel::OnSetFocus()
{
	Panel *focus = m_NavGroup.GetCurrentFocus();
	if (focus && focus != this)
	{
		focus->RequestFocus();
	}
	else
	{
		focus = m_NavGroup.GetDefaultPanel();
		if (focus)
		{
			focus->RequestFocus();
			focus->OnSetFocus();
		}
	}

	BaseClass::OnSetFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the resource file is loaded to set up the panel state
// Input  : *inResourceData - 
//-----------------------------------------------------------------------------
void EditablePanel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	_buildGroup->ApplySettings(inResourceData);

	m_bShouldSkipAutoResize = inResourceData->GetBool( "skip_autoresize", false );
}


//-----------------------------------------------------------------------------
// Purpose: Update focus info for navigation
//-----------------------------------------------------------------------------
void EditablePanel::OnRequestFocus(VPANEL subFocus, VPANEL defaultPanel)
{
	if (!ipanel()->IsPopup(subFocus))
    {
		defaultPanel = m_NavGroup.SetCurrentFocus(subFocus, defaultPanel);
    }
	BaseClass::OnRequestFocus(GetVPanel(), defaultPanel);
}

//-----------------------------------------------------------------------------
// Purpose: Get the panel that currently has keyfocus
//-----------------------------------------------------------------------------
VPANEL EditablePanel::GetCurrentKeyFocus()
{
	Panel *focus = m_NavGroup.GetCurrentFocus();
	if (focus == this)
		return NULL;

	if (focus)
	{
		if (focus->IsPopup())
			return BaseClass::GetCurrentKeyFocus();

		// chain down the editpanel hierarchy
		VPANEL subFocus = focus->GetCurrentKeyFocus();
		if (subFocus)
			return subFocus;

		// hit a leaf panel, return that
		return focus->GetVPanel();
	}
	return BaseClass::GetCurrentKeyFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Gets the panel with the specified hotkey
//-----------------------------------------------------------------------------
Panel *EditablePanel::HasHotkey(wchar_t key)
{
	if( !IsVisible() || !IsEnabled()) // not visible, so can't respond to a hot key 
	{
		return NULL;
	}

	for (int i = 0; i < GetChildCount(); i++)
	{
		Panel *hot = GetChild(i)->HasHotkey(key);
		if (hot && hot->IsVisible() && hot->IsEnabled())
		{
			return hot;
		}
	}
	
	return NULL;
	
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut function to setting enabled state of control
//-----------------------------------------------------------------------------
void EditablePanel::SetControlEnabled(const char *controlName, bool enabled, bool bRecurseDown /*= false*/ )
{
	Panel *control = FindChildByName(controlName, bRecurseDown);
	if (control)
	{
		control->SetEnabled(enabled);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut function to setting visibility state of control
//-----------------------------------------------------------------------------
void EditablePanel::SetControlVisible(const char *controlName, bool visible, bool bRecurseDown /*= false*/ )
{
	Panel *control = FindChildByName(controlName, bRecurseDown);
	if (control)
	{
		control->SetVisible(visible);
	}
}
	
//-----------------------------------------------------------------------------
// Purpose: Shortcut function to set data in child controls
//-----------------------------------------------------------------------------
void EditablePanel::SetControlString(const char *controlName, const char *string)
{
	Panel *control = FindChildByName(controlName);
	if (control)
	{
		if (string[0] == '#')
		{
			const wchar_t *wszText = g_pVGuiLocalize->Find(string);
			if (wszText)
			{
				PostMessage(control, new KeyValues("SetText", "text", wszText));
			}
		}
		else
		{
			PostMessage(control, new KeyValues("SetText", "text", string));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut function to set data in child controls
//-----------------------------------------------------------------------------
void EditablePanel::SetControlString(const char *controlName, const wchar_t *string)
{
	Panel *control = FindChildByName(controlName);
	if (control)
	{
		PostMessage(control, new KeyValues("SetText", "text", string));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut function to set data in child controls
//-----------------------------------------------------------------------------
void EditablePanel::SetControlInt(const char *controlName, int state)
{
	Panel *control = FindChildByName(controlName);
	if (control)
	{
		PostMessage(control, new KeyValues("SetState", "state", state));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut function to get data in child controls
//-----------------------------------------------------------------------------
int EditablePanel::GetControlInt(const char *controlName, int defaultState)
{
	Panel *control = FindChildByName(controlName);
	if (control)
	{
		KeyValues *data = new KeyValues("GetState");
		if (control->RequestInfo(data))
		{
			int state = data->GetInt("state", defaultState);
			data->deleteThis();
			return state;
		}
	}
	return defaultState;
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut function to get data in child controls
//-----------------------------------------------------------------------------
const char *EditablePanel::GetControlString(const char *controlName, const char *defaultString)
{
	static char buf[512];
	GetControlString(controlName, buf, sizeof(buf) - 1, defaultString);
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut function to get data in child controls
//-----------------------------------------------------------------------------
void EditablePanel::GetControlString(const char *controlName, char *buf, int bufSize, const char *defaultString)
{
	Panel *control = FindChildByName(controlName);
	KeyValues *data = new KeyValues("GetText");
	if (control && control->RequestInfo(data))
	{
		Q_strncpy(buf, data->GetString("text", defaultString), bufSize);
	}
	else
	{
		// no value found, copy in default text
		Q_strncpy(buf, defaultString, bufSize);
	}

	// ensure null termination of string
	buf[bufSize - 1] = 0;

	// free
	data->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: localization variables (used in constructing UI strings)
//-----------------------------------------------------------------------------
void EditablePanel::SetDialogVariable(const char *varName, const char *value)
{
	GetDialogVariables()->SetString(varName, value);
	ForceSubPanelsToUpdateWithNewDialogVariables();
}

//-----------------------------------------------------------------------------
// Purpose: localization variables (used in constructing UI strings)
//-----------------------------------------------------------------------------
void EditablePanel::SetDialogVariable(const char *varName, const wchar_t *value)
{
	GetDialogVariables()->SetWString(varName, value);
	ForceSubPanelsToUpdateWithNewDialogVariables();
}

//-----------------------------------------------------------------------------
// Purpose: localization variables (used in constructing UI strings)
//-----------------------------------------------------------------------------
void EditablePanel::SetDialogVariable(const char *varName, int value)
{
	GetDialogVariables()->SetInt(varName, value);
	ForceSubPanelsToUpdateWithNewDialogVariables();
}

//-----------------------------------------------------------------------------
// Purpose: localization variables (used in constructing UI strings)
//-----------------------------------------------------------------------------
void EditablePanel::SetDialogVariable(const char *varName, float value)
{
	GetDialogVariables()->SetFloat(varName, value);
	ForceSubPanelsToUpdateWithNewDialogVariables();
}

//-----------------------------------------------------------------------------
// Purpose: redraws child panels with new localization vars
//-----------------------------------------------------------------------------
void EditablePanel::ForceSubPanelsToUpdateWithNewDialogVariables()
{
	if (m_pDialogVariables)
	{
		ipanel()->SendMessage(GetVPanel(), m_pDialogVariables, GetVPanel());
		for (int i = 0; i < ipanel()->GetChildCount(GetVPanel()); i++)
		{
			ipanel()->SendMessage(ipanel()->GetChild(GetVPanel(), i), m_pDialogVariables, GetVPanel());
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: lazy creation of localization vars object
//-----------------------------------------------------------------------------
KeyValues *EditablePanel::GetDialogVariables()
{
	if (m_pDialogVariables) 
		return m_pDialogVariables;

	m_pDialogVariables = new KeyValues("DialogVariables");
	return m_pDialogVariables;
}

//-----------------------------------------------------------------------------
// Purpose: Virtual factory for control creation
//-----------------------------------------------------------------------------
Panel *EditablePanel::CreateControlByName(const char *controlName)
{
	Panel *fromFactory = CBuildFactoryHelper::InstancePanel( controlName );
	if ( fromFactory )
	{
		return fromFactory;
	}

	return NULL;
}
