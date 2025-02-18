//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store_page_new.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "gamestringpool.h"
#include "econ_item_inventory.h"
#include "econ_item_system.h"
#include "item_model_panel.h"
#include "store/store_panel.h"
#include "store_preview_item.h"
#include "store_viewcart.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel_New::CStorePricePanel_New( vgui::Panel *pParent, const char *pPanelName ) 
: CStorePricePanel( pParent, pPanelName )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel_New::SetItem( const econ_store_entry_t *pEntry )
{
	BaseClass::SetItem( pEntry );

	CExLabel *pNew = dynamic_cast< CExLabel* >( FindChildByName( "New" ) );
	if ( pNew )
	{
		pNew->SetVisible( false );
	}

	pNew = dynamic_cast< CExLabel* >( FindChildByName( "NewLarge" ) );
	if ( pNew )
	{
		int contentWidth, contentHeight;
		pNew->GetContentSize( contentWidth, contentHeight );
		int iTextInsetX, iTextInsetY;
		pNew->GetTextInset( &iTextInsetX, &iTextInsetY );
		pNew->SetWide( contentWidth + iTextInsetX );
		int iPosX, iPosY;
		pNew->GetPos( iPosX, iPosY );
		pNew->SetPos( GetWide() - pNew->GetWide(), iPosY );
		pNew->SetVisible( true );
	}

	vgui::Panel* pLimited = FindChildByName( "LimitedLarge" );
	if ( pLimited )
	{
		int iPosX, iPosY;
		pLimited->GetPos( iPosX, iPosY );

		if ( pNew && pEntry->m_bLimited )
		{
			iPosY = pNew->GetTall() + YRES( 3 );
		}

		pLimited->SetPos( GetWide() - pLimited->GetWide() - XRES( 3 ), iPosY );
		pLimited->SetVisible( pEntry->m_bLimited );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel_Bundles::CStorePricePanel_Bundles( vgui::Panel *pParent, const char *pPanelName ) 
:	CStorePricePanel( pParent, pPanelName ),
	m_pLimitedLarge( NULL )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel_Bundles::SetItem( const econ_store_entry_t *pEntry )
{
	BaseClass::SetItem( pEntry );

	if ( m_pLimitedLarge )
	{
		m_pLimitedLarge->SetVisible( pEntry->m_bLimited );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel_Bundles::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	m_pLimitedLarge = dynamic_cast<ImagePanel *>( FindChildByName( "LimitedLarge" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel_Bundles::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pLimitedLarge )
	{
		int aPos[2];
		m_pLimitedLarge->GetPos( aPos[0], aPos[1] );

		if ( m_pNew && m_pNew->IsVisible() )
		{
			aPos[1] = m_pNew->GetTall() + YRES( 3 );
		}

		m_pLimitedLarge->SetPos( GetWide() - m_pLimitedLarge->GetWide(), aPos[1] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel_Jumbo::CStorePricePanel_Jumbo( vgui::Panel *pParent, const char *pPanelName ) 
: CStorePricePanel( pParent, pPanelName )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel_Popular::CStorePricePanel_Popular( vgui::Panel *pParent, const char *pPanelName, int iPopularityRank )
: CStorePricePanel( pParent, pPanelName )
, m_iPopularityRank( iPopularityRank )
{
	m_pNewLarge = NULL;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel_Popular::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pNewLarge = dynamic_cast< CExLabel* >( FindChildByName( "NewLarge" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel_Popular::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pNewLarge )
	{
		int contentWidth, contentHeight;
		m_pNewLarge->GetContentSize( contentWidth, contentHeight );
		int iTextInsetX, iTextInsetY;
		m_pNewLarge->GetTextInset( &iTextInsetX, &iTextInsetY );
		m_pNewLarge->SetWide( contentWidth + iTextInsetX );
		int iPosX, iPosY;
		m_pNewLarge->GetPos( iPosX, iPosY );
		m_pNewLarge->SetPos( GetWide() - m_pNewLarge->GetWide(), iPosY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel_Popular::SetItem( const econ_store_entry_t *pEntry )
{
	BaseClass::SetItem( pEntry );

	CExLabel *pNew = dynamic_cast< CExLabel* >( FindChildByName( "New" ) );
	if ( pNew )
	{
		pNew->SetVisible( false );
	}

	if ( m_pNewLarge )
	{
		if ( pEntry->m_bNew )
		{
			m_pNewLarge->SetVisible( true );
		}
		else
		{
			m_pNewLarge->SetVisible( false );
		}
	}

	vgui::Panel* pLimited = FindChildByName( "LimitedLarge" );
	if ( pLimited && m_pNewLarge )
	{
		int iPosX, iPosY;
		pLimited->GetPos( iPosX, iPosY );

		if ( pEntry->m_bLimited && pEntry->m_bNew )
		{
			iPosY = m_pNewLarge->GetTall() + YRES( 3 );
		}

		pLimited->SetPos( GetWide() - m_pNewLarge->GetWide() - 14, iPosY );
		pLimited->SetVisible( pEntry->m_bLimited );
	}

	wchar_t wszRank[10];
	_snwprintf( wszRank, ARRAYSIZE( wszRank ), L"%d", m_iPopularityRank );
	wchar_t wszText[8];
	g_pVGuiLocalize->ConstructString_safe( wszText, g_pVGuiLocalize->Find( "TF_Popularity_Rank" ), 1, wszRank );
	SetDialogVariable( "rank1", wszText );
	SetDialogVariable( "rank2", wszText );

	// Show rank or rank2 based on old store/new store
	CExLabel *pRank = dynamic_cast<CExLabel *>( FindChildByName( CFmtStr( "Rank%i", GetStoreVersion() ).Access() ) );
	if ( pRank )
	{
		pRank->SetVisible( true );
	}
}