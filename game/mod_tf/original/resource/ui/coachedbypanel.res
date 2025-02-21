"Resource/UI/CoachedByPanel.res"
{
	"Background_Blue"
	{
		"ControlName"	"ScalableImagePanel"
		"fieldName"		"Background_Blue"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-2"
		"wide"			"252"
		"tall"	 		"40"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"image"			"../hud/color_panel_blu"
		"image_lodef"	"../hud/color_panel_blu"
		
		"src_corner_height"		"23"			// pixels inside the image
		"src_corner_width"		"23"
			
		"draw_corner_width"		"5"				// screen size of the corners ( and sides ), proportional
		"draw_corner_height" 	"5"
	}
	"Background_Red"
	{
		"ControlName"	"ScalableImagePanel"
		"fieldName"		"Background_Red"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-2"
		"wide"			"252"
		"tall"	 		"40"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"image"			"../hud/color_panel_red"
		"image_lodef"	"../hud/color_panel_red"
		
		"src_corner_height"		"23"			// pixels inside the image
		"src_corner_width"		"23"
			
		"draw_corner_width"		"5"				// screen size of the corners ( and sides ), proportional
		"draw_corner_height" 	"5"
	}

	"AvatarBGPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"AvatarBGPanel"
		"xpos"			"4"
		"ypos"			"4"
		"zpos"			"-1"
		"wide"			"36"
		"tall"			"36"
		"visible"		"1"
		"PaintBackgroundType"	"2"
		"bgcolor_override"	"117 107 94 255"
	}
	"AvatarImage"
	{
		"ControlName"	"CAvatarImagePanel"
		"fieldName"		"AvatarImage"
		"xpos"			"6"
		"ypos"			"6"
		"zpos"			"0"
		"wide"			"32"
		"tall"			"32"
		"visible"		"1"
		"enabled"		"1"
		"image"			""
		"scaleImage"	"1"	
		"color_outline"	"52 48 45 255"
	}
	
	"CoachNameLabel"
	{	
		"ControlName"	"Label"
		"fieldName"		"CoachNameLabel"
		"font"			"HudFontMediumSmallSecondary"
		"font_minmode"	"TFFontLarge"
		"xpos"			"42"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"110"
		"tall"			"25"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"	"West"
		"dulltext"		"0"
		"brighttext"	"0"
	}

	"KickCoachLabel"
	{	
		"ControlName"	"CExLabel"
		"fieldName"		"KickCoachLabel"
		"fgcolor"		"TanLight"
		"xpos"			"42"
		"ypos"			"30"
		"zpos"			"1"
		"wide"			"110"
		"tall"			"10"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"labelText"		"#TF_Coach_KickCoach"
		"textAlignment"	"West"
		"font"			"TFFontSmall"
	}

	"LikeCoachLabel"
	{	
		"ControlName"	"CExLabel"
		"fieldName"		"LikeCoachLabel"
		"fgcolor"		"TanLight"
		"xpos"			"42"
		"ypos"			"20"
		"zpos"			"1"
		"wide"			"110"
		"tall"			"10"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"wrap"			"1"
		"labelText"		"#TF_Coach_LikeCoach"
		"textAlignment"	"West"
		"font"			"TFFontSmall"
	}

}
