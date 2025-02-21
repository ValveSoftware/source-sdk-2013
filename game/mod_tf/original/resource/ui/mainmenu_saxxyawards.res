"Resource/UI/MainMenu_SaxxyAwards.res"
{
	"Background"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"Background"
		
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-100"
		"wide"			"f0"
		"tall"			"480"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		
		"PaintBackgroundType"	"2"
		"bgcolor_override" "0 0 0 255"
	}
	
	"BackgroundTexture"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BackgroundTexture"
		
		"xpos"			"c-400"
		"ypos"			"100"
		"zpos"			"-100"
		"wide"			"360"
		"tall"			"340"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		
		"tileImage"		"0"
		"scaleImage"	"1"
		"scaleAmount"	"0"
		"image"			"replay/saxxycontest/bg_texture"
		
		"PaintBackgroundType"	"2"
		"bgcolor_override" "0 0 0 255"
	}
	
	"SpotlightPanel"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"SpotlightPanel"
		
		"tileImage"		"0"
		"scaleImage"	"1"
		"scaleAmount"	"0"
		"image"			"replay/saxxycontest/mainmenu_spotlight"
		
		"xpos"			"0"
		"ypos"			"297"
		"zpos"			"1000"
		"wide"			"128"
		"tall"			"128"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
	}
	
	"InfoLabel"
	{
		"ControlName"		"CExLabel"
		"FieldName"			"InfoLabel"
		"LabelText"			"#Replay_Contest_Info"
		"zpos"				"1000"
		"font"				"HudFontSmallestBold"
		"visible"			"0"
		"fgcolor_override" "220 220 220 255"
		"centerwrap"		"1"
		
		"xpos"			"c20"
		"ypos"			"180"
		"zpos"			"100"
		"wide"			"120"
		"tall"			"22"
	}

	"ContestOverLabel"
	{
		"ControlName"		"CExLabel"
		"FieldName"			"ContestOverLabel"
		"LabelText"			"#Replay_Contest_Over"
		"zpos"				"1000"
		"font"				"HudFontSmallestBold"
		"visible"			"0"
		"fgcolor_override" "220 220 220 255"
		"centerwrap"		"1"
		
		"xpos"			"c15"
		"ypos"			"385"
		"zpos"			"100"
		"wide"			"120"
		"tall"			"22"
	}

	"SubmitButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"SubmitButton"
		"xpos"			"c40"
		"ypos"			"385"
		"zpos"			"200"
		"wide"			"95"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"2"
		"labelText"		"#Replay_Contest_SubmitEntry"
		"font"			"HudFontSmallestBold"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"submit"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		"border_default"	"ButtonBorder"
		
        "defaultBgColor_override"		"56 74 94 255"
        "armedBgColor_override"			"67 89 113 255"
        "depressedBgColor_override"		"56 74 94 255"
	}		
	
	"DetailsButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"DetailsButton"
		"xpos"			"c140"
		"ypos"			"385"
		"zpos"			"200"
		"wide"			"95"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"2"
		"labelText"		"#Replay_Contest_Details"
		"font"			"HudFontSmallestBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"viewdetails"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		"border_default"	"ButtonBorder"
		
        "defaultBgColor_override"		"56 74 94 255"
        "armedBgColor_override"			"67 89 113 255"
        "depressedBgColor_override"		"56 74 94 255"
	}		
	
	"StageBackground"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"StageBackground"
		
		"tileImage"		"0"
		"scaleImage"	"1"
		"scaleAmount"	"0"
		"image"			"replay/saxxycontest/saxxy_bg"
		
		"xpos"			"c-35"
		"ypos"			"130"
		"zpos"			"90"
		"wide"			"350"
		"tall"			"290"

		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
	}
	
	"CurtainsPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"CurtainsPanel"
		
		"xpos"			"c-35"
		"ypos"			"130"
		"zpos"			"102"
		"wide"			"350"
		"tall"			"290"
		
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		
		"PaintBackgroundType"	"2"
		"bgcolor_override" "0 0 0 0"
		
		"SaxxyTitleImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SaxxyTitleImage"
			"visible"		"1"
			"enabled"		"1"
			"tileImage"		"0"
			"scaleImage"	"1"
			"scaleAmount"	"0"
			"image"			"replay/saxxycontest/logo"
			
			"xpos"			"60"
			"ypos"			"40"
			"wide"			"240"
			"tall"			"32"
			"zpos"			"94"
		}		
	
		"SaxxyModelPanel"
		{
			"ControlName"	"CBaseModelPanel"
			"fieldName"		"SaxxyModelPanel"
			
			"xpos"			"125"
			"ypos"			"60"
			"zpos"			"95"		
			"wide"			"100"
			"tall"			"195"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"

			"fov"			"20"
			"allow_rot"		"0"
			"start_framed"	"1"
			"spotlight"		"0"
				
			"model"
			{
				"modelname"		""
				"force_pos"		"1"
				"spotlight"		"1"
			}
		}
	
		"CurtainsLeft"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"CurtainsLeft"
			
			"tileImage"		"0"
			"scaleImage"	"1"
			"scaleAmount"	"0"
			"image"			"replay/mainmenu_curtain_L"
			
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"101"
			"wide"			"175"
			"tall"			"290"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			
			"PaintBackgroundType"	"2"
			"bgcolor_override" "0 0 0 255"
		}
		
		"CurtainsRight"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"CurtainsRight"
			
			"tileImage"		"0"
			"scaleImage"	"1"
			"scaleAmount"	"0"
			"image"			"replay/mainmenu_curtain_R"
			
			"xpos"			"175"
			"ypos"			"0"
			"zpos"			"101"
			"wide"			"175"
			"tall"			"290"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
		}
		
		"CurtainsStatic"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"CurtainsStatic"
			
			"tileImage"		"0"
			"scaleImage"	"1"
			"scaleAmount"	"0"
			"image"			"replay/mainmenu_curtains_static"
			
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"102"
			"wide"			"350"
			"tall"			"290"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
		}
		
		"TestPanel"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"TestPanel"
			
			"xpos"	"50"
			"ypos"	"65"
			"wide"	"250"
			"tall"	"120"
			"zpos"	"1002"
			
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"0"
			"enabled"		"1"
			
			"PaintBackgroundType"	"2"
			"bgcolor_override" "0 255 0 255"
		}
	}
}