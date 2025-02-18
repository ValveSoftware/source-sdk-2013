//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ECON_CONTROLS_H
#define ECON_CONTROLS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/IVGui.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/ImagePanel.h>
#include "utlvector.h"
#include "vgui_controls/PHandle.h"
#include <vgui_controls/Tooltip.h>
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose: Changes the visibility of the child panel if it is different.
//			Returns true if the child exists, false otherwise.
//-----------------------------------------------------------------------------
bool SetChildPanelVisible( vgui::Panel *pParent, const char *pChildName, bool bVisible, bool bSearchForChildRecursively = false );

//-----------------------------------------------------------------------------
// Purpose: Changes the enable state of the child panel if it is different.
//			Returns true if the child exists, false otherwise.
//-----------------------------------------------------------------------------
bool SetChildPanelEnabled( vgui::Panel *pParent, const char *pChildName, bool bEnabled, bool bSearchForChildRecursively = false );

//-----------------------------------------------------------------------------
// Purpose: Changes the selected state of the child button if it is different.
//			Returns true if the child exists, false otherwise.
//-----------------------------------------------------------------------------
bool SetChildButtonSelected( vgui::Panel *pParent, const char *pChildName, bool bSelected, bool bSearchForChildRecursively = false );

//-----------------------------------------------------------------------------
// Purpose: Returns true if the child button exists and is selected, false otherwise.
//-----------------------------------------------------------------------------
bool IsChildButtonSelected( vgui::Panel *pParent, const char *pChildName, bool bSearchForChildRecursively = false );

//-----------------------------------------------------------------------------
// Purpose: Adds the child panel as an action signal target. Returns true if the child exists, false otherwise.
//-----------------------------------------------------------------------------
bool AddChildActionSignalTarget( vgui::Panel *pParent, const char *pChildName, vgui::Panel *messageTarget, bool bSearchForChildRecursively = false );

//-----------------------------------------------------------------------------
// Purpose: Modify the color of a label/button's text  - if it starts with "X "
//			or "x ", set the X to red.
//-----------------------------------------------------------------------------
bool SetXToRed( vgui::Label *pPanel );

//-----------------------------------------------------------------------------
// Purpose: Simple panel tooltip. Just calls setvisible on the other panel.
//			Ignores all other input.
//-----------------------------------------------------------------------------
class CSimplePanelToolTip : public vgui::BaseTooltip 
{
	DECLARE_CLASS_SIMPLE( CSimplePanelToolTip, vgui::BaseTooltip );
public:
	CSimplePanelToolTip(vgui::Panel *parent, const char *text = NULL) : vgui::BaseTooltip( parent, text )
	{
		m_pControlledPanel = NULL;
	}
	void SetText(const char *text) { return; }
	const char *GetText() { return NULL; }

	virtual void ShowTooltip( vgui::Panel *currentPanel ) { if ( m_pControlledPanel ) m_pControlledPanel->SetVisible( true ); }
	virtual void HideTooltip() { if ( m_pControlledPanel ) m_pControlledPanel->SetVisible( false ); }
	void SetControlledPanel( vgui::EditablePanel *pPanel ) { m_pControlledPanel = pPanel; }

protected:
	vgui::Panel	*m_pControlledPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Expanded Button class that allows font & color overriding in .res files
//-----------------------------------------------------------------------------
class CExButton : public vgui::Button
{
public:
	DECLARE_CLASS_SIMPLE( CExButton, vgui::Button );

	CExButton( vgui::Panel *parent, const char *name, const char *text, vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );
	CExButton( vgui::Panel *parent, const char *name, const wchar_t *wszText, vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );

	virtual void ApplySettings( KeyValues *inResourceData );

	void SetFontStr( const char *pFont );
	void SetColorStr( const char *pColor );

	virtual vgui::IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

