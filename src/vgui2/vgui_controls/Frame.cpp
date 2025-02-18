//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include <assert.h>
#include <math.h> // for ceil()
#define PROTECTED_THINGS_DISABLE

#include "tier1/utlstring.h"
#include "vgui/Cursor.h"
#include "vgui/MouseCode.h"
#include "vgui/IBorder.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui/KeyCode.h"

#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/MenuButton.h"
#include "vgui_controls/TextImage.h"

#include "KeyValues.h"

#include <stdio.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

static const int DEFAULT_SNAP_RANGE = 10; // number of pixels distance before the frame will snap to an edge
static const int CAPTION_TITLE_BORDER = 7;
static const int CAPTION_TITLE_BORDER_SMALL = 0;

namespace
{
	//-----------------------------------------------------------------------------
	// Purpose: Invisible panel to handle dragging/resizing frames
	//-----------------------------------------------------------------------------
	class GripPanel : public Panel
	{
	public:
		GripPanel(Frame *dragFrame, const char *name, int xdir, int ydir) : Panel(dragFrame, name)
		{
			_frame = dragFrame;
			_dragging = false;
			_dragMultX = xdir;
			_dragMultY = ydir;
			SetPaintEnabled(false);
			SetPaintBackgroundEnabled(false);
			SetPaintBorderEnabled(false);
			m_iSnapRange = DEFAULT_SNAP_RANGE;

			if (xdir == 1 && ydir == 1)
			{
				// bottom-right grip gets an image
				SetPaintEnabled(true);
				SetPaintBackgroundEnabled(true);
			}

			SetBlockDragChaining( true );
		}
		
		// Purpose- handle window resizing
		// Input- dx, dy, the offet of the mouse pointer from where we started dragging
		virtual void moved(int dx, int dy)
		{
			if (!_frame->IsSizeable())
				return;
			
			// Start off with x, y at the coords of where we started to drag
			int newX = _dragOrgPos[0], newY =_dragOrgPos[1];
			// Start off with width and tall equal from window when we started to drag
			int newWide = _dragOrgSize[0], newTall = _dragOrgSize[1];
			
			// get window's minimum size
			int minWide, minTall;
			_frame->GetMinimumSize( minWide, minTall);
			
			// Handle  width resizing
			newWide += (dx * _dragMultX);
			// Handle the position of the corner x position
			if (_dragMultX == -1)
			{
				// only move if we are not at the minimum
				// if we are at min we have to force the proper offset (dx)
				if (newWide < minWide)
				{
					dx=_dragOrgSize[0]-minWide;
				}
				newX += dx;	  // move window to its new position
			}
			
			// Handle height resizing
			newTall += (dy * _dragMultY);
			// Handle position of corner y position
			if (_dragMultY == -1)
			{
				if (newTall < minTall)
				{
					dy=_dragOrgSize[1]-minTall;
				}
				newY += dy;
			}
			
			if ( _frame->GetClipToParent() )
			{
				// If any coordinate is out of range, snap it back
				if ( newX < 0 )
					newX = 0;
				if ( newY < 0 )
					newY = 0;
				
				int sx, sy;
				surface()->GetScreenSize( sx, sy );

				int w, h;
				_frame->GetSize( w, h );
				if ( newX + w > sx )
				{
					newX = sx - w;
				}
				if ( newY + h > sy )
				{
					newY = sy - h;
				}
			}

			// set new position
			_frame->SetPos(newX, newY);
			// set the new size			
			// if window is below min size it will automatically pop to min size
			_frame->SetSize(newWide, newTall);
			_frame->InvalidateLayout();
			_frame->Repaint();
		}
		
		void OnCursorMoved(int x, int y)
		{
			if (!_dragging)
				return;

			if (!input()->IsMouseDown(MOUSE_LEFT))
			{
				// for some reason we're marked as dragging when the mouse is released
				// trigger a release
				OnMouseReleased(MOUSE_LEFT);
				return;
			}

			input()->GetCursorPos(x, y);
			moved((x - _dragStart[0]), ( y - _dragStart[1]));
			_frame->Repaint();
		}
		
		void OnMousePressed(MouseCode code)
		{
			if (code == MOUSE_LEFT)
			{ 
				_dragging=true;
				int x,y;
				input()->GetCursorPos(x,y);
				_dragStart[0]=x;
				_dragStart[1]=y;
				_frame->GetPos(_dragOrgPos[0],_dragOrgPos[1]);
				_frame->GetSize(_dragOrgSize[0],_dragOrgSize[1]);
				input()->SetMouseCapture(GetVPanel());
				
				// if a child doesn't have focus, get it for ourselves
				VPANEL focus = input()->GetFocus();
				if (!focus || !ipanel()->HasParent(focus, _frame->GetVPanel()))
				{
					_frame->RequestFocus();
				}
				_frame->Repaint();
			}
			else
			{
				GetParent()->OnMousePressed(code);
			}
		}

		void OnMouseDoublePressed(MouseCode code)
		{
			GetParent()->OnMouseDoublePressed(code);
		}

		void Paint()
		{
			// draw the grab handle in the bottom right of the frame
			surface()->DrawSetTextFont(_marlettFont);
			surface()->DrawSetTextPos(0, 0);
			
			// thin highlight lines
			surface()->DrawSetTextColor(GetFgColor());
			surface()->DrawUnicodeChar('p'); 
		}

		void PaintBackground()
		{
			// draw the grab handle in the bottom right of the frame
			surface()->DrawSetTextFont(_marlettFont);
			surface()->DrawSetTextPos(0, 0);
			
			// thick shadow lines
			surface()->DrawSetTextColor(GetBgColor());
			surface()->DrawUnicodeChar('o'); 
		}
		
		void OnMouseReleased(MouseCode code)
		{
			_dragging = false;
			input()->SetMouseCapture(NULL);
		}

		void OnMouseCaptureLost()
		{
			Panel::OnMouseCaptureLost();
			_dragging = false;
		}

		void ApplySchemeSettings(IScheme *pScheme)
		{
			Panel::ApplySchemeSettings(pScheme);
			bool isSmall = ((Frame *)GetParent())->IsSmallCaption();

			_marlettFont = pScheme->GetFont( isSmall ? "MarlettSmall" : "Marlett", IsProportional());
			SetFgColor(GetSchemeColor("FrameGrip.Color1", pScheme));
			SetBgColor(GetSchemeColor("FrameGrip.Color2", pScheme));

			const char *snapRange = pScheme->GetResourceString("Frame.AutoSnapRange");
			if (snapRange && *snapRange)
			{
				m_iSnapRange = atoi(snapRange);
			}
		}
		
	protected:
		Frame *_frame;
		int  _dragMultX;
		int  _dragMultY;
		bool _dragging;
		int  _dragOrgPos[2];
		int  _dragOrgSize[2];
		int  _dragStart[2];
		int  m_iSnapRange;
		HFont _marlettFont;
	};
	
	//-----------------------------------------------------------------------------
	// Purpose: Handles caption grip input for moving dialogs around
	//-----------------------------------------------------------------------------
	class CaptionGripPanel : public GripPanel
	{
	public:
		CaptionGripPanel(Frame* frame, const char *name) : GripPanel(frame, name, 0, 0)
		{
		}
		
		void moved(int dx, int dy)
		{
			if (!_frame->IsMoveable())
				return;

			int newX = _dragOrgPos[0] + dx;
			int newY = _dragOrgPos[1] + dy;

			if (m_iSnapRange)
			{
				// first check docking to desktop
				int wx, wy, ww, wt;
				surface()->GetWorkspaceBounds(wx, wy, ww, wt);
				getInsideSnapPosition(wx, wy, ww, wt, newX, newY);

				// now lets check all windows and see if we snap to those
				// root panel
				VPANEL root = surface()->GetEmbeddedPanel();
				// cycle through panels
				// look for panels that are visible and are popups that we can dock to
				for (int i = 0; i < ipanel()->GetChildCount(root); ++i)
				{
					VPANEL child = ipanel()->GetChild(root, i);
					tryToDock (child, newX, newY);
				}
			}

			if ( _frame->GetClipToParent() )
			{
				// If any coordinate is out of range, snap it back
				if ( newX < 0 )
					newX = 0;
				if ( newY < 0 )
					newY = 0;
				
				int sx, sy;
				surface()->GetScreenSize( sx, sy );

				int w, h;
				_frame->GetSize( w, h );
				if ( newX + w > sx )
				{
					newX = sx - w;
				}
				if ( newY + h > sy )
				{
					newY = sy - h;
				}
			}

			_frame->SetPos(newX, newY);

		}
		
