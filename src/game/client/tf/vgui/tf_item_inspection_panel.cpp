//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_item_inspection_panel.h"
#include "item_model_panel.h"
#include "navigationpanel.h"
#include "gc_clientsystem.h"
#include "econ_ui.h"
#include "backpack_panel.h"
#include "vgui_int.h"
#include "cdll_client_int.h"
#include "clientmode_tf.h"
#include "ienginevgui.h"
#include "tf_hud_mainmenuoverride.h"
#include "econ_item_system.h"
#include "vgui_controls/ComboBox.h"
#include "econ_paintkit.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls//TextEntry.h"
#include "econ_item_description.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/IInput.h>
#include "character_info_panel.h"

using namespace vgui;


ConVar tf_item_inspect_model_spin_rate( "tf_item_inspect_model_spin_rate", "30", FCVAR_ARCHIVE );
ConVar tf_item_inspect_model_auto_spin( "tf_item_inspect_model_auto_spin", "1", FCVAR_ARCHIVE );


// ********************************************************************************************************************************
// Given proto data, create an inspect panel
void Helper_LaunchPreviewWithPreviewDataBlock( CEconItemPreviewDataBlock const &protoData )
{
	// Create an econ item view
	CEconItem econItem;
	econItem.DeserializeFromProtoBufItem( protoData.econitem() );
	
	CEconItemView itemView;
	itemView.Init( econItem.GetDefinitionIndex(), econItem.GetQuality(), econItem.GetItemLevel(), econItem.GetAccountID() );
	itemView.SetNonSOEconItem( &econItem );

	// Get the Backpack to tell the inspect panel to render
	if ( EconUI() )
	{
		EconUI()->GetBackpackPanel()->OpenInspectModelPanelAndCopyItem( &itemView );
	}
}

// ********************************************************************************************************************************
// Open the inspection panel
void Helper_LaunchPreview()
{
	// Get the Backpack to tell the inspect panel to render
	if ( EconUI() )
	{
		CCharacterInfoPanel* pCharInfo = dynamic_cast< CCharacterInfoPanel* >( EconUI() );
		EconUI()->OpenEconUI( ECONUI_BACKPACK );
		pCharInfo->OpenToPaintkitPreview( NULL, false, false );
	}
}

// ********************************************************************************************************************************
// Request an item from the GC to inspect
static void Helper_RequestEconActionPreview( uint64 paramS, uint64 paramA, uint64 paramD, uint64 paramM )
{
	if ( !paramA || !paramD )
		return;
	if ( !paramS && !paramM )
		return;

	static double s_flTime = 0.0f;
	double flNow = Plat_FloatTime();
	if ( s_flTime && ( flNow - s_flTime <= 2.5 ) )
		return;

	s_flTime = flNow;
	GCSDK::CProtoBufMsg< CMsgGC_Client2GCEconPreviewDataBlockRequest > msg( k_EMsgGC_Client2GCEconPreviewDataBlockRequest );
	msg.Body().set_param_s( paramS );
	msg.Body().set_param_a( paramA );
	msg.Body().set_param_d( paramD );
	msg.Body().set_param_m( paramM );
	GCClientSystem()->GetGCClient()->BSendMessage( msg );
}

// ********************************************************************************************************************************
class CGCClient2GCEconPreviewDataBlockResponse : public GCSDK::CGCClientJob
{
public:
	CGCClient2GCEconPreviewDataBlockResponse( GCSDK::CGCClient *pGCClient ) : GCSDK::CGCClientJob( pGCClient ) { }

