"Resource/UI/hudpvewinpanel.res"
{
	"WinPanelBG"
	{
		"ControlName"		"ScalableImagePanel"
		"fieldName"		"WinPanelBG"
		"xpos"			"0"
		"ypos"			"50"
		"zpos"			"0"
		"wide"			"300"
		"tall"			"90"
		"image"			"../hud/tournament_panel_brown"
		"autoResize"	"0"
		"pinCorner"		"0"
		
		"visible"		"1"
		"enabled"		"1"
		
		"src_corner_height"	"22"				// pixels inside the image
		"src_corner_width"	"22"
	
		"draw_corner_width"	"5"				// screen size of the corners ( and sides ), proportional
		"draw_corner_height" 	"5"	
	}

	"WaveCompleteContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"WaveCompleteContainer"
		"xpos"			"0"
		"ypos"			"50"
		"wide"			"300"
		"tall"			"24"
		"visible"		"1"
		"PaintBackgroundType" "2"
		
		"WinningTeamLabelDropshadow"
		{	
			"ControlName"		"CExLabel"
			"fieldName"		"WinningTeamLabelDropshadow"
			"font"			"HudFontMediumSmallBold"
			"fgcolor"		"black"
			"xpos"			"3"
			"ypos"			"3"
			"wide"			"295"
			"tall"			"26"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		"#Winpanel_PVE_Evil_Wins"
			"textAlignment"		"Center"
			"dulltext"		"0"
			"brighttext"		"0"
			
			// blue Background
			"PaintBackgroundType" "2"
			"fgcolor_override"	"black"		
			"bgcolor_override"	"91 122 142 200"
		}
		
		"WinningTeamLabel"
		{	
			"ControlName"		"CExLabel"
			"fieldName"		"WinningTeamLabel"
			"font"			"HudFontMediumSmallBold"
			"xpos"			"2"
			"ypos"			"2"
			"wide"			"295"
			"tall"			"26"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		"#Winpanel_PVE_Evil_Wins"
			"textAlignment"		"Center"
		}
	}
	
	"WinReasonLabel"
	{	
		"ControlName"		"CExLabel"
		"fieldName"		"WinReasonLabel"
		"font"			"HudFontSmall"
		"xpos"			"0"
		"ypos"			"80"
		"zpos"			"1"
		"wide"			"292"
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"%WinReasonLabel%"
		"textAlignment"		"Center"
		"dulltext"		"0"
		"brighttext"		"0"
	}

	"DetailsLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"DetailsLabel"
		"font"			"HudFontSmallest"
		"xpos"			"15"
		"ypos"			"100"
		"zpos"			"1"
		"wide"			"265"
		"tall"			"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"%DetailsLabel%"
		"textAlignment"		"Center"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"			"1"
		"centerwrap"	"1"
	}

	"RespecBackground"
	{
		"ControlName"	"ScalableImagePanel"
		"fieldName"		"RespecBackground"
		"xpos"			"0"
		"ypos"			"142"
		"wide"			"300"
		"tall"			"25"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"image"			"../HUD/tournament_panel_brown"

		"src_corner_height"	"22"				// pixels inside the image
		"src_corner_width"	"22"
	
		"draw_corner_width"	"5"				// screen size of the corners ( and sides ), proportional
		"draw_corner_height" 	"5"	
	}

	"RespecContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"RespecContainer"
		"xpos"			"0"
		"ypos"			"142"
		"wide"			"300"
		"tall"			"25"
		"visible"		"1"
		"PaintBackgroundType" "2"
	
		"RespecTextLabelLoss"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"RespecTextLabelLoss"
			"font"			"HudFontSmall"
			"labelText"		"#TF_PVE_Respecs"
			"textAlignment" "center"
			"xpos"			"0"
			"ypos"			"3"
			"wide"			"160"
			"tall"			"20"
			"fgcolor"		"tanlight"
		}
		
		"RespecCountLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"RespecCountLabel"
			"font"			"HudFontSmall"
			"labelText"		"%respeccount%"
			"textAlignment" "center"
			"xpos"			"145"
			"ypos"			"3"
			"wide"			"30"
			"tall"			"20"
			"fgcolor"		"CreditsGreen"
		}
	}
}