	virtual void OnMouseFocusTicked() OVERRIDE;
	virtual void OnCursorEntered() OVERRIDE;
	virtual void OnCursorExited() OVERRIDE;
	void		PassMouseTicksTo( vgui::Panel *pPanel, bool bCursorEnterExitEvent = false ) 
	{ 
		m_hMouseTickTarget.Set( pPanel ? pPanel->GetVPanel() : NULL );
		m_bbCursorEnterExitEvent = bCursorEnterExitEvent;
	}

private:
	char		m_szFont[64];
	char		m_szColor[64];

	vgui::IBorder		*m_pArmedBorder;
	vgui::IBorder		*m_pDefaultBorderOverride;
	vgui::IBorder		*m_pSelectedBorder;
	vgui::IBorder		*m_pDisabledBorder;
	vgui::VPanelHandle	m_hMouseTickTarget;
	bool				m_bbCursorEnterExitEvent;
};

//-----------------------------------------------------------------------------
// Purpose: Expanded image button, that handles images per button state, and color control in the .res file
//-----------------------------------------------------------------------------
class CExImageButton : public CExButton
{
public:
	DECLARE_CLASS_SIMPLE( CExImageButton, CExButton );

	CExImageButton( vgui::Panel *parent, const char *name, const char *text = "", vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );
	CExImageButton( vgui::Panel *parent, const char *name, const wchar_t *wszText = L"", vgui::Panel *pActionSignalTarget = NULL, const char *cmd = NULL );
	~CExImageButton( void );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void SetArmed(bool state);
	virtual void SetEnabled(bool state);
	virtual void SetSelected(bool state);
	void		 SetSubImage( const char *pszImage );

	void		 SetImageDefault( const char *pszImageDefault );
	void		 SetImageArmed( const char *pszImageArmed );
	void		 SetImageSelected( const char *pszImageSelected );

	Color		 GetImageColor( void );

	vgui::ImagePanel	*GetImage( void ) { return m_pEmbeddedImagePanel; }

private:
	// Embedded image panels
	vgui::ImagePanel	*m_pEmbeddedImagePanel;
	Color				m_ImageDrawColor;
	Color				m_ImageArmedColor;
	Color				m_ImageDisabledColor;
	Color				m_ImageSelectedColor;
	Color				m_ImageDepressedColor;
	char				m_szImageDefault[MAX_PATH];
	char				m_szImageArmed[MAX_PATH];
	char				m_szImageSelected[MAX_PATH];
};

//-----------------------------------------------------------------------------
// Purpose: Expanded Label class that allows color control in .res files
//-----------------------------------------------------------------------------
class CExLabel : public vgui::Label
{
public:
	DECLARE_CLASS_SIMPLE( CExLabel, vgui::Label );

	CExLabel( vgui::Panel *parent, const char *panelName, const char *text );
	CExLabel( vgui::Panel *parent, const char *panelName, const wchar_t *wszText );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void SetColorStr( const char *pColor );
	void SetColorStr( Color cColor );

private:
	char		m_szColor[64];
};

//-----------------------------------------------------------------------------
// Purpose: Expanded Richtext control that allows customization of scrollbar display, font, and color .res controls.
//-----------------------------------------------------------------------------
class CExRichText : public vgui::RichText
{
public:
	DECLARE_CLASS_SIMPLE( CExRichText, vgui::RichText );

	CExRichText( vgui::Panel *parent, const char *panelName );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void SetText( const char *text );
	virtual void SetText( const wchar_t *text );

	virtual void OnTick( void );
	void SetScrollBarImagesVisible( bool visible );

	void SetFontStr( const char *pFont );
	void SetColorStr( const char *pColor );
	void SetCustomImage( vgui::Panel *pImage, const char *pszImage, char *pszStorage );

	void CreateImagePanels( void );

protected:
	char		m_szFont[64];
	char		m_szColor[64];
	char		m_szImageUpArrow[MAX_PATH];
	char		m_szImageDownArrow[MAX_PATH];
	char		m_szImageLine[MAX_PATH];
	char		m_szImageBox[MAX_PATH];
	bool		m_bUseImageBorders;