	virtual bool BYieldingRunJobFromMsg( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg<CMsgGC_Client2GCEconPreviewDataBlockResponse> msg( pNetPacket );
		Helper_LaunchPreviewWithPreviewDataBlock( msg.Body().iteminfo() );
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGCClient2GCEconPreviewDataBlockResponse, "CGCClient2GCEconPreviewDataBlockResponse", k_EMsgGC_Client2GCEconPreviewDataBlockResponse, GCSDK::k_EServerTypeGCClient );

// ********************************************************************************************************************************
CON_COMMAND_F( tf_econ_item_preview, "Preview an economy item", FCVAR_DONTRECORD | FCVAR_HIDDEN | FCVAR_CLIENTCMD_CAN_EXECUTE )
{
	if ( args.ArgC() < 2 )
		return;

	//// kill the workshop preview dialog if it's up
	//if ( g_pWorkshopWorkbenchDialog )
	//{
	//	delete g_pWorkshopWorkbenchDialog;
	//	g_pWorkshopWorkbenchDialog = NULL;
	//}

	//extern float g_flReadyToCheckForPCBootInvite;
	//if ( !g_flReadyToCheckForPCBootInvite || !gpGlobals->curtime || !gpGlobals->framecount )
	//{
	//	ConMsg( "Deferring csgo_econ_action_preview command!\n" );
	//	return;
	//}

	// Encoded parameter, validate basic length
	char const *pchEncodedAscii = args.Arg( 1 );
	int nLen = Q_strlen( pchEncodedAscii );
	if ( nLen <= 16 ) { Assert( 0 ); return; }

	// If we are launched with new format requesting steam_ownerid and assetid then do async query
	if ( *pchEncodedAscii == 'S' )
	{
		uint64 uiParamS = Q_atoui64( pchEncodedAscii + 1 );
		uint64 uiParamA = 0;
		uint64 uiParamD = 0;
		if ( char const *pchParamA = strchr( pchEncodedAscii, 'A' ) )
		{
			uiParamA = Q_atoui64( pchParamA + 1 );
			if ( char const *pchParamD = strchr( pchEncodedAscii, 'D' ) )
			{
				uiParamD = Q_atoui64( pchParamD + 1 );
				Helper_RequestEconActionPreview( uiParamS, uiParamA, uiParamD, 0ull );
			}
		}
		return;
	}

	// Else if we are launched with new format requesting market listing id and assetid then do async query
	if ( *pchEncodedAscii == 'M' )
	{
		uint64 uiParamM = Q_atoui64( pchEncodedAscii + 1 );
		uint64 uiParamA = 0;
		uint64 uiParamD = 0;
		if ( char const *pchParamA = strchr( pchEncodedAscii, 'A' ) )
		{
			uiParamA = Q_atoui64( pchParamA + 1 );
			if ( char const *pchParamD = strchr( pchEncodedAscii, 'D' ) )
			{
				uiParamD = Q_atoui64( pchParamD + 1 );
				Helper_RequestEconActionPreview( 0ull, uiParamA, uiParamD, uiParamM );
			}
		}
		return;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItemInspectionPanel::CTFItemInspectionPanel( Panel* pPanel, const char *pszName )
	: BaseClass( pPanel, pszName )
	, m_pItemViewData( NULL )
	, m_pSOEconItemData( NULL )
	, m_nValidItemsPaintkitDefindex( INVALID_ITEM_DEF_INDEX )
	, m_nValidPaintkitsItemDefindex( 0xFFFF )
{
	m_pModelInspectPanel = new CEmbeddedItemModelPanel( this, "ModelInspectionPanel" );
	m_pTeamColorNavPanel = new CNavigationPanel( this, "TeamNavPanel" );
	m_pItemNamePanel = new CItemModelPanel( this, "ItemName" );
	m_pPaintkitPreviewContainer = new EditablePanel( this, "PaintkitPreviewContainer" );
	m_pWearSlider = new Slider( m_pPaintkitPreviewContainer, "WearSlider" );
	m_pComboBoxValidPaintkits = new ComboBox( m_pPaintkitPreviewContainer, "ComboBoxValidPaintkits", 0, false );
	m_pComboBoxValidItems = new ComboBox( m_pPaintkitPreviewContainer, "ComboBoxValidItems", 0, false );
	m_pSeedTextEntry = new TextEntry( m_pPaintkitPreviewContainer, "SeedTextEntry" );
	m_pRandomSeedButton = new Button( m_pPaintkitPreviewContainer, "NewSeedButton", (const char*)nullptr );
	m_pMarketButton = new Button( m_pPaintkitPreviewContainer, "MarketButton", (const char*)nullptr );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::ApplySchemeSettings( IScheme *pScheme )
{
	const char* pszFileName = "Resource/UI/econ/InspectionPanel.res";

	KeyValuesAD pkvConditions( "conditions" );
	if ( m_bFixedItem )
	{
		pkvConditions->AddSubKey( new KeyValues( "fixed_item" ) );
	}

	if ( m_bFixedPaintkit )
	{
		pkvConditions->AddSubKey( new KeyValues( "fixed_paintkit" ) );
	}

	if ( m_bConsumeMode )
	{
		pkvConditions->AddSubKey( new KeyValues( "consume_mode" ) );
	}

	LoadControlSettings( pszFileName, nullptr, nullptr, pkvConditions );

	BaseClass::ApplySchemeSettings( pScheme );
	m_pPaintkitPreviewContainer->SetVisible( !m_bFixedItem || !m_bFixedPaintkit );
	m_pWearSlider->SetValue( 5 );

	// This is some magic so when you hit the enter key we don't automagically
	// perform OnClick on the random seed button, because you probably were
	// typing in the seed TextEntry and hit enter.
	KeyValues *msg = new KeyValues("DefaultButtonSet");
	msg->SetInt("button", 0 );
	PostMessage( m_pPaintkitPreviewContainer, msg);

	// Start spinning
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "spin_vel", 1.f, 0.f, 2.f, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::PerformLayout()
{

	CEconItemView* pItem = m_pModelInspectPanel->GetItem();
	if ( pItem && pItem->IsValid() )
	{
		// Find out if we should show the team buttons if the item has team skins
		if ( m_pTeamColorNavPanel )
		{
			static CSchemaAttributeDefHandle pAttrTeamColoredPaintkit( "has team color paintkit" );
			uint32 iHasTeamColoredPaintkit = 0;

			// Find out if the item has the attribute
			bool bShowTeamButton = FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttrTeamColoredPaintkit, &iHasTeamColoredPaintkit ) && iHasTeamColoredPaintkit > 0;

			m_pTeamColorNavPanel->SetVisible( bShowTeamButton );
		}

		// Show only the name if there's no rarity
		//m_pItemNamePanel->SetNameOnly( true );

		// Force the description to update right now, or else it might be caught up
		// in the queue of 50 panels in the backpack which want to load their stuff first
		m_pItemNamePanel->UpdateDescription();	
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "close" ) )
	{
		// Clean up after ourselves and set the item back to red
		CEconItemView* pItem = m_pModelInspectPanel->GetItem();
		if ( pItem )
		{
			pItem->SetTeamNumber( TF_TEAM_RED );
			// We need to recomposite again because we might have a blue version
			// in the cache that might get used in a tooltip.
			RecompositeItem();
		}

		SetVisible( false );
		return;
	}
	else if ( FStrEq( command, "random_seed" ) )
	{
		uint64 nRandomSeed = RandomInt( 0, 0x7FFFFFFFUL );
		nRandomSeed = nRandomSeed | (uint64)RandomInt( 0, 0x7FFFFFFFUL ) << 32;
		m_pSeedTextEntry->SetText( CFmtStr( "%llu", nRandomSeed ) );
		m_pSeedTextEntry->PostActionSignal( new KeyValues("TextChanged" ) ); // Is this a hack?  It feels like hack
		return;
	}
	else if ( FStrEq( command, "market" ) )
	{
		if ( m_pItemViewData && steamapicontext && steamapicontext->SteamFriends() )
		{
			const char *pszPrefix = "";
			if ( GetUniverse() == k_EUniverseBeta )
			{
				pszPrefix = "beta.";
			}

			CEconItemLocalizedMarketNameGenerator generator( GLocalizationProvider(), m_pItemViewData );

			char szURL[512];
			V_snprintf( szURL, sizeof(szURL), "http://%ssteamcommunity.com/market/listings/%d/%s", pszPrefix, engine->GetAppID(), CStrAutoEncode( generator.GetFullName() ).ToString() );
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );
		}

		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::OnThink()
{
	BaseClass::OnThink();

	if ( tf_item_inspect_model_auto_spin.GetBool() )
	{
		float flDt = 0.f;
		if ( m_flLastThink != 0.f )
		{
			flDt = Plat_FloatTime() - m_flLastThink;
		}
		m_flLastThink = Plat_FloatTime();

		// If the user is manipulating the panel, stop spinning right away
		if ( m_pModelInspectPanel->BIsBeingManipulated() )
		{
			if ( m_flLastManipulatedTime == 0.f )
			{
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "spin_vel", 0.f, 0.f, 0.0f, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
			}

			m_flLastManipulatedTime = Plat_FloatTime();
		}
		else
		{
			// If they've stopped manipulating the panel, wait a bit, then start auto-spinning again
			float flTimeSinceManip = Plat_FloatTime() - m_flLastManipulatedTime;
			if ( flTimeSinceManip > 2.f && m_flLastManipulatedTime != 0.f )
			{
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "spin_vel", 1.f, 0.f, 2.f, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
				m_flLastManipulatedTime = 0.f;
			}
		}

		m_pModelInspectPanel->RotateYaw( tf_item_inspect_model_spin_rate.GetFloat() * m_flSpinVel * flDt );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::UpdateSeedLabel( const CEconItemView* pItem )
{
	static CSchemaAttributeDefHandle pAttr_CustomPaintKitSeedLo( "custom_paintkit_seed_lo" );
	static CSchemaAttributeDefHandle pAttr_CustomPaintKitSeedHi( "custom_paintkit_seed_hi" );

	uint64 nSeedValue = 0;

	if ( pItem )
	{
		if ( pItem->GetSOCData() )
		{
			nSeedValue = pItem->GetOriginalID();
		}

		attrib_value_t val = 0;
		if ( pItem->FindAttribute( pAttr_CustomPaintKitSeedLo, &val ) )
		{
			nSeedValue = val;
		}

		if ( pItem->FindAttribute( pAttr_CustomPaintKitSeedHi, &val ) )
		{
			nSeedValue = nSeedValue | ( (uint64)val << 32 );
		}
	}

	CUtlString strNewSeed( CFmtStr( "%llu", nSeedValue ) );

	char szBuff[ 128 ];
	m_pSeedTextEntry->GetText( szBuff, sizeof( szBuff ) );

	if ( !FStrEq( szBuff, strNewSeed ) )
	{
		m_pSeedTextEntry->SetText( strNewSeed );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::SetItem( CEconItemView *pItem, bool bReset )
{
	PostActionSignal( new KeyValues( "ItemSelected", "defindex", pItem ? pItem->GetItemDefIndex() : INVALID_ITEM_DEF_INDEX ) );

	UpdateSeedLabel( pItem );

	m_pItemNamePanel->SetItem( pItem );

	QAngle angCurrent = m_pModelInspectPanel->GetPlayerAngles();
	Vector vecCurrent = m_pModelInspectPanel->GetPlayerPos();
	m_pModelInspectPanel->SetItem( pItem ); 

	// SetItem() calls InvalidateLayout( false ) which will cause the orientation of
	// the weapon to reset.  So we're going to call InvalidateLayout( true ) to make
	// that reset happen right now, then put the angles back to what they were before
	// the item got set so the transition is a bit more seamless when the item is the 
	// same defindex
	if ( !bReset )
	{
		m_pModelInspectPanel->InvalidateLayout( true );
		m_pModelInspectPanel->SetModelAnglesAndPosition( angCurrent, vecCurrent );
	}
	else
	{
		m_nValidItemsPaintkitDefindex = INVALID_ITEM_DEF_INDEX;
		m_nValidPaintkitsItemDefindex = 0xFFFF;
	}

	uint32 unPaintKitIndex = 0;
	bool bPaintkit = false;
	bPaintkit = pItem && GetPaintKitDefIndex( pItem, &unPaintKitIndex );
	
	m_pWearSlider->SetEnabled( bPaintkit );
	m_pSeedTextEntry->SetEnabled( bPaintkit );
	m_pRandomSeedButton->SetEnabled( bPaintkit );
	m_pMarketButton->SetEnabled( bPaintkit || ( pItem && pItem->IsMarketable() ) );

	RecompositeItem();
	m_pTeamColorNavPanel->UpdateButtonSelectionStates( 0 );

	RepopulatePaintKitsForItem( pItem );
	RepopulateItemsForPaintkit( pItem );

	SetControlVisible( "ShowPreviewControlsButton", m_pComboBoxValidPaintkits->GetItemCount() > 1, true );

	// Update the team color nav buttons
	{
		CSchemaAttributeDefHandle pAttrTeamColoredPaintkit( "has team color paintkit" );
		uint32 iHasTeamColoredPaintkit = 0;

		// Find out if the item has the attribute
		bool bShowTeamButton = pItem && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttrTeamColoredPaintkit, &iHasTeamColoredPaintkit ) && iHasTeamColoredPaintkit > 0;

		if ( bPaintkit )
		{
			const CPaintKitDefinition* pPaintKitDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CPaintKitDefinition >( unPaintKitIndex  );
			bShowTeamButton |= pPaintKitDef && pPaintKitDef->BHasTeamTextures();
		}

		m_pTeamColorNavPanel->SetVisible( bShowTeamButton );
	}
	
	// Update the wear slider
	if ( pItem )
	{
		static CSchemaAttributeDefHandle pAttrDef_PaintKitWear( "set_item_texture_wear" );
		float flWear = 0.f;
		FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttrDef_PaintKitWear, &flWear );

		m_pWearSlider->SetValue( 6.f - ( flWear * 5.f ), false );
	}

	// Force the description to update right now, or else it might be caught up
	// in the queue of 50 panels in the backpack which want to load their stuff first
	m_pItemNamePanel->UpdateDescription();	

	// We might've just selected an paintkit that doesn't support the item we're showing.  Check
	// for that case, and change the item to something that is supported.
	KeyValues *pUserData = m_pComboBoxValidItems->GetActiveItemUserData();
	if ( pUserData )
	{
		item_definition_index_t unSelectedDefIndex = pUserData->GetInt( "defindex", INVALID_ITEM_DEF_INDEX );
		if ( m_pItemViewData && m_pItemViewData->GetItemDefinition()->GetRemappedItemDefIndex() != unSelectedDefIndex )
		{
			UpdateItemFromControls();
		}
	}
}
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::SetSpecialAttributesOnly( bool bSpecialOnly ) 
{ 
	if ( m_pItemNamePanel )
	{
		if ( bSpecialOnly )
		{
			m_pItemNamePanel->SetNameOnly( false );
		}
		m_pItemNamePanel->SetSpecialAttributesOnly( bSpecialOnly ); 
	}
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::Reset()
{
	InvalidateLayout( true, true );
	SetItem( m_pItemViewData, true );
}

//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::SetItemCopy( CEconItemView *pItem, bool bReset )
{
	// Make a copy of SO data since it comes from the market and will fall out of scope
	if ( m_pSOEconItemData )
	{
		delete m_pSOEconItemData;
		m_pSOEconItemData = NULL;
	}

	if ( pItem )
	{
		if ( !m_pItemViewData )
		{
			m_pItemViewData = new CEconItemView( *pItem );
		}
		else
		{
			*m_pItemViewData = *pItem;
		}
		
		m_pSOEconItemData = pItem->GetSOCData() ? new CEconItem( *pItem->GetSOCData() ) : new CEconItem(); 
		// This next line is very important.  Above, we potentially just created a CEconItem with no defindex.
		// We need to set its defindex to match pItem's defindex so code way far away will iterate the correct
		// attributes when it comes time to render the item's skin.
		m_pSOEconItemData->SetDefinitionIndex( pItem->GetItemDefIndex() );
		m_pItemViewData->SetNonSOEconItem( m_pSOEconItemData );
		// always use high res for inspect
		m_pItemViewData->SetWeaponSkinUseHighRes( true );

		CSchemaAttributeDefHandle pAttrib_WeaponAllowInspect( "weapon_allow_inspect" );
		CEconItemAttribute attrInspect( pAttrib_WeaponAllowInspect->GetDefinitionIndex(), 1.f );
		m_pItemViewData->GetAttributeList()->AddAttribute( &attrInspect );

		SetItem( m_pItemViewData, bReset );
	}
	else
	{
		SetItem( NULL, bReset );
	}
}

void CTFItemInspectionPanel::SetOptions( bool bFixedItem, bool bFixedPaintkit, bool bConsumptionMode )
{
	bool bChange = false;
	bChange = bChange || ( m_bFixedItem		!= bFixedItem );
	bChange = bChange || ( m_bFixedPaintkit != bFixedPaintkit );
	bChange = bChange || ( m_bConsumeMode != bConsumptionMode );
	
	m_bFixedItem = bFixedItem;
	m_bFixedPaintkit = bFixedPaintkit;
	m_bConsumeMode = bConsumptionMode;

	if ( bChange )
	{
		InvalidateLayout( true, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::RepopulateItemsForPaintkit( const CEconItemView* pItem )
{
	const CPaintKitDefinition* pPaintKitDef = NULL;
	uint32 unPaintKitIndex = 0;
	if ( pItem && GetPaintKitDefIndex( pItem, &unPaintKitIndex ) )
	{
		pPaintKitDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CPaintKitDefinition >( unPaintKitIndex  );
	}

	int nActiveRow = 0;
	bool bNewList = false;

	if ( !pPaintKitDef )
	{
		if ( pItem )
		{
			bNewList = true;
			m_pComboBoxValidItems->RemoveAll();

			// No paintkit.  Just show the current item.
			KeyValuesAD pDataKV( "data" );
			pDataKV->SetInt( "defindex", pItem->GetItemDefinition()->GetRemappedItemDefIndex() );
			m_pComboBoxValidItems->AddItem( pItem->GetItemDefinition()->GetItemBaseName(), pDataKV );
		}
	}
	else if ( m_nValidItemsPaintkitDefindex != pPaintKitDef->GetDefIndex() )
	{
		bNewList = true;
		m_pComboBoxValidItems->RemoveAll();

		CUtlVector< const CEconItemDefinition* > vecSortedDefs;

		CUtlVector< item_definition_index_t > vecSupportedItems;

		if ( m_bFixedPaintkit || ( !m_bFixedPaintkit && !m_bFixedItem ) )
		{
			// Add the paintkit in free and fixed modes
			pPaintKitDef->GetItemsThatCanRenderThisPaintkit( &vecSupportedItems );
		}
		else
		{
			// Add all the items supported by the paintkit
			pPaintKitDef->GetSupportedItems( &vecSupportedItems );
		}
	

		FOR_EACH_VEC( vecSupportedItems, i )
		{
			item_definition_index_t unItemDefIndex = vecSupportedItems[i];

			CTFItemDefinition *pItemDef = ItemSystem()->GetStaticDataForItemByDefIndex( unItemDefIndex );
			Assert( pItemDef );
			if ( pItemDef )
			{
				static CSchemaItemDefHandle spPaintkitItemDef( "Paintkit" );
				bool bItemDefIsPaintkit = pItemDef->GetRemappedItemDefIndex() == spPaintkitItemDef->GetDefinitionIndex();
				if ( !m_bConsumeMode || !bItemDefIsPaintkit )
				{
					vecSortedDefs.AddToTail( pItemDef );
				}
			}
		}

		vecSortedDefs.SortPredicate( []( const CEconItemDefinition* const &pA,  const CEconItemDefinition* const &pB )
		{
			const wchar_t* pwszA = g_pVGuiLocalize->Find( pA->GetItemBaseName() );
			const wchar_t* pwszB = g_pVGuiLocalize->Find( pB->GetItemBaseName() );

			if ( !pwszA )
				return true;

			if ( !pwszB )
				return false;

			int nDif = wcsicmp( pwszA, pwszB );

			if ( nDif == 0 )
				return false;

			return nDif < 0;
		} );

		FOR_EACH_VEC( vecSortedDefs, i )
		{
			const CEconItemDefinition *pItemDef = vecSortedDefs[ i ];
			auto defindex = pItemDef->GetRemappedItemDefIndex();

			KeyValuesAD pDataKV( "data" );
			pDataKV->SetInt( "defindex", defindex );

			if ( defindex == m_pItemViewData->GetItemDefinition()->GetRemappedItemDefIndex() )
			{
				nActiveRow = i;
			}

			m_pComboBoxValidItems->AddItem( pItemDef->GetItemBaseName(), pDataKV );
		}

		m_nValidItemsPaintkitDefindex = pPaintKitDef->GetDefIndex();
	}

	if ( bNewList )
	{
		m_pComboBoxValidItems->SetNumberOfEditLines( Min( m_pComboBoxValidItems->GetItemCount(), 10 ) );
		m_pComboBoxValidItems->SilentActivateItemByRow( nActiveRow );
		m_pComboBoxValidItems->SetEnabled( m_pComboBoxValidItems->GetItemCount() > 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::RepopulatePaintKitsForItem( const CEconItemView* pItem )
{
	if ( !pItem )
		return;

	if ( pItem->GetItemDefinition()->GetRemappedItemDefIndex() == m_nValidPaintkitsItemDefindex )
		return;

	auto pItemDef = pItem->GetItemDefinition();
	Assert( pItemDef );
	if ( !pItemDef )
		return;

	// When we dont have a fixed item or fixed paintkit, show ALL paintkits, even the ones
	// that don't support the item we're viewing
	bool bAllPaintkits = !m_bFixedItem && !m_bFixedPaintkit;

	const CPaintKitDefinition* pAppliedPaintKitDef = NULL;
	uint32 unPaintKitIndex = 0;
	if ( pItem && GetPaintKitDefIndex( pItem, &unPaintKitIndex ) )
	{
		pAppliedPaintKitDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CPaintKitDefinition >( unPaintKitIndex  );
	}

	m_pComboBoxValidPaintkits->RemoveAll();
	int nActiveRow = 0;

	// Add a "no paintkit" option
	{
		KeyValuesAD pDataKV( "data" );
		pDataKV->SetInt( "defindex", -1 );
		m_pComboBoxValidPaintkits->AddItem( g_pVGuiLocalize->Find( "#Store_ItemDesc_Slot_None" ), pDataKV );
	}

	uint nRow = 1;

	// Comboboxes dont have a sort, so we'll first fill a vector and sort that, then add its contents to the combobox
	CUtlVector< const CPaintKitDefinition* > vecSortedDefs;

	if ( bAllPaintkits )
	{
		// If we're in all paintkit mode, just show everything
		auto& mapPaintkitsDefs = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_PAINTKIT_DEFINITION );

		FOR_EACH_MAP_FAST( mapPaintkitsDefs, i )
		{
			const CPaintKitDefinition* pPaintkitDef = (const CPaintKitDefinition*)mapPaintkitsDefs[ i ];
			vecSortedDefs.AddToTail( pPaintkitDef );
		}
	}
	else if ( IsPaintKitTool( pItemDef ) )
	{
		// If they're previewing the paintkit item, we'll just add every paintkit that knows how to 
		// render onto the paintkit
		auto& mapPaintkitsDefs = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_PAINTKIT_DEFINITION );

		FOR_EACH_MAP_FAST( mapPaintkitsDefs, i )
		{
			const CPaintKitDefinition* pPaintkitDef = (const CPaintKitDefinition*)mapPaintkitsDefs[ i ];
			CUtlVector< item_definition_index_t > vecItems;
			pPaintkitDef->GetItemsThatCanRenderThisPaintkit( &vecItems );
			if ( vecItems.Find( pItemDef->GetRemappedItemDefIndex() ) != vecItems.InvalidIndex() )
			{
				vecSortedDefs.AddToTail( pPaintkitDef );
			}
		}
	}
	else
	{
		// If the item isn't the paintkit item, then show all of the paintkits that can target this item
		auto& vecPaintkitDefs = pItemDef->GetValidPaintkits();

		FOR_EACH_VEC( vecPaintkitDefs, i )
		{
			const CPaintKitDefinition* pPaintkitDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CPaintKitDefinition >( vecPaintkitDefs[ i ] );
			Assert( pPaintkitDef );
			if ( !pPaintkitDef )
				continue;

			if ( pItemDef->GetValidPaintkits().Find( pPaintkitDef->GetDefIndex() ) != CUtlVector< int >::InvalidIndex() )
			{
				vecSortedDefs.AddToTail( pPaintkitDef );
			}
		}
	}

	vecSortedDefs.SortPredicate( []( const CPaintKitDefinition* const &pA,  const CPaintKitDefinition* const &pB )
	{
		const wchar_t* pwszA = g_pVGuiLocalize->Find( pA->GetDescriptionToken() );
		const wchar_t* pwszB = g_pVGuiLocalize->Find( pB->GetDescriptionToken() );

		if ( !pwszA )
			return false;

		if ( !pwszB )
			return true;

		int nDif = wcsicmp( pwszA, pwszB );

		if ( nDif == 0 )
			return false;

		return nDif < 0;

	} );

	FOR_EACH_VEC( vecSortedDefs, i )
	{
		const CPaintKitDefinition* pPaintkitDef = vecSortedDefs[ i ];

		if ( pPaintkitDef == pAppliedPaintKitDef )
		{
			nActiveRow = nRow;
		}

		// Add the paintkit
		KeyValuesAD pDataKV( "data" );
		pDataKV->SetInt( "defindex", pPaintkitDef->GetDefIndex() );
		wchar_t* pwszPaintkitName = g_pVGuiLocalize->Find( pPaintkitDef->GetDescriptionToken() );
		if ( pwszPaintkitName )
		{
			m_pComboBoxValidPaintkits->AddItem( pwszPaintkitName, pDataKV );
		}
		else
		{
			m_pComboBoxValidPaintkits->AddItem( CFmtStr( "[NYI] %s", pPaintkitDef->GetName() ).Get(), pDataKV );
		}

		++nRow;
	}

	m_pComboBoxValidPaintkits->SetNumberOfEditLines( Min( m_pComboBoxValidPaintkits->GetItemCount(), 10 ) );
	m_pComboBoxValidPaintkits->SilentActivateItemByRow( nActiveRow );
	m_pComboBoxValidPaintkits->SetEnabled( true );

	// Update which source th paintkits came from
	m_nValidPaintkitsItemDefindex = pItem->GetItemDefinition()->GetRemappedItemDefIndex();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::OnRadioButtonChecked( vgui::Panel *panel )
{
	OnCommand( panel->GetName() );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::OnNavButtonSelected( KeyValues *pData )
{
	const int iTeam = pData->GetInt( "userdata", -1 );	AssertMsg( iTeam >= 0, "Bad filter" );
	if ( iTeam < 0 )
		return;

	CEconItemView* pItem = m_pModelInspectPanel->GetItem();
	if ( pItem )
	{
		pItem->SetTeamNumber( iTeam );

		RecompositeItem();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::RecompositeItem()
{
	// Force a reload of the skin
	CEconItemView* pItem = m_pModelInspectPanel->GetItem();
	if ( pItem )
	{
		{
			pItem->CancelWeaponSkinComposite();
			pItem->SetWeaponSkinBase( NULL );
			pItem->SetWeaponSkinBaseCompositor( NULL );
		}
	}
}

void FindAndAddAttributeTo( CEconItemView *pSrc, CEconItemView *pDest, const CEconItemAttributeDefinition *pAttr, attrib_value_t defaultVal, bool bAlwaysAddAttr )
{
	attrib_value_t val = defaultVal;
	if ( !pSrc->FindAttribute( pAttr, &val ) && !bAlwaysAddAttr )
	{
		return;
	}
	
	CEconItemAttribute attribute( pAttr->GetDefinitionIndex(), val );
	pDest->GetAttributeList()->AddAttribute( &attribute );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::UpdateItemFromControls()
{
	if ( !m_pItemViewData )
		return;

	// Itemdef
	KeyValues *pItemUserData = m_pComboBoxValidItems->GetActiveItemUserData();
	if ( !pItemUserData )
		return;

	item_definition_index_t unDefIndex = pItemUserData->GetInt( "defindex", INVALID_ITEM_DEF_INDEX );

	// Paintkti def
	KeyValues* pPaintkitUserData = m_pComboBoxValidPaintkits->GetActiveItemUserData();
	if ( !pPaintkitUserData )
		return;

	uint32 nPaintkitDef = pPaintkitUserData->GetInt( "defindex", 0 );

	CEconItemView tempItem;
	tempItem.Init( unDefIndex, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );

	// Wear seed
	char szBuff[ 256 ];
	m_pSeedTextEntry->GetText( szBuff, sizeof( szBuff ) );

	uint64 nSeed = 0;
	sscanf( szBuff, "%llu", &nSeed );

	// copy from source item
	static CSchemaAttributeDefHandle pAttrib_WeaponAllowInspect( "weapon_allow_inspect" );
	static CSchemaAttributeDefHandle pAttr_Particle( "attach particle effect" );
	FindAndAddAttributeTo( m_pItemViewData, &tempItem, pAttrib_WeaponAllowInspect, 1, true );
	FindAndAddAttributeTo( m_pItemViewData, &tempItem, pAttr_Particle, 0, false );

	// copy strange
	if ( m_pItemViewData->GetQuality() == AE_STRANGE || m_pItemViewData->GetItemQuality() == AE_STRANGE )
	{
		tempItem.SetItemQuality( AE_STRANGE );
	}
	else
	{
		// Go over the attributes of the item, if it has any strange attributes the item is strange and don't apply
		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			FindAndAddAttributeTo( m_pItemViewData, &tempItem, GetKillEaterAttr_Score( i ), 0, false );
		}
	}

	// UI custom attrs
	static CSchemaAttributeDefHandle pAttrDef_PaintKitProtoDefIndex( "paintkit_proto_def_index" );
	static CSchemaAttributeDefHandle pAttrDef_PaintKitWear( "set_item_texture_wear" );
	static CSchemaAttributeDefHandle pAttr_CustomPaintKitSeedLo( "custom_paintkit_seed_lo" );
	static CSchemaAttributeDefHandle pAttr_CustomPaintKitSeedHi( "custom_paintkit_seed_hi" );

	CEconItemAttribute attrPaintKitDef( pAttrDef_PaintKitProtoDefIndex->GetDefinitionIndex(), nPaintkitDef );
	tempItem.GetAttributeList()->AddAttribute( &attrPaintKitDef );
	CEconItemAttribute attrWear( pAttrDef_PaintKitWear->GetDefinitionIndex(), GetSliderWear() );
	tempItem.GetAttributeList()->AddAttribute( &attrWear );
	CEconItemAttribute attrSeedLo( pAttr_CustomPaintKitSeedLo->GetDefinitionIndex(), (uint32)( nSeed ) );
	tempItem.GetAttributeList()->AddAttribute( &attrSeedLo );
	CEconItemAttribute attrSeedHi( pAttr_CustomPaintKitSeedHi->GetDefinitionIndex(), (uint32)( nSeed >> 32 ) );
	tempItem.GetAttributeList()->AddAttribute( &attrSeedHi );

	bool bReset = tempItem.GetItemDefinition()->GetRemappedItemDefIndex() != m_pItemViewData->GetItemDefinition()->GetRemappedItemDefIndex();

	SetItemCopy( &tempItem, bReset );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemInspectionPanel::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );

	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );
	if ( pComboBox )
	{
		if ( pComboBox == m_pComboBoxValidItems )
		{
			UpdateItemFromControls();
		}
		else if ( pComboBox == m_pComboBoxValidPaintkits )
		{
			UpdateItemFromControls();
		}
	}

	vgui::TextEntry* pTextEntry = dynamic_cast< vgui::TextEntry *>( pPanel );
	if ( pTextEntry == m_pSeedTextEntry )
	{
		UpdateItemFromControls();
	}
}

void CTFItemInspectionPanel::OnSliderMoved( KeyValues *data )
{
	UpdateItemFromControls();
}

float CTFItemInspectionPanel::GetSliderWear() const
{
	return ( 6 - m_pWearSlider->GetValue() ) / 5.f;
}