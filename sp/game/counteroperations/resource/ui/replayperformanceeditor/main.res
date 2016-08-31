"Resource/UI/replayperformanceeditor/main.res"
{
	"ReplayPerformanceEditor"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"ReplayPerformanceEditor"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"proportional"	"1"
		"bgcolor_override"	"0 0 0 0"
		
		"right_margin_width"	"20"
		
		"PlayerCell"
		{
			"zpos"			"3"
			"bgcolor_override"	"255 107 95 255"
			"wide"			"10"
			"tall"			"10"
			"sound_depressed"	"UI/buttonclick.wav"
			"sound_released"	"UI/buttonclickrelease.wav"
			
			"SubImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SubImage"
				"visible"		"1"
				"enabled"		"1"
				"tileImage"		"0"
				"scaleImage"	"1"
				"wide"			"10"
				"tall"			"10"
			}				
		}
	}
	
	"BottomPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"BottomPanel"
		"tall"			"40"
		"visible"		"1"
		"bgcolor_override"	"0 0 0 255"
		
		"LeftLine"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"LeftLine"
			"xpos"			"c-107"
			"ypos"			"17"
			"wide"			"85"
			"tall"			"1"
			"visible"		"1"
			"enabled"		"1"
			"pinCorner"		"0"
			"bgcolor_override" "122 111 98 255"
			"paintbackground"	"1"
		}
		
		"RightLine"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"RightLine"
			"xpos"			"c22"
			"ypos"			"17"
			"wide"			"85"
			"tall"			"1"
			"visible"		"1"
			"enabled"		"1"
			"pinCorner"		"0"
			"bgcolor_override" "122 111 98 255"
			"paintbackground"	"1"
		}
	
		"PlayerCellsPanel"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"PlayerCellsPanel"
			"visible"		"1"
			"tall"			"40"
			"bgcolor_override"	"255 0 0 255"
			"paintbackground"	"0"
			
			"RedLabel"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"RedLabel"
				"zpos"			"3"
				"visible"		"1"
				"enabled"		"1"
				"font"			"PerformanceModeSmall"
				"LabelText"		"#Replay_Team0"
				"fgcolor_override" "251 246 220 255"
				"pinCorner"		"0"
			}
			
			"BlueLabel"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"BlueLabel"
				"zpos"			"3"
				"visible"		"1"
				"enabled"		"1"
				"font"			"PerformanceModeSmall"
				"LabelText"		"#Replay_Team1"
				"fgcolor_override" "251 246 220 255"
				"pinCorner"		"0"
			}
		}
	}
	
	"TimeScaleButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"TimeScaleButton"
		"ypos"			"0"
		"wide"			"40"
		"tall"			"40"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"timescale_showpanel"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_TimeScaleButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_timescale"
			"wide"			"40"
			"tall"			"40"
		}				
	}
	
	"CameraFirst"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"CameraFirst"
		"ypos"			"0"
		"wide"			"40"
		"tall"			"40"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"setcamera_first"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_FirstPersonButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_camera_first"
			"wide"			"40"
			"tall"			"40"
		}				
	}
	
	"CameraThird"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"CameraThird"
		"ypos"			"0"
		"wide"			"40"
		"tall"			"40"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"setcamera_third"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_ThirdPersonButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_camera_third"
			"wide"			"40"
			"tall"			"40"
		}
	}	
	
	"CameraFree"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"CameraFree"
		"xpos"			"c100"
		"ypos"			"0"
		"wide"			"40"
		"tall"			"40"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"setcamera_free"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_FreeCamButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_camera_free"
			"wide"			"40"
			"tall"			"40"
		}				
	}	
	
	"CurTimeLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"CurTimeLabel"
		"xpos"			"c-74"
		"ypos"			"0"
		"zpos"			"2"
		"visible"		"1"
		"enabled"		"1"
		"font"			"ReplayMediumBig"
		"fgcolor_override" "122 111 98 255"
		"LabelText"		"00:00"
	}
	
	"TotalTimeLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"TotalTimeLabel"
		"xpos"			"c35"
		"ypos"			"0"
		"zpos"			"2"
		"visible"		"1"
		"enabled"		"1"
		"font"			"ReplayMediumBig"
		"fgcolor_override" "122 111 98 255"
		"LabelText"		"00:00"
	}
	
	"PlayerNameLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"PlayerNameLabel"
		"zpos"			"3"
		"visible"		"1"
		"enabled"		"1"
		"font"			"PerformanceModeSmall"
		"LabelText"		""
		"fgcolor_override" "251 246 220 255"
	}
	
	"MouseHighlightPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"MouseHighlightPanel"
		"visible"		"0"
		"zpos"			"1"
		"bgcolor_override" "218 209 187 255"
	}
	
	"MouseTargetPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"MouseTargetPanel"
		"visible"		"0"
		"zpos"			"2"
		"bgcolor_override" "255 255 255 255"
	}
	
	"InButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"InButton"
		"xpos"			"c-105"
		"ypos"			"17"
		"wide"			"22"
		"tall"			"22"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"settick_in"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		"image_selectedcolor"	"230 128 128 255"
		
		"tipname"			"#Replay_EditorButtonTip_InButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_icon_in"
			"wide"			"22"
			"tall"			"22"
		}				
	}
	
	"GotoBeginningButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"GotoBeginningButton"
		"xpos"			"c-80"
		"ypos"			"17"
		"wide"			"22"
		"tall"			"22"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"goto_start"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_RwHardButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_icon_begin"
			"wide"			"22"
			"tall"			"22"
		}				
	}
	
	"RewindButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"RewindButton"
		"xpos"			"c-55"
		"ypos"			"17"
		"wide"			"22"
		"tall"			"22"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"goto_back"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_RwButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_icon_rewind"
			"wide"			"22"
			"tall"			"22"
		}				
	}
	
	"PlayButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"PlayButton"
		"xpos"			"c-25"
		"ypos"			"-6"
		"wide"			"48"
		"tall"			"48"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"play"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_PlayButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_control_play"
			"wide"			"48"
			"tall"			"48"
		}				
	}	
	
	"FastForwardButton"
	{
		"ControlName"	"CReplayEditorFastForwardButton"
		"fieldName"		"FastForwardButton"
		"xpos"			"c33"
		"ypos"			"17"
		"wide"			"22"
		"tall"			"22"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_FfButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_icon_forward"
			"wide"			"22"
			"tall"			"22"
		}				
	}
	
	"GotoEndButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"GotoEndButton"
		"xpos"			"c55"
		"ypos"			"17"
		"wide"			"22"
		"tall"			"22"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"goto_end"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		
		"tipname"			"#Replay_EditorButtonTip_FfHardButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_icon_end"
			"wide"			"22"
			"tall"			"22"
		}				
	}
	
	"OutButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"OutButton"
		"xpos"			"c80"
		"ypos"			"17"
		"wide"			"22"
		"tall"			"22"
		"zpos"			"3"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"Command"		"settick_out"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		"image_selectedcolor"	"230 128 128 255"
		
		"tipname"		"#Replay_EditorButtonTip_OutButton"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_icon_out"
			"wide"			"22"
			"tall"			"22"
		}				
	}
	
	"MenuButton"
	{
		"ControlName"	"CReplayButton"
		"fieldName"		"MenuButton"
		"xpos"			"5"
		"ypos"			"5"
		"zpos"			"3"
		"wide"			"25"
		"tall"			"25"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		""
		"font"			""
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"default"		"0"
		"Command"		"toggle_menu"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"paintbackground"	"0"
		"image_drawcolor"	"255 255 255 191"
		"image_armedcolor"	"255 255 255 255"
		"image_selectedcolor"	"230 128 128 255"
		
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"image"			"replay/replay_icon_menu"
			"wide"			"22"
			"tall"			"22"
		}	
	}	
	
	"ButtonTip"
	{
		"ControlName"	"CReplayTipLabel"
		"fieldName"		"ButtonTip"
		"LabelText"		""
		"wide"			"350"
		"tall"			"50"
		"zpos"			"100"
		"visible"		"0"
		"enabled"		"1"
		"font"			"ReplayMediumSmall"
		"centerwrap"	"1"
		"paintbackground"	"1"
		"bgcolor_override"	"60 53 45 255"
		"border"		"ButtonBorder"
	}
}
