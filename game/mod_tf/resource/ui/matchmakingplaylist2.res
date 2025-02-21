"Resource/UI/MatchMakingDashboardCasualCriteria.res"
{
	"EventEntry"
	{
		"ControlName"	"CEventPlayListEntry"
		"fieldName"		"EventEntry"
		"xpos"			"0"
		"ypos"			"3"
		"tall"			"45"
		"wide"			"255"
		"proportionaltoparent"	"1"

		"button_command"	"play_event"
	}
	
	"CasualEntryButton"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"CasualEntryButton"
		"xpos"			"0"
		"ypos"			"5"
		"zpos"			"2"
		"wide"			"250"
		"tall"			"26"
		"visible"		"1"
		"enabled"		"1"
		"PaintBackgroundType"	"0"
		
		"SubButton"
		{
			"ControlName"	"CExImageButton"
			"fieldName"		"SubButton"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"250"
			"tall"			"26"
			"autoResize"	"0"
			"pinCorner"		"3"
			"visible"		"1"
			"enabled"		"1"
			"labelText"     "#MMenu_PlayMultiplayer"
			"tabPosition"	"0"
			"textinsetx"	"25"
			"command"       "play_casual"
			"use_proportional_insets" "1"
			"actionsignallevel"	"2"
			"font"			"HudFontSmallBold"
			"textAlignment"	"west"
			"dulltext"		"0"
			"brighttext"	"0"
			"default"		"1"
			"sound_depressed"	"UI/buttonclick.wav"
			"sound_released"	"UI/buttonclickrelease.wav"
			
			"border_default"	"0"
			"border_armed"		"MainMenuButtonArmed"
			"paintbackground"	"0"
			
			"defaultFgColor_override" "0 0 0 0"
			"armedFgColor_override" "235 226 202 255"
			"depressedFgColor_override" "0 0 0 0"
			
			"image_drawcolor"	"0 0 0 0"
			"image_armedcolor"	"235 226 202 255"

			"SubImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SubImage"
				"xpos"			"6"
				"ypos"			"6"
				"zpos"			"1"
				"wide"			"14"
				"tall"			"14"
				"visible"		"1"
				"enabled"		"1"
				"image"         "glyph_multiplayer"
				"scaleImage"	"1"
			}
		}
	}	

	"MvMEntryButton"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"MvMEntryButton"
		"xpos"			"0"
		"ypos"			"35"
		"zpos"			"2"
		"wide"			"250"
		"tall"			"26"
		"visible"		"1"
		"enabled"		"1"
		"PaintBackgroundType"	"0"
		
		"SubButton"
		{
			"ControlName"	"CExImageButton"
			"fieldName"		"SubButton"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"250"
			"tall"			"26"
			"autoResize"	"0"
			"pinCorner"		"3"
			"visible"		"1"
			"enabled"		"1"
			"labelText"     "#MMenu_PlayCoop"
			"tabPosition"	"0"
			"textinsetx"	"25"
			"command"       "play_mvm"
			"use_proportional_insets" "1"
			"actionsignallevel"	"2"
			"font"			"HudFontSmallBold"
			"textAlignment"	"west"
			"dulltext"		"0"
			"brighttext"	"0"
			"default"		"1"
			"sound_depressed"	"UI/buttonclick.wav"
			"sound_released"	"UI/buttonclickrelease.wav"
			
			"border_default"	"0"
			"border_armed"		"MainMenuButtonArmed"
			"paintbackground"	"0"
			
			"defaultFgColor_override" "0 0 0 0"
			"armedFgColor_override" "235 226 202 255"
			"depressedFgColor_override" "0 0 0 0"
			
			"image_drawcolor"	"0 0 0 0"
			"image_armedcolor"	"235 226 202 255"

			"SubImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SubImage"
				"xpos"			"6"
				"ypos"			"6"
				"zpos"			"1"
				"wide"			"14"
				"tall"			"14"
				"visible"		"1"
				"enabled"		"1"
				"image"         "glyph_coop"
				"scaleImage"	"1"
			}
		}
	}	
	
	"CasualEntry"
	{
		"ControlName"	"CPlayListEntry"
		"fieldName"		"CasualEntry"
		"xpos"			"0"
		"ypos"			"0"
		"tall"			"0"
		"wide"			"0"
		"proportionaltoparent"	"1"
	
		//"image_name"		"main_menu/main_menu_button_casual"
		//"button_token"		"#MMenu_PlayMultiplayer"
		//"button_command"	"play_casual"
		//"desc_token"		"#MMenu_PlayList_Casual_Desc"
		//"matchgroup"		"7" // k_eTFMatchGroup_Casual_12v12
	}
	
	"CompetitiveEntry"
	{
		"ControlName"	"CPlayListEntry"
		"fieldName"		"CompetitiveEntry"
		"xpos"			"0"
		"ypos"			"0"
		"tall"			"0"
		"wide"			"0"
		"proportionaltoparent"	"1"
	
		//"image_name"		"main_menu/main_menu_button_competitive"
		//"button_token"		"#MMenu_PlayList_Competitive_Button"
		//"button_command"	"play_competitive"
		//"desc_token"		"#MMenu_PlayList_Competitive_Desc"
		//"matchgroup"		"2" // k_eTFMatchGroup_Ladder_6v6
	}
	
	"MvMEntry"
	{
		"ControlName"	"CPlayListEntry"
		"fieldName"		"MvMEntry"
		"xpos"			"0"
		"ypos"			"0"
		"tall"			"0"
		"wide"			"0"
		"proportionaltoparent"	"1"
	
		//"image_name"		"main_menu/main_menu_button_mvm"
		//"button_token"		"#MMenu_PlayCoop"
		//"button_command"	"play_mvm"
		//"desc_token"		"#MMenu_PlayList_MvM_Desc"
		//"matchgroup"		"1" // k_eTFMatchGroup_MvM_MannUp
	}
	
	"ServerBrowserEntry"
	{
		"ControlName"	"CPlayListEntry"
		"fieldName"		"ServerBrowserEntry"
		"xpos"			"0"
		"ypos"			"0"
		"tall"			"0"
		"wide"			"0"
		"proportionaltoparent"	"1"
	
		//"image_name"		"main_menu/main_menu_button_community_server"
		//"button_token"		"#MMenu_PlayList_ServerBrowser_Button"
		//"button_command"	"play_community"
		//"desc_token"		"#MMenu_PlayList_ServerBrowser_Desc"
	
		if_event
		{
			"ypos"			"203"
		}
	}
	
	"TrainingEntry"
	{
		"ControlName"	"CPlayListEntry"
		"fieldName"		"TrainingEntry"
		"xpos"			"0"
		"ypos"			"0"
		"tall"			"0"
		"wide"			"0"
		"proportionaltoparent"	"1"
	
		//"image_name"		"main_menu/main_menu_button_training"
		//"button_token"		"#MMenu_PlayList_Training_Button"
		//"button_command"	"play_training"
		//"desc_token"		"#MMenu_PlayList_Training_Desc"
	
		if_event
		{
			"ypos"			"253"
		}
	}
	
	"CreateServerEntry"
	{
		"ControlName"	"CPlayListEntry"
		"fieldName"		"CreateServerEntry"
		"xpos"			"0"
		"ypos"			"0"
		"tall"			"0"
		"wide"			"0"
		"proportionaltoparent"	"1"
	
		//"image_name"		"main_menu/main_menu_button_custom_server"
		//"button_token"		"#MMenu_PlayList_CreateServer_Button"
		//"button_command"	"create_server"
		//"desc_token"		"#MMenu_PlayList_CreateServer_Desc"
	
		if_event
		{
			"ypos"			"303"
		}
	}
	
	"ScrollBar"
	{
		"ControlName"	"ScrollBar"
		"FieldName"		"ScrollBar"
		"xpos"			"rs1-1"
		"ypos"			"0"
		"tall"			"f0"
		"wide"			"5" // This gets slammed from client schme.  GG.
		"zpos"			"1000"
		"nobuttons"		"1"
		"proportionaltoparent"	"1"

		"Slider"
		{
			"fgcolor_override"	"TanDark"
		}
		
		"UpButton"
		{
			"ControlName"	"Button"
			"FieldName"		"UpButton"
			"visible"		"0"
		}
		
		"DownButton"
		{
			"ControlName"	"Button"
			"FieldName"		"DownButton"
			"visible"		"0"
		}
	}
}
