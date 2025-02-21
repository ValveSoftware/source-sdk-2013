"Resource/UI/MatchMakingDashboardPopup_NewMatch.res"
{
	"NewMatchFound"
	{
		"ControlName"	"CExpandablePanel"
		"fieldName"		"NewMatchFound"
		"xpos"			"cs-0.5"
		"ypos"			"0"
		"zpos"			"9999"
		"wide"			"200"
		"tall"			"60"
		"visible"		"1"
		"proportionaltoparent"	"0"
		"keyboardinputenabled"	"0"
		"mouseinputenabled"		"1"

		"collapsed_height"	"0"
		"expanded_height"	"60"

		"pinCorner"		"2"
		"autoResize"	"1"

		"OuterShadow"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"OuterShadow"
			"xpos"			"0"
			"ypos"			"rs1"
			"zpos"			"-2"
			"wide"			"f0"
			"tall"			"f0"
			"visible"		"1"
			"PaintBackgroundType"	"0"
			"border"		"OuterShadowBorder"
			"proportionaltoparent"	"1"

			"pinCorner"		"0"
			"autoResize"	"2"
		}

		"BGPanel"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"BGPanel"
			"xpos"			"cs-0.5"
			"ypos"			"cs-0.5"
			"zpos"			"-1"
			"wide"			"f5"
			"tall"			"f5"
			"visible"		"1"
			"PaintBackgroundType"	"2"
			"border"		"ReplayDefaultBorder"
			"proportionaltoparent"	"1"
			"pinCorner"		"3"

			"DescLabel"
			{
				"ControlName"	"CAutoFittingLabel"
				"fieldName"		"DescLabel"
				"xpos"			"0"
				"ypos"			"7"
				"wide"			"f0"
				"zpos"			"100"
				"tall"			"20"
				"visible"		"1"
				"enabled"		"1"
				"font"			"HudFontSmallBold"
				"fgcolor_override"	"TanLight"
				"textAlignment"	"center"
				"labelText"		"%match_type%"
				"proportionaltoparent"	"1"
				"pinCorner"		"3"

				"fonts"
				{
					"0"		"HudFontSmallestBold"
					"1"		"StorePromotionsTitle"
					"2"		"FontStorePrice"
				}
			}

			"AutoJoinLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"AutoJoinLabel"
				"xpos"			"0"
				"ypos"			"rs1-17"
				"wide"			"f0"
				"zpos"			"100"
				"tall"			"20"
				"visible"		"1"
				"enabled"		"1"
				"font"			"HudFontSmallest"
				"fgcolor_override"	"TanLight"
				"textAlignment"	"center"
				"labelText"		"%auto_join%"
				"proportionaltoparent"	"1"
				"pinCorner"		"3"
			}

			"AbandonButton"
			{
				"ControlName"	"CExButton"
				"fieldName"		"AbandonButton"
				"xpos"			"rs1-10"
				"ypos"			"rs1-5"
				"zpos"			"100"
				"wide"			"75"
				"tall"			"15"
				"autoResize"	"0"
				"pinCorner"		"3"
				"visible"		"1"

				"enabled"		"1"
				"tabPosition"	"0"
				"font"			"HudFontSmallestBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"Command"		"abandon_match"
				"proportionaltoparent"	"1"
				"labeltext"		"#TF_MM_Rejoin_Abandon"
				"mouseinputenabled"	"1"
				"keyboardinputenabled"	"0"
				"actionsignallevel"	"2"
				"paintbackground"	"1"

				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
			}

			"SmallJoinButton"
			{
				"ControlName"	"CExButton"
				"fieldName"		"SmallJoinButton"
				"xpos"			"10"
				"ypos"			"rs1-5"
				"wide"			"90"
				"zpos"			"100"
				"tall"			"15"

				if_expected
				{
					"xpos"			"cs-0.5"
					"wide"			"150"
				}
				
				"autoResize"	"0"
				"pinCorner"		"3"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"font"			"HudFontSmallestBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"Command"		"join_match"
				"proportionaltoparent"	"1"
				"labeltext"		"#TF_Matchmaking_RollingQueue_JoinNow"
				"mouseinputenabled"	"1"
				"keyboardinputenabled"	"0"
				"actionsignallevel"	"2"

				"armedBgColor_override"		"CreditsGreen"
				"defaultBgColor_override"	"SaleGreen"

				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
			}

			"WideJoinButton"
			{
				"ControlName"	"CExButton"
				"fieldName"		"WideJoinButton"
				"xpos"			"cs-0.5"
				"ypos"			"rs1-5"
				"wide"			"150"
				"zpos"			"100"
				"tall"			"15"
				
				"autoResize"	"0"
				"pinCorner"		"3"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"font"			"HudFontSmallestBold"
				"textAlignment"	"center"
				"dulltext"		"0"
				"brighttext"	"0"
				"Command"		"join_match"
				"proportionaltoparent"	"1"
				"labeltext"		"#TF_Matchmaking_RollingQueue_JoinNow"
				"mouseinputenabled"	"1"
				"keyboardinputenabled"	"0"
				"actionsignallevel"	"2"

				"armedBgColor_override"		"CreditsGreen"
				"defaultBgColor_override"	"SaleGreen"

				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
			}

			"JoiningLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"JoiningLabel"
				"xpos"			"cs-1"
				"ypos"			"rs1-17"
				"wide"			"f0"
				"zpos"			"100"
				"tall"			"20"
				"visible"		"1"
				"enabled"		"1"
				"font"			"HudFontSmallest"
				"fgcolor_override"	"TanLight"
				"textAlignment"	"east"
				"labelText"		"#TF_Matchmaking_RollingQueue_MatchReadyJoining"
				"proportionaltoparent"	"1"
				"pinCorner"		"3"
			}

			"Spinner"
			{
				"ControlName"	"CTFLogoPanel"
				"fieldName"		"Spinner"
				"xpos"			"c+5"
				"ypos"			"rs1-17"
				"zpos"			"104"
				"wide"			"o1"
				"tall"			"25"
				"visible"		"1"

				"proportionaltoparent"	"1"
				"mouseinputenabled"	"0"
				"alpha"			"200"

				"radius"		"10"
				"velocity"		"250"
				"fgcolor_override"	"Orange"
			}
		}
	}
}
