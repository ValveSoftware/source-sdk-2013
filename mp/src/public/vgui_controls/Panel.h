//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef PANEL_H
#define PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlflags.h"
#include "vgui/VGUI.h"
#include "vgui/Dar.h"
#include "vgui_controls/MessageMap.h"
#if defined( VGUI_USEKEYBINDINGMAPS )
#include "vgui_controls/KeyBindingMap.h"
#endif
#include "vgui/IClientPanel.h"
#include "vgui/IScheme.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/PanelAnimationVar.h"
#include "Color.h"
#include "vstdlib/IKeyValuesSystem.h"
#include "tier1/utlsymbol.h"
#include "vgui_controls/BuildGroup.h"

// undefine windows function macros that overlap 
#ifdef PostMessage
#undef PostMessage
#endif

#ifdef SetCursor
#undef SetCursor
#endif

class CUtlBuffer;

namespace vgui
{

#if !defined( _X360 )
#define VGUI_USEDRAGDROP 1
#endif

#if defined( VGUI_USEKEYBINDINGMAPS )
struct PanelKeyBindingMap;
#endif
//-----------------------------------------------------------------------------
// Purpose: Helper functions to construct vgui panels
//
//  SETUP_PANEL - will make a panel ready for use right now (i.e setup its colors, borders, fonts, etc)
//
template< class T >
inline T *SETUP_PANEL(T *panel)
{
	panel->MakeReadyForUse();
	return panel;
}

//
// CREATE_PANEL - creates a panel that is ready to use right now
//
//   example of use = to set the FG Color of a panel inside of a constructor (i.e before ApplySchemeSettings() has been run on the child)
//
#define CREATE_PANEL(type, parent, name) (SETUP_PANEL(new type(parent, name)))

//-----------------------------------------------------------------------------
// Purpose: Drag/drop support context info (could defined within Panel...)
//-----------------------------------------------------------------------------
#if defined( VGUI_USEDRAGDROP )
struct DragDrop_t;
class Menu;
#endif

//-----------------------------------------------------------------------------
// Purpose: Macro to handle Colors that can be overridden in .res files
//-----------------------------------------------------------------------------
struct OverridableColorEntry
{
	char const *name() { return m_pszScriptName; }

	char const	*m_pszScriptName;
	Color		*m_pColor;
	Color		m_colFromScript;
	bool		m_bOverridden;
};

#define REGISTER_COLOR_AS_OVERRIDABLE( name, scriptname )			\
	AddToOverridableColors( &name, scriptname );



//-----------------------------------------------------------------------------
// Purpose: For hudanimations.txt scripting of vars
//-----------------------------------------------------------------------------
class IPanelAnimationPropertyConverter
{
public:
	virtual void GetData( Panel *panel, KeyValues *kv, PanelAnimationMapEntry *entry ) = 0;
	virtual void SetData( Panel *panel, KeyValues *kv, PanelAnimationMapEntry *entry ) = 0;
	virtual void InitFromDefault( Panel *panel, PanelAnimationMapEntry *entry ) = 0;
};

#if defined( VGUI_USEKEYBINDINGMAPS )
enum KeyBindingContextHandle_t
{
	INVALID_KEYBINDINGCONTEXT_HANDLE = 0xffffffff,
};
#endif

class IForceVirtualInheritancePanel
{
	// We need Panel to use virtual inheritance so that
	// pointers to its members are max size.
	// This is due to a limitation in C++ with ahead
	// declarations of points to members as used in MessageMap.
};

//=============================================================================
// HPE_BEGIN:
// [tj] bitwise defines for rounded corners
//=============================================================================
#define PANEL_ROUND_CORNER_TOP_LEFT		(1 << 0)
#define PANEL_ROUND_CORNER_TOP_RIGHT	(1 << 1)
#define PANEL_ROUND_CORNER_BOTTOM_LEFT	(1 << 2)
#define PANEL_ROUND_CORNER_BOTTOM_RIGHT (1 << 3)
#define PANEL_ROUND_CORNER_ALL			PANEL_ROUND_CORNER_TOP_LEFT | PANEL_ROUND_CORNER_TOP_RIGHT | PANEL_ROUND_CORNER_BOTTOM_LEFT | PANEL_ROUND_CORNER_BOTTOM_RIGHT
//=============================================================================
// HPE_END
//=============================================================================//-----------------------------------------------------------------------------
// Purpose: Base interface to all vgui windows
//			All vgui controls that receive message and/or have a physical presence
//			on screen are be derived from Panel.
//			This is designed as an easy-access to the vgui-functionality; for more
//			low-level access to vgui functions use the IPanel/IClientPanel interfaces directly
//-----------------------------------------------------------------------------
class Panel : public IClientPanel, virtual IForceVirtualInheritancePanel
{
	DECLARE_CLASS_SIMPLE_NOBASE( Panel );

public:
	// For property mapping
	static void InitPropertyConverters( void );
	static void AddPropertyConverter( char const *typeName, IPanelAnimationPropertyConverter *converter );

	//-----------------------------------------------------------------------------
	// CONSTRUCTORS
	// these functions deal with the creation of the Panel
	// the Panel automatically gets a handle to a vgui-internal panel, the ipanel(), upon construction
	// vgui interfaces deal only with ipanel(), not Panel directly
	Panel();
	Panel(Panel *parent);
	Panel(Panel *parent, const char *panelName);
	Panel(Panel *parent, const char *panelName, HScheme scheme);

	virtual ~Panel();

	// returns pointer to Panel's vgui VPanel interface handle
	virtual VPANEL GetVPanel() { return _vpanel; }
	HPanel ToHandle() const;

	virtual void Init( int x, int y, int wide, int tall );

	//-----------------------------------------------------------------------------
	// PANEL METHODS
	// these functions all manipulate panels
	// they cannot be derived from
	void SetName(const char *panelName);  // sets the name of the panel - used as an identifier
	const char *GetName();		// returns the name of this panel... never NULL
	const char *GetClassName(); // returns the class name of the panel (eg. Panel, Label, Button, etc.)

