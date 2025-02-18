//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/v2/tf_store_preview_item2.h"
#include "econ_item_description.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ScrollBarSlider.h"
#include "vgui/IInput.h"
#include "tf_item_schema.h"
#include "econ_item_system.h"
#include "store/store_panel.h"
#include "c_tf_gamestats.h"
#include "tf_playermodelpanel.h"
#include "navigationpanel.h"
#include "tf_mouseforwardingpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline float LerpScale( float flIn, float flInMin, float flInMax, float flOutMin, float flOutMax )
{
	float flDenom = flInMax - flInMin;
	if ( flDenom == 0.0f )
		return 0.0f;

	float t = clamp( ( flIn - flInMin ) / flDenom, 0.0f, 1.0f );
	return Lerp( t, flOutMin, flOutMax );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline float SCurve( float t )
{
	t = clamp( t, 0.0f, 1.0f );
	return t * t * (3 - 2*t);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFullscreenStorePreviewItem::CFullscreenStorePreviewItem( vgui::Panel *pParent, EditablePanel *pOwner )
:	BaseClass( pParent, "FullscreenStorePreview" ),
	m_iItemDef( INVALID_ITEM_ID ),
	m_pCycleTextLabel( NULL ),
	m_pTeamNavPanel( NULL ),
	m_pPreviewButton( NULL ),
	m_pRotLeftButton( NULL ),
	m_pRotRightButton( NULL ),
	m_pZoomButton( NULL ),
	m_pOverlayPanel( NULL ),
	m_flGoFullscreenStartTime( 0.0f ),
	m_flLastMouseMoveTime( 0.0f ),
	m_bIsHalloweenOrFullmoonOnlyItem( false )
{
	m_hOwner = pOwner;
}

void CFullscreenStorePreviewItem::SetItemDef( itemid_t iItemDef )
{
	m_iItemDef = iItemDef;

	CStorePanel	*pStorePanel = EconUI()->GetStorePanel();
	if ( pStorePanel )
	{
		const CEconStorePriceSheet *pPriceSheet = pStorePanel->GetPriceSheet();
		CExButton *pPreviewButton = dynamic_cast<CExButton *>( FindChildByName( "TryItOutButton" ) );
		if ( pPriceSheet && pPreviewButton )
		{
			const econ_store_entry_t *pStoreEntry = pPriceSheet->GetEntry( m_iItemDef );
			pPreviewButton->SetVisible( pStoreEntry && pStoreEntry->CanPreview() );
		}
	}
}

void CFullscreenStorePreviewItem::GoFullscreen( CTFPlayerModelPanel *pPlayerModelPanel )
{
	m_Stats.Clear();

	m_flGoFullscreenStartTime = gpGlobals->realtime;

	SetAlpha( 0 );
	SetVisible( true );
	MoveToFront();

	m_pPlayerModelPanel = pPlayerModelPanel;

	if ( !m_pPlayerModelPanel.Get() )
		return;

	// Cache old player model panel bounds and such
	m_pPlayerModelPanel->GetBounds( m_OldModelState.m_aPlayerModelPanelBounds[0], m_OldModelState.m_aPlayerModelPanelBounds[1], m_OldModelState.m_aPlayerModelPanelBounds[2], m_OldModelState.m_aPlayerModelPanelBounds[3] );
	m_OldModelState.m_vecPlayerPos = m_pPlayerModelPanel->m_vecPlayerPos;
	m_OldModelState.m_bZoomed = m_pPlayerModelPanel->IsZoomed();

	// Fullscreen panel is new parent
	m_pPlayerModelPanel->SetParent( this );

	// Get team state
	if ( m_pTeamNavPanel )
	{
		m_pTeamNavPanel->UpdateButtonSelectionStates( m_pPlayerModelPanel->GetTeam() == TF_TEAM_RED ? 0 : 1 );
	}
}

void CFullscreenStorePreviewItem::ExitFullscreen()
{
	if ( !m_hOwner.Get() )
		return;

	if ( m_pPlayerModelPanel.Get() )
	{
		m_pPlayerModelPanel->SetParent( m_hOwner.Get() );
		m_pPlayerModelPanel->SetBounds( m_OldModelState.m_aPlayerModelPanelBounds[0], m_OldModelState.m_aPlayerModelPanelBounds[1], m_OldModelState.m_aPlayerModelPanelBounds[2], m_OldModelState.m_aPlayerModelPanelBounds[3] );
		
		// Reset the player position to it's pre-fullscreen location, but add on any zoom delta if needed.
		const Vector vecZoomOffset = m_OldModelState.m_bZoomed != m_pPlayerModelPanel->IsZoomed() ? m_pPlayerModelPanel->GetZoomOffset() : vec3_origin;
		m_pPlayerModelPanel->m_vecPlayerPos = m_OldModelState.m_vecPlayerPos + vecZoomOffset;
	}

	m_flGoFullscreenStartTime = 0.0f;
	SetVisible( false );

	PostMessage( m_hOwner.Get(), new KeyValues( "ExitFullscreen" ) );
}

bool CFullscreenStorePreviewItem::IsFullscreenMode()
{
	return IsVisible();
}

void CFullscreenStorePreviewItem::OnNavButtonSelected( KeyValues *pData )
{
	const int iTeam = pData->GetInt( "userdata", -1 );	AssertMsg( iTeam >= 0, "Bad filter" );
	if ( iTeam < 0 )
		return;

	if ( !m_pPlayerModelPanel.Get() )
		return;

	m_pPlayerModelPanel->SetTeam( iTeam );

	C_CTFGameStats::ImmediateWriteInterfaceEvent( "team_switch_%s(store_preview_item_panel_fullscreen)", iTeam == TF_TEAM_RED ? "red" : "blu" );
}

void CFullscreenStorePreviewItem::OnThink()
{
	BaseClass::OnThink();

	if ( m_flGoFullscreenStartTime == 0.0f )
	{
		SetVisible( false );
		return;
	}

	// We are fading, or already faded in
	SetVisible( true );

	// Keep track of mouse movement - if the mouse button is down, force the mouse-moving state so we
	// don't end up fading out while the player is rotating or clicking-and-holding anywhere else
	int nMouseX, nMouseY;
	vgui::input()->GetCursorPos( nMouseX, nMouseY );
	bool bMouseMoved = false;
	const bool bMouseButtonDown = vgui::input()->IsMouseDown( MOUSE_LEFT );
	const bool bForceMouseMoving = bMouseButtonDown;
	if ( bForceMouseMoving || nMouseX != m_nLastMouseX || nMouseY != m_nLastMouseY )
	{
		bMouseMoved = true;
		m_nLastMouseX = nMouseX;
		m_nLastMouseY = nMouseY;
		m_flLastMouseMoveTime = gpGlobals->realtime;
	}
		
	// Fade in the button blocker if the mouse has been idle for some period of time
	if ( m_pOverlayPanel )
	{
		const float flButtonBlockerFade = SCurve(
			LerpScale(
				gpGlobals->realtime,
				m_flLastMouseMoveTime + m_flUiFadeoutTime,
				m_flLastMouseMoveTime + m_flUiFadeoutTime + m_flUiFadeoutDuration,
				0.0f,
				1.0f
			)
		);

		m_pOverlayPanel->SetVisible( flButtonBlockerFade > 0.0f );

		m_pOverlayPanel->SetAlpha( (int)( 255 * flButtonBlockerFade ) );

		// Set to layer above overlay panel, so it will always be visible
		m_pPlayerModelPanel->SetZPos( flButtonBlockerFade > 0.0f ? ( m_pOverlayPanel->GetZPos() + 1 ) : 0 );
	}

	const float flFade = SCurve(
		LerpScale(
			gpGlobals->realtime,
			m_flGoFullscreenStartTime,
			m_flGoFullscreenStartTime + m_flFullscreenFadeToBlackDuration,
			0.0f,
			1.0f
		)
	);

	SetAlpha( (int)( 255 * flFade ) );

	if ( !m_pPlayerModelPanel.Get() )
		return;

	// Resize 3D model panel
	const int aDstBounds[4] = { 0, 0, ScreenWidth(), ScreenHeight() };
	const int aBounds[4] = {
		Lerp( flFade, m_OldModelState.m_aPlayerModelPanelBounds[0], aDstBounds[0] ),
		Lerp( flFade, m_OldModelState.m_aPlayerModelPanelBounds[1], aDstBounds[1] ),
		Lerp( flFade, m_OldModelState.m_aPlayerModelPanelBounds[2], aDstBounds[2] ),
		Lerp( flFade, m_OldModelState.m_aPlayerModelPanelBounds[3], aDstBounds[3] )
	};
	m_pPlayerModelPanel->SetBounds( aBounds[0], aBounds[1], aBounds[2], aBounds[3] );

	if ( flFade < 0.999f )
	{
		const Vector vecZoomOffset = m_pPlayerModelPanel->IsZoomed() ? m_pPlayerModelPanel->GetZoomOffset() : vec3_origin;
		const Vector vecFullscreenOrigin( m_flModelPanelOriginX, m_flModelPanelOriginY, m_flModelPanelOriginZ );
		m_pPlayerModelPanel->m_vecPlayerPos = Lerp( flFade, m_OldModelState.m_vecPlayerPos, vecFullscreenOrigin + vecZoomOffset );
	}
		
	if ( m_pZoomButton )
	{
		m_pZoomButton->SetEnabled( flFade == 1.0f );
	}

	if ( bMouseButtonDown && m_pRotLeftButton && m_pRotRightButton )
	{
		float flDeltaAngle = 0;
		const float kScale = 100.0f;
		if ( m_pRotLeftButton->IsWithin( nMouseX, nMouseY ) )
		{
			flDeltaAngle = -gpGlobals->frametime * kScale;
		}
		else if ( m_pRotRightButton->IsWithin( nMouseX, nMouseY ) )
		{
			flDeltaAngle = gpGlobals->frametime * kScale;
		}
		
		m_pPlayerModelPanel->RotateYaw( flDeltaAngle );

		// Accumulate time rotation buttons are being pressed for stat tracking
		m_Stats.m_flRotationTime += gpGlobals->frametime;
	}
}

void CFullscreenStorePreviewItem::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/store/v2/StorePreviewItemPanel_Fullscreen.res" );

	m_pOverlayPanel = dynamic_cast<EditablePanel *>( FindChildByName( "OverlayPanel" ) );
	m_pRotLeftButton = dynamic_cast<CExButton *>( FindChildByName( "RotateLeftButton" ) );
	m_pRotRightButton = dynamic_cast<CExButton *>( FindChildByName( "RotateRightButton" ) );
	m_pZoomButton = dynamic_cast<CExButton *>( FindChildByName( "ZoomButton" ) );
	m_pTeamNavPanel = dynamic_cast<CNavigationPanel *>( FindChildByName( "TeamNavPanel" ) );
}

void CFullscreenStorePreviewItem::OnCommand( const char *command )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "on_command(store_preview_item_panel_fullscreen)", command );

	if ( !V_strnicmp( command, "close", 6 ) )
	{
		ExitFullscreen();
		return;
	}

	if ( m_hOwner.Get() )
	{
		// Let the owner handle the command
		m_hOwner->OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePreviewItemPanel2::CTFStorePreviewItemPanel2( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner )
:	BaseClass( pParent, pResFile, "storepreviewitem", pOwner )
{								   
	m_pScrollBar = new ScrollBar( this, "ScrollBar", true );
	m_pScrollBar->AddActionSignalTarget( this );

	m_pFullscreenPanel = new CFullscreenStorePreviewItem( this, this );
	m_pFullscreenPanel->AddActionSignalTarget( this );

	m_pMouseOverItemPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "mouseoveritempanel" ) );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );
	m_pMouseOverTooltip->SetPositioningStrategy( IPTTP_BOTTOM_SIDE );
	m_pMouseOverItemPanel->MoveToFront();

	m_pItemViewData = NULL;
	m_pSOEconItemData = NULL;
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::Clear()
{
	m_pPlayerModelPanel = NULL;
	
	m_pPreviewButton = NULL;
	m_pDialogFrame = NULL;
	m_pPreviewViewportBg = NULL;
	m_pItemNameLabel = NULL;
	m_pAttributesLabel = NULL;
	m_pItemCollectionHighlight = NULL;
	m_pDetailsView = NULL;
	m_pDetailsViewChild = NULL;	// Scrollable
	m_pItemWikiPageButton = NULL;
	m_pTeamNavPanel = NULL;
	m_pCycleTextLabel = NULL;
	m_pScrollableChild = NULL;
	m_pGoFullscreenButton = NULL;
	m_nNumAttribLinesAdded = 0;
	m_bArmoryTextAdded = false;
	m_nNumAttribLinesAdded = 0;
	m_iSliderPos = 0;
	m_aClickPos[0] = m_aClickPos[1] = 0;
	m_bCloseOnUp = false;
	m_bMouseWasDown = false;
	m_bIsHalloweenOrFullmoonOnlyItem = false;

	for ( int i = 0; i < ARRAYSIZE( m_pAddRentalToCartButtons ); i++ )
	{
		m_pAddRentalToCartButtons[i] = NULL;
	}

	m_vecReferenceItemPanels.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	Clear();

	BaseClass::ApplySchemeSettings( pScheme );

	FOR_EACH_VEC( m_pItemIcons, i )
	{
		// Strip tooltips.
		// We have all the same data already
		m_pItemIcons[i]->GetItemPanel()->SetTooltip( NULL, NULL );
	}

	m_pDialogFrame = dynamic_cast<EditablePanel *>( FindChildByName( "DialogFrame" ) );
	m_pPreviewButton = dynamic_cast<CExButton *>( FindChildByName( "TryItOutButton" ) );
	m_pCycleTextLabel = dynamic_cast<CExLabel *>( FindChildByName( "CycleTextLabel" ) );
	m_pGoFullscreenButton = dynamic_cast<CExImageButton *>( FindChildByName( "GoFullscreenButton" ) );

	COMPILE_TIME_ASSERT( ARRAYSIZE( m_pAddRentalToCartButtons ) == 3 );
#ifdef ENABLE_STORE_RENTAL_BACKEND
	m_pAddRentalToCartButtons[0] = dynamic_cast<CExButton *>( FindChildByName( "AddRentalToCartButton_1Day" ) );
	m_pAddRentalToCartButtons[1] = dynamic_cast<CExButton *>( FindChildByName( "AddRentalToCartButton_3Day" ) );
	m_pAddRentalToCartButtons[2] = dynamic_cast<CExButton *>( FindChildByName( "AddRentalToCartButton_7Day" ) );
#endif

	if ( m_pDialogFrame )
	{
		m_pPreviewViewportBg = dynamic_cast<EditablePanel *>( m_pDialogFrame->FindChildByName( "PreviewViewportBg" ) );
		m_pItemNameLabel = dynamic_cast<CExLabel *>( m_pDialogFrame->FindChildByName( "ItemNameLabel" ) );
		m_pDetailsView = dynamic_cast<EditablePanel *>( m_pDialogFrame->FindChildByName( "DetailsView" ) );

		if ( m_pDetailsView )
		{
			m_pDetailsViewChild = dynamic_cast<EditablePanel *>( m_pDetailsView->FindChildByName( "ScrollableChild" ) );

			if ( !m_pDetailsViewChild )
			{
				m_pDetailsViewChild = m_pDetailsView;
			}

			m_pAttributesLabel = dynamic_cast<CExLabel *>( m_pDetailsViewChild->FindChildByName( "AttributesLabel" ) );
			m_pItemCollectionHighlight = dynamic_cast<EditablePanel *>( m_pDetailsViewChild->FindChildByName( "collectionhighlight" ) );
			if ( m_pItemCollectionHighlight )
			{
				m_pItemCollectionHighlight->InvalidateLayout( true, true );
			}
			m_pItemWikiPageButton = dynamic_cast<CExButton *>( m_pDetailsViewChild->FindChildByName( "ItemWikiPageButton" ) );

			if ( m_pItemWikiPageButton )
			{
				m_pItemWikiPageButton->AddActionSignalTarget( this );
			}
		}
	}

	m_pTeamNavPanel = dynamic_cast<CNavigationPanel *>( FindChildByName( "TeamNavPanel" ) );

	m_pMouseOverItemPanel->SetBorder( pScheme->GetBorder("LoadoutItemPopupBorder") );

	SetState( PS_ITEM );
}