	CExImageButton		*m_pUpArrow;
	vgui::Panel			*m_pLine;
	CExImageButton		*m_pDownArrow;
	vgui::Panel			*m_pBox;
};

//-----------------------------------------------------------------------------
// Purpose: Rich text control that knows how to fill itself with information 
//			that describes a specific item definition.
//-----------------------------------------------------------------------------
class CRichTextWithScrollbarBorders : public CExRichText
{
public:
	DECLARE_CLASS_SIMPLE( CRichTextWithScrollbarBorders, CExRichText );

	CRichTextWithScrollbarBorders( vgui::Panel *parent, const char *panelName ) : BaseClass( parent, panelName )
	{
		m_bUseImageBorders = true;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Rich text control that knows how to fill itself with information 
//			that describes a specific item definition.
//-----------------------------------------------------------------------------
class CEconItemDetailsRichText : public CRichTextWithScrollbarBorders
{
public:
	DECLARE_CLASS_SIMPLE( CEconItemDetailsRichText, CRichTextWithScrollbarBorders );

	CEconItemDetailsRichText( vgui::Panel *parent, const char *panelName );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void	UpdateDetailsForItem( const CEconItemDefinition *pDef );

	void	AllowItemSetLinks( bool bAllow )	{ m_bAllowItemSetLinks = bAllow; }

	void	SetLimitedItem( bool bLimited ) { m_bLimitedItem = bLimited; }

private:
	void	InsertItemLink( const wchar_t *pwzItemName, int nItemIndex, Color *pColorOverride = NULL );
	void	AddDataText( const char *pszText, bool bAddPostLines = true, const wchar_t *wpszArg = NULL, const wchar_t *wpszArg2 = NULL, const int *pItemDefIndex = NULL );
	void	DataText_AppendStoreFlags( const CEconItemDefinition *pDef );
	void	DataText_AppendItemData( const CEconItemDefinition *pDef );
	void	DataText_AppendBundleData( const CEconItemDefinition *pDef );
	void	DataText_AppendUsageData( const CEconItemDefinition *pBaseDef );
	void	DataText_AppendAttributeData( const CEconItemDefinition *pDef );
	void	DataText_AppendSetData( const CEconItemDefinition *pDef );
	void	DataText_AppendToolUsage( const CEconItemDefinition *pDef );
	void	UpdateToolList( void );

private:
	Color	m_colTextHighlight;
	Color	m_colItemSet;
	Color	m_colLink;
	bool	m_bAllowItemSetLinks;
	vgui::HFont	m_hLinkFont;
	CUtlVector<item_definition_index_t>	m_ToolList;

	bool	m_bLimitedItem;
};


#define EXC_SIDE_TOP		0
#define EXC_SIDE_RIGHT		1
#define EXC_SIDE_BOTTOM		2
#define EXC_SIDE_LEFT		3

//-----------------------------------------------------------------------------
// Purpose: A small callout arrow that's created by a CExplanationPopup to
//			connect to the point that the explanation is referring to.
//-----------------------------------------------------------------------------
class CExplanationPopupCalloutArrow : public vgui::Panel
{
public:
	CExplanationPopupCalloutArrow( Panel *parent ) : vgui::Panel( parent, "calloutarrow" )
	{
		SetPaintBackgroundEnabled( false );
		SetMouseInputEnabled( false );
		PrecacheMaterial( "vgui/callout_tail" );
	}

	void SetArrowPoints( int iAx, int iAy, int iBx, int iBy, int iCx, int iCy )
	{
		m_iArrowA[0] = iAx;
		m_iArrowA[1] = iAy;
		m_iArrowB[0] = iBx;
		m_iArrowB[1] = iBy;
		m_iArrowC[0] = iCx;
		m_iArrowC[1] = iCy;
	}

	virtual void	Paint( void );

private:
	int m_iArrowA[2];
	int m_iArrowB[2];
	int m_iArrowC[2];
};

//-----------------------------------------------------------------------------
// Purpose: A bubble that contains a blob of text and an arrow to a specific place onscreen
//-----------------------------------------------------------------------------
class CExplanationPopup : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CExplanationPopup, vgui::EditablePanel );
public:
	CExplanationPopup(Panel *parent, const char *panelName);
	~CExplanationPopup( void );