	void MakeReadyForUse(); // fully construct this panel so its ready for use right now (i.e fonts loaded, colors set, default label text set, ...)

	// panel position & size
	// all units are in pixels
	void SetPos(int x,int y);		// sets position of panel, in local space (ie. relative to parent's position)
	void GetPos(int &x,int &y);		// gets local position of panel
	void SetSize(int wide,int tall);	// sets size of panel
	void GetSize(int &wide, int &tall);	// gets size of panel
	void SetBounds(int x, int y, int wide, int tall);		// combination of SetPos/SetSize
	void GetBounds(int &x, int &y, int &wide, int &tall);	// combination of GetPos/GetSize
	int  GetWide();	// returns width of panel
	void SetWide(int wide);	// sets width of panel
	int  GetTall();	// returns height of panel
	void SetTall(int tall);	// sets height of panel
	void SetMinimumSize(int wide,int tall);		// sets the minimum size the panel can go
	void GetMinimumSize(int& wide,int& tall);	// gets the minimum size
	bool IsBuildModeEditable();	  // editable in the buildModeDialog?
	void SetBuildModeEditable(bool state);  // set buildModeDialog editable
	bool IsBuildModeDeletable();  // deletable in the buildModeDialog?
	void SetBuildModeDeletable(bool state);	// set buildModeDialog deletable
	bool IsBuildModeActive();	// true if we're currently in edit mode
	void SetZPos(int z);	// sets Z ordering - lower numbers are always behind higher z's
	int  GetZPos( void );
	void SetAlpha(int alpha);	// sets alpha modifier for panel and all child panels [0..255]
	int GetAlpha();	// returns the current alpha

	// panel visibility
	// invisible panels and their children do not drawn, updated, or receive input messages
	virtual void SetVisible(bool state);
	virtual bool IsVisible();

	// painting
	virtual VPANEL IsWithinTraverse(int x, int y, bool traversePopups);	// recursive; returns a pointer to the panel at those coordinates
	MESSAGE_FUNC( Repaint, "Repaint" );							// marks the panel as needing to be repainted
	virtual void PostMessage(VPANEL target, KeyValues *message, float delaySeconds = 0.0f);

	bool IsWithin(int x, int y); //in screen space
	void LocalToScreen(int &x, int &y);
	void ScreenToLocal(int &x, int &y);
	void ParentLocalToScreen(int &x, int &y);
	void MakePopup(bool showTaskbarIcon = true,bool disabled = false);		// turns the panel into a popup window (ie. can draw outside of it's parents space)
	virtual void OnMove();

	// panel hierarchy
	virtual Panel *GetParent();
	virtual VPANEL GetVParent();
	virtual void SetParent(Panel *newParent);
	virtual void SetParent(VPANEL newParent);
	virtual bool HasParent(VPANEL potentialParent);
	
	int GetChildCount();
	Panel *GetChild(int index);
	virtual CUtlVector< VPANEL > &GetChildren();
	int FindChildIndexByName( const char *childName );
	Panel *FindChildByName(const char *childName, bool recurseDown = false);
	Panel *FindSiblingByName(const char *siblingName);
	void CallParentFunction(KeyValues *message);

	template <class T>
	T *FindControl( const char *pszName, bool recurseDown = false ) { return dynamic_cast<T *>( FindChildByName( pszName, recurseDown ) ); }

	virtual void SetAutoDelete(bool state);		// if set to true, panel automatically frees itself when parent is deleted
	virtual bool IsAutoDeleteSet();
	virtual void DeletePanel();				// simply does a  { delete this; }

	// messaging
	virtual void AddActionSignalTarget(Panel *messageTarget);
	virtual void AddActionSignalTarget(VPANEL messageTarget);
	virtual void RemoveActionSignalTarget(Panel *oldTarget);
	virtual void PostActionSignal(KeyValues *message);			// sends a message to the current actionSignalTarget(s)
	virtual bool RequestInfoFromChild(const char *childName, KeyValues *outputData);
	virtual void PostMessageToChild(const char *childName, KeyValues *messsage);
	virtual void PostMessage(Panel *target, KeyValues *message, float delaySeconds = 0.0f);
	virtual bool RequestInfo(KeyValues *outputData);				// returns true if output is successfully written.  You should always chain back to the base class if info request is not handled
	virtual bool SetInfo(KeyValues *inputData);						// sets a specified value in the control - inverse of the above
	virtual void SetSilentMode( bool bSilent );						//change the panel's silent mode; if silent, the panel will not post any action signals

	// install a mouse handler
	virtual void InstallMouseHandler( Panel *pHandler );	// mouse events will be send to handler panel instead of this panel

	// drawing state
	virtual void   SetEnabled(bool state);
	virtual bool   IsEnabled();
	virtual bool   IsPopup();	// has a parent, but is in it's own space
	virtual void   GetClipRect(int &x0, int &y0, int &x1, int &y1);
	virtual void   MoveToFront();

	// pin positions for auto-layout
	enum PinCorner_e 
	{
		PIN_TOPLEFT = 0,
		PIN_TOPRIGHT,
		PIN_BOTTOMLEFT,
		PIN_BOTTOMRIGHT,

		// For sibling pinning
		PIN_CENTER_TOP,
		PIN_CENTER_RIGHT,
		PIN_CENTER_BOTTOM,
		PIN_CENTER_LEFT,
	};

	// specifies the auto-resize directions for the panel
	enum AutoResize_e
	{
		AUTORESIZE_NO = 0,
		AUTORESIZE_RIGHT,
		AUTORESIZE_DOWN,
		AUTORESIZE_DOWNANDRIGHT,
	};

	// Sets the pin corner for non-resizing panels
	void SetPinCorner( PinCorner_e pinCorner, int nOffsetX, int nOffsetY );

