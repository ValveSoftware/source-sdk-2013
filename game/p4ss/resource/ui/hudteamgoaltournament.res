"Resource/UI/HudTeamGoalTournament.res"
{
	"HudStopWatchObjective"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"HudStopWatchObjective"
		"xpos"				"15"
		"ypos"				"105"
		"zpos"				"1"
		"wide"				"275"
		"tall"				"190"
		"visible"			"0"
		"enabled"			"1"

		if_comp
		{
			"ypos"	"120"
		}

		"HudStopWatchObjectiveBG"
		{
			"ControlName"		"ScalableImagePanel"
			"fieldName"		"HudStopWatchObjectiveBG"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"-1"
			"wide"			"275"
			"tall"			"170"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"image"					"../hud/color_panel_brown"
			"src_corner_height"		"14"				// pixels inside the image
			"src_corner_width"		"14"
				
			"draw_corner_width"		"8"				// screen size of the corners ( and sides ), proportional
			"draw_corner_height" 	"8"
		}
		"HudStopWatchObjectiveBGSmall"
		{
			"ControlName"		"ScalableImagePanel"
			"fieldName"		"HudStopWatchObjectiveBGSmall"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"-1"
			"wide"			"275"
			"tall"			"100"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"0"
			"enabled"		"1"
			"image"					"../hud/color_panel_brown"
			"src_corner_height"		"14"				// pixels inside the image
			"src_corner_width"		"14"
				
			"draw_corner_width"		"8"				// screen size of the corners ( and sides ), proportional
			"draw_corner_height" 	"8"
		}
		"HudStopWatchObjectiveLabel"
		{	
			"ControlName"		"Label"
			"fieldName"		"HudStopWatchObjectiveLabel"
			"font"			"MontserratBlack20Shadow"
			"xpos"			"25"
			"ypos"			"15"
			"zpos"			"1"
			"wide"			"275"
			"tall"			"15"
			"autoResize"		"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"wrap"			"1"
			"labelText"		"%objectivelabel%"
			"textAlignment"		"west"
			"fgcolor_override"		"White"
		}
		"HudStopWatchObjectiveText1"
		{
			"ControlName"		"CExRichText"
			"fieldName"		"HudStopWatchObjectiveText1"
			"xpos"			"25"
			"ypos"			"35"
			"wide"			"225"
			"tall"			"150"
			"wrap"			"1"
			"autoResize"		"1"
			"pinCorner"		"1"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		""
			"textAlignment"		"west"
			"font"			"ScoreboardMediumSmall"
			"maxchars"		"-1"
		}

		"HudStopWatchObjectiveShadedBar"
		{
			"ControlName"		"ImagePanel"
			"fieldName"		"HudStopWatchObjectiveShadedBar"
			"xpos"			"25"
			"ypos"			"65"
			"zpos"			"2"
			"wide"			"225"
			"tall"			"2"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"	
			"fillcolor"		"125 125 125 125"
			"PaintBackgroundType"	"0"
		}

		"HudStopWatchObjectiveText2"
		{
			"ControlName"		"CExRichText"
			"fieldName"		"HudStopWatchObjectiveText2"
			"xpos"			"25"	[$WIN32]
			"ypos"			"75"	[$WIN32]
			"wide"			"225"
			"tall"			"165"
			"wrap"			"1"
			"autoResize"		"1"
			"pinCorner"		"1"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		""
			"textAlignment"		"west"
			"font"			"ScoreboardMediumSmall"
			"maxchars"		"-1"
		}
	}	
}