	void	SetCalloutInParentsX( int nXPos ) { m_iCalloutInParentsX = nXPos; }
	void	SetCalloutInParentsY( int nYPos ) { m_iCalloutInParentsY = nYPos; }
	void	Popup( int iPosition = 0, int iTotalPanels = 0 );
	void	Hide( int iExplanationDelta = 0 );
	const char	*GetNextExplanation( void ) { return m_szNextExplanation; }
	void	SetPrevExplanation( const char *pszPrev );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void OnCommand( const char *command );
	virtual void OnTick( void );
	virtual void OnKeyCodeTyped( vgui::KeyCode code );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void OnSizeChanged( int newWide, int newTall ) OVERRIDE;

	void	PositionCallout( float flElapsed );
	virtual void	FireGameEvent( IGameEvent *event );

private:
	int		m_iCalloutSide;
	float	m_flStartTime;
	float	m_flEndTime;
	char	m_szNextExplanation[128];
	char	m_szPrevExplanation[128];
	CExplanationPopupCalloutArrow	*m_pCallout;
	int		m_iPositionInChain;
	int		m_iTotalInChain;
	bool	m_bFinishedPopup;

	CUtlString m_strTitle;
	CUtlString m_strBody;

	CPanelAnimationVar( bool, m_bForceClose, "force_close", "0" );
	CPanelAnimationVar( bool, m_bUseResFileForControls, "res_file_controls", "0" );

	CPanelAnimationVarAliasType( int, m_iCalloutInParentsX, "callout_inparents_x", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iCalloutInParentsY, "callout_inparents_y", "0", "proportional_ypos" );

	CPanelAnimationVarAliasType( int, m_iStartX, "start_x", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iStartY, "start_y", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iStartW, "start_wide", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iStartH, "start_tall", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iEndX, "end_x", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iEndY, "end_y", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iEndW, "end_wide", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iEndH, "end_tall", "0", "proportional_int" );
};

//-----------------------------------------------------------------------------
// Purpose: A stack to keep track of the modal dialogs that have been popped up.
//-----------------------------------------------------------------------------
class CPanelModalStack
{
public:
	void PushModal( vgui::Panel *pDialog );
	void PopModal( vgui::Panel *pDialog );

	void Update( void );

	vgui::VPanelHandle Top();

	bool IsEmpty() const;

private:
	void PopModal( int iIdx );

private:
	CUtlVector<vgui::VPanelHandle> m_pDialogs;	
};

CPanelModalStack *TFModalStack( void );

//-----------------------------------------------------------------------------
// Purpose: Generic waiting dialog
//-----------------------------------------------------------------------------
class CGenericWaitingDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CGenericWaitingDialog, vgui::EditablePanel );

public:
	CGenericWaitingDialog( vgui::Panel *pParent );

	void Close();
	void ShowStatusUpdate( bool bAnimateEllipses, bool bAllowClose, float flMaxWaitTime = 0 );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void OnTick( void );
	virtual void OnTimeout();
	virtual void OnUserClose();
	virtual const char *GetResFile() const { return "resource/UI/econ/GenericWaitingDialog.res"; }
	virtual const char *GetResFilePathId() const { return "MOD"; }

	bool			m_bAnimateEllipses;
	int				m_iNumEllipses;
	CountdownTimer	m_timer;
};

void ShowWaitingDialog( CGenericWaitingDialog *pWaitingDialog, const char* pUpdateText, bool bAnimate, bool bShowCancel, float flMaxDuration );
void CloseWaitingDialog();

#endif // ECON_CONTROLS_H