	// Sets the pin corner + resize mode for resizing panels
	void SetAutoResize( PinCorner_e pinCorner, AutoResize_e resizeDir, int nPinOffsetX, int nPinOffsetY, int nUnpinnedCornerOffsetX, int nUnpinnedCornerOffsetY );

	AutoResize_e GetAutoResize();
	PinCorner_e GetPinCorner();

	// Gets the relative offset of the control from the pinned + non-pinned corner (for resizing)
	void GetPinOffset( int &dx, int &dy );
	void GetResizeOffset( int &dx, int &dy );

	void PinToSibling( const char *pszSibling, PinCorner_e pinOurCorner, PinCorner_e pinSibling );
	void UpdateSiblingPin( void );

	// colors
	virtual void SetBgColor(Color color);
	virtual void SetFgColor(Color color);
	virtual Color GetBgColor();
	virtual Color GetFgColor();

	virtual void SetCursor(HCursor cursor);
	virtual HCursor GetCursor();
	virtual void SetCursorAlwaysVisible( bool visible );
	virtual void RequestFocus(int direction = 0);
	virtual bool HasFocus();
	virtual void InvalidateLayout(bool layoutNow = false, bool reloadScheme = false);
	virtual bool RequestFocusPrev(VPANEL panel = NULL);
	virtual bool RequestFocusNext(VPANEL panel = NULL);
	// tab positioning
	virtual void   SetTabPosition(int position);
	virtual int    GetTabPosition();
	// border
	virtual void SetBorder(IBorder *border);
	virtual IBorder *GetBorder();
	virtual void SetPaintBorderEnabled(bool state);
	virtual void SetPaintBackgroundEnabled(bool state);
	virtual void SetPaintEnabled(bool state);
	virtual void SetPostChildPaintEnabled(bool state);
	virtual void SetPaintBackgroundType(int type);  // 0 for normal(opaque), 1 for single texture from Texture1, and 2 for rounded box w/ four corner textures
	virtual void GetInset(int &left, int &top, int &right, int &bottom);
	virtual void GetPaintSize(int &wide, int &tall);
	virtual void SetBuildGroup(BuildGroup *buildGroup);
	virtual bool IsBuildGroupEnabled();
	virtual bool IsCursorNone();
	virtual bool IsCursorOver();		// returns true if the cursor is currently over the panel
	virtual void MarkForDeletion();		// object will free it's memory next tick
	virtual bool IsLayoutInvalid();		// does this object require a perform layout?
	virtual Panel *HasHotkey(wchar_t key);			// returns the panel that has this hotkey
	virtual bool IsOpaque();
	bool IsRightAligned();		// returns true if the settings are aligned to the right of the screen
	bool IsBottomAligned();		// returns true if the settings are aligned to the bottom of the screen

	// scheme access functions
	virtual HScheme GetScheme();
	virtual void SetScheme(const char *tag);
	virtual void SetScheme(HScheme scheme);
	virtual Color GetSchemeColor(const char *keyName,IScheme *pScheme);
	virtual Color GetSchemeColor(const char *keyName, Color defaultColor,IScheme *pScheme);

	// called when scheme settings need to be applied; called the first time before the panel is painted
	virtual void ApplySchemeSettings(IScheme *pScheme);

	// interface to build settings
	// takes a group of settings and applies them to the control
	virtual void ApplySettings(KeyValues *inResourceData);

	// records the settings into the resource data
	virtual void GetSettings(KeyValues *outResourceData);

	// gets a description of the resource for use in the UI
	// format: <type><whitespace | punctuation><keyname><whitespace| punctuation><type><whitespace | punctuation><keyname>...
	// unknown types as just displayed as strings in the UI (for future UI expansion)
	virtual const char *GetDescription();

	// returns the name of the module that this instance of panel was compiled into
	virtual const char *GetModuleName();

	// user configuration settings
	// this is used for any control details the user wants saved between sessions
	// eg. dialog positions, last directory opened, list column width
	virtual void ApplyUserConfigSettings(KeyValues *userConfig);

	// returns user config settings for this control
	virtual void GetUserConfigSettings(KeyValues *userConfig);

	// optimization, return true if this control has any user config settings
	virtual bool HasUserConfigSettings();

	// message handlers
	// override to get access to the message
	// override to get access to the message
	virtual void OnMessage(const KeyValues *params, VPANEL fromPanel);	// called when panel receives message; must chain back
	MESSAGE_FUNC_CHARPTR( OnCommand, "Command", command );	// called when a panel receives a command
	MESSAGE_FUNC( OnMouseCaptureLost, "MouseCaptureLost" );	// called after the panel loses mouse capture
	MESSAGE_FUNC( OnSetFocus, "SetFocus" );			// called after the panel receives the keyboard focus
	MESSAGE_FUNC( OnKillFocus, "KillFocus" );		// called after the panel loses the keyboard focus
	MESSAGE_FUNC( OnDelete, "Delete" );				// called to delete the panel; Panel::OnDelete() does simply { delete this; }
	virtual void OnThink();							// called every frame before painting, but only if panel is visible
	virtual void OnChildAdded(VPANEL child);		// called when a child has been added to this panel
	virtual void OnSizeChanged(int newWide, int newTall);	// called after the size of a panel has been changed
	
	// called every frame if ivgui()->AddTickSignal() is called
	virtual void OnTick();

	// input messages
	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnMouseWheeled(int delta);

	// Trip pressing (e.g., select all text in a TextEntry) requires this to be enabled
	virtual void SetTriplePressAllowed( bool state );
	virtual bool IsTriplePressAllowed() const;
	virtual void OnMouseTriplePressed( MouseCode code );

	static char const	*KeyCodeToString( KeyCode code );
	static wchar_t const *KeyCodeToDisplayString( KeyCode code );
	static wchar_t const *KeyCodeModifiersToDisplayString( KeyCode code, int modifiers ); // L"Ctrl+Alt+Shift+Backspace"

