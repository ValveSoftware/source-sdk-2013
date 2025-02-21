"Resource/UI/HudStopWatch.res"
{
	"HudStopWatchBG"
	{
		"ControlName"		"ScalableImagePanel"
		"fieldName"		"HudStopWatchBG"
		"xpos"			"0"
		"ypos"			"5"
		"zpos"			"-1"
		"wide"			"125"
		"tall"			"31"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../HUD/tournament_panel_brown"

		if_comp
		{
			"ypos"	"20"
		}


		"src_corner_height"	"23"				// pixels inside the image
		"src_corner_width"	"23"
		
		"draw_corner_width"	"7"				// screen size of the corners ( and sides ), proportional
		"draw_corner_height" 	"7"	
	}

	"StopWatchImageCaptureTime"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"StopWatchImageCaptureTime"
		"xpos"			"5"
		"ypos"			"12"
		"zpos"			"0"
		"wide"			"17"
		"tall"			"17"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../hud/ico_time_10"
		"scaleImage"		"1"	
		//"teambg_2"		"../hud/objectives_timepanel_red_bg"
		//"teambg_3"		"../hud/objectives_timepanel_blue_bg"		

		if_comp
		{
			"ypos"	"27"
		}
	}

	"ObjectiveStatusTimePanel"
	{
		"ControlName"			"EditablePanel"
		"fieldName"			"ObjectiveStatusTimePanel"
		"xpos"				"2"
		"ypos"				"-4"	[$WIN32]
		"zpos"				"1"
		"wide"				"110"
		"tall"				"150"
		"visible"			"0"
		"enabled"			"1"

		if_comp
		{
			"ypos"	"11"
		}

		"TimePanelValue"
		{
			"ControlName"		"CExLabel"
			"fieldName"		"TimePanelValue"
			"font"			"HudFontMediumSmall"
			"font_minmode"		"HudFontMediumSmall"
			"font_lodef"		"HudFontMedium"
			"fgcolor"		"TanLight"
			"xpos"			"23"
			"xpos_minmode"		"23"
			"xpos_hidef"		"114"
			"xpos_lodef"		"114"
			"ypos"			"11"
			"ypos_minmode"		"11"
			"ypos_hidef"		"15"
			"ypos_lodef"		"18"
			"zpos"			"3"
			"wide"			"45"
			"wide_minmode"		"45"
			"tall"			"31"
			"visible"		"1"
			"enabled"		"1"
			"textAlignment"		"center"
		}	
	}

	"StopWatchScoreToBeat"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"StopWatchScoreToBeat"
		"font"			"HudFontMediumBold"
		"labelText"		"%scoretobeat%"
		"textAlignment"		"east"
		"xpos"			"0"
		"ypos"			"6"
		"zpos"			"4"
		"wide"			"93"
		"tall"			"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"

		if_comp
		{
			"ypos"	"21"
		}
	}
	"StopWatchPointsLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"StopWatchPointsLabel"
		"font"			"HudFontSmallest"
		"labelText"		"%pointslabel%"
		"textAlignment"		"east"
		"xpos"			"0"
		"ypos"			"9"
		"zpos"			"4"
		"wide"			"120"
		"tall"			"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"0"

		if_comp
		{
			"ypos"	"24"
		}
	}

	"StopWatchLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"StopWatchLabel"
		"font"			"HudFontSmallest"
		"labelText"		"%stopwatchlabel%"
		"textAlignment"		"west"
		"xpos"			"25"
		"ypos"			"5"
		"zpos"			"4"
		"wide"			"90"
		"tall"			"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"wrap"			"1"

		if_comp
		{
			"ypos"	"20"
		}
	}

	"HudStopWatchDescriptionBG"
	{
		"ControlName"		"ScalableImagePanel"
		"fieldName"		"HudStopWatchDescriptionBG"
		"xpos"			"0"
		"ypos"			"27"
		"zpos"			"-1"
		"wide"			"125"
		"tall"			"19"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"image"			"../hud/objectives_timepanel_suddendeath"


		"src_corner_height"	"2"				// pixels inside the image
		"src_corner_width"	"2"
		
		"draw_corner_width"	"2"				// screen size of the corners ( and sides ), proportional
		"draw_corner_height" 	"2"	

		if_comp
		{
			"ypos"	"42"
		}
	}

	"StopWatchDescriptionLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"StopWatchDescriptionLabel"
		"font"			"ClockSubTextTiny"
		"labelText"		"%descriptionlabel%"
		"textAlignment"		"center"
		"xpos"			"0"
		"ypos"			"23"
		"zpos"			"4"
		"wide"			"125"
		"tall"			"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"wrap"			"0"

		if_comp
		{
			"ypos"	"38"
		}
	}
}