"Resource/UI/LeaderboardEntry.res"
{
	"LeaderboardEntry"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"LeaderboardEntry"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"f0"
		"tall"			"30"
		"visible"		"1"
		"proportionaltoparent"	"1"

		"Border"
		{
			"ControlName"			"EditablePanel"
			"fieldName"				"Border"
			"xpos"					"0"
			"ypos"					"0"
			"wide"					"f0"
			"tall"					"f0"
			"visible"				"1"
			"border"				"MainMenuBGBorderAlpha"
			"proportionaltoparent"	"1"
		}
	
		"Background"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"AvatarPanel0"
			"xpos"			"5"
			"ypos"			"cs-0.5"	
			"zpos"			"2"
			"wide"			"20"
			"tall"			"20"
			"visible"		"1"
			"PaintBackgroundType"	"2"
			"bgcolor_override"	"117 107 94 255"
			"proportionaltoparent"	"1"
		}

		"AvatarImage"
		{
			"ControlName"	"CAvatarImagePanel"
			"fieldName"		"AvatarImage"
			"xpos"			"7"
			"ypos"			"cs-0.5"
			"zpos"			"3"
			"wide"			"16"
			"tall"			"16"
			"visible"		"1"
			"enabled"		"1"
			"image"			""
			"scaleImage"	"1"	
			"color_outline"	"52 48 45 255"
			"proportionaltoparent"	"1"
		}

		"UserName"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"UserName"
			"font"			"HudFontSmallestBold"
			"labelText"		"%username%"
			"textAlignment"	"west"
			"xpos"			"35"
			"ypos"			"cs-0.5"
			"zpos"			"2"
			"wide"			"250"
			"tall"			"16"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"wrap"			"0"
			"proportionaltoparent"	"1"
		}

		"Score"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Score"
			"font"			"HudFontSmallestBold"
			"labelText"		"%score%"
			"textAlignment"	"east"
			"xpos"			"r200"
			"ypos"			"cs-0.5"
			"zpos"			"2"
			"wide"			"100"
			"tall"			"16"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"wrap"			"0"
			"proportionaltoparent"	"1"
		}		

		"ProgressContainer"
		{
			"ControlName"			"EditablePanel"
			"fieldName"				"ProgressContainer"
			"xpos"					"rs1.15"
			"ypos"					"cs-0.5"
			"zpos"					"100"
			"wide"					"80"
			"tall"					"10"
			"visible"				"1"
			"border"				"MainMenuBGBorderAlpha"
			"proportionaltoparent"	"1"
		
			"ProgressToNextLevel"
			{
				"ControlName"		"ContinuousProgressBar"
				"fieldName"			"ProgressToNextLevel"
				"xpos"				"cs-0.5"
				"ypos"				"cs-0.5"
				"zpos"				"10"
				"wide"				"p0.9"
				"tall"				"p0.4"
				"autoResize"		"1"
				"pinCorner"			"0"
				"visible"			"1"
				"enabled"			"1"
				"fgcolor_override"	"89 81 71 255"
				"bgcolor_override"	"196 145 38 255"
				"proportionaltoparent" "1"

				"variable"			"var_progress"
			}

			"Overlay"
			{
				"ControlName"			"EditablePanel"
				"fieldName"				"Overlay"
				"xpos"					"0"
				"ypos"					"0"
				"zpos"					"1"
				"wide"					"f0"
				"tall"					"f0"
				"visible"				"1"
				"border"				"MainMenuBGBorderAlpha"
				"proportionaltoparent"	"1"
			}
		}
	}	
}