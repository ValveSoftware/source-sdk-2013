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
#include "vgui_int.h"
#include "cdll_client_int.h"
#include "clientmode_tf.h"
#include "ienginevgui.h"
#include "tf_hud_mainmenuoverride.h"
#include "econ_item_system.h"
#include "vgui_controls/ComboBox.h"
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
{
	m_pModelInspectPanel = new CEmbeddedItemModelPanel( this, "ModelInspectionPanel" );
	m_pItemNamePanel = new CItemModelPanel( this, "ItemName" );
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

	if ( m_bConsumeMode )
	{
		pkvConditions->AddSubKey( new KeyValues( "consume_mode" ) );
	}

	LoadControlSettings( pszFileName, nullptr, nullptr, pkvConditions );

	BaseClass::ApplySchemeSettings( pScheme );

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
void CTFItemInspectionPanel::SetItem( CEconItemView *pItem, bool bReset )
{
	PostActionSignal( new KeyValues( "ItemSelected", "defindex", pItem ? pItem->GetItemDefIndex() : INVALID_ITEM_DEF_INDEX ) );

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

	uint32 unPaintKitIndex = 0;

	RecompositeItem();
	SetControlVisible( "ShowPreviewControlsButton", false, true );
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
	bChange = bChange || ( m_bConsumeMode != bConsumptionMode );
	
	m_bFixedItem = bFixedItem;
	m_bConsumeMode = bConsumptionMode;

	if ( bChange )
	{
		InvalidateLayout( true, true );
	}
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