	static KeyCode		StringToKeyCode( char const *str );
#if defined( VGUI_USEKEYBINDINGMAPS )
	static KeyBindingContextHandle_t   CreateKeyBindingsContext( char const *filename, char const *pathID = 0 );
	virtual void		SetKeyBindingsContext( KeyBindingContextHandle_t handle );
	virtual KeyBindingContextHandle_t	GetKeyBindingsContext() const;
	virtual bool		IsValidKeyBindingsContext() const;

	static int			GetPanelsWithKeyBindingsCount( KeyBindingContextHandle_t handle );
	static Panel		*GetPanelWithKeyBindings( KeyBindingContextHandle_t handle, int index );

	static void			RevertKeyBindings( KeyBindingContextHandle_t handle );

	static void			ReloadKeyBindings( KeyBindingContextHandle_t handle );
	static void			SaveKeyBindings( KeyBindingContextHandle_t handle );
	static void			SaveKeyBindingsToFile( KeyBindingContextHandle_t handle, char const *filename, char const *pathID = 0 );
	static void			LoadKeyBindings( KeyBindingContextHandle_t handle );
	static void			LoadKeyBindingsForOnePanel( KeyBindingContextHandle_t handle, Panel *panelOfInterest );

	// OnKeyCodeTyped hooks into here for action
	virtual bool		IsKeyRebound( KeyCode code, int modifiers );
	// If a panel implements this and returns true, then the IsKeyRebound check will fail and OnKeyCodeTyped messages will pass through..
	//  sort of like setting the SetAllowKeyBindingChainToParent flag to false for specific keys
	virtual bool		IsKeyOverridden( KeyCode code, int modifiers );

	virtual void		AddKeyBinding( char const *bindingName, int keycode, int modifiers );

	KeyBindingMap_t		*LookupBinding( char const *bindingName );
	KeyBindingMap_t		*LookupBindingByKeyCode( KeyCode code, int modifiers );
	void				LookupBoundKeys( char const *bindingName, CUtlVector< BoundKey_t * >& list );
	BoundKey_t			*LookupDefaultKey( char const *bindingName );
	PanelKeyBindingMap	*LookupMapForBinding( char const *bindingName );

	// Returns the number of keybindings
	int				GetKeyMappingCount( );

	void			RevertKeyBindingsToDefault();
	void			RemoveAllKeyBindings();
	void			ReloadKeyBindings(); 
	virtual void	EditKeyBindings();

	// calls RevertKeyBindingsToDefault() and then LoadKeyBindingsForOnePanel( GetKeyBindingsContext(), this );
	void			SaveKeyBindingsToBuffer( int level, CUtlBuffer& buf );
	bool			ParseKeyBindings( KeyValues *kv );

	virtual char const *GetKeyBindingsFile() const;
	virtual char const *GetKeyBindingsFilePathID() const;

	// Set this to false to disallow IsKeyRebound chaining to GetParent() Panels...
	void			SetAllowKeyBindingChainToParent( bool state );
	bool			IsKeyBindingChainToParentAllowed() const;
#endif // VGUI_USEKEYBINDINGMAPS

	// base implementation forwards Key messages to the Panel's parent 
	// - override to 'swallow' the input
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void OnKeyCodeReleased(KeyCode code);
	virtual void OnKeyFocusTicked(); // every window gets key ticked events

	// forwards mouse messages to the panel's parent
	MESSAGE_FUNC( OnMouseFocusTicked, "OnMouseFocusTicked" );

	// message handlers that don't go through the message pump
	virtual void PaintBackground();
	virtual void Paint();
	virtual void PaintBorder();
	virtual void PaintBuildOverlay();		// the extra drawing for when in build mode
	virtual void PostChildPaint();
	virtual void PerformLayout();

	// this enables message mapping for this class - requires matching IMPLEMENT_PANELDESC() in the .cpp file
	DECLARE_PANELMAP();

	virtual VPANEL GetCurrentKeyFocus();

	// returns a pointer to the tooltip object associated with the panel
	// creates a new one if none yet exists
	BaseTooltip *GetTooltip();
	void	SetTooltip( BaseTooltip *pToolTip, const char *pszText );

	/// Returns the effective tooltip text to use, whether stored
	/// locally in this panel, or in the tooltip object.  Returns
	/// an empty string if no tooltip text
	const char *GetEffectiveTooltipText() const;

	// proportional mode settings
	virtual bool IsProportional() { return _flags.IsFlagSet( IS_PROPORTIONAL ); }
	virtual void SetProportional(bool state);

	// input interest
	virtual void SetMouseInputEnabled( bool state );
	virtual void SetKeyBoardInputEnabled( bool state );
	virtual bool IsMouseInputEnabled();
	virtual bool IsKeyBoardInputEnabled();

	virtual void DrawTexturedBox( int x, int y, int wide, int tall, Color color, float normalizedAlpha );
	virtual void DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, bool hollow = false );
	virtual void DrawBoxFade(int x, int y, int wide, int tall, Color color, float normalizedAlpha, unsigned int alpha0, unsigned int alpha1, bool bHorizontal, bool hollow = false );
	virtual void DrawHollowBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha );
	//=============================================================================
	// HPE_BEGIN:
	//=============================================================================
	 
	// [menglish] Draws a hollow box similar to the already existing draw hollow box function, but takes the indents as params
	virtual void DrawHollowBox( int x, int y, int wide, int tall, Color color, float normalizedAlpha, int cornerWide, int cornerTall );

	// [tj] Simple getters and setters to decide which corners to draw rounded
    unsigned char GetRoundedCorners() { return m_roundedCorners; }
	void SetRoundedCorners (unsigned char cornerFlags) { m_roundedCorners = cornerFlags; }
	bool ShouldDrawTopLeftCornerRounded() { return ( m_roundedCorners & PANEL_ROUND_CORNER_TOP_LEFT ) != 0; }
	bool ShouldDrawTopRightCornerRounded() { return ( m_roundedCorners & PANEL_ROUND_CORNER_TOP_RIGHT ) != 0; }
	bool ShouldDrawBottomLeftCornerRounded() { return ( m_roundedCorners & PANEL_ROUND_CORNER_BOTTOM_LEFT ) != 0; }
	bool ShouldDrawBottomRightCornerRounded() { return ( m_roundedCorners & PANEL_ROUND_CORNER_BOTTOM_RIGHT ) != 0; }
	 
	//=============================================================================
	// HPE_END
	//=============================================================================

