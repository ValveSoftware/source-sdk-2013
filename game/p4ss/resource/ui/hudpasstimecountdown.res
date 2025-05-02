"Resource/UI/HudPasstimeCountdown.res"
{
	"HudPasstimeCountdown"
	{
		"fieldName"		"HudPasstimeCountdown"
		"visible"		"1"
		"enabled"		"1"
		"xpos"			"162"	[$WIN32]
		"ypos"			"r52"	[$WIN32]
		"wide"			"100"
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
		"visible"		"1"
		"visible_minmode"	"0"
		"enabled"		"1"
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
			"xpos"			"50"
			"xpos_minmode"	"42"
			"ypos"			"18"
			"ypos_minmode"	"12"
			"zpos"			"2"
			"wide"			"35"
			"tall"			"20"
			"tall_lodef"	"28"
			"autoResize"	"1"
			"pinCorner"		"2"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"labelText"		"#P4SS_Countdown"
			"textAlignment"	"west"
			"dulltext"		"0"
			"brighttext"	"0"
			"font"			"HudFontMedium"
			"font_minmode"	"HudFontSmall"
			"font_lodef"	"HudFontMediumBig"
	}
}