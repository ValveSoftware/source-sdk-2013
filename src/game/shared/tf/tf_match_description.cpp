//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_match_description.h"
#include "tf_match_description_shared.h"
#include "tf_ladder_data.h"
#include "tf_rating_data.h"


#if defined CLIENT_DLL || defined GAME_DLL
#include "tf_gamerules.h"
#endif


static const IMatchGroupDescription* s_arMatchDesc[ ETFMatchGroup_ARRAYSIZE ] = { NULL };
void RegisterMatchDesc( const ETFMatchGroup& eGroup, const IMatchGroupDescription* pDesc )
{
	Assert( s_arMatchDesc[ eGroup ] == NULL );
	s_arMatchDesc[ eGroup ] = pDesc;
}

const IMatchGroupDescription* GetMatchGroupDescription( const ETFMatchGroup& eGroup )
{
	if ( eGroup == k_eTFMatchGroup_Invalid )
		return NULL;
	if ( eGroup < 0 || (unsigned int)eGroup >= V_ARRAYSIZE( s_arMatchDesc ) )
	{
		AssertMsg( false, "Bogus matchgroup passed to GetMatchGroupDescription" );
		return NULL;
	}

	const IMatchGroupDescription* pMatchDesc = s_arMatchDesc[ eGroup ];
	if ( pMatchDesc )
		return pMatchDesc;

	return NULL;
}


IMatchGroupDescription::IMatchGroupDescription( ETFMatchGroup eMatchGroup )
	: m_eMatchGroup( eMatchGroup )
	, m_pProgressionDesc( NULL )
{}


#ifdef CLIENT_DLL
bool IMatchGroupDescription::BPlayerIsInPlacement( CSteamID steamID ) const
{
	return GetNumPlacementMatchesToGo( steamID ) != 0;
}

bool IMatchGroupDescription::BLocalPlayerIsInPlacement() const
{
	return BPlayerIsInPlacement( ClientSteamContext().GetLocalPlayerSteamID() );
}

int IMatchGroupDescription::GetNumPlacementMatchesToGo( CSteamID steamID ) const
{
	if ( BUsesPlacementMatches() )
	{
		const CTFRatingData* pRatingData = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( steamID, GetCurrentDisplayRank() );

		// No data?  You're definitely in placement
		if ( !pRatingData )
		{
			return GetNumWinsToExitPlacement();
		}

		// Real ranks start at 1.  If your rank (which is stored in primary rating) is 0, then that's our code to know
		// that you're still in placement.  If that's true, then secondary contains your number of wins.
		if ( pRatingData->GetRatingData().unRatingPrimary == k_nPrimaryFieldPlacementValue )
		{
			return GetNumWinsToExitPlacement() - pRatingData->GetRatingData().unRatingSecondary;
		}
	}

	return 0;
}

void IMatchGroupDescription::SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const
{
	if ( m_pProgressionDesc )
	{
		m_pProgressionDesc->SetupBadgePanel( pModelPanel, level, steamID, BUsesPlacementMatches() && bInPlacement );
	}
}
#endif

#ifdef GAME_DLL
bool IMatchGroupDescription::InitServerSettingsForMatch( const CTFGSLobby* pLobby ) const
{
	extern bool IsCustomGameMode( const char *pszMapName );
	if ( IsCustomGameMode( pLobby->GetMapName() ) )
	{
		// HACK(misyl): Force a custom cfg for custom game modes.
		servercfgfile.SetValue( "server_custom.cfg" );
		lservercfgfile.SetValue( "server_custom.cfg" );
	}
	else
	{
		// Setting servercfgfile to our mode-specific config causes the server to exec it once it finishes
		// loading the map from the changelevel below
		servercfgfile.SetValue( m_pszExecFileName );
		lservercfgfile.SetValue( m_pszExecFileName );
	}

	return TFGameRules()->StartManagedMatch();
}
#endif