// Drag Drop Public interface

	virtual void SetDragEnabled( bool enabled );
	virtual bool IsDragEnabled() const;

	virtual void SetShowDragHelper( bool enabled );

	// Called if drag drop is started but not dropped on top of droppable panel...
	virtual void OnDragFailed( CUtlVector< KeyValues * >& msglist );

	// Use this to prevent chaining up from a parent which can mess with mouse functionality if you don't want to chain up from a child panel to the best
	//  draggable parent.
	virtual void SetBlockDragChaining( bool block );
	virtual bool IsBlockingDragChaining() const;

	virtual int GetDragStartTolerance() const;
	virtual void SetDragSTartTolerance( int nTolerance );

	// If hover context time is non-zero, then after the drop cursor is hovering over the panel for that amount of time
	// the Show hover context menu function will be invoked
	virtual void SetDropEnabled( bool enabled, float m_flHoverContextTime = 0.0f );
	virtual bool IsDropEnabled() const;

	// Called if m_flHoverContextTime was non-zero, allows droppee to preview the drop data and show an appropriate menu
	// Return false if not using context menu
	virtual bool GetDropContextMenu( Menu *menu, CUtlVector< KeyValues * >& msglist );
	virtual void OnDropContextHoverShow( CUtlVector< KeyValues * >& msglist );
	virtual void OnDropContextHoverHide( CUtlVector< KeyValues * >& msglist );

#if defined( VGUI_USEDRAGDROP )
	virtual DragDrop_t *GetDragDropInfo();
#endif
	// For handling multiple selections...
	virtual void OnGetAdditionalDragPanels( CUtlVector< Panel * >& dragabbles );

	virtual void OnCreateDragData( KeyValues *msg );
	// Called to see if a drop enabled panel can accept the specified data blob
	virtual bool IsDroppable( CUtlVector< KeyValues * >& msglist );

	// Mouse is on draggable panel and has started moving, but is not over a droppable panel yet
	virtual void OnDraggablePanelPaint();
	// Mouse is now over a droppable panel
	virtual void OnDroppablePanelPaint( CUtlVector< KeyValues * >& msglist, CUtlVector< Panel * >& dragPanels );

	virtual void OnPanelDropped( CUtlVector< KeyValues * >& msglist );

	// called on droptarget when draggable panel entered/exited droptarget
	virtual void OnPanelEnteredDroppablePanel( CUtlVector< KeyValues * >& msglist );
	virtual void OnPanelExitedDroppablePanel ( CUtlVector< KeyValues * >& msglist );

	// Chains up to any parent marked DropEnabled
	virtual Panel *GetDropTarget( CUtlVector< KeyValues * >& msglist );
	// Chains up to first parent marked DragEnabled
	virtual Panel *GetDragPanel();
	virtual bool	IsBeingDragged();
	virtual HCursor GetDropCursor( CUtlVector< KeyValues * >& msglist );

	Color GetDropFrameColor();
	Color GetDragFrameColor();

	// Can override to require custom behavior to start the drag state
	virtual bool	CanStartDragging( int startx, int starty, int mx, int my );

	// Draws a filled rect of specified bounds, but omits the bounds of the skip panel from those bounds
	virtual void FillRectSkippingPanel( const Color &clr, int x, int y, int w, int h, Panel *skipPanel );

	virtual int	GetPaintBackgroundType();
	virtual void GetCornerTextureSize( int& w, int& h );

	bool		IsChildOfModalSubTree();
	bool		IsChildOfSurfaceModalPanel();

	bool		ShouldHandleInputMessage();

	virtual void SetSkipChildDuringPainting( Panel *child );

	// If this is set, then the drag drop won't occur until the mouse leaves the drag panels current rectangle
	void		SetStartDragWhenMouseExitsPanel( bool state );
	bool		IsStartDragWhenMouseExitsPanel() const;

	void		DisableMouseInputForThisPanel( bool bDisable );
	bool		IsMouseInputDisabledForThisPanel() const;

	bool		GetForceStereoRenderToFrameBuffer() const { return m_bForceStereoRenderToFrameBuffer; }
	void		SetForceStereoRenderToFrameBuffer( bool bForce ) { m_bForceStereoRenderToFrameBuffer = bForce; }

	void		PostMessageToAllSiblings( KeyValues *msg, float delaySeconds = 0.0f );
	template< class S >
	void		PostMessageToAllSiblingsOfType( KeyValues *msg, float delaySeconds = 0.0f );

	void		SetConsoleStylePanel( bool bConsoleStyle );
	bool		IsConsoleStylePanel() const;

	void		SetParentNeedsCursorMoveEvents( bool bNeedsEvents ) { m_bParentNeedsCursorMoveEvents = bNeedsEvents; }
	bool		ParentNeedsCursorMoveEvents() const { return m_bParentNeedsCursorMoveEvents; }

	int ComputePos( const char *pszInput, int &nPos, const int& nSize, const int& nParentSize, const bool& bX );

	// For 360: support directional navigation between UI controls via dpad
	enum NAV_DIRECTION { ND_UP, ND_DOWN, ND_LEFT, ND_RIGHT, ND_BACK, ND_NONE };
	virtual Panel* NavigateUp();
	virtual Panel* NavigateDown();
	virtual Panel* NavigateLeft();
	virtual Panel* NavigateRight();
	virtual Panel* NavigateActivate();
	virtual Panel* NavigateBack();
	virtual void NavigateTo();
	virtual void NavigateFrom();
	virtual void NavigateToChild( Panel *pNavigateTo ); //mouse support
	// if set, Panel gets PerformLayout called after the camera and the renderer's m_matrixWorldToScreen has been setup, so panels can be correctly attached to entities in the world
	inline void SetWorldPositionCurrentFrame( bool bWorldPositionCurrentFrame ) { m_bWorldPositionCurrentFrame = bWorldPositionCurrentFrame; }
	inline bool GetWorldPositionCurrentFrame() { return m_bWorldPositionCurrentFrame; }

	Panel* SetNavUp( Panel* navUp );
	Panel* SetNavDown( Panel* navDown );
	Panel* SetNavLeft( Panel* navLeft );
	Panel* SetNavRight( Panel* navRight );
	Panel* SetNavToRelay( Panel* navToRelay );
	Panel* SetNavActivate( Panel* navActivate );
	Panel* SetNavBack( Panel* navBack );
	NAV_DIRECTION GetLastNavDirection();
	MESSAGE_FUNC_CHARPTR( OnNavigateTo, "OnNavigateTo", panelName );
	MESSAGE_FUNC_CHARPTR( OnNavigateFrom, "OnNavigateFrom", panelName );

