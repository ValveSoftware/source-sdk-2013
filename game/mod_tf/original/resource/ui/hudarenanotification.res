"Resource/UI/HudArenaNotification.res"
{
	"HudArenaNotificationBG"
	{
		"ControlName"		"CTFImagePanel"
		"fieldName"		"HudArenaNotificationBG"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-1"
		"wide"			"320"
		"tall"			"200"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../hud/death_panel_blue_bg"
		"scaleImage"		"1"
		"teambg_2"		"../hud/death_panel_red_bg"
		"teambg_3"		"../hud/death_panel_blue_bg"
	}

	"WarningLabel"
	{	
		"ControlName"		"Label"
		"fieldName"		"WarningLabel"
		"font"			"ScoreboardTeamScore"
		"xpos"			"40"
		"ypos"			"40"
		"zpos"			"1"
		"wide"			"50"
		"tall"			"50"
		"wide"			"230"	[$WIN32]
		"wide"			"180"	[$X360]
		"tall"			"55"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"labelText"		"!"
		"textAlignment"		"north"
	}
	"BalanceLabel"
	{	
		"ControlName"		"Label"
		"fieldName"		"BalanceLabel"
		"font"			"HudFontSmallBold"
		"xpos"			"75"
		"ypos"			"30"
		"ypos_hidef"	"23"
		"ypos_lodef"	"20"
		"zpos"			"1"
		"wide"			"230"	[$WIN32]
		"wide"			"180"	[$X360]
		"tall"			"55"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"labelText"		""
		"textAlignment"		"west"
	}

	"BalanceLabelTip"
	{	
		"ControlName"		"Label"
		"fieldName"		"BalanceLabelTip"
		"font"			"HudFontSmallest"
		"xpos"			"75"
		"ypos"			"75"
		"ypos_hidef"	"23"
		"ypos_lodef"	"20"
		"zpos"			"1"
		"wide"			"230"	[$WIN32]
		"wide"			"180"	[$X360]
		"tall"			"30"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"labelText"		"%notificationtip%"
		"textAlignment"		"west"
	}
	"SwitchImage"
	{
		"ControlName"		"CTFImagePanel"
		"fieldName"		"SwitchImage"
		"xpos"			"25"
		"ypos"			"23"
		"zpos"			"1"
		"wide"			"50"
		"tall"			"50"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"image"			"../hud/ico_teambalance"
		"scaleImage"		"1"
	}
}