		void tryToDock(VPANEL window, int &newX, int & newY)
		{
			// bail if child is this window	
			if ( window == _frame->GetVPanel())
				return;
			
			int cx, cy, cw, ct;
			if ( (ipanel()->IsVisible(window)) && (ipanel()->IsPopup(window)) )
			{
				// position
				ipanel()->GetAbsPos(window, cx, cy);
				// dimensions
				ipanel()->GetSize(window, cw, ct);
				bool snapped = getOutsideSnapPosition (cx, cy, cw, ct, newX, newY);
				if (snapped)
				{ 
					// if we snapped, we're done with this path
					// dont try to snap to kids
					return;
				}
			}

			// check all children
			for (int i = 0; i < ipanel()->GetChildCount(window); ++i)
			{
				VPANEL child = ipanel()->GetChild(window, i);
				tryToDock(child, newX, newY);
			}

		}

		// Purpose: To calculate the windows new x,y position if it snaps
		//          Will snap to the INSIDE of a window (eg desktop sides
		// Input: boundX boundY, position of candidate window we are seeing if we snap to
		//        boundWide, boundTall, width and height of window we are seeing if we snap to
		// Output: snapToX, snapToY new coords for window, unchanged if we dont snap
		// Returns true if we snapped, false if we did not snap.
		bool getInsideSnapPosition(int boundX, int boundY, int boundWide, int boundTall,
			int &snapToX, int &snapToY)
		{
			
			int wide, tall;
			_frame->GetSize(wide, tall);
			Assert (wide > 0);
			Assert (tall > 0);
			
			bool snapped=false;
			if (abs(snapToX - boundX) < m_iSnapRange)
			{
				snapToX = boundX;
				snapped=true;
			}
			else if (abs((snapToX + wide) - (boundX + boundWide)) < m_iSnapRange)
			{
				snapToX = boundX + boundWide - wide;
				snapped=true;
			}

			if (abs(snapToY - boundY) < m_iSnapRange)
			{
				snapToY = boundY;
				snapped=true;
			}
			else if (abs((snapToY + tall) - (boundY + boundTall)) < m_iSnapRange)
			{
				snapToY = boundY + boundTall - tall;
				snapped=true;
			}
			return snapped;
			
		}

		// Purpose: To calculate the windows new x,y position if it snaps
		//          Will snap to the OUTSIDE edges of a window (i.e. will stick peers together
		// Input: left, top, position of candidate window we are seeing if we snap to
		//        boundWide, boundTall, width and height of window we are seeing if we snap to
		// Output: snapToX, snapToY new coords for window, unchanged if we dont snap
		// Returns true if we snapped, false if we did not snap.
		bool getOutsideSnapPosition(int left, int top, int boundWide, int boundTall,
			int &snapToX, int &snapToY)
		{
			Assert (boundWide >= 0);
			Assert (boundTall >= 0);
						
			bool snapped=false;
			
			int right=left+boundWide;
			int bottom=top+boundTall;

			int wide, tall;
			_frame->GetSize(wide, tall);
			Assert (wide > 0);
			Assert (tall > 0);

			// we now see if we are going to be able to snap to a window side, and not
			// just snap to the "open air"
			// want to make it so that if any part of the window can dock to the candidate, it will

			// is this window horizontally snappable to the candidate
			bool horizSnappable=( 
				//  top of window is in range
				((snapToY > top) && (snapToY < bottom)) 
				// bottom of window is in range
				|| ((snapToY+tall > top) && (snapToY+tall < bottom)) 
				// window is just plain bigger than the window we wanna dock to
				|| ((snapToY < top) && (snapToY+tall > bottom)) ); 
			
			
			// is this window vertically snappable to the candidate
			bool vertSnappable=	( 
				 //  left of window is in range
				((snapToX > left) && (snapToX < right))
				//  right of window is in range
				|| ((snapToX+wide > left) && (snapToX+wide < right)) 
				// window is just plain bigger than the window we wanna dock to
				|| ((snapToX < left) && (snapToX+wide > right)) ); 
			
			// if neither, might as well bail
			if ( !(horizSnappable || vertSnappable) )
				return false;

			//if we're within the snap threshold then snap
			if ( (snapToX <= (right+m_iSnapRange)) && 
				(snapToX >= (right-m_iSnapRange)) ) 
			{  
				if (horizSnappable)
				{
					//disallow "open air" snaps
					snapped=true;
					snapToX = right;  
				}
			}
			else if ((snapToX + wide) >= (left-m_iSnapRange) &&
				(snapToX + wide) <= (left+m_iSnapRange)) 
			{
				if (horizSnappable)
				{
					snapped=true;
					snapToX = left-wide;
				}
			}
			
			if ( (snapToY <= (bottom+m_iSnapRange)) &&
				(snapToY >= (bottom-m_iSnapRange)) ) 
			{
				if (vertSnappable)
				{
					snapped=true;
					snapToY = bottom;
				}
			}
			else if ((snapToY + tall) <= (top+m_iSnapRange) &&
				(snapToY + tall) >= (top-m_iSnapRange)) 
			{
				if (vertSnappable)
				{
					snapped=true;
					snapToY = top-tall;
				}
			}
			return snapped;
		}
	};
	
}

namespace vgui
{
	//-----------------------------------------------------------------------------
	// Purpose: overrides normal button drawing to use different colors & borders
	//-----------------------------------------------------------------------------
	class FrameButton : public Button
	{
	private:
		IBorder *_brightBorder, *_depressedBorder, *_disabledBorder;
		Color _enabledFgColor, _enabledBgColor;
		Color _disabledFgColor, _disabledBgColor;
		bool _disabledLook;
	
	public:
	
		static int GetButtonSide( Frame *pFrame )
		{
			if ( pFrame->IsSmallCaption() )
			{
				return 12;
			}

			return 18;
		}
		
		
		FrameButton(Panel *parent, const char *name, const char *text) : Button(parent, name, text)
		{
			SetSize( QuickPropScale( FrameButton::GetButtonSide( (Frame *)parent ) ), QuickPropScale( FrameButton::GetButtonSide( (Frame *)parent ) ) );
			_brightBorder = NULL;
			_depressedBorder = NULL;
			_disabledBorder = NULL;
			_disabledLook = true;
			SetContentAlignment(Label::a_northwest);
			SetTextInset( QuickPropScale( 2 ), QuickPropScale( 1 ) );
			SetBlockDragChaining( true );
		}
		
		virtual void ApplySchemeSettings(IScheme *pScheme)
		{
			Button::ApplySchemeSettings(pScheme);
			
			_enabledFgColor = GetSchemeColor("FrameTitleButton.FgColor", pScheme);
			_enabledBgColor = GetSchemeColor("FrameTitleButton.BgColor", pScheme);

			_disabledFgColor = GetSchemeColor("FrameTitleButton.DisabledFgColor", pScheme);
			_disabledBgColor = GetSchemeColor("FrameTitleButton.DisabledBgColor", pScheme);
			
			_brightBorder = pScheme->GetBorder("TitleButtonBorder");
			_depressedBorder = pScheme->GetBorder("TitleButtonDepressedBorder");
			_disabledBorder = pScheme->GetBorder("TitleButtonDisabledBorder");
			
			SetDisabledLook(_disabledLook);
		}
		
		virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
		{
			if (_disabledLook)
			{
				return _disabledBorder;
			}
			
			if (depressed)
			{
				return _depressedBorder;
			}
			
			return _brightBorder;
		}
		
		virtual void SetDisabledLook(bool state)
		{
			_disabledLook = state;
			if (!_disabledLook)
			{
				SetDefaultColor(_enabledFgColor, _enabledBgColor);
				SetArmedColor(_enabledFgColor, _enabledBgColor);
				SetDepressedColor(_enabledFgColor, _enabledBgColor);
			}
			else
			{
				// setup disabled colors
				SetDefaultColor(_disabledFgColor, _disabledBgColor);
				SetArmedColor(_disabledFgColor, _disabledBgColor);
				SetDepressedColor(_disabledFgColor, _disabledBgColor);
			}
		}

        virtual void PerformLayout()
        {
            Button::PerformLayout();
            Repaint();
        }
		
