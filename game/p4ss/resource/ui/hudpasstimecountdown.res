"Resource/UI/HudPasstimeCountdown.res"
{
	"HudPasstimeCountdown"
	{
		"fieldName"		"HudPasstimeCountdown"
		"visible"		"1"
		"enabled"		"1"
		"xpos"			"cs-0.5"	[$WIN32]
		"ypos"			"r94"	[$WIN32]
		"zpos"			"20"
		"wide"			"20"
		"tall"			"50"

	}	

	"background"
	{
		"ControlName"	"CTFImagePanel"
		"fieldName"		"background"
		"xpos"			"12"
		"ypos"			"6"
		"zpos"			"0"
		"wide"			"76"
		"wide_minmode"	"56"
		"tall"			"38"
		"visible"		"0"
		"visible_minmode"	"0"
		"enabled"		"0"
		"image"			"../hud/misc_ammo_area_blue"
		"scaleImage"	"1"	
		"teambg_2"		"../hud/misc_ammo_area_red"
		"teambg_2_lodef"	"../hud/misc_ammo_area_red_lodef"
		"teambg_3"		"../hud/misc_ammo_area_blue"
		"teambg_3_lodef"	"../hud/misc_ammo_area_blue_lodef"				
	}

	"CountdownLabel"
	{
			"ControlName"	"CExLabel"
			"fieldName"		"CountdownLabel"
			"xpos"			"0"	
			"xpos_minmode"	"42"
			"ypos"			"0"
			"ypos_minmode"	"12"
			"zpos"			"3"
			"wide"			"20"
			"tall"			"20"
			"tall_lodef"	"28"
			"autoResize"	"1"
			"pinCorner"		"2"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"labelText"		"#P4SS_Countdown"
			"textAlignment"	"center"
			"dulltext"		"0"
			"brighttext"	"0"
			"font"			"MontserratBlack26"
			"font_minmode"	"MontserratSemibold16"
			"font_lodef"	"MontserratSemibold32"
			"fgcolor_override"	"255 255 246 255"
			"bgcolor_override"	"255 255 246 0"
		"textinsetx"		"0"

	}

}