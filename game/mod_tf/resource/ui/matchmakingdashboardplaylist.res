#base "MatchMakingDashboardSidePanel.res"

"Resource/UI/MatchMakingDashboardPlayList.res"
{
	"ExpandableList"
	{
		"fieldName"		"ExpandableList"
		"xpos"			"r0"
		"ypos"			"c35"
		"zpos"			"1001"
		"wide"			"270"
		"tall"			"146"
		"visible"		"1"
		"proportionaltoparent"	"1"
    }
	
	"Title"
	{
		"ControlName"		"Label"
		"fieldName"		"Title"
		"xpos"		"8"
		"ypos"		"20"
		"zpos"		"99"
		"wide"		"f0"
		"tall"		"20"
		"proportionaltoparent"	"1"
		"labeltext"		"#TF_Matchmaking_HeaderModeSelect"
		"textAlignment"	"west"
		"font"			"HudFontMediumBigBold"
		"fgcolor_override"	"TanDark"
		
		"mouseinputenabled"	"0"
	}

	"playlist"
	{
		"ControlName"	"CTFPlaylistPanel"
		"fieldName"		"playlist"
		"xpos"			"8"
		"ypos"			"50"
		"zpos"			"5"
		"wide"			"255"
		"tall"			"150"
		"visible"		"1"
		"proportionaltoparent"	"1"
		
		"FakeQuickPlayButton"
		{
			"ControlName"	"CExImageButton"
			"fieldname"		"FakeQuickPlayButton"
			"xpos"			"0"
			"ypos"			"5"
			"zpos"			"-75"
			"wide"			"250"
			"tall"			"26"
			"visible"		"1"
			"autoResize"	"0"
			"pinCorner"		"3"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"textinsetx"	"25"
			"use_proportional_insets" "1"
			"font"			"HudFontSmallBold"
			"textAlignment"	"west"
			"labeltext"     "#MMenu_PlayMultiplayer"
			"dulltext"		"0"
			"brighttext"	"0"
			"default"		"1"
			"sound_depressed"	"UI/buttonclick.wav"
			"sound_released"	"UI/buttonclickrelease.wav"
			
			"border_default"	"MainMenuButtonDefault"
			"border_armed"		"MainMenuButtonArmed"
			"paintbackground"	"0"
			
			"defaultFgColor_override" "46 43 42 255"
			"armedFgColor_override" "235 226 202 255"
			"depressedFgColor_override" "46 43 42 255"
			
			"image_drawcolor"	"117 107 94 255"
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
		
		"FakeMveButton"
		{
			"ControlName"	"CExImageButton"
			"fieldname"		"FakeMveButton"
			"xpos"			"0"
			"ypos"			"35"
			"zpos"			"-75"
			"wide"			"250"
			"tall"			"26"
			"visible"		"1"
			"PaintBackgroundType"	"0"
			"autoResize"	"0"
			"pinCorner"		"3"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"textinsetx"	"25"
			"use_proportional_insets" "1"
			"font"			"HudFontSmallBold"
			"labeltext"     "#MMenu_PlayCoop"
			"textAlignment"	"west"
			"dulltext"		"0"
			"brighttext"	"0"
			"default"		"1"
			"sound_depressed"	"UI/buttonclick.wav"
			"sound_released"	"UI/buttonclickrelease.wav"
			
			"border_default"	"MainMenuButtonDefault"
			"border_armed"		"MainMenuButtonArmed"
			"paintbackground"	"0"
			
			"defaultFgColor_override" "46 43 42 255"
			"armedFgColor_override" "235 226 202 255"
			"depressedFgColor_override" "46 43 42 255"
			
			"image_drawcolor"	"117 107 94 255"
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
	
	"BackToMenuButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"BackToMenuButton"
		"xpos"			"c-128"
		"labelText"		"#TF_BackToMainMenu"
		"ypos"			"c42"
		"zpos"			"19"
		"wide"			"250"
		"tall"			"26"
		"visible"		"1"
		"enabled"       "1"
		"proportionaltoparent"	"1"
		"command"		"nav_close"
		"textinsetx"    "15"

		"textAlignment"	"west"
		"font"			"HudFontSmallBold"

		"border_default"	"MainMenuButtonDefault"
		"border_armed"		"MainMenuButtonArmed"
		"paintbackground"	"0"
		
		"defaultFgColor_override" "46 43 42 255"
		"armedFgColor_override" "235 226 202 255"
		"depressedFgColor_override" "46 43 42 255"
	}
}