// Drag Drop protected/internal interface
protected:

	virtual void OnStartDragging();
	virtual void OnContinueDragging();
	virtual void OnFinishDragging( bool mousereleased, MouseCode code, bool aborted = false );

	virtual void DragDropStartDragging();

	virtual void GetDragData( CUtlVector< KeyValues * >& list );
	virtual void CreateDragData();

	virtual void PaintTraverse(bool Repaint, bool allowForce = true);

protected:
	MESSAGE_FUNC_ENUM_ENUM( OnRequestFocus, "OnRequestFocus", VPANEL, subFocus, VPANEL, defaultPanel);
	MESSAGE_FUNC_INT_INT( OnScreenSizeChanged, "OnScreenSizeChanged", oldwide, oldtall );
	virtual void *QueryInterface(EInterfaceID id);

	void AddToOverridableColors( Color *pColor, char const *scriptname )
	{
		int iIdx = m_OverridableColorEntries.AddToTail();
		m_OverridableColorEntries[iIdx].m_pszScriptName = scriptname;
		m_OverridableColorEntries[iIdx].m_pColor = pColor;
		m_OverridableColorEntries[iIdx].m_bOverridden = false;
	}

	void ApplyOverridableColors( void );
	void SetOverridableColor( Color *pColor, const Color &newColor );

public:
	void SetNavUp( const char* controlName );
	void SetNavDown( const char* controlName );
	void SetNavLeft( const char* controlName );
	void SetNavRight( const char* controlName );
	void SetNavToRelay( const char* controlName );
	void SetNavActivate( const char* controlName );
	void SetNavBack( const char* controlName );

	/*
	Will recursively look for the next visible panel in the navigation chain, parameters are for internal use.
	It will stop looking if first == nextpanel (to prevent infinite looping).
	*/
	Panel* GetNavUp( Panel *first = NULL ); 
	Panel* GetNavDown( Panel *first = NULL );
	Panel* GetNavLeft( Panel *first = NULL );
	Panel* GetNavRight( Panel *first = NULL );
	Panel* GetNavToRelay( Panel *first = NULL );
	Panel* GetNavActivate( Panel *first = NULL );
	Panel* GetNavBack( Panel *first = NULL );

	const char* GetNavUpName( void ) const { return m_sNavUpName.String(); }
	const char* GetNavDownName( void ) const { return m_sNavDownName.String(); }
	const char* GetNavLeftName( void ) const { return m_sNavLeftName.String(); }
	const char* GetNavRightName( void ) const { return m_sNavRightName.String(); }
	const char* GetNavToRelayName( void ) const { return m_sNavToRelayName.String(); }
	const char* GetNavActivateName( void ) const { return m_sNavActivateName.String(); }
	const char* GetNavBackName( void ) const { return m_sNavBackName.String(); }

protected:
	//this will return m_NavDown and will not look for the next visible panel
	Panel* GetNavUpPanel();
	Panel* GetNavDownPanel();
	Panel* GetNavLeftPanel();
	Panel* GetNavRightPanel();
	Panel* GetNavToRelayPanel();
	Panel* GetNavActivatePanel();
	Panel* GetNavBackPanel();

	bool m_PassUnhandledInput;
	NAV_DIRECTION m_LastNavDirection;

