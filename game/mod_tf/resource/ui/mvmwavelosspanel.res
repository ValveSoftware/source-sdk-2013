"Resource/UI/MvMWaveLossPanel.res"
{
	"DarkMask"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"DarkMask"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"480"
		"visible"		"1"
		"bgcolor_override"		"20 20 20 200"
	}

	"PanelBackground"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"PanelBackground"
		"xpos"			"c-275"
		"ypos"			"100"
		"wide"			"550"
		"tall"			"350"
		"visible"		"1"
		
		"PaintBackgroundType"	"2"
		"paintbackground"	"0"
		"border"		"MainMenuHighlightBorder"
	}
	
	"WaveFailHeaderShadow"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"WaveFailHeaderShadow"
		"font"			"HudFontMediumBold"
		"labelText"		"%waveheader%"
		"textAlignment" "center"
		"xpos"			"c-274"
		"ypos"			"106"
		"wide"			"550"
		"tall"			"30"
		"fgcolor"		"Black"
	}
	
	"WaveFailHeader"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"HeaderLabel"
		"font"			"HudFontMediumBold"
		"labelText"		"%waveheader%"
		"textAlignment" "center"
		"xpos"			"c-275"
		"ypos"			"105"
		"wide"			"550"
		"tall"			"30"
		"fgcolor"		"Orange"
	}
	
	"HeaderDivider"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"HeaderDivider"
		"xpos"			"c-250"
		"ypos"			"128"
		"wide"			"500"
		"tall"			"2"
		"visible"		"1"
		"bgcolor_override"		"20 20 20 100"
	}
	
	"SummaryHeader"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"SummaryHeader"
		"font"			"HudFontMediumSmallBold"
		"labelText"		"%summaryheader%"
		"textAlignment" "west"
		"xpos"			"c-250"
		"ypos"			"135"
		"wide"			"550"
		"tall"			"30"
		"fgcolor"		"Black"
	}
	
	"CollectionContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"CollectionContainer"
		"xpos"			"190"
		"ypos"			"160"
		"wide"			"300"
		"tall"			"100"
		"visible"		"1"
		
		"Header"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Header"
			"font"			"HudFontSmallBold"
			"labelText"		"#TF_PVE_Credit_Collection"
			"textAlignment" "center"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"140"
			"fgcolor"		"Black"
		}
	
		"CollectedLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CollectedLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Collected"
			"textAlignment" "west"
			"xpos"			"0"
			"ypos"			"25"
			"wide"			"120"
			"fgcolor"		"TanDarker"
		}
		
		"CollectedCount"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CollectedCount"
			"font"			"HudFontSmallestBold"
			"labelText"		"%creditscollected%"
			"textAlignment" "east"
			"xpos"			"120"
			"ypos"			"25"
			"wide"			"30"
			"fgcolor"		"CreditsGreen"
		}
		
		"MissedLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"MissedLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Missed"
			"textAlignment" "west"
			"xpos"			"0"
			"ypos"			"40"
			"wide"			"120"
			"fgcolor"		"TanDarker"
		}
		
		"MissedCount"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"MissedCount"
			"font"			"HudFontSmallestBold"
			"labelText"		"%creditsmissed%"
			"textAlignment" "east"
			"xpos"			"120"
			"ypos"			"40"
			"wide"			"30"
			"fgcolor"		"RedSolid"
		}
		
		"BonusLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BonusLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Bonus"
			"textAlignment" "west"
			"xpos"			"0"
			"ypos"			"55"
			"wide"			"120"
			"fgcolor"		"TanDarker"
		}
		
		"BonusCount"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BonusCount"
			"font"			"HudFontSmallestBold"
			"labelText"		"%creditbonus%"
			"textAlignment" "east"
			"xpos"			"120"
			"ypos"			"55"
			"wide"			"30"
			"fgcolor"		"CreditsGreen"
		}
	}
	
	"UsageContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"UsageContainer"
		"xpos"			"390"
		"ypos"			"160"
		"wide"			"300"
		"tall"			"100"
		"visible"		"1"
		
		"Header"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Header"
			"font"			"HudFontSmallBold"
			"labelText"		"#TF_PVE_Credit_Use"
			"textAlignment" "center"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"195"
			"fgcolor"		"Black"
		}
	
		"YouLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"YouLabel"
			"font"			"FontStorePriceSmall"
			"labelText"		"#TF_PVE_You"
			"textAlignment" "east"
			"xpos"			"120"
			"ypos"			"15"
			"wide"			"35"
			"fgcolor"		"TanDarker"
		}
		
		"TeamLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"TeamLabel"
			"font"			"FontStorePriceSmall"
			"labelText"		"#Winpanel_Team2"
			"textAlignment" "east"
			"xpos"			"175"
			"ypos"			"15"
			"wide"			"35"
			"fgcolor"		"TanDarker"
		}
		
		"BuybackLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BuybackLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Buybacks"
			"textAlignment" "west"
			"xpos"			"0"
			"ypos"			"25"
			"wide"			"120"
			"fgcolor"		"TanDarker"
		}
		
		"BuybackCountYou"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BuybackCountYou"
			"font"			"HudFontSmallestBold"
			"labelText"		"%buybacksyou%"
			"textAlignment" "east"
			"xpos"			"120"
			"ypos"			"25"
			"wide"			"35"
			"fgcolor"		"TanDarker"
		}
		
		"BuybackCountTeam"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BuybackCountTeam"
			"font"			"HudFontSmallestBold"
			"labelText"		"%buybacksteam%"
			"textAlignment" "east"
			"xpos"			"175"
			"ypos"			"25"
			"wide"			"35"
			"fgcolor"		"TanDarker"
		}
		
		"BottleLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BottleLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Bottles"
			"textAlignment" "west"
			"xpos"			"0"
			"ypos"			"40"
			"wide"			"120"
			"fgcolor"		"TanDarker"
		}
		
		"BottleCountYou"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BottleCountYou"
			"font"			"HudFontSmallestBold"
			"labelText"		"%bottlesyou%"
			"textAlignment" "east"
			"xpos"			"120"
			"ypos"			"40"
			"wide"			"35"
			"fgcolor"		"TanDarker"
		}
		
		"BottleCountTeam"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"BottleCountTeam"
			"font"			"HudFontSmallestBold"
			"labelText"		"%bottlesteam%"
			"textAlignment" "east"
			"xpos"			"175"
			"ypos"			"40"
			"wide"			"35"
			"fgcolor"		"TanDarker"
		}
		
		"InactiveLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"InactiveLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Inactive_Upgrades"
			"textAlignment" "west"
			"xpos"			"0"
			"ypos"			"55"
			"wide"			"120"
			"fgcolor"		"TanDarker"
		}
		
		"InactiveCountYou"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"ActiveCountYou"
			"font"			"HudFontSmallestBold"
			"labelText"		"%inactiveupgradesyou%"
			"textAlignment" "east"
			"xpos"			"120"
			"ypos"			"55"
			"wide"			"35"
			"fgcolor"		"RedSolid"
		}
		
		"InactiveCountTeam"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"ActiveCountTeam"
			"font"			"HudFontSmallestBold"
			"labelText"		"%inactiveupgradesteam%"
			"textAlignment" "east"
			"xpos"			"175"
			"ypos"			"55"
			"wide"			"35"
			"fgcolor"		"RedSolid"
		}
	}
	
	"HintContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"HintContainer"
		"xpos"			"c-250"
		"ypos"			"250"
		"wide"			"500"
		"tall"			"150"
		"visible"		"1"
		
		"Background"
		{
			"ControlName"	"ScalableImagePanel"
			"fieldName"		"Background"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"500"
			"tall"			"150"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"image"			"../HUD/tournament_panel_black"

			"src_corner_height"	"22"				// pixels inside the image
			"src_corner_width"	"22"
		
			"draw_corner_width"	"5"				// screen size of the corners ( and sides ), proportional
			"draw_corner_height" 	"5"	
		}
		
		"Header"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Header"
			"font"			"HudFontSmallBold"
			"labelText"		"#TF_PVE_Hint_Header"
			"textAlignment" "center"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"500"
			"fgcolor"		"Gray"
		}

		"CptCntnBody"
		{
			"ControlName"		"ImagePanel"
			"fieldName"		"CptCntnBody"
			"xpos"		"5"
			"ypos"		"70"
			"zpos"		"2"
			"wide"		"50"
			"tall"		"50"
			"image"			""
			"visible"		"0"
			"enabled"		"1"
			"scaleImage"		"1"
		}

		"CptCntnMisc"
		{
			"ControlName"		"ImagePanel"
			"fieldName"		"CptCntnMisc"
			"xpos"		"5"
			"ypos"		"45"
			"zpos"		"3"
			"wide"		"50"
			"tall"		"50"
			"image"			""
			"visible"		"0"
			"enabled"		"1"
			"scaleImage"		"1"
		}

		"CptCntnHat"
		{
			"ControlName"		"ImagePanel"
			"fieldName"		"CptCntnHat"
			"xpos"		"5"
			"ypos"		"20"
			"zpos"		"1"
			"wide"		"50"
			"tall"		"50"
			"image"			""
			"visible"		"0"
			"enabled"		"1"
			"scaleImage"		"1"
		}
		
		"HintImage1"
		{
			"ControlName"		"ImagePanel"
			"fieldName"		"HintImage1"
			"xpos"		"5"
			"ypos"		"20"
			"wide"		"50"
			"tall"		"100"
			"image"			"class_sel_sm_engineer_inactive"
			"visible"		"1"
			"enabled"		"1"
			"scaleImage"		"1"
		}
		
		"Hint1"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Hint1"
			"font"			"HudFontSmallest"
			"labelText"		"%hint1%"
			"textAlignment" "north-west"
			"xpos"			"55"
			"ypos"			"60"
			"wide"			"185"
			"tall"			"55"
			"fgcolor"		"Gray"
			"wrap"			"1"
		}
		
		"HintImage2"
		{
			"ControlName"		"ImagePanel"
			"fieldName"		"HintImage2"
			"xpos"		"255"
			"ypos"		"20"
			"wide"		"50"
			"tall"		"100"
			"image"			"class_sel_sm_engineer_inactive"
			"visible"		"1"
			"enabled"		"1"
			"scaleImage"		"1"
		}
		
		"Hint2"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Hint2"
			"font"			"HudFontSmallest"
			"labelText"		"%hint2%"
			"textAlignment" "north-west"
			"xpos"			"305"
			"ypos"			"60"
			"wide"			"185"
			"tall"			"55"
			"fgcolor"		"Gray"
			"wrap"			"1"
		}
	}
	
	"VoteButton"
	{
		"ControlName"	"CExImageButton"
		"fieldName"		"VoteButton"
		"xpos"			"c-250"
		"ypos"			"410"
		"zpos"			"999"
		"wide"			"200"
		"tall"			"25"
		"autoResize"	"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#TF_PVE_Vote_MissionRestart"
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"default"		"1"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"image_drawcolor"	"235 226 202 255"
		"Command"		"vote_restart"
	}
		
	"ContinueButton"
	{
		"ControlName"	"CExImageButton"
		"fieldName"		"ContinueButton"
		"xpos"			"c150"
		"ypos"			"410"
		"zpos"			"1000"
		"wide"			"100"
		"tall"			"25"
		"autoResize"	"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#ConfirmButtonText"
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"default"		"1"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"image_drawcolor"	"235 226 202 255"
		"Command"		"continue"
	}
}
