#include "cbase.h"
#include "tf_leaderboardpanel.h"
#include "econ_controls.h"
#include "tf_asyncpanel.h"
#include "tf_mapinfo.h"
#include "vgui_avatarimage.h"
#include "tf_item_inventory.h"


CTFLeaderboardPanel::CTFLeaderboardPanel( Panel *pParent, const char *pszPanelName )
	: CBaseASyncPanel( pParent, pszPanelName )
{}

//-----------------------------------------------------------------------------
// Purpose: Create leaderboard panels
//-----------------------------------------------------------------------------
void CTFLeaderboardPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/LeaderboardPanel.res" );
}

void CTFLeaderboardPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	m_EvenTextColor = GetSchemeColor( inResourceData->GetString( "EvenTextColor" ), pScheme);
	m_OddTextColor = GetSchemeColor( inResourceData->GetString( "OddTextColor" ), pScheme);
	m_LocalPlayerTextColor = GetSchemeColor( inResourceData->GetString( "LocalPlayerTextColor" ), pScheme);

	EditablePanel *pScoresContainer = dynamic_cast< EditablePanel* >( FindChildByName( "ScoresContainer", true ) );

	if ( pScoresContainer )
	{
		m_vecLeaderboardEntries.Purge();
		for ( int i = 0; i < 7; ++ i )
		{
			vgui::EditablePanel *pEntryUI = new vgui::EditablePanel( pScoresContainer, "LeaderboardEntry" );
			pEntryUI->ApplySchemeSettings( pScheme );
			pEntryUI->LoadControlSettings( "Resource/UI/LeaderboardSpreadEntry.res" );
			m_vecLeaderboardEntries.AddToTail( pEntryUI );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check for leaderboard data
//-----------------------------------------------------------------------------
bool CTFLeaderboardPanel::CheckForData_Internal()
{
	return UpdateLeaderboards();
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we have friends leaderboard data downloaded.  If so, sets
//			the data into the panels
//-----------------------------------------------------------------------------
bool CTFLeaderboardPanel::UpdateLeaderboards()
{
	CUtlVector< LeaderboardEntry_t* > scores;
	if ( !GetLeaderboardData( scores ) )
		return false;
	
	int x=0,y=0;
	FOR_EACH_VEC( m_vecLeaderboardEntries, i )
	{
		Color colorToUse = i % 2 == 1 ? m_OddTextColor : m_EvenTextColor;
		EditablePanel *pContainer = dynamic_cast< EditablePanel* >( m_vecLeaderboardEntries[i] );
		if ( pContainer )
		{
			bool bIsEntryVisible = i < scores.Count();
			pContainer->SetVisible( bIsEntryVisible );
			pContainer->SetPos( x, y );
			y += m_yEntryStep;
			if ( bIsEntryVisible )
			{
				const LeaderboardEntry_t* leaderboardEntry = scores[i];
				const CSteamID &steamID = leaderboardEntry->m_steamIDUser;
				bool bIsLocalPlayer = steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUser()->GetSteamID() == steamID;
				pContainer->SetDialogVariable( "rank", leaderboardEntry->m_nGlobalRank );
				pContainer->SetDialogVariable( "username", InventoryManager()->PersonaName_Get( steamID.GetAccountID() ) );
				pContainer->SetDialogVariable( "score", leaderboardEntry->m_nScore );
				
				CExLabel *pText = dynamic_cast< CExLabel* >( pContainer->FindChildByName( "UserName" ) );
				if ( pText )
				{			
					pText->SetColorStr( bIsLocalPlayer ? m_LocalPlayerTextColor : colorToUse );
				}

				CAvatarImagePanel *pAvatar = dynamic_cast< CAvatarImagePanel* >( pContainer->FindChildByName( "AvatarImage" ) );
				if ( pAvatar )
				{
					pAvatar->SetShouldDrawFriendIcon( false );
					pAvatar->SetPlayer( steamID, k_EAvatarSize32x32 );
				}
			}								
		}			
	}

	return true;
}
