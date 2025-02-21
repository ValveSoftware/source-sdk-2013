"Resource/UI/SpectatorCoach.res"
{
	"Spectator"
	{
		"ControlName"		"Frame"
		"fieldName"		"Spectator"
		"tall"			"480"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
	}
	"specgui"
	{
	}
	"topbar"
	{
		"ControlName"		"Panel"
		"fieldName"		"TopBar"
		"xpos"			"0"
		"ypos"			"0"
		"tall"			"70"	[$WIN32]
		"tall_minmode"			"70"	[$WIN32]
		"wide"			"f0"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"border"		"TFThinLineBorder"
	}

	"AvatarBGPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"AvatarBGPanel"
		"xpos"			"5"
		"ypos"			"5"
		"zpos"			"-1"
		"wide"			"42"
		"tall"			"42"
		"visible"		"1"
		"PaintBackgroundType"	"2"
		"bgcolor_override"	"117 107 94 255"
	}
	"AvatarImage"
	{
		"ControlName"	"CAvatarImagePanel"
		"fieldName"		"AvatarImage"
		"xpos"			"10"
		"ypos"			"10"
		"zpos"			"0"
		"wide"			"32"
		"tall"			"32"
		"visible"		"1"
		"enabled"		"1"
		"image"			""
		"scaleImage"	"1"	
		"color_outline"	"52 48 45 255"
	}
	"HealthPositioning"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"HealthPositioning"
		"xpos"			"23"
		"ypos"			"23"
		"zpos"			"1"
		"wide"			"250"
		"tall"			"120"
	}

	"Crosshair"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"Crosshair"
		"xpos"			"c-8"
		"ypos"			"c-8"
		"zpos"			"-100"
		"wide"			"16"
		"tall"			"16"
		"visible"		"1"
		"enabled"		"1"
		"image"			"crosshairs/default"
		"scaleImage"	"1"
	}

	"CoachingLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"CoachingLabel"
		"font"			"HudFontSmall"
		"font_hidef"		"HudFontMedium"
		"xpos"			"57"
		"ypos"			"5"
		"wide"			"240"
		"tall"			"20"
		"tall_hidef"		"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"visible_minmode" "1"
		"enabled"		"1"
		"labelText"		"%student_name%"
		"textAlignment"		"west"
	}
	"MapLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"MapLabel"
		"font"			"HudFontSmall"
		"font_hidef"		"HudFontMedium"
		"xpos"			"57"
		"ypos"			"20"
		"wide"			"240"
		"tall"			"20"
		"tall_hidef"		"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"west"
	}	
	"DistanceLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"DistanceLabel"
		"font"			"HudFontSmall"
		"font_hidef"		"HudFontMedium"
		"xpos"			"57"
		"ypos"			"35"
		"wide"			"240"
		"tall"			"20"
		"tall_hidef"		"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"visible_minmode" "1"
		"enabled"		"1"
		"labelText"		"%student_distance%"
		"textAlignment"		"west"
	}

	"DirectionsLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"DirectionsLabel"
		"xpos"			"r200"		[$WIN32]
		"ypos"			"0"	[$WIN32]
		"wide"			"200"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"enabled"		"1"
		"labelText"		"#TF_Coach_StudentCommands"
		"textAlignment"		"west"
		"font"			"SpectatorKeyHints"		
	}

	"SwitchCamModeKeyLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"SwitchCamModeKeyLabel"
		"xpos"			"r200"		[$WIN32]
		"ypos"			"10"	[$WIN32]
		"wide"			"60"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"east"
		"font"			"SpectatorKeyHints"		
	}
	"SwitchCamModeLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"SwitchCamModeLabel"
		"xpos"			"r125"	[$WIN32]
		"ypos"			"10"	[$WIN32]
		"wide"			"125"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible"		"0"	[$X360]
		"enabled"		"1"
		"labelText"		"#TF_Coach_ControlView"
		"textAlignment"		"west"
		"font"			"SpectatorKeyHints"
	}
	"CycleTargetFwdKeyLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"CycleTargetFwdKeyLabel"
		"xpos"			"r200"	[$WIN32]
		"ypos"			"20"	[$WIN32]
		"wide"			"60"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"east"
		"font"			"SpectatorKeyHints"
	}
	"CycleTargetFwdLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"CycleTargetFwdLabel"
		"xpos"			"r125"	[$WIN32]
		"ypos"			"20"	[$WIN32]
		"wide"			"125"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		"#TF_Coach_AttackDesc"
		"textAlignment"		"west"
		"font"			"SpectatorKeyHints"
	}
	"CycleTargetRevKeyLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"CycleTargetRevKeyLabel"
		"xpos"			"r200"	[$WIN32]
		"ypos"			"30"	[$WIN32]
		"wide"			"60"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"east"
		"font"			"SpectatorKeyHints"
	}
	"CycleTargetRevLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"CycleTargetRevLabel"
		"xpos"			"r125"	[$WIN32]
		"ypos"			"30"	[$WIN32]	
		"wide"			"125"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		"#TF_Coach_DefendDesc"
		"textAlignment"		"west"
		"font"			"SpectatorKeyHints"
	}
	"Slot1KeyLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"Slot1KeyLabel"
		"xpos"			"r200"	[$WIN32]
		"ypos"			"40"	[$WIN32]
		"wide"			"60"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		"%coach_command_1%"
		"textAlignment"		"east"
		"font"			"SpectatorKeyHints"
	}
	"Slot1Label"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"Slot1Label"
		"xpos"			"r125"	[$WIN32]
		"ypos"			"40"	[$WIN32]	
		"wide"			"125"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		"#TF_Coach_Slot1Desc"
		"textAlignment"		"west"
		"font"			"SpectatorKeyHints"
	}
	"Slot2KeyLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"Slot2KeyLabel"
		"xpos"			"r200"	[$WIN32]
		"ypos"			"50"	[$WIN32]
		"wide"			"60"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		"%coach_command_2%"
		"textAlignment"		"east"
		"font"			"SpectatorKeyHints"
	}
	"Slot2Label"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"Slot2Label"
		"xpos"			"r125"	[$WIN32]
		"ypos"			"50"	[$WIN32]	
		"wide"			"125"	[$WIN32]
		"tall"			"20"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"	[$WIN32]
		"visible_minmode"		"1"
		"enabled"		"1"
		"labelText"		"#TF_Coach_Slot2Desc"
		"textAlignment"		"west"
		"font"			"SpectatorKeyHints"
	}

	"itempanel"
	{
		"ControlName"	"CItemModelPanel"
		"fieldName"		"itempanel"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"10"
		"wide"			"190"
		"tall"			"100"
		"visible"		"0"
		"bgcolor_override"		"255 255 255 0"
		"PaintBackgroundType"	"0"
		
		"model_ypos"		"10"
		"model_center_x"	"1"
		"model_wide"		"90"
		"model_tall"		"60"
		
		"text_xpos"		"10"
		"text_ypos"		"10"
		"text_wide"		"170"
		"text_center"	"1"
		
		"max_text_height"	"100"
		"padding_height"	"10"
		"resize_to_text"	"1"
		"text_forcesize"	"2"
		
		"itemmodelpanel"
		{
			"fieldName"		"itemmodelpanel"
			"use_item_rendertarget" "0"
			"useparentbg"		"1"
		}
		
		"ItemLabel"
		{	
			"ControlName"	"Label"
			"fieldName"		"ItemLabel"
			"font"			"DefaultSmall"
			"xpos"			"10"
			"ypos"			"3"
			"zpos"			"1"
			"wide"			"270"
			"tall"			"9"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		"#FreezePanel_Item"
			"textAlignment"	"Left"
			"dulltext"		"0"
			"brighttext"	"0"
		}
		
		"attriblabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"attriblabel"
			"font"			"ItemFontAttribLarge"
			"xpos"			"0"
			"ypos"			"30"
			"zpos"			"2"
			"wide"			"140"
			"tall"			"60"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		"%attriblist%"
			"textAlignment"	"south"
			"fgcolor"		"235 226 202 255"
			"centerwrap"	"1"
		}
	}	
}