		// Don't request focus.
		// This will keep items in the listpanel selected.
		virtual void OnMousePressed(MouseCode code)
		{
			if (!IsEnabled())
				return;
			
			if (!IsMouseClickEnabled(code))
				return;
			
			if (IsUseCaptureMouseEnabled())
			{
				{
					SetSelected(true);
					Repaint();
				}
				
				// lock mouse input to going to this button
				input()->SetMouseCapture(GetVPanel());
			}
		}
};


//-----------------------------------------------------------------------------
// Purpose: icon button
//-----------------------------------------------------------------------------
class FrameSystemButton : public MenuButton
{
	DECLARE_CLASS_SIMPLE( FrameSystemButton, MenuButton );

private:
	IImage *_enabled, *_disabled;
	Color _enCol, _disCol;
	bool _respond;
	CUtlString m_EnabledImage;
	CUtlString m_DisabledImage;
	
public:
	FrameSystemButton(Panel *parent, const char *panelName) : MenuButton(parent, panelName, "")
	{
		_disabled = _enabled = NULL;
		_respond = true;
		SetEnabled(false);
		// This menu will open if we use the left or right mouse button
		SetMouseClickEnabled( MOUSE_RIGHT, true );
		SetBlockDragChaining( true );
	}
	
	void SetImages( const char *pEnabledImage, const char *pDisabledImage = NULL )
	{
		m_EnabledImage = pEnabledImage;
		m_DisabledImage = pDisabledImage ? pDisabledImage : pEnabledImage;
	}

	void GetImageSize( int &w, int &h )
	{
		w = h = 0;

		int tw = 0, th = 0;
		if ( _enabled )
		{
			_enabled->GetSize( w, h );
		}
		if ( _disabled )
		{
			_disabled->GetSize( tw, th );
		}
		if ( tw > w )
		{
			w = tw;
		}
		if ( th > h )
		{
			h = th;
		}
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		_enCol = GetSchemeColor("FrameSystemButton.FgColor", pScheme);
		_disCol = GetSchemeColor("FrameSystemButton.BgColor", pScheme);
		
		const char *pEnabledImage = m_EnabledImage.Length() ? m_EnabledImage.Get() : 
			pScheme->GetResourceString( "FrameSystemButton.Icon" );
		const char *pDisabledImage = m_DisabledImage.Length() ? m_DisabledImage.Get() : 
			pScheme->GetResourceString( "FrameSystemButton.DisabledIcon" );
		_enabled = scheme()->GetImage( pEnabledImage, false);
		_disabled = scheme()->GetImage( pDisabledImage, false);

		SetTextInset(0, 0);
	
		// get our iconic image
		SetEnabled(IsEnabled());
	}
	
	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
	{
		return NULL;
	}

	virtual void SetEnabled(bool state)
	{
		Button::SetEnabled(state);
		
		if (IsEnabled())
		{
			if ( _enabled )
			{
				SetImageAtIndex(0, _enabled, 0);
			}
			SetBgColor(_enCol);
			SetDefaultColor(_enCol, _enCol);
			SetArmedColor(_enCol, _enCol);
			SetDepressedColor(_enCol, _enCol);
		}
		else
		{
			if ( _disabled )
			{
				SetImageAtIndex(0, _disabled, 0);
			}
			SetBgColor(_disCol);
			SetDefaultColor(_disCol, _disCol);
			SetArmedColor(_disCol, _disCol);
			SetDepressedColor(_disCol, _disCol);
		}
	}
	
	void SetResponsive(bool state)
	{
		_respond = state;
	}

	virtual void OnMousePressed(MouseCode code)
	{
		// button may look enabled but not be responsive
		if (!_respond)
			return;

		BaseClass::OnMousePressed(code);
	}

