#include "cbase.h"
#include "mk_topmenu.h"
#include "mk_topmenu_builder.h"
#include "hl2mp_player.h"

ConVar mk_menu_next_btn( "mk_menu_next_btn", "> Next", FCVAR_NONE );
ConVar mk_menu_prev_btn( "mk_menu_prev_btn", "< Previous", FCVAR_NONE );
ConVar mk_menu_pagination_fmt( "mk_menu_pagination_fmt", "[ Page %d of %d ]", FCVAR_NONE );

constexpr auto MENU_CONTROL_BTN_HANDLER = "menu_control_btn_handler";
constexpr auto MENU_NEXT_PAGE_TOKEN = -1;
constexpr auto MENU_PREV_PAGE_TOKEN = -2;
constexpr auto MAX_COMMAND_LENGTH = 512;

void CMenuBuilder::CreateMenu( const char* menu_name, const char* title, const char* msg, Color color, const int hold_time )
{
	UpdateMenuExpiry();

	menuName = menu_name;

	MenuEntry* m = &menuTemplate;

	m->items.Purge();

	m->title = title;
	m->description = msg;
	m->textColor = color;
	m->holdTime = hold_time;
	m->endTime = gpGlobals->curtime + (float)hold_time;
	m->selectedIndex = 0;
}

bool CMenuBuilder::AddMenuOption( const char* caption, const char* command )
{
	MenuItem item;
	item.title = caption;
	item.title.Trim();
	item.command = command;
	item.command.Trim();

	for ( auto it = menuTemplate.items.begin(); it != menuTemplate.items.end(); ++it )
	{
		if ( item.title.IsEqual_CaseInsensitive( (*it).title ) )
			return false;
	}

	menuTemplate.items.AddToTail( item );
	return true;
}

void CMenuBuilder::UpdateMenuExpiry()
{
	for ( auto& entry : menuArray )
	{
		if ( entry.endTime < gpGlobals->curtime )
		{
			entry = MenuEntry();
		}
	}
};

void CMenuBuilder::SendMenuToPlayer( const int clientIndex )
{
	if ( IsValidClientIndex( clientIndex ) == false )
		return;
	
	menuArray[clientIndex] = menuTemplate;
	RenderMenuForClient( clientIndex );
}

void CMenuBuilder::RenderMenuForClient( const int clientIndex )
{
	if ( !IsValidClientIndex( clientIndex ) )
		return;

	CBasePlayer* pPlayer = UTIL_PlayerByIndex( clientIndex + 1 );
	if ( pPlayer == NULL || pPlayer->IsBot() || pPlayer->IsFakeClient() || !pPlayer->IsConnected() )
		return;

	MenuEntry& entry = menuArray[clientIndex];

	const int maxItemsPerPage = GetMaxItemsPerPage( clientIndex );
	const int startIdx = entry.selectedIndex;
	const int endIdx = min( startIdx + maxItemsPerPage, entry.items.Count() );

	CUtlString description;

	if ( ShouldUsePagination( clientIndex ) )
	{
		int curPage = (startIdx / maxItemsPerPage) + 1;
		int totalPages = (entry.items.Count() + maxItemsPerPage - 1) / maxItemsPerPage;

		description.Format( 
			"%s\n%s",
			CFmtStr( mk_menu_pagination_fmt.GetString(), curPage, totalPages ).Get(),
			entry.description.Get()
		);
	}
	else
	{
		description = entry.description;
	}

	CPluginMenu* menu = GetPluginMenu()->CreateMenu( entry.title, description, entry.textColor, entry.holdTime );

	if ( alwaysOnTop )
	{
		menu->BringToTop();
	}

	for ( int i = startIdx; i < endIdx; ++i )
	{
		CUtlString cmd;
		cmd.Format( "%s \"%s\"", MENU_CONTROL_BTN_HANDLER, entry.items[i].command.Get() );
		menu->AddMenuOption( entry.items[i].title, cmd.Get() );
	}

	if ( ShouldUsePagination( clientIndex ) )
	{
		auto AddButton = [&]( const char* text, int token ) {
			CUtlString cmd;
			cmd.Format( "%s %d %s", MENU_CONTROL_BTN_HANDLER, token, menuName.Get() );
			menu->AddMenuOption( text, cmd );
			};

		AddButton( mk_menu_next_btn.GetString(), MENU_NEXT_PAGE_TOKEN );
		AddButton( mk_menu_prev_btn.GetString(), MENU_PREV_PAGE_TOKEN );
	}
	
	menu->SendMenuToPlayer( pPlayer );
	delete menu;
}

