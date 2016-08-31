"Resource/UI/replaymessage.res"
{
	"ReplayMessagePanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"		"ReplayMessagePanel"
		"zpos"			"1000"
		"tall"			"84"	[$WIN32]
		"tall_minmode"			"0"	[$WIN32]
		"wide"			"200"
		"autoResize"		"0"
		"visible"		"1"
		"enabled"		"1"
	}
	
	"ReplayLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ReplayLabel"
		"font"			"HudFontMediumBold"
		"xpos"			"35"
		"ypos"			"2"
		"zpos"			"0"
		"wide"			"180"
		"tall"			"60"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#Replay_ReplayMsgTitle"
		"textAlignment"	"north-west"
		"fgcolor"		"235 226 202 255"
	}
	"MessageLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"MessageLabel"
		"font"			"ItemFontAttribLarge"
		"zpos"			"0"
		"wide"			"180"
		"tall"			"0"
		"visible"		"1"
		"enabled"		"1"
		"textAlignment"	"north"
		"fgcolor"		"235 226 202 255"
		"centerwrap"	"0"
		"wrap"			"1"
	}
	"Icon"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"Icon"
		"xpos"			"8"
		"ypos"			"5"
		"zpos"			"1"
		"wide"			"18"
		"tall"			"18"
		"visible"		"1"
		"enabled"		"1"
		"image"			"replay/replayicon"
		"scaleImage"	"1"	
	}		
}