	virtual void OnMouseDoublePressed(MouseCode code)
	{
		// button may look enabled but not be responsive
		if (!_respond)
			return;

		// only close if left is double pressed 
		if (code == MOUSE_LEFT)
		{
			// double click on the icon closes the window
			// But only if the menu contains a 'close' item
			vgui::Menu *pMenu = GetMenu();
			if ( pMenu && pMenu->FindChildByName("Close") )
			{
				PostMessage(GetVParent(), new KeyValues("CloseFrameButtonPressed"));
			}
		}
	}

};

} // namespace vgui
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
Frame::Frame(Panel *parent, const char *panelName, bool showTaskbarIcon /*=true*/, bool bPopup /*=true*/ ) : EditablePanel(parent, panelName)
{
	// frames start invisible, to avoid having window flicker in on taskbar
	SetVisible(false);
	if ( bPopup )
	{
		MakePopup(showTaskbarIcon);
	}

	m_hPreviousModal = 0;

	_title=null;
	_moveable=true;
	_sizeable=true;
	m_bHasFocus=false;
	_flashWindow=false;
	_drawTitleBar = true; 
	m_bPreviouslyVisible = false;
	m_bFadingOut = false;
	m_bDisableFadeEffect = false;
	m_flTransitionEffectTime = 0.0f;
	m_flFocusTransitionEffectTime = 0.0f;
	m_bDeleteSelfOnClose = false;
	m_iClientInsetX = 5; 
	m_iClientInsetY = 5;
	m_iClientInsetXOverridden = false;
	m_iTitleTextInsetX = 28;
	m_bClipToParent = false;
	m_bSmallCaption = false;
	m_bChainKeysToParent = false;
	m_bPrimed = false;
	m_hCustomTitleFont = INVALID_FONT;

	SetTitle("#Frame_Untitled", parent ? false : true);
	
	// add ourselves to the build group
	SetBuildGroup(GetBuildGroup());
	
	SetMinimumSize(128,66);
	
	GetFocusNavGroup().SetFocusTopLevel(true);
	
#if !defined( _X360 )
	_sysMenu = NULL;

	// add dragging grips
	_topGrip = new GripPanel(this, "frame_topGrip", 0, -1);
	_bottomGrip = new GripPanel(this, "frame_bottomGrip", 0, 1);
	_leftGrip = new GripPanel(this, "frame_leftGrip", -1, 0);
	_rightGrip = new GripPanel(this, "frame_rightGrip", 1, 0);
	_topLeftGrip = new GripPanel(this, "frame_tlGrip", -1, -1);
	_topRightGrip = new GripPanel(this, "frame_trGrip", 1, -1);
	_bottomLeftGrip = new GripPanel(this, "frame_blGrip", -1, 1);
	_bottomRightGrip = new GripPanel(this, "frame_brGrip", 1, 1);
	_captionGrip = new CaptionGripPanel(this, "frame_caption" );
	_captionGrip->SetCursor(dc_arrow);

	_minimizeButton = new FrameButton(this, "frame_minimize","0");
	_minimizeButton->AddActionSignalTarget(this);
	_minimizeButton->SetCommand(new KeyValues("Minimize"));
	
	_maximizeButton = new FrameButton(this, "frame_maximize", "1");
	//!! no maximize handler implemented yet, so leave maximize button disabled
	SetMaximizeButtonVisible(false);

	char str[] = { 0x6F, 0 };
	_minimizeToSysTrayButton = new FrameButton(this, "frame_mintosystray", str);
	_minimizeToSysTrayButton->SetCommand("MinimizeToSysTray");
	SetMinimizeToSysTrayButtonVisible(false);
	
	_closeButton = new FrameButton(this, "frame_close", "r");
	_closeButton->AddActionSignalTarget(this);
	_closeButton->SetCommand(new KeyValues("CloseFrameButtonPressed"));
	
	if (!surface()->SupportsFeature(ISurface::FRAME_MINIMIZE_MAXIMIZE))
	{
		SetMinimizeButtonVisible(false);
		SetMaximizeButtonVisible(false);
	}

	if (parent)
	{
		// vgui doesn't support subwindow minimization
		SetMinimizeButtonVisible(false);
		SetMaximizeButtonVisible(false);
	}

	_menuButton = new FrameSystemButton(this, "frame_menu");
	_menuButton->SetMenu(GetSysMenu());
#endif
	
	SetupResizeCursors();

	REGISTER_COLOR_AS_OVERRIDABLE( m_InFocusBgColor, "infocus_bgcolor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_OutOfFocusBgColor, "outoffocus_bgcolor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _titleBarBgColor, "titlebarbgcolor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _titleBarDisabledBgColor, "titlebardisabledbgcolor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _titleBarFgColor, "titlebarfgcolor_override" );
	REGISTER_COLOR_AS_OVERRIDABLE( _titleBarDisabledFgColor, "titlebardisabledfgcolor_override" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Frame::~Frame()
{
	if ( input()->GetAppModalSurface() == GetVPanel() )
	{
		vgui::input()->ReleaseAppModalSurface();
		if ( m_hPreviousModal != 0 )
		{
			vgui::input()->SetAppModalSurface( m_hPreviousModal );
			m_hPreviousModal = 0;
		}
	}

#if !defined( _X360 )
	delete _topGrip;
	delete _bottomGrip;
	delete _leftGrip;
	delete _rightGrip;
	delete _topLeftGrip;
	delete _topRightGrip;
	delete _bottomLeftGrip;
	delete _bottomRightGrip;
	delete _captionGrip;
	delete _minimizeButton;
	delete _maximizeButton;
	delete _closeButton;
	delete _menuButton;
	delete _minimizeToSysTrayButton;
#endif
	delete _title;
}

//-----------------------------------------------------------------------------
// Purpose: Setup the grips on the edges of the panel to resize it.
//-----------------------------------------------------------------------------
void Frame::SetupResizeCursors()
{
#if !defined( _X360 )
	if (IsSizeable())
	{
		_topGrip->SetCursor(dc_sizens);
		_bottomGrip->SetCursor(dc_sizens);
		_leftGrip->SetCursor(dc_sizewe);
		_rightGrip->SetCursor(dc_sizewe);
		_topLeftGrip->SetCursor(dc_sizenwse);
		_topRightGrip->SetCursor(dc_sizenesw);
		_bottomLeftGrip->SetCursor(dc_sizenesw);
		_bottomRightGrip->SetCursor(dc_sizenwse);

		_bottomRightGrip->SetPaintEnabled(true);
		_bottomRightGrip->SetPaintBackgroundEnabled(true);
	}
	else
	{
		// not resizable, so just use the default cursor
		_topGrip->SetCursor(dc_arrow);
		_bottomGrip->SetCursor(dc_arrow);
		_leftGrip->SetCursor(dc_arrow);
		_rightGrip->SetCursor(dc_arrow);
		_topLeftGrip->SetCursor(dc_arrow);
		_topRightGrip->SetCursor(dc_arrow);
		_bottomLeftGrip->SetCursor(dc_arrow);
		_bottomRightGrip->SetCursor(dc_arrow);

		_bottomRightGrip->SetPaintEnabled(false);
		_bottomRightGrip->SetPaintBackgroundEnabled(false);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Bring the frame to the front and requests focus, ensures it's not minimized
//-----------------------------------------------------------------------------
void Frame::Activate()
{
	MoveToFront();
	if ( IsKeyBoardInputEnabled() )
	{
		RequestFocus();
	}
	SetVisible(true);
	SetEnabled(true);
	if (m_bFadingOut)
	{
		// we were fading out, make sure to fade back in
		m_bFadingOut = false;
		m_bPreviouslyVisible = false;
	}

	surface()->SetMinimized(GetVPanel(), false);
}


//-----------------------------------------------------------------------------
// Sets up, cleans up modal dialogs
//-----------------------------------------------------------------------------
void Frame::DoModal( )
{
	// move to the middle of the screen
	MoveToCenterOfScreen();
	InvalidateLayout();
	Activate();
	m_hPreviousModal = vgui::input()->GetAppModalSurface();
	vgui::input()->SetAppModalSurface( GetVPanel() );
}


//-----------------------------------------------------------------------------
// Closes a modal dialog
//-----------------------------------------------------------------------------
void Frame::CloseModal()
{
	vgui::input()->ReleaseAppModalSurface();
	if ( m_hPreviousModal != 0 )
	{
		vgui::input()->SetAppModalSurface( m_hPreviousModal );
		m_hPreviousModal = 0;
	}
	PostMessage( this, new KeyValues("Close") );
}


//-----------------------------------------------------------------------------
// Purpose: activates the dialog 
//			if dialog is not currently visible it starts it minimized and flashing in the taskbar
//-----------------------------------------------------------------------------
void Frame::ActivateMinimized()
{
	if ( ( IsVisible() && !IsMinimized() ) || !surface()->SupportsFeature( ISurface::FRAME_MINIMIZE_MAXIMIZE ) )
	{
		Activate();
	}
	else
	{
		ipanel()->MoveToBack(GetVPanel());
		surface()->SetMinimized(GetVPanel(), true);
		SetVisible(true);
		SetEnabled(true);
		if (m_bFadingOut)
		{
			// we were fading out, make sure to fade back in
			m_bFadingOut = false;
			m_bPreviouslyVisible = false;
		}
		FlashWindow();
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the dialog is currently minimized
//-----------------------------------------------------------------------------
bool Frame::IsMinimized()
{
	return surface()->IsMinimized(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Center the dialog on the screen
//-----------------------------------------------------------------------------
void Frame::MoveToCenterOfScreen()
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
}


void Frame::LayoutProportional( FrameButton *bt )
{
	float scale = 1.0;

	if ( IsProportional() )
	{	
		scale = scheme()->GetProportionalScaledValueEx( GetScheme(), 65535 ) / 65535.f;
	}

	bt->SetSize( (int)( FrameButton::GetButtonSide( this ) * scale ), (int)( FrameButton::GetButtonSide( this ) * scale ) );
	bt->SetTextInset( (int)( ceil( 2 * scale ) ), (int) ( ceil(1 * scale ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: per-frame thinking, used for transition effects
//			only gets called if the Frame is visible
//-----------------------------------------------------------------------------
void Frame::OnThink()
{
	BaseClass::OnThink();

	// check for transition effects
	if (IsVisible() && m_flTransitionEffectTime > 0 && ( !m_bDisableFadeEffect ))
	{
		if (m_bFadingOut)
		{
			// we're fading out, see if we're done so we can fully hide the window
			if (GetAlpha() < ( IsX360() ? 64 : 1 ))
			{
				FinishClose();
			}
		}
		else if (!m_bPreviouslyVisible)
		{
			// need to fade-in
			m_bPreviouslyVisible = true;
			
			// fade in
			if (IsX360())
			{
				SetAlpha(64);
			}
			else
			{
				SetAlpha(0);
			}
			GetAnimationController()->RunAnimationCommand(this, "alpha", 255.0f, 0.0f, m_flTransitionEffectTime, AnimationController::INTERPOLATOR_LINEAR);
		}
	}

	// check for focus changes
	bool hasFocus = false;

    if (input())
    {
	    VPANEL focus = input()->GetFocus();
	    if (focus && ipanel()->HasParent(focus, GetVPanel()))
	    {
		    if ( input()->GetAppModalSurface() == 0 || 
			    input()->GetAppModalSurface() == GetVPanel() )
		    {
			    hasFocus = true;
		    }
	    }
    }
	if (hasFocus != m_bHasFocus)
	{
		// Because vgui focus is message based, and focus gets reset to NULL when a focused panel is deleted, we defer the flashing/transition
		//  animation for an extra frame in case something is deleted, a message is sent, and then we become the focused panel again on the
		//  next frame
		if ( !m_bPrimed )
		{
			m_bPrimed = true;
			return;
		}
		m_bPrimed = false;
		m_bHasFocus = hasFocus;
		OnFrameFocusChanged(m_bHasFocus);
	}
	else
	{
		m_bPrimed = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the frame focus changes
//-----------------------------------------------------------------------------
void Frame::OnFrameFocusChanged(bool bHasFocus)
{
#if !defined( _X360 )
	// enable/disable the frame buttons
	_minimizeButton->SetDisabledLook(!bHasFocus);
	_maximizeButton->SetDisabledLook(!bHasFocus);
	_closeButton->SetDisabledLook(!bHasFocus);
	_minimizeToSysTrayButton->SetDisabledLook(!bHasFocus);
	_menuButton->SetEnabled(bHasFocus);
	_minimizeButton->InvalidateLayout();
	_maximizeButton->InvalidateLayout();
	_minimizeToSysTrayButton->InvalidateLayout();
	_closeButton->InvalidateLayout();
	_menuButton->InvalidateLayout();
#endif

	if (bHasFocus)
	{
		_title->SetColor(_titleBarFgColor);
	}
	else
	{
		_title->SetColor(_titleBarDisabledFgColor);
	}

	// set our background color
	if (bHasFocus)
	{
		if (m_flFocusTransitionEffectTime && ( !m_bDisableFadeEffect ))
		{
			GetAnimationController()->RunAnimationCommand(this, "BgColor", m_InFocusBgColor, 0.0f, m_bDisableFadeEffect ? 0.0f : m_flTransitionEffectTime, AnimationController::INTERPOLATOR_LINEAR);
		}
		else
		{
			SetBgColor(m_InFocusBgColor);
		}
	}
	else
	{
		if (m_flFocusTransitionEffectTime && ( !m_bDisableFadeEffect ))
		{
			GetAnimationController()->RunAnimationCommand(this, "BgColor", m_OutOfFocusBgColor, 0.0f, m_bDisableFadeEffect ? 0.0f : m_flTransitionEffectTime, AnimationController::INTERPOLATOR_LINEAR);
		}
		else
		{
			SetBgColor(m_OutOfFocusBgColor);
		}
	}

	// Stop flashing when we get focus
	if (bHasFocus && _flashWindow)
	{
		FlashWindowStop();
	}
}

int Frame::GetDraggerSize()
{
	const int DRAGGER_SIZE = 5;
	if ( m_bSmallCaption )
	{
		return 3;
	}
	
	return DRAGGER_SIZE;
}

int Frame::GetCornerSize()
{
	const int CORNER_SIZE = 8;
	if ( m_bSmallCaption )
	{
		return 6;
	}
	
	return CORNER_SIZE;
}

int Frame::GetBottomRightSize()
{
	const int BOTTOMRIGHTSIZE = 18;
	if ( m_bSmallCaption )
	{
		return 12;
	}
	
	return BOTTOMRIGHTSIZE;
}

int Frame::GetCaptionHeight()
{
	const int CAPTIONHEIGHT = 23;
	if ( m_bSmallCaption )
	{
		return 12;
	}
	return CAPTIONHEIGHT;
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the position of all items
//-----------------------------------------------------------------------------
void Frame::PerformLayout()
{
	// chain back
	BaseClass::PerformLayout();
	
	// move everything into place
	int wide, tall;
	GetSize(wide, tall);
		
#if !defined( _X360 )
	int DRAGGER_SIZE = QuickPropScale( GetDraggerSize() );
	int CORNER_SIZE = QuickPropScale( GetCornerSize() );
	int CORNER_SIZE2 = CORNER_SIZE * 2;
	int BOTTOMRIGHTSIZE = QuickPropScale( GetBottomRightSize() );

	_topGrip->SetBounds(CORNER_SIZE, 0, wide - CORNER_SIZE2, DRAGGER_SIZE);
	_leftGrip->SetBounds(0, CORNER_SIZE, DRAGGER_SIZE, tall - CORNER_SIZE2);
	_topLeftGrip->SetBounds(0, 0, CORNER_SIZE, CORNER_SIZE);
	_topRightGrip->SetBounds(wide - CORNER_SIZE, 0, CORNER_SIZE, CORNER_SIZE);
	_bottomLeftGrip->SetBounds(0, tall - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE);

	// make the bottom-right grip larger
	_bottomGrip->SetBounds(CORNER_SIZE, tall - DRAGGER_SIZE, wide - (CORNER_SIZE + BOTTOMRIGHTSIZE), DRAGGER_SIZE);
	_rightGrip->SetBounds(wide - DRAGGER_SIZE, CORNER_SIZE, DRAGGER_SIZE, tall - (CORNER_SIZE + BOTTOMRIGHTSIZE));

	_bottomRightGrip->SetBounds(wide - BOTTOMRIGHTSIZE, tall - BOTTOMRIGHTSIZE, BOTTOMRIGHTSIZE, BOTTOMRIGHTSIZE);
	
	_captionGrip->SetSize(wide- QuickPropScale(10), QuickPropScale( GetCaptionHeight() ));
	
	_topGrip->MoveToFront();
	_bottomGrip->MoveToFront();
	_leftGrip->MoveToFront();
	_rightGrip->MoveToFront();
	_topLeftGrip->MoveToFront();
	_topRightGrip->MoveToFront();
	_bottomLeftGrip->MoveToFront();
	_bottomRightGrip->MoveToFront();
	
	_maximizeButton->MoveToFront();
	_menuButton->MoveToFront();
	_minimizeButton->MoveToFront();
	_minimizeToSysTrayButton->MoveToFront();
	_menuButton->SetBounds( QuickPropScale(5+2), QuickPropScale(5+3), QuickPropScale( GetCaptionHeight()-5 ), QuickPropScale( GetCaptionHeight()-5 ));
#endif

	float scale = 1;
	if (IsProportional())
	{
		scale = scheme()->GetProportionalScaledValueEx( GetScheme(), 65535 ) / 65535.f;
	}
	
#if !defined( _X360 )
	int offset_start = (int)( 20 * scale );
	int offset = offset_start;

	int top_border_offset = (int) ( ( 5+3 ) * scale );
	if ( m_bSmallCaption )
	{
		top_border_offset = (int) ( ( 3 ) * scale );
	}

	int side_border_offset = (int) ( 5 * scale );
	// push the buttons against the east side
	if (_closeButton->IsVisible())
	{
		_closeButton->SetPos((wide-side_border_offset)-offset,top_border_offset);
		offset += offset_start;
		LayoutProportional( _closeButton );

	}
	if (_minimizeToSysTrayButton->IsVisible())
	{
		_minimizeToSysTrayButton->SetPos((wide-side_border_offset)-offset,top_border_offset);
		offset += offset_start;
		LayoutProportional( _minimizeToSysTrayButton );
	}
	if (_maximizeButton->IsVisible())
	{
		_maximizeButton->SetPos((wide-side_border_offset)-offset,top_border_offset);
		offset += offset_start;
		LayoutProportional( _maximizeButton );
	}
	if (_minimizeButton->IsVisible())
	{
		_minimizeButton->SetPos((wide-side_border_offset)-offset,top_border_offset);
		offset += offset_start;
		LayoutProportional( _minimizeButton );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Set the text in the title bar.
//-----------------------------------------------------------------------------
void Frame::SetTitle(const char *title, bool surfaceTitle)
{
	if (!_title)
	{
		_title = new TextImage( "" );
	}

	Assert(title);
	_title->SetText(title);

    // see if the combobox text has changed, and if so, post a message detailing the new text
	const char *newTitle = title;

	// check if the new text is a localized string, if so undo it
	wchar_t unicodeText[128];
	unicodeText[0] = 0;
	if (*newTitle == '#')
	{
		// try lookup in localization tables
		StringIndex_t unlocalizedTextSymbol = g_pVGuiLocalize->FindIndex(newTitle + 1);
		if (unlocalizedTextSymbol != INVALID_LOCALIZE_STRING_INDEX)
		{
			// we have a new text value
			wcsncpy( unicodeText, g_pVGuiLocalize->GetValueByIndex(unlocalizedTextSymbol), sizeof( unicodeText) / sizeof(wchar_t) );
		}
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( newTitle, unicodeText, sizeof(unicodeText) );
	}

	if (surfaceTitle)
	{
		surface()->SetTitle(GetVPanel(), unicodeText);
	}
	
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the unicode text in the title bar
//-----------------------------------------------------------------------------
void Frame::SetTitle(const wchar_t *title, bool surfaceTitle)
{
	if (!_title)
	{
		_title = new TextImage( "" );
	}
	_title->SetText(title);
	if (surfaceTitle)
	{
		surface()->SetTitle(GetVPanel(), title);
	}
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Set the text in the title bar.
//-----------------------------------------------------------------------------
void Frame::InternalSetTitle(const char *title)
{
	SetTitle(title, true);
}

//-----------------------------------------------------------------------------
// Purpose: Set the movability of the panel
//-----------------------------------------------------------------------------
void Frame::SetMoveable(bool state)
{
	_moveable=state;
}

//-----------------------------------------------------------------------------
// Purpose: Set the resizability of the panel
//-----------------------------------------------------------------------------
void Frame::SetSizeable(bool state)
{
	_sizeable=state;
	
	SetupResizeCursors();
}

// When moving via caption, don't let any part of window go outside parent's bounds
void Frame::SetClipToParent( bool state )
{
	m_bClipToParent = state;
}

bool Frame::GetClipToParent() const
{
	return m_bClipToParent;
}

//-----------------------------------------------------------------------------
// Purpose: Check the movability of the panel
//-----------------------------------------------------------------------------
bool Frame::IsMoveable()
{
	return _moveable;
}

//-----------------------------------------------------------------------------
// Purpose: Check the resizability of the panel
//-----------------------------------------------------------------------------
bool Frame::IsSizeable()
{
	return _sizeable;
}

//-----------------------------------------------------------------------------
// Purpose: Get the size of the panel inside the frame edges.
//-----------------------------------------------------------------------------
void Frame::GetClientArea(int &x, int &y, int &wide, int &tall)
{
	x = m_iClientInsetX;

	GetSize(wide, tall);

	if (_drawTitleBar)
	{
		int captionTall = surface()->GetFontTall(_title->GetFont());

		int border = QuickPropScale( m_bSmallCaption ? CAPTION_TITLE_BORDER_SMALL : CAPTION_TITLE_BORDER );
		int yinset = QuickPropScale( m_bSmallCaption ? 0 : m_iClientInsetY );

		yinset += QuickPropScale( m_iTitleTextInsetYOverride );

		y = yinset + captionTall + border + QuickPropScale( 1 );
		tall = (tall - yinset) - y;
	}
	
	if ( m_bSmallCaption )
	{
		tall -= QuickPropScale( 5 );
	}

	wide = (wide - m_iClientInsetX) - x;
}

// 
//-----------------------------------------------------------------------------
// Purpose: applies user configuration settings
//-----------------------------------------------------------------------------
void Frame::ApplyUserConfigSettings(KeyValues *userConfig)
{
	// calculate defaults
	int wx, wy, ww, wt;
	vgui::surface()->GetWorkspaceBounds(wx, wy, ww, wt);

	int x, y, wide, tall;
	GetBounds(x, y, wide, tall);
	bool bNoSettings = false;
	if (_moveable)
	{
		// check to see if anything is set
		if (!userConfig->FindKey("xpos", false))
		{
			bNoSettings = true;
		}

		// get the user config position
		// default to where we're currently at
		x = userConfig->GetInt("xpos", x);
		y = userConfig->GetInt("ypos", y);
	}
	if (_sizeable)
	{
		wide = userConfig->GetInt("wide", wide);
		tall = userConfig->GetInt("tall", tall);

		// Make sure it's no larger than the workspace
		if ( wide > ww )
		{
			wide = ww;
		}
		if ( tall > wt )
		{
			tall = wt; 
		}
	}

	// see if the dialog has a place on the screen it wants to start
	if (bNoSettings && GetDefaultScreenPosition(x, y, wide, tall))
	{
		bNoSettings = false;
	}

	// make sure it conforms to the minimum size of the dialog
	int minWide, minTall;
	GetMinimumSize(minWide, minTall);
	if (wide < minWide)
	{
		wide = minWide;
	}
	if (tall < minTall)
	{
		tall = minTall;
	}

	// make sure it's on the screen
	if (x + wide > ww)
	{
		x = wx + ww - wide;
	}
	if (y + tall > wt)
	{
		y = wy + wt - tall;
	}

	if (x < wx)
	{
		x = wx;
	}
	if (y < wy)
	{
		y = wy;
	}

	SetBounds(x, y, wide, tall);

	if (bNoSettings)
	{
		// since nothing was set, default our position to the middle of the screen
		MoveToCenterOfScreen();
	}

	BaseClass::ApplyUserConfigSettings(userConfig);
}

//-----------------------------------------------------------------------------
// Purpose: returns user config settings for this control
//-----------------------------------------------------------------------------
void Frame::GetUserConfigSettings(KeyValues *userConfig)
{
	if (_moveable)
	{
		int x, y;
		GetPos(x, y);
		userConfig->SetInt("xpos", x);
		userConfig->SetInt("ypos", y);
	}
	if (_sizeable)
	{
		int w, t;
		GetSize(w, t);
		userConfig->SetInt("wide", w);
		userConfig->SetInt("tall", t);
	}

	BaseClass::GetUserConfigSettings(userConfig);
}

//-----------------------------------------------------------------------------
// Purpose: optimization, return true if this control has any user config settings
//-----------------------------------------------------------------------------
bool Frame::HasUserConfigSettings()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: gets the default position and size on the screen to appear the first time (defaults to centered)
//-----------------------------------------------------------------------------
bool Frame::GetDefaultScreenPosition(int &x, int &y, int &wide, int &tall)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: draws title bar
//-----------------------------------------------------------------------------
void Frame::PaintBackground()
{
	// take the panel with focus and check up tree for this panel
	// if you find it, than some child of you has the focus, so
	// you should be focused
	Color titleColor = _titleBarDisabledBgColor;
	if (m_bHasFocus)
	{
		titleColor = _titleBarBgColor;
	}

	BaseClass::PaintBackground();

	if (_drawTitleBar)
	{
		int wide = GetWide();
		int tall = surface()->GetFontTall(_title->GetFont());

		// caption
		surface()->DrawSetColor(titleColor);
		int inset = QuickPropScale( m_bSmallCaption ? 3 : 5 );
		int captionHeight = QuickPropScale( m_bSmallCaption ? 14: 28 );

		surface()->DrawFilledRect(inset, inset, wide - inset, captionHeight );
		
		if (_title)
		{
			int nTitleX = QuickPropScale( m_iTitleTextInsetXOverride ? m_iTitleTextInsetXOverride : m_iTitleTextInsetX );
			int nTitleWidth = wide - QuickPropScale( 72 );
#if !defined( _X360 )
			if ( _menuButton && _menuButton->IsVisible() )
			{
				int mw, mh;
				_menuButton->GetImageSize( mw, mh );
				nTitleX += mw;
				nTitleWidth -= mw;
			}
#endif
			int nTitleY;
			if ( m_iTitleTextInsetYOverride )
			{
				nTitleY = QuickPropScale( m_iTitleTextInsetYOverride );
			}
			else
			{
				nTitleY = QuickPropScale( m_bSmallCaption ? 2 : 9 );
			}
			_title->SetPos( nTitleX, nTitleY );		
			_title->SetSize( nTitleWidth, tall);
			_title->Paint();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Frame::ApplySchemeSettings(IScheme *pScheme)
{
	// always chain back
	BaseClass::ApplySchemeSettings(pScheme);
	
	SetOverridableColor( &_titleBarFgColor, GetSchemeColor("FrameTitleBar.TextColor", pScheme) );
	SetOverridableColor( &_titleBarBgColor, GetSchemeColor("FrameTitleBar.BgColor", pScheme) );
	SetOverridableColor( &_titleBarDisabledFgColor, GetSchemeColor("FrameTitleBar.DisabledTextColor", pScheme) );
	SetOverridableColor( &_titleBarDisabledBgColor, GetSchemeColor("FrameTitleBar.DisabledBgColor", pScheme) );

	const char *font = NULL;
	if ( m_bSmallCaption )
	{
		font = pScheme->GetResourceString("FrameTitleBar.SmallFont");
	}
	else
	{
		font = pScheme->GetResourceString("FrameTitleBar.Font");
	}

	HFont titlefont;
	if ( m_hCustomTitleFont )
	{
		titlefont = m_hCustomTitleFont;
	}
	else
	{
		titlefont = pScheme->GetFont((font && *font) ? font : "Default", IsProportional());
	}

	_title->SetFont( titlefont );
	_title->ResizeImageToContent();

#if !defined( _X360 )
	HFont marfont = (HFont)0;
	if ( m_bSmallCaption )
	{
		marfont = pScheme->GetFont( "MarlettSmall", IsProportional() );
	}
	else
	{
		marfont = pScheme->GetFont( "Marlett", IsProportional() );
	}

	_minimizeButton->SetFont(marfont);
	_maximizeButton->SetFont(marfont);
	_minimizeToSysTrayButton->SetFont(marfont);
	_closeButton->SetFont(marfont);
#endif

	m_flTransitionEffectTime = atof(pScheme->GetResourceString("Frame.TransitionEffectTime"));
	m_flFocusTransitionEffectTime = atof(pScheme->GetResourceString("Frame.FocusTransitionEffectTime"));

	SetOverridableColor( &m_InFocusBgColor, pScheme->GetColor("Frame.BgColor", GetBgColor()) );
	SetOverridableColor( &m_OutOfFocusBgColor, pScheme->GetColor("Frame.OutOfFocusBgColor", m_InFocusBgColor) );

	const char *resourceString = pScheme->GetResourceString("Frame.ClientInsetX");
	if ( resourceString )
	{
		m_iClientInsetX = atoi(resourceString);
	}
	resourceString = pScheme->GetResourceString("Frame.ClientInsetY");
	if ( resourceString )
	{
		m_iClientInsetY = atoi(resourceString);
	}
	resourceString = pScheme->GetResourceString("Frame.TitleTextInsetX");
	if ( resourceString )
	{
		m_iTitleTextInsetX = atoi(resourceString);
	}

	SetBgColor(m_InFocusBgColor);
	SetBorder(pScheme->GetBorder("FrameBorder"));

	OnFrameFocusChanged( m_bHasFocus );
}

// Disables the fade-in/out-effect even if configured in the scheme settings
void Frame::DisableFadeEffect( void )
{
	m_flFocusTransitionEffectTime = 0.f;
	m_flTransitionEffectTime = 0.f;
}

void Frame::SetFadeEffectDisableOverride( bool disabled )
{
	m_bDisableFadeEffect = disabled;
}

//-----------------------------------------------------------------------------
// Purpose: Apply settings loaded from a resource file
//-----------------------------------------------------------------------------
void Frame::ApplySettings(KeyValues *inResourceData)
{
	// Don't change the frame's visibility, remove that setting from the config data
	inResourceData->SetInt("visible", -1);
	BaseClass::ApplySettings(inResourceData);

	SetCloseButtonVisible( inResourceData->GetBool( "setclosebuttonvisible", true ) );

	if( !inResourceData->GetInt("settitlebarvisible", 1 ) ) // if "title" is "0" then don't draw the title bar
	{
		SetTitleBarVisible( false );
	}
	
	// set the title
	const char *title = inResourceData->GetString("title", "");
	if (title && *title)
	{
		SetTitle(title, true);
	}

	const char *titlefont = inResourceData->GetString("title_font", "");
	if ( titlefont && titlefont[0] )
	{
		IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		if ( pScheme )
		{
			m_hCustomTitleFont = pScheme->GetFont( titlefont );
		}
	}

	KeyValues *pKV = inResourceData->FindKey( "clientinsetx_override", false );
	if ( pKV )
	{
		m_iClientInsetX = pKV->GetInt();
		m_iClientInsetXOverridden = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Apply settings loaded from a resource file
//-----------------------------------------------------------------------------
void Frame::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);
	outResourceData->SetInt("settitlebarvisible", _drawTitleBar );

	if (_title)
	{
		char buf[256];
		_title->GetUnlocalizedText( buf, 255 );
		if (buf[0])
		{
			outResourceData->SetString("title", buf);
		}
	}

	if ( m_iClientInsetXOverridden )
	{
		outResourceData->SetInt( "clientinsetx_override", m_iClientInsetX );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns a description of the settings possible for a frame
//-----------------------------------------------------------------------------
const char *Frame::GetDescription()
{
	static char buf[512];
	Q_snprintf(buf, sizeof(buf), "%s, string title", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Go invisible when a close message is recieved.
//-----------------------------------------------------------------------------
void Frame::OnClose()
{
	// if we're modal, release that before we hide the window else the wrong window will get focus
	if (input()->GetAppModalSurface() == GetVPanel())
	{
		input()->ReleaseAppModalSurface();
		if ( m_hPreviousModal != 0 )
		{
			vgui::input()->SetAppModalSurface( m_hPreviousModal );
			m_hPreviousModal = 0;
		}
	}
	
	BaseClass::OnClose();

	if (m_flTransitionEffectTime && !m_bDisableFadeEffect)
	{
		// begin the hide transition effect
		GetAnimationController()->RunAnimationCommand(this, "alpha", 0.0f, 0.0f, m_flTransitionEffectTime, AnimationController::INTERPOLATOR_LINEAR);
		m_bFadingOut = true;
		// move us to the back of the draw order (so that fading out over the top of other dialogs doesn't look wierd)
		surface()->MovePopupToBack(GetVPanel());
	}
	else
	{
		// hide us immediately
		FinishClose();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Close button in frame pressed
//-----------------------------------------------------------------------------
void Frame::OnCloseFrameButtonPressed()
{
	OnCommand("Close");
}

//-----------------------------------------------------------------------------
// Purpose: Command handling
//-----------------------------------------------------------------------------
void Frame::OnCommand(const char *command)
{
	if (!stricmp(command, "Close"))
	{
		Close();
	}
	else if (!stricmp(command, "CloseModal"))
	{
		CloseModal();
	}
	else if (!stricmp(command, "Minimize"))
	{
		OnMinimize();
	}
	else if (!stricmp(command, "MinimizeToSysTray"))
	{
		OnMinimizeToSysTray();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get the system menu 
//-----------------------------------------------------------------------------
Menu *Frame::GetSysMenu()
{
#if !defined( _X360 )
	if (!_sysMenu)
	{
		_sysMenu = new Menu(this, NULL);
		_sysMenu->SetVisible(false);
		_sysMenu->AddActionSignalTarget(this);

		_sysMenu->AddMenuItem("Minimize", "#SysMenu_Minimize", "Minimize", this);
		_sysMenu->AddMenuItem("Maximize", "#SysMenu_Maximize", "Maximize", this);
		_sysMenu->AddMenuItem("Close", "#SysMenu_Close", "Close", this);

		// check for enabling/disabling menu items
		// this might have to be done at other times as well. 
		Panel *menuItem = _sysMenu->FindChildByName("Minimize");
		if (menuItem)
		{
			menuItem->SetEnabled(_minimizeButton->IsVisible());
		}
		menuItem = _sysMenu->FindChildByName("Maximize");
		if (menuItem)
		{
			menuItem->SetEnabled(_maximizeButton->IsVisible());
		}
		menuItem = _sysMenu->FindChildByName("Close");
		if (menuItem)
		{
			menuItem->SetEnabled(_closeButton->IsVisible());
		}
	}
	
	return _sysMenu;
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Set the system menu  
//-----------------------------------------------------------------------------
void Frame::SetSysMenu(Menu *menu)
{
#if !defined( _X360 )
	if (menu == _sysMenu)
		return;
	
	_sysMenu->MarkForDeletion();
	_sysMenu = menu;

	_menuButton->SetMenu(_sysMenu);
#endif
}


//-----------------------------------------------------------------------------
// Set the system menu images
//-----------------------------------------------------------------------------
void Frame::SetImages( const char *pEnabledImage, const char *pDisabledImage )
{
#if !defined( _X360 )
	_menuButton->SetImages( pEnabledImage, pDisabledImage );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Close the window 
//-----------------------------------------------------------------------------
void Frame::Close()
{
	OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Finishes closing the dialog
//-----------------------------------------------------------------------------
void Frame::FinishClose()
{
	SetVisible(false);
	m_bPreviouslyVisible = false;
	m_bFadingOut = false;

	OnFinishedClose();
	
	if (m_bDeleteSelfOnClose)
	{
		// Must be last because if vgui is not running then this will call delete this!!!
		MarkForDeletion();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Frame::OnFinishedClose()
{
}

//-----------------------------------------------------------------------------
// Purpose: Minimize the window on the taskbar.
//-----------------------------------------------------------------------------
void Frame::OnMinimize()
{
	surface()->SetMinimized(GetVPanel(), true);
}

//-----------------------------------------------------------------------------
// Purpose: Does nothing by default
//-----------------------------------------------------------------------------
void Frame::OnMinimizeToSysTray()
{
}

//-----------------------------------------------------------------------------
// Purpose: Respond to mouse presses
//-----------------------------------------------------------------------------
void Frame::OnMousePressed(MouseCode code)
{
	if (!IsBuildGroupEnabled())
	{
		// if a child doesn't have focus, get it for ourselves
		VPANEL focus = input()->GetFocus();
		if (!focus || !ipanel()->HasParent(focus, GetVPanel()))
		{
			RequestFocus();
		}
	}
	
	BaseClass::OnMousePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose: Toggle visibility of the system menu button
//-----------------------------------------------------------------------------
void Frame::SetMenuButtonVisible(bool state)
{
#if !defined( _X360 )
	_menuButton->SetVisible(state);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Toggle respond of the system menu button
//			it will look enabled or disabled in response to the title bar
//			but may not activate.
//-----------------------------------------------------------------------------
void Frame::SetMenuButtonResponsive(bool state)
{
#if !defined( _X360 )
	_menuButton->SetResponsive(state);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Toggle visibility of the minimize button
//-----------------------------------------------------------------------------
void Frame::SetMinimizeButtonVisible(bool state)
{
#if !defined( _X360 )
	_minimizeButton->SetVisible(state);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Toggle visibility of the maximize button
//-----------------------------------------------------------------------------
void Frame::SetMaximizeButtonVisible(bool state)
{
#if !defined( _X360 )
	_maximizeButton->SetVisible(state);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Toggles visibility of the minimize-to-systray icon (defaults to false)
//-----------------------------------------------------------------------------
void Frame::SetMinimizeToSysTrayButtonVisible(bool state)
{
#if !defined( _X360 )
	_minimizeToSysTrayButton->SetVisible(state);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Toggle visibility of the close button
//-----------------------------------------------------------------------------
void Frame::SetCloseButtonVisible(bool state)
{
#if !defined( _X360 )
	_closeButton->SetVisible(state);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: soaks up any remaining messages
//-----------------------------------------------------------------------------
void Frame::OnKeyCodeReleased(KeyCode code)
{
}

//-----------------------------------------------------------------------------
// Purpose: soaks up any remaining messages
//-----------------------------------------------------------------------------
void Frame::OnKeyFocusTicked()
{
}

//-----------------------------------------------------------------------------
// Purpose: Toggles window flash state on a timer
//-----------------------------------------------------------------------------
void Frame::InternalFlashWindow()
{
	if (_flashWindow)
	{
		// toggle icon flashing
		_nextFlashState = true;
		surface()->FlashWindow(GetVPanel(), _nextFlashState);
		_nextFlashState = !_nextFlashState;
		
		PostMessage(this, new KeyValues("FlashWindow"), 1.8f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds the child to the focus nav group
//-----------------------------------------------------------------------------
void Frame::OnChildAdded(VPANEL child)
{
	BaseClass::OnChildAdded(child);
}

//-----------------------------------------------------------------------------
// Purpose: Flash the window system tray button until the frame gets focus
//-----------------------------------------------------------------------------
void Frame::FlashWindow()
{
	_flashWindow = true;
	_nextFlashState = true;
	
	InternalFlashWindow();
}

//-----------------------------------------------------------------------------
// Purpose: Stops any window flashing
//-----------------------------------------------------------------------------
void Frame::FlashWindowStop()
{
	surface()->FlashWindow(GetVPanel(), false);
	_flashWindow = false;
}


//-----------------------------------------------------------------------------
// Purpose: load the control settings - should be done after all the children are added to the dialog
//-----------------------------------------------------------------------------
void Frame::LoadControlSettings( const char *dialogResourceName, const char *pathID, KeyValues *pPreloadedKeyValues, KeyValues *pConditions )
{
	BaseClass::LoadControlSettings( dialogResourceName, pathID, pPreloadedKeyValues, pConditions );

	// set the focus on the default control
	Panel *defaultFocus = GetFocusNavGroup().GetDefaultPanel();
	if (defaultFocus)
	{
		defaultFocus->RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks for ctrl+shift+b hits to enter build mode
//			Activates any hotkeys / default buttons
//			Swallows any unhandled input
//-----------------------------------------------------------------------------
void Frame::OnKeyCodeTyped(KeyCode code)
{
	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));
	
	if ( IsX360() )
	{
		vgui::Panel *pMap = FindChildByName( "ControllerMap" );
		if ( pMap && pMap->IsKeyBoardInputEnabled() )
		{
			pMap->OnKeyCodeTyped( code );
			return;
		}
	}

	if ( ctrl && shift && alt && code == KEY_B)
	{
		// enable build mode
		ActivateBuildMode();
	}
	else if (ctrl && shift && alt && code == KEY_R)
	{
		// reload the scheme
		VPANEL top = surface()->GetEmbeddedPanel();
		if (top)
		{
			// reload the data file
			scheme()->ReloadSchemes();

			Panel *panel = ipanel()->GetPanel(top, GetModuleName());
			if (panel)
			{
				// make the top-level panel reload it's scheme, it will chain down to all the child panels
				panel->InvalidateLayout(false, true);
			}
		}
	}
	else if (alt && code == KEY_F4)
	{
		// user has hit the close
		PostMessage(this, new KeyValues("CloseFrameButtonPressed"));
	}
	else if (code == KEY_ENTER)
	{
		// check for a default button
		VPANEL panel = GetFocusNavGroup().GetCurrentDefaultButton();
		if (panel && ipanel()->IsVisible( panel ) && ipanel()->IsEnabled( panel ))
		{
			// Activate the button
			PostMessage(panel, new KeyValues("Hotkey"));
		}
	}
	else if ( code == KEY_ESCAPE && 
		surface()->SupportsFeature(ISurface::ESCAPE_KEY) && 
		input()->GetAppModalSurface() == GetVPanel() )
	{
		// ESC cancels, unless we're in the engine - in the engine ESC flips between the UI and the game
		CloseModal();
	}
	// Usually don't chain back as Frames are the end of the line for key presses, unless
	// m_bChainKeysToParent is set
	else if ( m_bChainKeysToParent )
	{
		BaseClass::OnKeyCodeTyped( code );
	}
	else
	{
		input()->OnKeyCodeUnhandled( (int)code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: If true, then OnKeyCodeTyped messages continue up past the Frame
// Input  : state - 
//-----------------------------------------------------------------------------
void Frame::SetChainKeysToParent( bool state )
{
	m_bChainKeysToParent = state;
}

//-----------------------------------------------------------------------------
// Purpose: If true, then OnKeyCodeTyped messages continue up past the Frame
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Frame::CanChainKeysToParent() const
{
	return m_bChainKeysToParent;
}

//-----------------------------------------------------------------------------
// Purpose: Checks for ctrl+shift+b hits to enter build mode
//			Activates any hotkeys / default buttons
//			Swallows any unhandled input
//-----------------------------------------------------------------------------
void Frame::OnKeyTyped(wchar_t unichar)
{
	Panel *panel = GetFocusNavGroup().FindPanelByHotkey(unichar);
	if (panel)
	{
		// tell the panel to Activate
		PostMessage(panel, new KeyValues("Hotkey"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets all title bar controls
//-----------------------------------------------------------------------------
void Frame::SetTitleBarVisible( bool state )
{
	_drawTitleBar = state; 
	SetMenuButtonVisible(state);
	SetMinimizeButtonVisible(state);
	SetMaximizeButtonVisible(state);
	SetCloseButtonVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: sets the frame to delete itself on close
//-----------------------------------------------------------------------------
void Frame::SetDeleteSelfOnClose( bool state )
{
	m_bDeleteSelfOnClose = state;
}

//-----------------------------------------------------------------------------
// Purpose: updates localized text
//-----------------------------------------------------------------------------
void Frame::OnDialogVariablesChanged( KeyValues *dialogVariables )
{
	StringIndex_t index = _title->GetUnlocalizedTextSymbol();
	if (index != INVALID_LOCALIZE_STRING_INDEX)
	{
		// reconstruct the string from the variables
		wchar_t buf[1024];
		g_pVGuiLocalize->ConstructString(buf, sizeof(buf), index, dialogVariables);
		SetTitle(buf, true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles staying on screen when the screen size changes
//-----------------------------------------------------------------------------
void Frame::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
	BaseClass::OnScreenSizeChanged(iOldWide, iOldTall);

	if (IsProportional())
		return;

	// make sure we're completely on screen
	int iNewWide, iNewTall;
	surface()->GetScreenSize(iNewWide, iNewTall);

	int x, y, wide, tall;
	GetBounds(x, y, wide, tall);

	// make sure the bottom-right corner is on the screen first
	if (x + wide > iNewWide)
	{
		x = iNewWide - wide;
	}
	if (y + tall > iNewTall)
	{
		y = iNewTall - tall;
	}

	// make sure the top-left is visible
	x = max( 0, x );
	y = max( 0, y );

	// apply
	SetPos(x, y);
}

//-----------------------------------------------------------------------------
// Purpose: For supporting thin caption bars
// Input  : state - 
//-----------------------------------------------------------------------------
void Frame::SetSmallCaption( bool state )
{
	m_bSmallCaption = state;
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Frame::IsSmallCaption() const
{
	return m_bSmallCaption;
}


//-----------------------------------------------------------------------------
// Purpose: Static method to place a frame under the cursor
//-----------------------------------------------------------------------------
void Frame::PlaceUnderCursor( )
{
	// get cursor position, this is local to this text edit window
	int cursorX, cursorY;
	input()->GetCursorPos( cursorX, cursorY );

	// relayout the menu immediately so that we know it's size
	InvalidateLayout(true);
	int w, h;
	GetSize( w, h );

	// work out where the cursor is and therefore the best place to put the frame
	int sw, sh;
	surface()->GetScreenSize( sw, sh );

	// Try to center it first
	int x, y;
	x = cursorX - ( w / 2 );
	y = cursorY - ( h / 2 );

	// Clamp to various sides
	if ( x + w > sw )
	{
		x = sw - w;
	}
	if ( y + h > sh )
	{
		y = sh - h;
	}
	if ( x < 0 )
	{
		x = 0;
	}
	if ( y < 0 )
	{
		y = 0;
	}

	SetPos( x, y );
}