void CMenuBuilder::BroadcastMenuToAllPlayers( void )
{
	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		SendMenuToPlayer( i );
	}
}

void CMenuBuilder::NavigatePage( int clientIndex, int direction )
{
	if ( !IsValidClientIndex( clientIndex ) ) return;

	int maxItems = menuArray[clientIndex].items.Count();
	int maxPerPage = GetMaxItemsPerPage( clientIndex );

	menuArray[clientIndex].selectedIndex += direction * maxPerPage;
	if ( menuArray[clientIndex].selectedIndex >= maxItems ) {
		menuArray[clientIndex].selectedIndex = 0;
	}
	else if ( menuArray[clientIndex].selectedIndex < 0 ) {
		menuArray[clientIndex].selectedIndex = max( 0, maxItems - (maxItems % maxPerPage) );
	}

	RenderMenuForClient( clientIndex );
}

void CMenuBuilder::NavigateToNextPage( const int clientIndex )
{
	NavigatePage( clientIndex, 1 );
}

void CMenuBuilder::NavigateToPreviousPage( const int clientIndex )
{
	NavigatePage( clientIndex, -1 );
}

static void cc_menu_control_btn_handler( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		UTIL_LogPrintf( "[menu_command_handler] Invalid command usage. Expected at least 2 arguments.\n" );
		return;
	}

	CBasePlayer* pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	int clientIndex = pPlayer->GetClientIndex();
	int navigate = atoi( args[1] );
	const char* menu_name = args[2];

	if ( navigate == MENU_NEXT_PAGE_TOKEN )
	{
		GetTopMenuManager()->NavigateToNextPage( menu_name, clientIndex );
		return;
	}
	
	if ( navigate == MENU_PREV_PAGE_TOKEN )
	{
		GetTopMenuManager()->NavigateToPreviousPage( menu_name, clientIndex );
		return;
	}

	CUtlString cmd;
	for ( int i = 1; i < args.ArgC(); i++ )
	{
		if ( i > 1 )
			cmd.Append( " " );
		cmd.Append( args[i] );
	}

	UTIL_FakePlayerCommand( pPlayer->edict(), cmd.Get() );
}

static ConCommand menu_control_btn_handler( "menu_control_btn_handler", cc_menu_control_btn_handler, "", FCVAR_HIDDEN );

#ifdef DEBUG

//
// Usage example:
//

void cc_mk_playermodel( const CCommand& args )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	if ( args.ArgC() > 1 )
	{
		CHL2MP_Player* plr = ToHL2MPPlayer( pPlayer );
		if ( plr )
		{
			CBaseEntity::PrecacheModel( args[1] );

			plr->SetModel( args[1] );

			if ( args.ArgC() > 2 )
			{
				plr->m_nSkin = atoi( args[2] );
			}
		}
	}
}
static ConCommand mk_playermodel( "mk_playermodel", cc_mk_playermodel, "", FCVAR_HIDDEN );

void cc_test_menu( const CCommand& args )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	CMenuBuilder *mb = GetTopMenuManager()->CreateMenu( "test", "Title", "Description", COLOR_MK_RED, 100,
		21,
		"Play", "start_game",
		"Settings", "open_settings",
		"Quit", "exit_game",
		"1", "mk_playermodel \"models/combine_soldier.mdl\" 0",
		"2", "mk_playermodel \"models/combine_soldier.mdl\" 1",
		"3", "mk_playermodel \"models/humans/group02/female_03.mdl\"",
		"4", "mk_playermodel \"models/humans/group01/male_09.mdl\"",
		"5", "5",
		"6", "6",
		"7", "7",
		"8", "8",
		"9", "9",
		"10", "10",
		"11", "11",
		"12", "12",
		"13", "13",
		"14", "14",
		"15", "15",
		"16", "16",
		"17", "17",
		"18", "18"
	);
	mb->SetAlwaysOnTop( true );
	mb->BroadcastMenuToAllPlayers();
}
static ConCommand test_menu( "test_menu", cc_test_menu, "", FCVAR_HIDDEN );

void cc_test_menu_2( const CCommand& args )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();

	if ( !pPlayer )
		return;

	CMenuBuilder* mb = GetTopMenuManager()->CreateMenu( "test", "Title", "Description", COLOR_MK_RED, 100,
		3,
		"# 1", "test",
		"# 2", "test",
		"# 3", "test"
	);
	mb->BroadcastMenuToAllPlayers();
	GetTopMenuManager()->ClearAll();

}
static ConCommand test_menu_2( "test_menu_2", cc_test_menu_2, "", FCVAR_HIDDEN );

#endif