private:
	enum BuildModeFlags_t
	{
		BUILDMODE_EDITABLE						= 1 << 0,
		BUILDMODE_DELETABLE						= 1 << 1,
		BUILDMODE_SAVE_XPOS_RIGHTALIGNED		= 1 << 2,
		BUILDMODE_SAVE_XPOS_CENTERALIGNED		= 1 << 3,
		BUILDMODE_SAVE_YPOS_BOTTOMALIGNED		= 1 << 4,
		BUILDMODE_SAVE_YPOS_CENTERALIGNED		= 1 << 5,
		BUILDMODE_SAVE_WIDE_FULL				= 1 << 6,
		BUILDMODE_SAVE_TALL_FULL				= 1 << 7,
		BUILDMODE_SAVE_PROPORTIONAL_TO_PARENT	= 1 << 8,
		BUILDMODE_SAVE_WIDE_PROPORTIONAL		= 1 << 9,
		BUILDMODE_SAVE_TALL_PROPORTIONAL		= 1 << 10,
		BUILDMODE_SAVE_XPOS_PROPORTIONAL_SELF	= 1 << 11,
		BUILDMODE_SAVE_YPOS_PROPORTIONAL_SELF	= 1 << 12,
		BUILDMODE_SAVE_WIDE_PROPORTIONAL_TALL	= 1 << 13,
		BUILDMODE_SAVE_TALL_PROPORTIONAL_WIDE	= 1 << 14,
		BUILDMODE_SAVE_XPOS_PROPORTIONAL_PARENT = 1 << 15,
		BUILDMODE_SAVE_YPOS_PROPORTIONAL_PARENT = 1 << 16
	};

	enum PanelFlags_t
	{
		MARKED_FOR_DELETION					= 0x0001,
		NEEDS_REPAINT						= 0x0002,
		PAINT_BORDER_ENABLED				= 0x0004,
		PAINT_BACKGROUND_ENABLED			= 0x0008,
		PAINT_ENABLED						= 0x0010,
		POST_CHILD_PAINT_ENABLED			= 0x0020,
		AUTODELETE_ENABLED					= 0x0040,
		NEEDS_LAYOUT						= 0x0080,
		NEEDS_SCHEME_UPDATE					= 0x0100,
		NEEDS_DEFAULT_SETTINGS_APPLIED		= 0x0200,
#if defined( VGUI_USEKEYBINDINGMAPS )
		ALLOW_CHAIN_KEYBINDING_TO_PARENT	= 0x0400,
#endif
		IN_PERFORM_LAYOUT					= 0x0800,
		IS_PROPORTIONAL						= 0x1000,
		TRIPLE_PRESS_ALLOWED				= 0x2000,
		DRAG_REQUIRES_PANEL_EXIT			= 0x4000,
		IS_MOUSE_DISABLED_FOR_THIS_PANEL_ONLY = 0x8000,
		ALL_FLAGS							= 0xFFFF,
	};

	int ComputeWide( KeyValues *inResourceData, int nParentWide, int nParentTall, bool bComputingForTall );
	int ComputeTall( KeyValues *inResourceData, int nParentWide, int nParentTall, bool bComputingForWide );

	// used to get the Panel * for users with only IClientPanel
	virtual Panel *GetPanel() { return this; }

	// private methods
	void Think();
	void PerformApplySchemeSettings();

	void InternalPerformLayout();
	void InternalSetCursor();

	MESSAGE_FUNC_INT_INT( InternalCursorMoved, "CursorMoved", xpos, ypos );
	MESSAGE_FUNC( InternalCursorEntered, "CursorEntered" );
	MESSAGE_FUNC( InternalCursorExited, "CursorExited" );
	
	MESSAGE_FUNC_INT( InternalMousePressed, "MousePressed", code );
	MESSAGE_FUNC_INT( InternalMouseDoublePressed, "MouseDoublePressed", code );
	// Triple presses are synthesized
	MESSAGE_FUNC_INT( InternalMouseTriplePressed, "MouseTriplePressed", code );
	MESSAGE_FUNC_INT( InternalMouseReleased, "MouseReleased", code );
	MESSAGE_FUNC_INT( InternalMouseWheeled, "MouseWheeled", delta );
	MESSAGE_FUNC_INT( InternalKeyCodePressed, "KeyCodePressed", code );
	MESSAGE_FUNC_INT( InternalKeyCodeTyped, "KeyCodeTyped", code );
	MESSAGE_FUNC_INT( InternalKeyTyped, "KeyTyped", unichar );
	MESSAGE_FUNC_INT( InternalKeyCodeReleased, "KeyCodeReleased", code );

	MESSAGE_FUNC( InternalKeyFocusTicked, "KeyFocusTicked" );
	MESSAGE_FUNC( InternalMouseFocusTicked, "MouseFocusTicked" );

	MESSAGE_FUNC( InternalInvalidateLayout, "Invalidate" );

	MESSAGE_FUNC( InternalMove, "Move" );
	virtual void InternalFocusChanged(bool lost);	// called when the focus gets changed

	void PreparePanelMap( PanelMap_t *panelMap );

	bool InternalRequestInfo( PanelAnimationMap *map, KeyValues *outputData );
	bool InternalSetInfo( PanelAnimationMap *map, KeyValues *inputData );

	PanelAnimationMapEntry *FindPanelAnimationEntry( char const *scriptname, PanelAnimationMap *map );

	// Recursively invoke settings for PanelAnimationVars
	void InternalApplySettings( PanelAnimationMap *map, KeyValues *inResourceData);
	void InternalInitDefaultValues( PanelAnimationMap *map );

	// Purpose: Loads panel details related to autoresize from the resource info
	void ApplyAutoResizeSettings(KeyValues *inResourceData);

	void FindDropTargetPanel_R( CUtlVector< VPANEL >& panelList, int x, int y, VPANEL check );
	Panel *FindDropTargetPanel();

	int GetProportionalScaledValue( int rootTall, int normalizedValue );

#if defined( VGUI_USEDRAGDROP )
	DragDrop_t		*m_pDragDrop;
	Color			m_clrDragFrame;
	Color			m_clrDropFrame;
#endif

	BaseTooltip		*m_pTooltips;
	bool			m_bToolTipOverridden;

	PHandle			m_SkipChild;
	long			m_lLastDoublePressTime;
	HFont			m_infoFont;

#if defined( VGUI_USEKEYBINDINGMAPS )
	KeyBindingContextHandle_t m_hKeyBindingsContext;