//-----------------------------------------------------------------------------
// Purpose: Given two controls A and B, place B such that it is (nXOffset,nYOffset)
// pixels offset from A, using it's content width or height, depending on bVertical.
// pControlNameA can be NULL, in which case zeros will be used as the offset.
//-----------------------------------------------------------------------------
int CTFStorePreviewItemPanel2::PlaceControl( Panel *pParent, const char *pControlNameA, const char *pControlNameB, int nOffset, bool bVertical, bool bSizeAToContents/*=true*/, bool bUseContentSize/*=true*/ )
{
	if ( !pParent || !pControlNameB )
	{
		AssertMsg( 0, "Bad!" );
		return 0;
	}

	Label *pControlA = pControlNameA ? dynamic_cast<Label *>( pParent->FindChildByName( pControlNameA ) ) : NULL;
	Label *pControlB = dynamic_cast<Label *>( pParent->FindChildByName( pControlNameB ) );
	if ( !pControlB )
	{
		return 0;
	}

	if ( !pControlA && bVertical )
	{
		pControlA = m_pLastNewLineControl;
	}

	int aSize[2] = { 0, 0 };
	int aPos[2] = { 0, 0 };
	if ( pControlA )
	{
		pControlA->SetVisible( true );

		if ( bSizeAToContents )
		{
			pControlA->SizeToContents();
			pControlA->InvalidateLayout( true );
		}

		if ( bUseContentSize )
		{
			pControlA->GetContentSize( aSize[0], aSize[1] );
		}
		else
		{
			pControlA->GetSize( aSize[0], aSize[1] );
		}

		pControlA->GetPos( aPos[0], aPos[1] );
	}

	int aOffset[2] = { 0, 0 };
	if ( bVertical )
	{
		aOffset[1] = aSize[1] + nOffset;
	}
	else
	{
		aOffset[0] = aSize[0] + nOffset;
	}

	// NOTE: We add in the slider position here
	pControlB->SetPos( aPos[0] + aOffset[0], aPos[1] + aOffset[1] );

	pControlB->SetVisible( true );

#if _DEBUG
	/*
	Msg( "control A: %s  size: w=%i  h=%i   pos: (%i, %i)\n", pControlNameA, aSize[0], aSize[1], aPos[0], aPos[1] );
	int x,y;
	pControlB->GetPos(x,y);
	Msg( "control B: %s  pos: (%i, %i)\n", pControlNameB, x,y );
	*/
#endif

	if ( bVertical )
	{
		m_pLastNewLineControl = pControlB;
	}

	m_nViewMaxHeight = MAX( m_nViewMaxHeight, aPos[1] + aOffset[1] + pControlB->GetTall() );
	return m_nViewMaxHeight;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::PerformLayout( void )
{
//	BaseClass::PerformLayout();	// We override completely here

	// center the icons (we need to redo some of the work of CStorePreviewItemPanel, because we 
	// center the base item icons along with our TF specific class ones)
	int iNumItemIcons = 0;
	FOR_EACH_VEC( m_pItemIcons, i )
	{
		if ( m_pItemIcons[i]->IsVisible() )
		{
			++iNumItemIcons;
		}
	}

	int iNumClassIcons = 0;
	FOR_EACH_VEC( m_pClassIcons, i )
	{
		if ( m_pClassIcons[i]->IsVisible() )
		{
			++iNumClassIcons;
		}
	}

	if ( m_pDialogFrame && ( iNumItemIcons || iNumClassIcons ) )
	{
		int aDialogFramePos[2];
		m_pDialogFrame->GetPos( aDialogFramePos[0], aDialogFramePos[1] );

		int iCenterX = aDialogFramePos[0] + m_pDialogFrame->GetWide() / 4;
		int interval = XRES(2);
		int totalWidth = (iNumItemIcons > 0 ? iNumItemIcons * m_pItemIcons[0]->GetWide() : 0) + (iNumClassIcons * m_pClassIcons[0]->GetWide()) + (interval * (iNumItemIcons + iNumClassIcons - 1));
		int iX = iCenterX - ( totalWidth / 2 );

		int posX, posY;
		if ( iNumItemIcons > 0 )
		{
			m_pItemIcons[0]->GetPos( posX, posY );
		}
		else
		{
			m_pClassIcons[0]->GetPos( posX, posY );
		}

		int iButton = 0;
		for ( int i = 0; i < m_pItemIcons.Count(); i++ )
		{
			if ( m_pItemIcons[i]->IsVisible() )
			{
				m_pItemIcons[i]->SetPos( iX, posY );
				iX += m_pItemIcons[i]->GetWide() + interval;

				iButton++;
			}
		}

		for ( int i = 0; i < m_pClassIcons.Count(); i++ )
		{
			if ( m_pClassIcons[i]->IsVisible() )
			{
				m_pClassIcons[i]->SetPos( iX, posY );
				iX += m_pClassIcons[i]->GetWide() + interval;

				iButton++;
			}
		}
	}

	if ( !m_pPreviewViewportBg || !m_pItemNameLabel || !m_pDetailsViewChild || !m_pDetailsView || !m_pDialogFrame )
	{
		m_pScrollBar->SetVisible( false );
		return;
	}

	// Make sure we size the item name in case it needs multiple lines.
	m_pItemNameLabel->SizeToContents();
	m_pItemNameLabel->InvalidateLayout( true );
	if ( m_pItemNameLabel && m_item.IsValid() )
	{
		const CEconItemRarityDefinition* pItemRarity = GetItemSchema()->GetRarityDefinition( m_item.GetItemDefinition()->GetRarity() );

		// Setup the rarity color overlay
		{
			Color color( 255, 255, 255, 255 );
			if ( pItemRarity )
			{
				attrib_colors_t attribColor = pItemRarity->GetAttribColor();
				vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
				color = pScheme->GetColor( GetColorNameForAttribColor( attribColor ), Color( 255, 255, 255, 255 ) );
			}
			m_pItemNameLabel->SetColorStr( color );
		}
	}

	int aItemNameLabelPos[2];
	m_pItemNameLabel->GetPos( aItemNameLabelPos[0], aItemNameLabelPos[1] );

	// This is the item label's new bottom y coordinate after it's been sized
	const int nNewYRelativeToDlgFrame = aItemNameLabelPos[1] + m_pItemNameLabel->GetTall();

	// Set ypos for details view and scroll bar
	int aDetailsViewPos[2];
	m_pDetailsView->GetPos( aDetailsViewPos[0], aDetailsViewPos[1] );
	m_pDetailsView->SetPos( aDetailsViewPos[0], nNewYRelativeToDlgFrame );

	int aDialogFramePos[2];
	m_pDialogFrame->GetPos( aDialogFramePos[0], aDialogFramePos[1] );

	if ( m_pScrollBar )
	{
		int aScrollBar[2];
		m_pScrollBar->GetPos( aScrollBar[0], aScrollBar[1] );
		m_pScrollBar->SetPos( aScrollBar[0], nNewYRelativeToDlgFrame + aDialogFramePos[1] );
	}

	int nNewHeight = m_pPreviewViewportBg->GetTall() - m_pItemNameLabel->GetTall();
	m_pDetailsView->SetTall( nNewHeight );
	if ( m_pScrollBar )
	{
		m_pScrollBar->SetTall( nNewHeight );
	}

	// Place paint and style buttons
	CUtlVector<CExButton *> vecVisibleButtons;
	const int nNumPossibilyVisibleWeapons = 1;
	CExButton *pPossiblyVisibleButtons[nNumPossibilyVisibleWeapons] = { m_pNextWeaponButton };
	for ( int i = 0; i < nNumPossibilyVisibleWeapons; ++i )
	{
		CExButton *pCurButton = pPossiblyVisibleButtons[i];
		if ( !pCurButton || !pCurButton->IsVisible() )
			continue;

		vecVisibleButtons.AddToTail( pCurButton );
	}

	int nNumButtonsNeeded = vecVisibleButtons.Count();
	if ( nNumButtonsNeeded )
	{
		// Center however many buttons we need to along the top of the viewport
		int aViewportPos[2];
		m_pPreviewViewportBg->GetPos( aViewportPos[0], aViewportPos[1] );
		for ( int i = 0; i < nNumButtonsNeeded; ++i )
		{
			CExButton *pCurButton = vecVisibleButtons[i];
			pCurButton->SetPos( aDialogFramePos[0] + aViewportPos[0] + ( i + 1 ) * m_pPreviewViewportBg->GetWide() / ( nNumButtonsNeeded + 1 ) - m_iControlButtonWidth / 2, m_iControlButtonY );
			pCurButton->SetSize( m_iControlButtonWidth, m_iControlButtonHeight );
		}
	}

	m_pLastNewLineControl = NULL;
	m_nViewMaxHeight = 0;

	PlaceControl( m_pDetailsViewChild, NULL, "ItemLevelInfoLabel", m_iSmallVerticalBreakSize, true );
	
	if ( m_bIsHalloweenOrFullmoonOnlyItem )
	{
		PlaceControl( m_pDetailsViewChild, "ItemLevelInfoLabel", "RestrictionsLabel", m_iMediumVerticalBreakSize, true );
		PlaceControl( m_pDetailsViewChild, "RestrictionsLabel", "RestrictionsTextLabel", m_iHorizontalBreakSize, false );
		PlaceControl( m_pDetailsViewChild, "RestrictionsLabel", "UsedByLabel", m_iSmallVerticalBreakSize, true );
	}
	else
	{
		PlaceControl( m_pDetailsViewChild, "ItemLevelInfoLabel", "UsedByLabel", m_iMediumVerticalBreakSize, true );
	}

	PlaceControl( m_pDetailsViewChild, "UsedByLabel", "UsedByTextLabel", m_iHorizontalBreakSize, false );
	PlaceControl( m_pDetailsViewChild, "UsedByLabel", "SlotLabel", m_iSmallVerticalBreakSize, true );
	PlaceControl( m_pDetailsViewChild, "SlotLabel", "SlotTextLabel", m_iHorizontalBreakSize, false );
	PlaceControl( m_pDetailsViewChild, "SlotLabel", "PriceLabel", m_iBigVerticalBreakSize, true );

	if ( m_bArmoryTextAdded )
	{
		PlaceControl( m_pDetailsViewChild, "PriceLabel", "ArmoryTextLabel", m_iBigVerticalBreakSize, true );
		PlaceControl( m_pDetailsViewChild, "ArmoryTextLabel", "AttributesLabel", m_iBigVerticalBreakSize, true );
	}
	else
	{
		PlaceControl( m_pDetailsViewChild, "PriceLabel", "AttributesLabel", m_iBigVerticalBreakSize, true );
	}
	
	PlaceControl( m_pDetailsViewChild, m_nNumAttribLinesAdded == 0 ? "PriceLabel" : "AttributesLabel", "ItemWikiPageButton", m_iBigVerticalBreakSize, true );
	
	PlaceControl( m_pDetailsViewChild, "ItemWikiPageButton", "TradableLabel", m_iBigVerticalBreakSize, true, false, false );
	PlaceControl( m_pDetailsViewChild, "TradableLabel", "TradableTextLabel", m_iHorizontalBreakSize, false );
	PlaceControl( m_pDetailsViewChild, "TradableLabel", "CraftableLabel", m_iSmallVerticalBreakSize, true );
	PlaceControl( m_pDetailsViewChild, "CraftableLabel", "CraftableTextLabel", m_iHorizontalBreakSize, false );
	PlaceControl( m_pDetailsViewChild, "CraftableLabel", "GiftableLabel", m_iSmallVerticalBreakSize, true );
	PlaceControl( m_pDetailsViewChild, "GiftableLabel", "GiftableTextLabel", m_iHorizontalBreakSize, false );
	PlaceControl( m_pDetailsViewChild, "GiftableLabel", "NameableLabel", m_iSmallVerticalBreakSize, true );
	PlaceControl( m_pDetailsViewChild, "NameableLabel", "NameableTextLabel", m_iHorizontalBreakSize, false );

	if ( m_pScrollBar )
	{
		m_pScrollBar->SetVisible( true );
		m_pScrollBar->InvalidateLayout( true );
		m_pScrollBar->SetRange( 0, m_nViewMaxHeight );
		m_pScrollBar->SetRangeWindow( m_pScrollBar->GetTall() );

		m_pDetailsViewChild->SetTall( m_nViewMaxHeight );
		UpdateScrollableChild();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::UpdateScrollableChild()
{
	m_pDetailsViewChild->SetPos( 0, -m_iSliderPos );
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnCommand( const char *command )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "on_command(store_preview_item_panel)", command );

	if ( !V_strnicmp( command, "closex", 5 ) )
	{
		// This is just a way for us to differentiate between 'x' button being pressed vs.
		// the "back" button vs. clicking outside the preview window.
		DoClose();
	}
	else if ( !V_strnicmp( command, "tryitout", 8 ) )
	{
		// OGS data gets written elsewhere for this event
		PostMessage( m_pOwner, new KeyValues( "PreviewItem", "item_def_index", m_item.GetItemDefIndex() ) );
		DoClose();
	}
	else if ( !V_strnicmp( command, "addtocart", 9 )
#ifdef ENABLE_STORE_RENTAL_BACKEND
		   || !V_strnicmp( command, "addrentaltocart", 15 )
#endif
			  )
	{
#ifdef ENABLE_STORE_RENTAL_BACKEND
		ECartItemType eCartItemType = !V_stricmp( command, "addrentaltocart_1day" )
									? kCartItem_Rental_1Day
									: !V_stricmp( command, "addrentaltocart_3day" )
									? kCartItem_Rental_3Day
									: !V_stricmp( command, "addrentaltocart_7day" )
									? kCartItem_Rental_7Day
									: kCartItem_Purchase;
#else
		ECartItemType eCartItemType = kCartItem_Purchase;
#endif

		KeyValues *pParams = new KeyValues( "AddItemToCart" );
		pParams->SetInt( "item_def", m_item.GetItemDefIndex() );
		pParams->SetInt( "cart_add_type", eCartItemType );
		PostMessage( m_pOwner, pParams );
		DoClose();
	}
	else if ( !V_strnicmp( command, "viewwikipage", 12 ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() && m_pItemFullImage )
		{
			CEconItemView *pItem = m_pItemFullImage->GetItem();
			if ( pItem && pItem->IsValid() )
			{
				// Determine which language we should use
				char uilanguage[ 64 ];
				uilanguage[0] = 0;
				engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );
				ELanguage iLang = PchLanguageToELanguage( uilanguage );

				char szURL[512];
				Q_snprintf( szURL, sizeof(szURL), "http://wiki.teamfortress.com/scripts/itemredirect.php?id=%d&lang=%s", pItem->GetItemDefIndex(), GetLanguageICUName( iLang ) );
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );

				C_CTF_GameStats.Event_Catalog( IE_ARMORY_BROWSE_WIKI, NULL, pItem );
			}
		}
	}
	else if ( !V_strnicmp( command, "team_", 5 ) )
	{
		const char *pTeam = command + 5;
		if ( !V_strnicmp( pTeam, "red", 3 ) )
		{
			m_pPlayerModelPanel->SetTeam( TF_TEAM_RED );
		}
		else
		{
			m_pPlayerModelPanel->SetTeam( TF_TEAM_BLUE );
		}
	}
	else if ( !V_strnicmp( command, "gofullscreen", 11 ) )
	{
		if ( m_pFullscreenPanel )
		{
			m_pFullscreenPanel->GoFullscreen( m_pPlayerModelPanel );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnClassIconSelected( KeyValues *data )
{
	BaseClass::OnClassIconSelected( data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnHideClassIconMouseover( void )
{
	BaseClass::OnHideClassIconMouseover();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnShowClassIconMouseover( KeyValues *data )
{
	// We decided not to show the "this item is
	// usable by the [Class name]" tooltip.
	//BaseClass::OnShowClassIconMouseover( data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::PreviewItemCopy( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry )
{
	// Make a copy of SO data since it comes from the market and will fall out of scope
	if ( m_pItemViewData )
	{
		delete m_pItemViewData;
		m_pItemViewData = NULL;
	}

	if ( m_pSOEconItemData )
	{
		delete m_pSOEconItemData;
		m_pSOEconItemData = NULL;
	}

	m_pItemViewData = new CEconItemView( *pItem );
	m_pSOEconItemData = new CEconItem( *pItem->GetSOCData() );
	m_pItemViewData->SetNonSOEconItem( m_pSOEconItemData );
	PreviewItem( iClass, m_pItemViewData, pEntry );
}
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry )
{
	BaseClass::PreviewItem( iClass, pItem, pEntry );

	C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_preview_item_panel(preview_item)", CFmtStr( "%i", m_item.GetItemDefIndex() ).Access() );

	// Reload the .res file right now, since we need to make sure the item name label, on which all other controls
	// base their position, is valid.
	InvalidateLayout( true, true );

	// Update the fullscreen item def index
	if ( m_pFullscreenPanel )
	{
		m_pFullscreenPanel->SetItemDef( m_item.GetItemDefIndex() );
	}

	// If we didn't have a store entry passed in, look for one.
	if ( !pEntry )
	{
		CStorePanel *pStorePanel = EconUI()->GetStorePanel();
		if ( pStorePanel )
		{
			pEntry = pStorePanel->GetPriceSheet()->GetEntry( m_item.GetItemDefIndex() );
		}
	}

	if ( m_pDialogFrame && m_pDetailsView && m_pItemFullImage && m_pItemFullImage->GetItem() && m_pAttributesLabel )
	{
		const CEconItemView *pFullItem = m_pItemFullImage->GetItem();
		const CEconItemDefinition *pBaseDef = pFullItem->GetItemDefinition();
		const CTFItemDefinition *pDef = dynamic_cast<const CTFItemDefinition *>( pBaseDef );

		if ( pFullItem && pDef )
		{
			m_pDialogFrame->SetDialogVariable( "itemname", pFullItem->GetItemName() );

			// Holiday restrictions?
			const char *pHolidayRestriction = pDef->GetHolidayRestriction() ? pDef->GetHolidayRestriction() : "";
			m_bIsHalloweenOrFullmoonOnlyItem = StringHasPrefix( pHolidayRestriction, "halloween" );

			CTFItemSchema *pSchema = ItemSystem()->GetItemSchema();
			if ( pSchema )
			{
				// Build a list of classes by which this item can be used
				const CBitVec<LOADOUT_COUNT> *pbvClassUsability = pDef->GetClassUsability();
				
				const int kClassNamesSize = 512;
				wchar_t wszClassNames[ kClassNamesSize ] = L"";
				int nClassAdded = 0;
				bool bAllClasses = true;
				for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
				{
					if ( !pbvClassUsability->IsBitSet( i ) )
					{
						bAllClasses = false;
						break;
					}
				}

				if ( bAllClasses )
				{
					m_pDetailsViewChild->SetDialogVariable( "used_by_classes", g_pVGuiLocalize->Find( "#Store_ItemDesc_AllClasses" ) );
				}
				else
				{
					for ( int i = 0; i < LOADOUT_COUNT; ++i )
					{
						if ( pbvClassUsability->IsBitSet( i ) )
						{
							V_wcscat_safe( wszClassNames, nClassAdded++ == 0 ? L"" : L", " );					// add empty lines everywhere except before the first line
							V_wcscat_safe( wszClassNames, g_pVGuiLocalize->Find( g_aPlayerClassNames[i] ) );
						}
					}
					m_pDetailsViewChild->SetDialogVariable( "used_by_classes", wszClassNames );
				}

				// Setup the slot string
				const CUtlVector< const char * > &vecLoadoutStrings = pSchema->GetLoadoutStrings( pDef->GetEquipType() );
				const int iSlot = pDef->GetDefaultLoadoutSlot();
				const bool bSlotValid = vecLoadoutStrings.IsValidIndex( iSlot );
				m_pDetailsViewChild->SetDialogVariable( "slot", g_pVGuiLocalize->Find( bSlotValid ? CFmtStr( "#LoadoutSlot_%s", vecLoadoutStrings[iSlot] ).Access() : "#Store_ItemDesc_Slot_None" ) );

				// Make an attempt to display tradability accurately even though we don't have an item to pull from.
				static CSchemaAttributeDefHandle pAttrib_CannotTrade( "cannot trade" );
				Assert( pAttrib_CannotTrade );

				// Get localized versions of "yes" and "no"
				const wchar_t *pYesNo[2] = {
					g_pVGuiLocalize->Find( "#Store_ItemDesc_Yes" ),
					g_pVGuiLocalize->Find( "#Store_ItemDesc_No" )
				};
				bool bIsMapStamp = pDef->GetItemClass() && !V_strncmp( pDef->GetItemClass(), "map_token", 9 );
				bool bIsTradeable = bIsMapStamp || FindAttribute( pDef, pAttrib_CannotTrade )
								  ? pYesNo[ 1 ]
								  : g_pVGuiLocalize->Find( "#Attrib_Store_TradableAfterDate" );

				m_pDetailsViewChild->SetDialogVariable( "giftable", bIsTradeable );
				m_pDetailsViewChild->SetDialogVariable( "nameable", ( pDef->GetCapabilities() & ITEM_CAP_NAMEABLE ) != 0 ? pYesNo[0] : pYesNo[1] );

				m_pDetailsViewChild->SetDialogVariable( "tradable", bIsTradeable );

				// No store-bought items are craftable, but items that were going to be tradable would show as craftable because
				// they weren't real items with a real origin that would prevent them from being crafted. In the short term it makes
				// more sense to just force this to always display "false" because at least it will never be wrong.
				m_pDetailsViewChild->SetDialogVariable( "craftable", pDef->IsBundle()
																   ? g_pVGuiLocalize->Find( "#Attrib_CannotCraftWeapons" )
																   : pDef->GetCapabilities() & ITEM_CAP_CAN_BE_CRAFTED_IF_PURCHASED
																   ? pYesNo[0]
																   : pYesNo[1] );

				m_pDetailsViewChild->SetDialogVariable( "item_level_info", "" );

				// Setup price
				if ( pEntry )
				{
					ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();

					int iTotalPrice = pEntry->GetCurrentPrice( eCurrency );
					wchar_t wzLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
					MakeMoneyString( wzLocalizedPrice, ARRAYSIZE( wzLocalizedPrice ), iTotalPrice, eCurrency );

#ifdef ENABLE_STORE_RENTAL_BACKEND
					const wchar_t *pwsRentalPriceFormat = GLocalizationProvider()->Find( "#TF_Store_RentalPriceFormat" );
					if ( pEntry->IsRentable() && pwsRentalPriceFormat )
					{
						wchar_t wzRentalLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
						float flRentalPriceScale = ( pEntry->GetRentalPriceScale() * 0.01f );
						MakeMoneyString( wzRentalLocalizedPrice, ARRAYSIZE( wzRentalLocalizedPrice ), flRentalPriceScale * iTotalPrice, eCurrency )

						wchar_t wzLocalizedPriceString[96];
						::ILocalize::ConstructString_safe( wzLocalizedPriceString, pwsRentalPriceFormat, 2, wzLocalizedPrice, wzRentalLocalizedPrice );
						m_pDetailsViewChild->SetDialogVariable( "price", wzLocalizedPriceString );
					}
					else
#endif
					{
						if ( pEntry->m_bIsMarketItem )
						{
							if ( iTotalPrice != 0 )
							{
								wchar_t wzMarketString[96];
								g_pVGuiLocalize->ConstructString_safe(
									wzMarketString,
									LOCCHAR( "%s1 %s2" ),
									2,
									g_pVGuiLocalize->Find( "#Store_StartingAt" ),
									wzLocalizedPrice );

								m_pDetailsViewChild->SetDialogVariable( "price", wzMarketString );
							}
							else
							{
								m_pDetailsViewChild->SetDialogVariable( "price", "..." );
							}
						}
						else
						{
							// if market item.  Prefix 'Starting at'
							m_pDetailsViewChild->SetDialogVariable( "price", wzLocalizedPrice );
						}
					}
				}

				// Show/hide rental button.
				for ( int i = 0; i < ARRAYSIZE( m_pAddRentalToCartButtons ); i++ )
				{
					if ( m_pAddRentalToCartButtons[i] )
					{
						m_pAddRentalToCartButtons[i]->SetVisible( pEntry && pEntry->IsRentable() );
					}
				}
			}

			// Final label value for our armory description text block.
			const wchar_t *pwszLabelValue = L"";
			m_bArmoryTextAdded = false;

			const char *pszArmoryDescString = pDef->GetArmoryDescString();
			if ( pszArmoryDescString )
			{
				const ArmoryStringDict_t& ArmoryKeys = GetItemSchema()->GetArmoryDataItems();
				const ArmoryStringDict_t::IndexType_t armoryIndex = ArmoryKeys.Find( pszArmoryDescString );

				if ( ArmoryKeys.IsValidIndex( armoryIndex ) )
				{
					const char *pszArmoryDescLocalizationKey = ArmoryKeys[ armoryIndex ].Get();

					pwszLabelValue = g_pVGuiLocalize->Find( pszArmoryDescLocalizationKey );
					m_bArmoryTextAdded = true;
				}
			}

			m_pDetailsViewChild->SetDialogVariable( "armory_text", pwszLabelValue );
		}

		// clear all old reference item panels before adding new ones
		FOR_EACH_VEC( m_vecReferenceItemPanels, i )
		{
			m_vecReferenceItemPanels[i]->MarkForDeletion();
		}
		m_vecReferenceItemPanels.RemoveAll();
		int iReferenceItemHeight = 0;

		const CEconItemDescription *pDescription = m_pItemFullImage->GetItem()->GetDescription();
		if ( pDescription )
		{
			TextImage *pAttributesTextImage = m_pAttributesLabel->GetTextImage();	// This pointer already verified above
			pAttributesTextImage->ClearColorChangeStream();

			const int kAttribBufferSize = 4 * 1024;
			wchar_t wszAttribBuffer[ kAttribBufferSize ] = L"";

			int iAttribLine = 0;
			m_nNumAttribLinesAdded = 0;
			Color clrPrev( 0, 0, 0, 0 );
			uint32 unCurrentTextStreamIndex = 0;
			IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
			int iFontHeight = surface()->GetFontTall( m_pAttributesLabel->GetFont() );
			
			if ( m_pItemCollectionHighlight )
			{
				m_pItemCollectionHighlight->SetVisible( false );
			}
			
			iReferenceItemHeight = iFontHeight;

			int iAttributePanelX, iAttributePanelY;
			m_pAttributesLabel->GetPos( iAttributePanelX, iAttributePanelY );

			for ( uint32 i = 0; i < pDescription->GetLineCount(); ++i )
			{
				const econ_item_description_line_t& line = pDescription->GetLine(i);

				if ( line.unMetaType & kDescLineFlag_MouseOverPanel )
					continue;

				int nLineLength = StringFuncs<locchar_t>::Length( line.sText.Get() );
				if ( ( line.unMetaType & kDescLineFlag_Type	) != 0 )
				{
					m_pDetailsViewChild->SetDialogVariable( "item_level_info", line.sText.Get() );
				}
				else if ( ( line.unMetaType & kDescLineFlagSet_DisplayInAttributeBlock ) != 0 && m_pAttributesLabel )
				{
					++iAttribLine;

					// current collection item line
					bool bIsCurrentCollectionItem = ( line.unMetaType & kDescLineFlag_CollectionCurrentItem ) != 0;
					// use bg color as text color for current item for a better highlight
					Color col = bIsCurrentCollectionItem ? Color( 0, 0, 0, 255 ) : pScheme->GetColor( GetColorNameForAttribColor( line.eColor ), Color( 255, 255, 255, 255 ) );

					// Output a color change if necessary.
					if ( i == 0 || clrPrev != col )
					{
						pAttributesTextImage->AddColorChange( col, unCurrentTextStreamIndex );
						clrPrev = col;
					}

					// Current line highlight
					if ( bIsCurrentCollectionItem && m_pItemCollectionHighlight )
					{
						// use text color as bg color for the current item for a better highlight
						Color bgColor = pScheme->GetColor( GetColorNameForAttribColor( line.eColor ), Color( 255, 255, 255, 255 ) );

						// Get the current ypos
						int x, y;
						m_pAttributesLabel->GetPos( x, y );
						m_pItemCollectionHighlight->SetPos( x, y + ( iAttribLine - 1 ) * iFontHeight );
						m_pItemCollectionHighlight->SetBgColor( bgColor );
						m_pItemCollectionHighlight->SetVisible( bIsCurrentCollectionItem );
					}

					if ( ( line.unMetaType & ( kDescLineFlag_Name | kDescLineFlag_Type ) ) == 0 )
					{
						V_wcscat_safe( wszAttribBuffer, m_nNumAttribLinesAdded++ == 0 ? L"" : L"\n" );					// add empty lines everywhere except before the first line
						V_wcscat_safe( wszAttribBuffer, line.sText.Get() );
						unCurrentTextStreamIndex += nLineLength + 1;	// add one character to deal with newlines
					}

					if ( line.unDefIndex != INVALID_ITEM_DEF_INDEX )
					{
						// set text and recalculate the size now to compute for button pos
						m_pAttributesLabel->SetText( wszAttribBuffer );
						m_pAttributesLabel->SizeToContents();

						CItemModelPanel* pItemModelPanel = new CItemModelPanel( m_pAttributesLabel, CFmtStr( "reference_item_%d", m_vecReferenceItemPanels.Count() ) );
						pItemModelPanel->SetActAsButton( true, true );
						pItemModelPanel->SetAutoDelete( true );

						pItemModelPanel->SetPos( 0, m_pAttributesLabel->GetTall() - iFontHeight );
						pItemModelPanel->SetZPos( m_pAttributesLabel->GetZPos() + 1 );

						CEconItemView itemData;
						itemData.Init( line.unDefIndex, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
						itemData.SetClientItemFlags( kEconItemFlagClient_Preview );
						pItemModelPanel->SetItem( &itemData );
						pItemModelPanel->MakeFakeButton();

						pItemModelPanel->SetTooltip( m_pMouseOverTooltip, "" );

						m_vecReferenceItemPanels.AddToTail( pItemModelPanel );
					}
				}
			}

			// Make sure our string is NUL-terminated.
			wszAttribBuffer[ kAttribBufferSize-1 ] = 0;

			m_pAttributesLabel->SetText( wszAttribBuffer );
		}

		// match the highlight width to attribute width
		if ( m_pItemCollectionHighlight && m_pItemCollectionHighlight->IsVisible() )
		{
			m_pAttributesLabel->SizeToContents();
			m_pItemCollectionHighlight->SetWide( m_pAttributesLabel->GetWide() );
		}

		if ( m_vecReferenceItemPanels.Count() )
		{
			m_pAttributesLabel->SizeToContents();
			FOR_EACH_VEC( m_vecReferenceItemPanels, i )
			{
				m_vecReferenceItemPanels[i]->SetSize( m_pAttributesLabel->GetWide(), iReferenceItemHeight );
			}
		}

		// Get PerformLayout() called, now that we have text in our controls - without this, SizeToContents() sizes all the labels as tall and narrow.
		InvalidateLayout( true );
	}

	// Set the visibility of the "Try it now!" button based on whether we as a client think this item should be able
	// to be previewed.
	const CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheet();
	if ( pPriceSheet && m_pPreviewButton )
	{
		const econ_store_entry_t *pStoreEntry = pPriceSheet->GetEntry( m_item.GetItemDefIndex() );
		m_pPreviewButton->SetVisible( pStoreEntry && pStoreEntry->CanPreview() );
	}
	
	m_pItemFullImage->InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::SetState( preview_state_t iState )
{
	BaseClass::SetState( iState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::UpdateIcons( void )
{
	BaseClass::UpdateIcons();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::UpdatePlayerModelButtons()
{
	BaseClass::UpdatePlayerModelButtons();

	if ( m_pPlayerModelPanel )
	{
		if ( m_pTeamNavPanel )
		{
			m_pTeamNavPanel->SetVisible( m_pPlayerModelPanel->IsVisible() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::SetPlayerModelVisible( bool bVisible )
{
	BaseClass::SetPlayerModelVisible( bVisible );

	if ( m_pCycleTextLabel )
	{
		m_pCycleTextLabel->SetVisible( bVisible );
	}

	if ( m_pGoFullscreenButton )
	{
		m_pGoFullscreenButton->SetVisible( bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::SetCycleLabelText( vgui::Label *pTargetLabel, const char *pCycleText )
{
	BaseClass::SetCycleLabelText( pTargetLabel, pCycleText );

	if ( m_pCycleTextLabel )
	{
		const wchar_t *pwszText = g_pVGuiLocalize->Find( pCycleText );
		m_pCycleTextLabel->SetText( pwszText ? pwszText : L"" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnNavButtonSelected( KeyValues *pData )
{
	const int iTeam = pData->GetInt( "userdata", -1 );	AssertMsg( iTeam >= 0, "Bad filter" );
	if ( iTeam < 0 )
		return;

	if ( !m_pPlayerModelPanel )
		return;

	m_pPlayerModelPanel->SetTeam( iTeam );
	CyclePaint( false );

	C_CTFGameStats::ImmediateWriteInterfaceEvent( "team_switch(store_preview_item_panel)", iTeam == TF_TEAM_RED ? "red" : "blu" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnExitFullscreen( KeyValues *pData )
{
	if ( !m_pPlayerModelPanel )
		return;

	// If team or class changed in fullscreen mode, update our ui components here
	if ( m_pTeamNavPanel )
	{
		m_pTeamNavPanel->UpdateButtonSelectionStates( m_pPlayerModelPanel->GetTeam() == TF_TEAM_RED ? 0 : 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnTick( void )
{
	BaseClass::OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnMouseWheeled( int delta )
{
	if ( !m_pScrollBar )
		return;

	int val = m_pScrollBar->GetValue();
	val -= (delta * 50);
	m_pScrollBar->SetValue( val );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnSliderMoved( int position )
{
	m_iSliderPos = position;
	UpdateScrollableChild();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::OnThink()
{
	BaseClass::OnThink();

	Assert( IsVisible() );

	// If the user clicks outside of the dialog frame, close the preview, like
	// many web sites do on the internet.
	bool bMouseDown = vgui::input()->IsMouseDown( MOUSE_LEFT );
	
	// User just clicked?
	if ( !m_pFullscreenPanel || !m_pFullscreenPanel->IsFullscreenMode() )
	{
		if ( !m_bMouseWasDown && bMouseDown )
		{
			vgui::input()->GetCursorPos( m_aClickPos[0], m_aClickPos[1] );
			m_bMouseWasDown = true;
		}
		else if ( m_pDialogFrame && bMouseDown && !m_pDialogFrame->IsWithin( m_aClickPos[0], m_aClickPos[1] ) )
		{
			//m_bCloseOnUp = true;
		}
		else if ( !bMouseDown )
		{
			if ( m_bCloseOnUp )
			{
				C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_preview_item_panel", "close_from_outside_click" );

				DoClose();
			}
			m_bCloseOnUp = false;
			m_bMouseWasDown = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanel2::DoClose()
{
	if ( m_pFullscreenPanel )
	{
		m_pFullscreenPanel->ExitFullscreen();
	}
	
	OnClose();

	SetVisible( false );
}
