"Resource/UI/WaveCompletePanel.res"
{		
	"Background"
	{
		"ControlName"		"ScalableImagePanel"
		"fieldName"		"Background"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"250"
		"tall"			"80"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../HUD/tournament_panel_brown"

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
		"ypos"			"0"
		"wide"			"250"
		"tall"			"35"
		"visible"		"1"
		"PaintBackgroundType" "2"
		
		"WaveCompleteLabelShadow"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"WaveCompleteLabelShadow"
			"font"			"HudFontMediumSmallBold"
			"textAlignment"	"center"
			"labelText"		"%titletext%"
			"xpos"			"3"
			"ypos"			"3"
			"wide"			"245"
			"tall"			"36"
			"visible"		"1"
			"enabled"		"1"
			"fgcolor"		"black"		
			
			// Red Background
			"PaintBackgroundType" "2"
			"fgcolor_override"	"black"		
			"bgcolor_override"	"190 60 60 225"
		}
		
		"WaveCompleteLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"WaveCompleteLabel"
			"font"			"HudFontMediumSmallBold"
			"textAlignment"	"center"
			"labelText"		"%titletext%"
			"xpos"			"2"
			"ypos"			"2"
			"wide"			"244"
			"tall"			"35"
			"visible"		"1"
			"enabled"		"1"
			"fgcolor"		"tanlight"		
		}
	}
		
	"CreditContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"CreditContainer"
		"xpos"			"0"
		"ypos"			"30"
		"wide"			"400"
		"tall"			"400"
		"autoResize"	"1"
		"visible"		"1"
		
		"CreditCollectedTextLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CreditCollectedTextLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_CurrencyCollected"
			"textAlignment" "north-east"
			"xpos"			"0"
			"ypos"			"9"
			"wide"			"105"
			"tall"			"20"
			"fgcolor"		"tanlight"
		}
		
		"CreditCollectedCountLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CreditCollectedCountLabel"
			"font"			"HudFontSmallest"
			"labelText"		"%creditscollected%"
			"textAlignment" "north-east"
			"xpos"			"115"
			"ypos"			"9"
			"wide"			"30"
			"tall"			"20"
			"fgcolor"		"CreditsGreen"
		}
		
		"CreditMissedTextLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CreditMissedTextLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Missed"
			"textAlignment" "north-east"
			"xpos"			"0"
			"ypos"			"21"
			"wide"			"105"
			"tall"			"20"
			"fgcolor"		"tanlight"
		}
		
		"CreditMissedCountLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CreditMissedCountLabel"
			"font"			"HudFontSmallest"
			"labelText"		"%creditsmissed%"
			"textAlignment" "north-east"
			"xpos"			"115"
			"ypos"			"21"
			"wide"			"30"
			"tall"			"20"
			"fgcolor"		"RedSolid"
		}
		
		"CreditBonusTextLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CreditBonusTextLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#TF_PVE_Bonus"
			"textAlignment" "north-east"
			"xpos"			"0"
			"ypos"			"33"
			"wide"			"105"
			"tall"			"20"
			"fgcolor"		"tanlight"
		}
		
		"CreditBonusCountLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"CreditBonusCountLabel"
			"font"			"HudFontSmallest"
			"labelText"		"%creditbonus%"
			"textAlignment" "north-east"
			"xpos"			"115"
			"ypos"			"33"
			"wide"			"30"
			"tall"			"20"
			"fgcolor"		"CreditsGreen"
		}
	}
	
	"RatingContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"RatingContainer"
		"xpos"			"140"
		"ypos"			"30"
		"wide"			"400"
		"tall"			"400"
		"autoResize"	"1"
		"visible"		"1"
		
		"RatingLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"RatingLabel"
			"font"			"HudFontSmall"
			"labelText"		"%ratinglabel%"
			"textAlignment" "center"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"115"
			"tall"			"20"
			"fgcolor"		"tanlight"
		}
		
		"RatingTextShadow"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"RatingTextShadow"
			"font"			"HudFontMediumBold"
			"labelText"		"%ratingscore%"
			"textAlignment" "center"
			"xpos"			"1"
			"ypos"			"20"
			"wide"			"115"
			"tall"			"30"
			"fgcolor"		"Black"
		}
		
		"RatingText"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"RatingText"
			"font"			"HudFontMediumBold"
			"labelText"		"%ratingscore%"
			"textAlignment" "center"
			"xpos"			"0"
			"ypos"			"19"
			"wide"			"115"
			"tall"			"30"
			"fgcolor"		"White"
		}
	}

	"RespecBackground"
	{
		"ControlName"	"ScalableImagePanel"
		"fieldName"		"RespecBackground"
		"xpos"			"0"
		"ypos"			"80"
		"wide"			"250"
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
		"ypos"			"80"
		"wide"			"250"
		"tall"			"25"
		"visible"		"1"
		"PaintBackgroundType" "2"
	
		"RespecTextLabelWin"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"RespecTextLabelWin"
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
