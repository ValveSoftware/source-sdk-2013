"Resource/UI/replayrenderoverlay.res"
{
	"BottomPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"BottomPanel"
		"visible"		"1"
		"bgcolor_override"	"46 43 42 255"
	}
	
	"FilenameLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"FilenameLabel"
		"font"			"DefaultVerySmall"
		"labelText"		""
		"textAlignment"	"west"
		"zpos"			"10"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "235 235 235 255"
	}
	
	"TitleLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"TitleLabel"
		"font"			"EconFontSmall"
		"labelText"		"#Replay_RenderOverlayText"
		"textAlignment"	"west"
		"zpos"			"10"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "200 80 60 255"
	}
	
	"ReplayRenderOverlay"
	{
		"ControlName"	"Frame"
		"fieldName"		"ReplayRenderOverlay"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"zpos"			"1000"
		"enabled"		"1"
		"tabPosition"	"0"
		"settitlebarvisible"	"0"
		"paintbackground"	"0"
	}
	
	"RenderProgress"
	{
		"ControlName"	"ProgressBar"
		"fieldName"		"RenderProgress"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"progress"		"0"
		"bgcolor_override" "117 107 95 255"
		"fgcolor_override" "255 255 255 255"
	}

	"ProgressLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ProgressLabel"
		"font"			"DefaultVerySmall"
		"labelText"		""
		"textAlignment"	"east"
		"zpos"			"10"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "235 235 235 255"
	}
	
	"CancelButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"CancelButton"
		"zpos"			"20"
		"wide"			"100"
		"tall"			"25"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#Replay_CancelRender"
		"font"			"EconFontSmall"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"confirmcancel"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}			
	
	"PreviewCheckButton"
	{
		"ControlName"	"CheckButton"
		"fieldName"		"PreviewCheckButton"
		"labelText"		"#Replay_RenderPreview"
		"Font"			"DefaultVerySmall"
		"textAlignment"	"west"
		"wrap"			"0"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"300"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"dulltext"		"0"
		"brighttext"	"0"
		"tabPosition"	"10"
	}
}