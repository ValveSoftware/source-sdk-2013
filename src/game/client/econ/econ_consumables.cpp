//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "econ_item_tools.h"

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
const char *IEconTool::GetUseCommandLocalizationToken( const IEconItemInterface *pItem, int i ) const
{
	Assert( i == 0 ); // Default only has 1 use, so this should be 0.
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	// If we have a custom schema-specified use string, use that.
	return GetUseString();
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
const char* IEconTool::GetUseCommand( const IEconItemInterface *pItem, int i ) const
{
	Assert( i == 0 ); // Default only has 1 use, so this should be 0.
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	const bool bIsGCConsumable = ( ( pItem->GetItemDefinition()->GetCapabilities() & ITEM_CAP_USABLE_GC ) != 0 );
	return bIsGCConsumable ? "Context_UseConsumableItem" : "Context_ApplyOnItem";
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
bool IsLocalPlayerWrappedGift( const IEconItemInterface *pItem )
{
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetTypedEconTool<CEconTool_WrappedGift>() );

	static CSchemaAttributeDefHandle pAttr_GifterAccountID( "gifter account id" );

	uint32 unGifterAccountID;
	if ( !pItem->FindAttribute( pAttr_GifterAccountID, &unGifterAccountID ) )
		return false;

	const uint32 unLocalAccountID = steamapicontext->SteamUser()->GetSteamID().GetAccountID();

	return unGifterAccountID == unLocalAccountID;
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
bool CEconTool_WrappedGift::CanBeUsedNow( const IEconItemInterface *pItem ) const
{
	static CSchemaItemDefHandle pItemDef_WrappedGiftapultPackage( "Wrapped Giftapult Package" );
	static CSchemaItemDefHandle pItemDef_DeliveredGiftapultPackage( "Delivered Giftapult Package" );
	static CSchemaItemDefHandle pItemDef_CompetitiveBetaPassGift( "Competitive Matchmaking Beta Giftable Invite" );

	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	if ( ( pItem->GetItemDefinition() == pItemDef_WrappedGiftapultPackage ) ||
	     ( pItem->GetItemDefinition() == pItemDef_CompetitiveBetaPassGift ) ||
	     ( pItem->GetItemDefinition() == pItemDef_DeliveredGiftapultPackage ) )
		return true;

	return pItem->IsTradable();
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
bool CEconTool_WrappedGift::ShouldShowContainedItemPanel( const IEconItemInterface *pItem ) const
{
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	return IsLocalPlayerWrappedGift( pItem );
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
const char *CEconTool_WrappedGift::GetUseCommandLocalizationToken( const IEconItemInterface *pItem, int i ) const
{
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	Assert( i == 0 || ( IsLocalPlayerWrappedGift( pItem ) && i == 1 ) );

	// NOTE! Keep in sync with CEconTool_WrappedGift::GetUseCommand
	if ( BIsDirectGift() ||
	     ( IsLocalPlayerWrappedGift( pItem ) && i == 0 ) )
		return "#DeliverGift";
	return "#UnwrapGift";
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
int CEconTool_WrappedGift::GetUseCommandCount( const IEconItemInterface *pItem ) const
{
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	if ( IsLocalPlayerWrappedGift( pItem ) )
		return 2;
	return 1;
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
const char* CEconTool_WrappedGift::GetUseCommand( const IEconItemInterface *pItem, int i ) const
{
	// NOTE! Keep in sync with CEconTool_WrappedGift::GetUseCommandLocalizationToken
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	Assert( i == 0 || ( IsLocalPlayerWrappedGift( pItem ) && i == 1 ) );

	// NOTE! Keep in sync with CEconTool_WrappedGift::GetUseCommand
	if ( BIsDirectGift() ||
	     ( IsLocalPlayerWrappedGift( pItem ) && i == 0 ) )
		return "Context_DeliverItem";
	return "Context_UnwrapItem";
}

//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
const char *CEconTool_WeddingRing::GetUseCommandLocalizationToken( const IEconItemInterface *pItem, int i ) const
{
	Assert( i == 0 ); // We only have one action.
	Assert( pItem );
	Assert( pItem->GetItemDefinition() );
	Assert( pItem->GetItemDefinition()->GetEconTool() == this );

	// If the wedding ring has been gifted to us, we can use it to accept/reject the proposal.
	// If it hasn't been gifted we can't use it at all.
	static CSchemaAttributeDefHandle pAttrDef_GifterAccountID( "gifter account id" );

	if ( !pItem->FindAttribute( pAttrDef_GifterAccountID ) )
		return NULL;

	return "#ToolAction_WeddingRing_AcceptReject";
}

#ifndef TF_CLIENT_DLL
//---------------------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------------------
void CEconTool_Noisemaker::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_Noisemaker::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_WrappedGift::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_WrappedGift::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_WeddingRing::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_WeddingRing::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_BackpackExpander::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_BackpackExpander::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_AccountUpgradeToPremium::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_AccountUpgradeToPremium::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_ClaimCode::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_ClaimCode::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_Collection::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_Collection::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_StrangifierBase::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_StrangifierBase::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_PaintCan::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_PaintCan::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_Gift::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_Gift::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_DuelingMinigame::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_DuelingMinigame::OnClientUseConsumable() is unimplemented!" );
}

//-----------------------------------------------------------------------------
void CEconTool_DuckToken::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_DuckToken::OnClientUseConsumable() is unimplemented!" );
}
//-----------------------------------------------------------------------------
void CEconTool_GrantOperationPass::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_GrantOperationPass::OnClientUseConsumable() is unimplemented!" );
}
//-----------------------------------------------------------------------------
void CEconTool_KeylessCase::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_KeylessCase::OnClientUseConsumable() is unimplemented!" );
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconTool_Default::OnClientUseConsumable( CEconItemView *pItem, vgui::Panel *pParent ) const
{
	Assert( !"CEconTool_Default::OnClientUseConsumable() is unimplemented!" );
}

#endif // !defined( TF_CLIENT_DLL )
