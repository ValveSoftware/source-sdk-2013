//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "c_tf_freeaccount.h"

#include "gcsdk/sharedobjectcache.h"
#include "tf_gcmessages.h"
#include "econ_game_account_client.h"
#include "tf_item_inventory.h"
#include "tf_player_info.h"

#include <vgui/ILocalize.h>
#include "confirm_dialog.h"
#include "econ/econ_notifications.h"
#include "select_player_dialog.h"

#include "gc_clientsystem.h"

#ifdef _DEBUG
CON_COMMAND( cl_free_trial_select_friend, "Bring up dialog to select most helpful friend" )
{
	OpenSelectMostHelpfulFriendDialog( NULL );
}

CON_COMMAND( cl_thanks_test, "Tests the thanked ui notification." )
{
	if ( steamapicontext == NULL || steamapicontext->SteamUser() == NULL )
		return;

	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
	NotificationQueue_Add( new CWasThankedBySomeoneNotification( steamID ) );
}
#endif

//-----------------------------------------------------------------------------
// External API

#if _DEBUG
ConVar tf_forcetrialaccount( "tf_forcetrialaccount", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
#endif


bool IsFreeTrialAccount()
{
#if _DEBUG
	if ( tf_forcetrialaccount.GetBool() )
		return true;
#endif

	if ( InventoryManager() && TFInventoryManager()->GetLocalTFInventory() && TFInventoryManager()->GetLocalTFInventory()->GetSOC() )
	{
		CEconGameAccountClient *pGameAccountClient = TFInventoryManager()->GetLocalTFInventory()->GetSOC()->GetSingleton<CEconGameAccountClient>();
		if ( pGameAccountClient )
			return pGameAccountClient->Obj().trial_account();
	}
	return false;
}

bool NeedsToChooseMostHelpfulFriend()
{
	{
		if ( InventoryManager() && TFInventoryManager()->GetLocalTFInventory() && TFInventoryManager()->GetLocalTFInventory()->GetSOC() )
		{
			CEconGameAccountClient *pGameAccountClient = TFInventoryManager()->GetLocalTFInventory()->GetSOC()->GetSingleton<CEconGameAccountClient>();
			if ( pGameAccountClient )
			{
				return !pGameAccountClient->Obj().trial_account()
					&& pGameAccountClient->Obj().need_to_choose_most_helpful_friend();
			}
		}
	}
	return false;
}

