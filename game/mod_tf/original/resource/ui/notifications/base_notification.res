"Resource/UI/notifications/base_notification.res"
{
	"Notification_Background"
	{
		"ControlName"	"CImagePanel"
		"fieldName"		"Notification_Background"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-1"
		"wide"			"140"
		"tall"			"44"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../hud/score_panel_blue_bg"
		"scaleImage"	"1"
	}
	
	"Notification_Icon"	
	{
		"ControlName"	"CIconPanel"
		"fieldName"		"Notification_Icon"
		"xpos"			"7"
		"ypos"			"9"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"	"1"	
		"icon"			"ico_notify_sixty_seconds"
	}
	
	"Notification_Label"
	{	
		"ControlName"	"CExLabel"
		"fieldName"		"Notification_Label"
		"font"			"Default"
		"xpos"			"30"
		"ypos"			"9"
		"wide"			"200"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"60 seconds until gates open"
		"textAlignment"	"West"
		"dulltext"		"0"
		"brighttext"	"0"
	}
}