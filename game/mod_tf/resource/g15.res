"Logitech G-15 Keyboard Layout"
{
	"game"		"Team Fortress 2"
	"chatlines"	"8"  // number of chat lines to keep (1-64)
	
	// These need to be 1bpp HICONs
	"icons"
	{
		"game_icon"			"resource/game_1bpp.ico"
	}
	
	// Global replacements
	"replace"
	{
		"team_Blue"	"Blue"
		"team_Red"			"Red"
		"team_Unassigned"	"Unassigned"
		"team_Spectator"	"Spectator"
		"alive_true"		"Alive"
		"alive_false"		"DEAD"
	}
		
	// title page is special
	"page"
	{
		// Special signal, this page is shown at startup and when disconnected from server
		"titlepage"		"1"
		
		"static_icon"
		{
			"x"			"0"
			"y"			"4"
			"w"			"32"
			"h"			"32"
			"name"		"game_icon"
		}
		
		"static_text"
		{
			"size"		"medium"
			"align"		"center"
			"x"			"34"
			"y"			"10"
			"w"			"120"
			"text"		"Team Fortress 2"
		}
			
		"static_text"
		{
			"size"		"medium"
			"align"		"center"
			"x"			"34"
			"y"			"25"
			"w"			"120"
			"text"		" "
		}
			
//		"icon"
//		{
//		}
	}
	
	"page"   // Player health/wpn info
	{
		// Only show this if the player has a player entity in the game
		"requiresplayer"	"1"
	
		"static_text"
		{
			"size"		"medium"
			"align"		"left"
			"x"			"10"
			"y"			"4"
			"w"			"150"
			"text"		"Team:  team_%(localteam)m_szTeamName%"
		}
	
		"static_icon"
		{
			"x"			"144"
			"y"			"4"
			"w"			"16"
			"h"			"16"
			"name"		"icon_%(localteam)m_szTeamName%"
		}
			
		"static_text"
		{
			"size"		"medium"
			"align"		"left"
			"x"			"10"
			"y"			"18"
			"w"			"150"
			"text"		"Health:  - %(localplayer)m_iHealth% -"
		}
		
		"static_text"
		{
			"size"		"medium"
			"align"		"left"
			"x"			"10"
			"y"			"32"
			"w"			"150"
			"text"		"(weapon_print_name):  - (ammo_primary) -"
		}
	}
	
	"page"   // My Team
	{
		// Create legend
		"requiresplayer"	"1"
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"2"
			"y"			"2"
			"w"			"10"
			"text"		"#"
			"header"	"1"
		}
			
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"12"
			"y"			"2"
			"w"			"85"
			"text"		"team_%(localteam)m_szTeamName"
			"header"	"1"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"right"
			"x"			"97"
			"y"			"2"
			"w"			"63"
			"text"		"Status"
			"header"	"1"
		}

		"iterate_team"
		{
			"x"				"2"
			"Y"				"13"
			"y_increment"	"10"
			
			"static_text"
			{
				"size"		"small"
				"align"		"left"
				"x"			"2"
				"y"			"0"
				"w"			"10"
				"text"		"(itemnumber)."
			}
			
			"static_text"
			{
				"size"		"small"
				"align"		"left"
				"x"			"12"
				"y"			"0"
				"w"			"85"
				"text"		"%(playerresource)m_szName[(playerindex)]%"
			}
			
			"static_text"
			{
				"size"		"small"
				"align"		"right"
				"x"			"97"
				"y"			"0"
				"w"			"60"
				"text"		"alive_%(playerresource)m_bAlive[(playerindex)]%"
			}
		}
	}
	
	"page"   // Scoreboard info
	{
		// Create legend
		"requiresplayer"	"1"
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"2"
			"y"			"2"
			"w"			"10"
			"text"		"#"
			"header"	"1"
		}
			
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"12"
			"y"			"2"
			"w"			"85"
			"text"		"Player"
			"header"	"1"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"right"
			"x"			"77"
			"y"			"2"
			"w"			"83"
			"text"		"Score/Deaths/Ping"
			"header"	"1"
		}

		"iterate_players"
		{
			"x"				"2"
			"Y"				"13"
			"y_increment"	"10"
			
			"static_text"
			{
				"size"		"small"
				"align"		"left"
				"x"			"2"
				"y"			"0"
				"w"			"10"
				"text"		"(itemnumber)."
			}
			
			"static_text"
			{
				"size"		"small"
				"align"		"left"
				"x"			"12"
				"y"			"0"
				"w"			"85"
				"text"		"%(playerresource)m_szName[(playerindex)]%"
			}
		
			"static_text"
			{
				"size"		"small"
				"align"		"right"
				"x"			"97"
				"y"			"0"
				"w"			"15"
				"text"		"%(playerresource)m_iScore[(playerindex)]%"
			}
		
			"static_text"
			{
				"size"		"small"
				"align"		"right"
				"x"			"112"
				"y"			"0"
				"w"			"15"
				"text"		"%(playerresource)m_iDeaths[(playerindex)]%"
			}
			
			"static_text"
			{
				"size"		"small"
				"align"		"right"
				"x"			"127"
				"y"			"0"
				"w"			"15"
				"text"		"%(playerresource)m_iPing[(playerindex)]%"
			}
		}
	}
	
	// Chat
	"page"   
	{
		"requiresplayer"	"1"
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"0"
			"w"			"160"
			"text"		"chat_1"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"10"
			"w"			"160"
			"text"		"chat_2"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"20"
			"w"			"160"
			"text"		"chat_3"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"30"
			"w"			"160"
			"text"		"chat_4"
		}
		
		// Insert a new subpage
		"newsubpage"	"1"
		
	"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"0"
			"w"			"160"
			"text"		"chat_5"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"10"
			"w"			"160"
			"text"		"chat_6"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"20"
			"w"			"160"
			"text"		"chat_7"
		}
		
		"static_text"
		{
			"size"		"small"
			"align"		"left"
			"x"			"0"
			"y"			"30"
			"w"			"160"
			"text"		"chat_8"
		}
	}
}