#endif

	// data
	VPANEL			_vpanel;	// handle to a vgui panel
	char			*_panelName;		// string name of the panel - only unique within the current context
	IBorder			*_border;

	CUtlFlags< unsigned short > _flags;	// see PanelFlags_t
	Dar<HPanel>		_actionSignalTargetDar;	// the panel to direct notify messages to ("Command", "TextChanged", etc.)

	CUtlVector<OverridableColorEntry>	m_OverridableColorEntries;

	Color			_fgColor;		// foreground color
	Color			_bgColor;		// background color

	HBuildGroup		_buildGroup;

	short			m_nPinDeltaX;		// Relative position of the pinned corner to the edge
	short			m_nPinDeltaY;
	short			m_nResizeDeltaX;	// Relative position of the non-pinned corner to the edge
	short			m_nResizeDeltaY;

	HCursor			_cursor;
	unsigned short	_buildModeFlags; // flags that control how the build mode dialog handles this panel

	byte			_pinCorner : 4;	// the corner of the dialog this panel is pinned to
	byte			_autoResizeDirection : 4; // the directions in which the panel will auto-resize to

	unsigned char	_tabPosition;		// the panel's place in the tab ordering
	HScheme			 m_iScheme; // handle to the scheme to use

	bool			m_bIsDMXSerialized : 1; // Is this a DMX panel?
	bool			m_bUseSchemeColors : 1; // Should we use colors from the scheme?
	bool			m_bIsSilent : 1; // should this panel PostActionSignals?
	bool			m_bIsConsoleStylePanel : 1;
	bool			m_bParentNeedsCursorMoveEvents : 1;

	// Sibling pinning
	char			*_pinToSibling;				// string name of the sibling panel we're pinned to
	byte			_pinToSiblingCorner;		// the corner of the sibling panel we're pinned to
	byte			_pinCornerToSibling;		// the corner of our panel that we're pinning to our sibling
	PHandle			m_pinSibling;

	CUtlString	m_sNavUpName;
	PHandle		m_NavUp;

	CUtlString m_sNavDownName;
	PHandle m_NavDown;

	CUtlString m_sNavLeftName;
	PHandle m_NavLeft;

	CUtlString m_sNavRightName;
	PHandle m_NavRight;

	CUtlString m_sNavToRelayName;
	PHandle m_NavToRelay;

	CUtlString m_sNavActivateName;
	PHandle m_NavActivate;

	CUtlString m_sNavBackName;
	PHandle m_NavBack;

private:

	char			*_tooltipText;		// Tool tip text for panels that share tooltip panels with other panels

	PHandle			m_hMouseEventHandler;

	bool			m_bWorldPositionCurrentFrame;		// if set, Panel gets PerformLayout called after the camera and the renderer's m_matrixWorldToScreen has been setup, so panels can be correctly attached to entities in the world

	bool			m_bForceStereoRenderToFrameBuffer;

	CPanelAnimationVar( float, m_flAlpha, "alpha", "255" );

	// 1 == Textured (TextureId1 only)
	// 2 == Rounded Corner Box
	CPanelAnimationVar( int, m_nPaintBackgroundType, "PaintBackgroundType", "0" );
	CPanelAnimationVarAliasType( int, m_nBgTextureId1, "Texture1", "vgui/hud/800corner1", "textureid" );
	CPanelAnimationVarAliasType( int, m_nBgTextureId2, "Texture2", "vgui/hud/800corner2", "textureid" );
	CPanelAnimationVarAliasType( int, m_nBgTextureId3, "Texture3", "vgui/hud/800corner3", "textureid" );
	CPanelAnimationVarAliasType( int, m_nBgTextureId4, "Texture4", "vgui/hud/800corner4", "textureid" );

	//=============================================================================
	// HPE_BEGIN:
	// [tj] A bitset of flags to determine which corners should be rounded
	//=============================================================================
	unsigned char m_roundedCorners;
	//=============================================================================
	// HPE_END
	//=============================================================================	
	friend class BuildGroup;
	friend class BuildModeDialog;
	friend class PHandle;

	// obselete, remove soon
	void OnOldMessage(KeyValues *params, VPANEL ifromPanel);
};

inline void Panel::DisableMouseInputForThisPanel( bool bDisable )
{
	_flags.SetFlag( IS_MOUSE_DISABLED_FOR_THIS_PANEL_ONLY, bDisable );
}

inline bool	Panel::IsMouseInputDisabledForThisPanel() const
{
	return _flags.IsFlagSet( IS_MOUSE_DISABLED_FOR_THIS_PANEL_ONLY );
}

#if 0
// This function cannot be defined here because it requires on a full definition of
// KeyValues (to call KeyValues::MakeCopy()) whereas the rest of this header file
// assumes a forward declared definition of KeyValues.
template< class S >
inline void Panel::PostMessageToAllSiblingsOfType( KeyValues *msg, float delaySeconds /*= 0.0f*/ )
{
	Panel *parent = GetParent();
	if ( parent )
	{
		int nChildCount = parent->GetChildCount();
		for ( int i = 0; i < nChildCount; ++i )
		{
			Panel *sibling = parent->GetChild( i );
			if ( sibling == this )
				continue;
			if ( dynamic_cast< S * >( sibling ) )
			{
				PostMessage( sibling->GetVPanel(), msg->MakeCopy(), delaySeconds );
			}
		}
	}

	msg->deleteThis();
}
#endif

class Button;

struct SortedPanel_t
{
	SortedPanel_t( Panel *panel );

	Panel *pPanel;
	Button *pButton;
};

class CSortedPanelYLess
{
public:
	bool Less( const SortedPanel_t &src1, const SortedPanel_t &src2, void *pCtx )
	{
		int nX1, nY1, nX2, nY2;
		src1.pPanel->GetPos( nX1, nY1 );
		src2.pPanel->GetPos( nX2, nY2 );

		if ( nY1 == nY2 )
		{
			return ( nX1 < nX2 );
		}

		if ( nY1 < nY2 )
		{
			return true;
		}

		return false;
	}
};


void VguiPanelGetSortedChildPanelList( Panel *pParentPanel, void *pSortedPanels );
void VguiPanelGetSortedChildButtonList( Panel *pParentPanel, void *pSortedPanels, char *pchFilter = NULL, int nFilterType = 0 );
int VguiPanelNavigateSortedChildButtonList( void *pSortedPanels, int nDir );


} // namespace vgui


#endif // PANEL_H
