//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_item_card_panel.h"
#include "econ_item_description.h"
#include "vgui_controls/TextImage.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "navigationpanel.h"
#include "IconPanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ScrollBarSlider.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Tooltip.h>
#include <vgui_controls/AnimationController.h>
#include "clientmode_tf.h"

using namespace vgui;



DECLARE_BUILD_FACTORY( CRepeatingContainer );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRepeatingContainer::CRepeatingContainer( Panel *pParent, const char *pszName )
	: EditablePanel( pParent, pszName )
	, m_eLayoutMethod( METHOD_EVEN )
{}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRepeatingContainer::~CRepeatingContainer()
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRepeatingContainer::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues* pCommonSettings = inResourceData->FindKey( "CommonSettings" );
	KeyValues* pIndividualSettings = inResourceData->FindKey( "IndividualSettings" );

	// Delete old panels
	m_vecChildren.PurgeAndDeleteElements();

	if ( pIndividualSettings && pCommonSettings )
	{
		// Go through every individual panel
		FOR_EACH_SUBKEY( pIndividualSettings, pSubKey )
		{
			// Merge the individual keys onto the common keys, keeping "individual" values if there's a conflict
			pSubKey->RecursiveMergeKeyValues( pCommonSettings );

			// Create each panel
			Panel *pNewPanel = CreateControlByName( pSubKey->GetString( "ControlName" ) );
			if ( pNewPanel )
			{
				pNewPanel->SetParent( this );	
				pNewPanel->SetBuildGroup( GetBuildGroup() );
				pNewPanel->ApplySettings( pSubKey );
				m_vecChildren.AddToTail( pNewPanel );
			}
		}
	}

	const char *pszSpacingMethod = inResourceData->GetString( "spacing_method", NULL );
	if ( pszSpacingMethod )
	{
		// Figure out how we're going to layout all these panels
		if ( FStrEq( pszSpacingMethod, "METHOD_STEP" ) )
		{
			m_eLayoutMethod = METHOD_STEP;
		}
		else // Default to event
		{
			m_eLayoutMethod = METHOD_EVEN;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRepeatingContainer::PerformLayout()
{
	BaseClass::PerformLayout();

	// No children?  We're done.
	if ( m_vecChildren.IsEmpty() )
		return;

	// Fixed step gaps
	if ( m_eLayoutMethod == METHOD_STEP )
	{
		FOR_EACH_VEC( m_vecChildren, i )
		{
			m_vecChildren[i]->SetPos( m_iXStep * i , 0 );
		}
	}
	else // default METHOD_EVEN
	{
		// Evently spaced
		int nParentWide = GetWide();
		int nTotalChildWide = 0;

		FOR_EACH_VEC( m_vecChildren, i )
		{
			nTotalChildWide += m_vecChildren[i]->GetWide();
		}
		
		int nXStep = 0;
		if ( nTotalChildWide < nParentWide )
		{
			nXStep = ( nParentWide - nTotalChildWide ) / ( m_vecChildren.Count() - 1 );
		}

		int nXPos = 0;
		FOR_EACH_VEC( m_vecChildren, i )
		{
			m_vecChildren[i]->SetPos( nXPos, 0 );
			nXPos += m_vecChildren[i]->GetWide() + nXStep;
		}
	}
}


DECLARE_BUILD_FACTORY( CTFItemCardPanel );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItemCardPanel::CTFItemCardPanel( Panel *pParent, const char *pszName )
	: BaseClass( pParent, pszName )
	, m_pItem( NULL )
	, m_bAllControlsValid( false )
	, m_bPinned( false )
	, m_pDropShadow( NULL )
	, m_pRarityBackgroundOverlay( NULL )
	, m_pCardTop( NULL )
	, m_pItemModel( NULL )
	, m_pRarityContainer( NULL )
	, m_pItemName( NULL )
	, m_pRarityName( NULL )
	, m_pInfoContainer( NULL )
	, m_pClassLabel( NULL )
	, m_pClassIconContainer( NULL )
	, m_pTypeLabel( NULL )
	, m_pTypeLabelValue( NULL )
	, m_pExteriorLabel( NULL )
	, m_pExteriorLabelValue( NULL )
	, m_pBottomContainer( NULL )
	, m_pBottomScrollingContainer( NULL )
	, m_pAttribsLabel( NULL )
	, m_pEquipSlotLabel( NULL )
{
	m_pDropShadow = new ImagePanel( pParent, "ItemCardShadow" );
	m_pDropShadow->SetVisible( false );
	m_pDropShadow->SetAutoDelete( false ); // We'll delete this panel
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItemCardPanel::~CTFItemCardPanel()
{
	m_pDropShadow->MarkForDeletion();
	m_pDropShadow = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Do FindControl() but also verify that we got what we were looking for
//-----------------------------------------------------------------------------
template < class T >
T* CTFItemCardPanel::FindAndVerifyControl( Panel* pParent, const char* pszPanelName )
{
	if ( !m_bAllControlsValid )
		return NULL;

	// Find the panel
	T* pChild = pParent->FindControl< T >( pszPanelName, true );
	// Make sure it's still there
	m_bAllControlsValid &= pChild != NULL;

	return pChild;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadResFileForCurrentItem();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( !m_pItem )
	{
		return;
	}

	if ( !m_bAllControlsValid )
	{
		return;
	}

	m_pBottomScrollingContainer->InvalidateLayout();

	UpdateDescription();
	UpdateModelOrIcon();

	// Position grime
	{
		// Randomize based on our original item ID, if we have one.  If not, just use defindex
		RandomSeed( m_pItem->GetSOCData() ? m_pItem->GetSOCData()->GetOriginalID() : m_pItem->GetItemDefIndex() );

		// Randomize X/Y
		int nGrimeX = RandomInt( -abs( m_pGrime->GetWide() - GetWide() ), 0 );
		int nGrimeY = RandomInt( -abs( m_pGrime->GetTall() - GetTall() ), 0 );
		m_pGrime->SetPos( nGrimeX, nGrimeY );
		// Randomize 0,90,180,270 rotation
		m_pGrime->GetImage()->SetRotation( RandomInt( 0, 3 ) ); // Have to GetImage()->SetRotation because ImagePanel::SetRotation does nothing!
	}

	// Update our shadow's settings
	{
		m_pDropShadow->SetZPos( GetZPos() - 1 );
		m_pDropShadow->SetShouldScaleImage( true );
		m_pDropShadow->SetMouseInputEnabled( false );
		m_pDropShadow->SetImage( "item_card/standard_background_dropshadow" );
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::SetVisible( bool bVisible )
{
	// Update the position of our external shadow panel
	if ( m_bAllControlsValid )
	{
		int x=0,y=0,wide,tall,xTemp,yTemp;
		m_pBackground->GetBounds( xTemp, yTemp, wide, tall );
		x += xTemp; y += yTemp;
		m_pMainContainer->GetPos( xTemp, yTemp );
		x += xTemp; y += yTemp;
		GetPos( xTemp, yTemp );
		x += xTemp; y += yTemp;
		
		m_pDropShadow->SetBounds( x + m_iShadowOffset, y + m_iShadowOffset, wide * 1.15f, tall * 1.15f );
	}

	BaseClass::SetVisible( bVisible );

	m_pDropShadow->SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: Force the scrolling container to have mouse input matching the panel's
//-----------------------------------------------------------------------------
void CTFItemCardPanel::SetMouseInputEnabled( bool state )
{
	BaseClass::SetVisible( state );

	if ( m_bAllControlsValid )
	{
		m_pBottomScrollingContainer->SetMouseInputEnabled( state );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::SetItem( CEconItemView* pItem )
{
	m_pItem = pItem;

	// Update the panels
	LoadResFileForCurrentItem();
	MakeReadyForUse();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::PinCard( bool bPin ) 
{

	bool bDiff = bPin != m_bPinned;
	m_bPinned = bPin; 

	if ( bDiff && bPin )
	{
		g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel( this );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "ItemCard_HidePinHint" );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "ItemCard_ShowCloseButton" );
		SetVisible( true );
	}
	else if ( bDiff && !bPin )
	{
		g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel( this );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "ItemCard_ShowPinHint" );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "ItemCard_HideCloseButton" );
		SetVisible( false );
	}

	// Force mouse input
	SetMouseInputEnabled( bPin );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::UpdateDescription()
{
	const GameItemDefinition_t *pItemDef = m_pItem->GetItemDefinition();
	if ( !pItemDef )
		return;

	// Itm name
	m_pItemName->SetText( m_pItem->GetItemName() );

	// If we dont have a rarity, then we assume we're an "old" item.
	if ( !GetItemSchema()->GetRarityColor(pItemDef->GetRarity() ) )
	{
		// Grab the item quality (ie. strange, unusual)
		const char *pszQualityColorString = EconQuality_GetColorString( (EEconItemQuality)m_pItem->GetItemQuality() );
		if ( m_pItem->IsValid() && pszQualityColorString )
		{
			IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
			Color colorName = pScheme->GetColor( pszQualityColorString, Color( 0, 255, 0, 255 ) );
			m_pItemName->SetFgColor( colorName );
		}
	}


	// Set highlighting on the class icons
	{
		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			CExImageButton *pExImage = dynamic_cast< CExImageButton* >( m_pClassIconContainer->GetRepeatingChild( GetRemappedMenuIndexForClass(i) - 1 ) );
			if ( pExImage )
			{
				pExImage->SetSelected( pItemDef->CanBeUsedByClass( i ) );
			}
		}
	}

	// Set type name into the label
	{
		const locchar_t *locTypename = g_pVGuiLocalize->Find( pItemDef->GetItemTypeName() );
		m_pTypeLabelValue->SetText( locTypename );
	}

	const CEconItemRarityDefinition* pItemRarity = GetItemSchema()->GetRarityDefinition( pItemDef->GetRarity() );

	// Setup the rarity color overlay
	{
		attrib_colors_t attribColor = ATTRIB_COL_RARITY_DEFAULT;

		if ( pItemRarity )
		{
			attribColor = pItemRarity->GetAttribColor();
		}

		vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
		Color color = pScheme->GetColor( GetColorNameForAttribColor( attribColor ), Color( 255, 255, 255, 255 ) );
		m_pRarityBackgroundOverlay->SetDrawColor( color );
		m_pRarityName->SetFgColor( color );
	}

	// Rarity name into the label
	{
		const char *pszRarityName = "#Rarity_Default";
		if ( pItemRarity )
		{
			pszRarityName = pItemRarity->GetLocKey();
		}
		
		m_pRarityName->SetText( g_pVGuiLocalize->Find( pszRarityName ) );
	}

	enum { kAttribBufferSize = 4 * 1024 };
	wchar_t wszAttribBuffer[ kAttribBufferSize ] = L"";

	// Space out the attributes
	const CEconItemDescription *pDescription = m_pItem->GetDescription();
	if ( pDescription )
	{
		unsigned int unWrittenLines = 0;
		for ( unsigned int i = 0; i < pDescription->GetLineCount(); i++ )
		{
			const econ_item_description_line_t& line = pDescription->GetLine(i);
			if ( (line.unMetaType & ( kDescLineFlag_Name ) ) == 0 )
			{
				V_wcscat_safe( wszAttribBuffer, L"\n" );					// add empty lines everywhere
				V_wcscat_safe( wszAttribBuffer, line.sText.Get() );
				++unWrittenLines;
			}
		}

		// Get all the attributes
		Assert( m_pItem->GetDescription() );
		if ( m_pAttribsLabel->GetTextImage() && m_pItem->GetDescription() )
		{
			m_pAttribsLabel->SetText( wszAttribBuffer );

			TextImage *pTextImage = m_pAttribsLabel->GetTextImage();
			Assert( pTextImage );

			pTextImage->ClearColorChangeStream();

			IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

			Color prevCol;
			unsigned int unCurrentTextStreamIndex = 0;
			for ( unsigned int i = 0; i < pDescription->GetLineCount(); i++ )
			{
				const econ_item_description_line_t& line = pDescription->GetLine(i);

				// Ignore the name line, it was added above
				if ( ( line.unMetaType & ( kDescLineFlag_Name ) ) != 0 )
				{
					continue;
				}

				Color col = pScheme->GetColor( GetColorNameForAttribColor( line.eColor ), Color( 255, 255, 255, 255 ) );

				// Output a color change if necessary.
				if ( i == 0 || prevCol != col )
				{
					pTextImage->AddColorChange( col, unCurrentTextStreamIndex );
					prevCol = col;
				}
			
				unCurrentTextStreamIndex += StringFuncs<locchar_t>::Length( line.sText.Get() ) + 1;	// add one character to deal with newlines
			}

			int nWide, nTall;
			pTextImage->GetContentSize( nWide, nTall );
			m_pAttribsLabel->SetTall( nTall );
		}
	}

	// Set equip slot
	{
		int nEquipSlot =  pItemDef->GetDefaultLoadoutSlot();
		if ( nEquipSlot != -1 )
		{
			m_pEquipSlotLabel->SetText( g_pVGuiLocalize->Find( GetItemSchema()->GetLoadoutStringsForDisplay( pItemDef->GetEquipType() )[ nEquipSlot ] ) );
		}
		else
		{
			m_pEquipSlotLabel->SetText( "" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::UpdateModelOrIcon()
{
	if ( !m_pItem )
	{
		return;
	}

	m_pItemModel->SetItem( m_pItem );

	const char *pszModelName = m_pItem->GetPlayerDisplayModel( 0, 0 );
	if ( pszModelName )
	{
		MDLHandle_t hMDL = mdlcache->FindMDL( pszModelName );
		m_pItemModel->SetMDL( hMDL, static_cast<IClientRenderable*>(m_pItem) );
		mdlcache->Release( hMDL ); // counterbalance addref from within FindMDL
		m_pItemModel->SetForceModelUsage( true );
	}
	else
	{
		m_pItemModel->SetInventoryImageType( CEmbeddedItemModelPanel::IMAGETYPE_LARGE );
		m_pItemModel->LoadInventoryImage();
		m_pItemModel->SetForceModelUsage( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemCardPanel::LoadResFileForCurrentItem()
{
	// Temp hack.  New items get the new cards
	const char *pszResFile = "Resource/UI/econ/ItemCardPanel_Series1.res";
	if ( m_pItem )
	{
		const GameItemDefinition_t *pItemDef = m_pItem->GetItemDefinition();
		if ( pItemDef )
		{
			const CEconItemRarityDefinition* pItemRarity = GetItemSchema()->GetRarityDefinition( pItemDef->GetRarity() );
			if ( pItemRarity && pItemRarity->GetDBValue() > 0 )
			{
				pszResFile = "Resource/UI/econ/ItemCardPanel_Series2.res";
			}
		}
	}

	m_bAllControlsValid = false;

	LoadControlSettings( pszResFile );

	m_bAllControlsValid = true;
	// Grab all the controls...
	m_pMainContainer = FindAndVerifyControl< EditablePanel >( this, "MainContainer" );
	m_pRarityBackgroundOverlay = FindAndVerifyControl< ImagePanel >( m_pMainContainer, "RarityBackgroundOverlay" );
	m_pBackground = FindAndVerifyControl< ImagePanel >( m_pMainContainer, "Background" );
	m_pGrime = FindAndVerifyControl< ImagePanel >( m_pMainContainer, "GrimeLayer" );
	m_pCardTop = FindAndVerifyControl< EditablePanel >( m_pMainContainer, "CardTop" );
	m_pItemModel = FindAndVerifyControl< CEmbeddedItemModelPanel >( m_pCardTop, "ItemModel" );
	m_pRarityContainer = FindAndVerifyControl< EditablePanel >( m_pMainContainer, "RarityContainer" );
	m_pItemName = FindAndVerifyControl< Label >( m_pRarityContainer, "ItemNameLabel" );
	m_pRarityName = FindAndVerifyControl< Label >( m_pRarityContainer, "ItemRarityLabel" );
	m_pClassIconContainer = FindAndVerifyControl< CRepeatingContainer >( m_pMainContainer, "ClassIconContainer" );
	m_pInfoContainer = FindAndVerifyControl< EditablePanel >( m_pMainContainer, "InfoContainer" );
	m_pClassLabel = FindAndVerifyControl< Label >( m_pInfoContainer, "ClassLabel" );
	m_pTypeLabel = FindAndVerifyControl< Label >( m_pInfoContainer, "TypeLabel" );
	m_pTypeLabelValue = FindAndVerifyControl< Label >( m_pInfoContainer, "TypeValueLabel" );
	m_pExteriorLabel = FindAndVerifyControl< Label >( m_pInfoContainer, "ExteriorLabel" );
	m_pExteriorLabelValue = FindAndVerifyControl< Label >( m_pInfoContainer, "ExteriorValueLabel" );
	m_pBottomContainer = FindAndVerifyControl< EditablePanel >( m_pMainContainer, "BottomContainer" );
	m_pBottomScrollingContainer = FindAndVerifyControl< CExScrollingEditablePanel >( m_pBottomContainer, "ScrollableBottomContainer" );
	m_pAttribsLabel = FindAndVerifyControl< Label >( m_pBottomScrollingContainer, "AttribsLabel" );
	m_pEquipSlotLabel = FindAndVerifyControl< Label >( this, "EquipSlotLabel" );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CItemCardPanelToolTip::CItemCardPanelToolTip( Panel *parent, const char *text ) 
: BaseTooltip( parent, text )
, m_pMouseOverItemPanel( NULL )
, m_iPositioningStrategy( IPTTP_BOTTOM_SIDE )
{
	m_hCurrentPanel = NULL;
	SetTooltipDelay( 100 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemCardPanelToolTip::GetPosition( itempanel_tooltippos_t iTooltipPosition, CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos )
{
	switch ( iTooltipPosition )
	{
	case IPTTP_LEFT:
		*iXPos = ( iItemX - m_pMouseOverItemPanel->GetWide() );
		*iYPos = ( iItemY + pItemPanel->GetTall() * 0.5f ) - ( m_pMouseOverItemPanel->GetTall() * 0.5f );
		break;
	case IPTTP_RIGHT: 
		*iXPos = ( iItemX + pItemPanel->GetWide() );
		*iYPos = ( iItemY + pItemPanel->GetTall() * 0.5f ) - ( m_pMouseOverItemPanel->GetTall() * 0.5f );
		break;
	}

	*iYPos = Clamp( *iYPos, (int)YRES( -30 ), int( m_pParentPanel->GetTall() - m_pMouseOverItemPanel->GetTall() - YRES( 30 ) ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CItemCardPanelToolTip::ValidatePosition( CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos )
{
	if ( *iXPos < 0 )
		return false;

	if ( ( *iXPos + m_pMouseOverItemPanel->GetWide() ) > m_pParentPanel->GetWide() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemCardPanelToolTip::PerformLayout() 
{
	BaseClass::PerformLayout();

	if ( !ShouldLayout() )
		return;

	_isDirty = false;

	CItemModelPanel *pItemPanel = m_hCurrentPanel.Get();
	if ( m_pMouseOverItemPanel && pItemPanel && !m_pMouseOverItemPanel->IsPinned() ) 
	{		
		CEconItemView *pItem = pItemPanel->GetItem();
		if ( pItem )
		{
			m_pMouseOverItemPanel->SetItem( pItem );
			

			int x,y;

			// If the panel is somewhere in a derived class, we need to get its position in our space
			if ( pItemPanel->GetParent() != m_pMouseOverItemPanel->GetParent() )
			{
				int iItemAbsX, iItemAbsY;
				ipanel()->GetAbsPos( pItemPanel->GetVPanel(), iItemAbsX, iItemAbsY );
				int iParentAbsX, iParentAbsY;
				ipanel()->GetAbsPos( m_pMouseOverItemPanel->GetParent()->GetVPanel(), iParentAbsX, iParentAbsY );

				x = (iItemAbsX - iParentAbsX);
				y = (iItemAbsY - iParentAbsY);
			}
			else
			{
				pItemPanel->GetPos( x, y );
			}

			int iXPos = 0;
			int iYPos = 0;

			// Loop through the positions in our strategy, and hope we find a valid spot
			for ( int i = 0; i < NUM_POSITIONS_PER_STRATEGY; i++ )
			{
				itempanel_tooltippos_t iPos = g_iTooltipStrategies[m_iPositioningStrategy][i];
				if ( iPos != IPTTP_LEFT && iPos != IPTTP_RIGHT )
					continue;

				GetPosition( iPos, pItemPanel, x, y, &iXPos, &iYPos );

				if ( ValidatePosition( pItemPanel, x, y, &iXPos, &iYPos ) )
					break;
			}

			m_pMouseOverItemPanel->SetPos( iXPos, iYPos );
			m_pMouseOverItemPanel->SetVisible( true );			
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemCardPanelToolTip::ShowTooltip( Panel *currentPanel ) 
{ 
	if ( m_pMouseOverItemPanel && currentPanel != m_hCurrentPanel.Get() ) 
	{
		CItemModelPanel *pItemPanel = assert_cast<CItemModelPanel *>(currentPanel);
		m_hCurrentPanel.Set( pItemPanel );
		pItemPanel->PostActionSignal( new KeyValues("ItemPanelEntered") );
		vgui::surface()->PlaySound( "ui/item_info_mouseover.wav" );
	}
	BaseClass::ShowTooltip( currentPanel );	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemCardPanelToolTip::HideTooltip() 
{ 
	if ( m_pMouseOverItemPanel ) 
	{
		if ( m_pMouseOverItemPanel->IsPinned() )
			return;

		m_pMouseOverItemPanel->SetVisible( false ); 
	}

	if ( m_hCurrentPanel )
	{
		m_hCurrentPanel.Get()->PostActionSignal( new KeyValues("ItemPanelExited") );
		m_hCurrentPanel = NULL;
	}
}
