"Resource/UI/MvMVictoryMannUpPanel.res"
{			
	//Mouse over panel
	"mouseoveritempanel"
	{
		"ControlName"	"CItemModelPanel"
		"fieldName"		"mouseoveritempanel"
		"xpos"			"0"
		"ypos"			"600"
		"zpos"			"9999"
		"wide"			"250"
		"tall"			"300"
		"visible"		"0"
		"bgcolor_override"		"0 0 0 0"
		"noitem_textcolor"		"117 107 94 255"
		"PaintBackgroundType"	"2"
		"paintborder"	"1"
		
		"text_ypos"			"0"
		"text_center"		"1"
		"model_hide"		"1"
		"resize_to_text"	"1"
		"padding_height"	"15"
		
		"attriblabel"
		{
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
			"textAlignment"	"center"
			"fgcolor"		"117 107 94 255"
			"centerwrap"	"1"
		}
	}

	"DoneButton"
	{
		"ControlName"	"CExImageButton"
		"fieldName"		"DoneButton"
		"xpos"			"c175"
		"ypos"			"372"
		"zpos"			"100"
		"wide"			"106"
		"tall"			"25"
		"autoResize"	"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#DoneButton"
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"default"		"1"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		
		"image_drawcolor"	"235 226 202 255"
		"Command"		"done"
	}

	"ParticlePanel"
	{
		"ControlName"	"CTFParticlePanel"
		"fieldName"		"ParticlePanel"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"200"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
	}


	"OutterBackground"
	{
		"ControlName"	"Panel"
		"fieldName"		"OutterBackground"
		"xpos"			"0"
		"ypos"			"90"
		"zpos"			"-8"
		"wide"			"f0"
		"tall"			"320"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
			
		"PaintBackgroundType" "2"
		"bgcolor_override"	"42 39 37 255"
	}

	"HeaderLine"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"HeaderLine"
		"xpos"			"0"
		"ypos"			"-10"
		"zpos"			"2"
		"wide"			"f0"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_solid_line"
		"scaleImage"	"1"

		"pin_to_sibling"               "OutterBackground"
		"pin_corner_to_sibling"        "2"          
		"pin_to_sibling_corner"        "0"   
	}	

	"FooterLine"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"FooterLine"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"f0"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_solid_line"
		"scaleImage"	"1"

		"pin_to_sibling"               "OutterBackground"
		"pin_corner_to_sibling"        "0"          
		"pin_to_sibling_corner"        "2" 
	}	
	

	"TabContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"TabContainer"
		"xpos"			"c-118"
		"ypos"			"55"
		"zpos"			"100"
		"wide"			"399"
		"tall"			"40"
		"visible"		"1"

		"PlayerTab1"
		{
			"ControlName"	"CMvMVictoryMannUpPlayerTab"
			"fieldName"		"PlayerTab1"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"-5"
			"wide"			"63"
			"tall"			"f0"
			"autoResize"	"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"bgcolor_override"	"42 41 39 255"
			"PaintBackgroundType"	"2"
			"proportionalToParent"	"1"
		}

		"PlayerTab2"
		{
			"ControlName"	"CMvMVictoryMannUpPlayerTab"
			"fieldName"		"PlayerTab2"
			"xpos"			"68"
			"ypos"			"0"
			"zpos"			"-5"
			"wide"			"62"
			"tall"			"f0"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"bgcolor_override"	"77 72 68 255"
			"PaintBackgroundType"	"2"
			"proportionalToParent"	"1"
		}

		"PlayerTab3"
		{
			"ControlName"	"CMvMVictoryMannUpPlayerTab"
			"fieldName"		"PlayerTab3"
			"xpos"			"135"
			"ypos"			"0"
			"zpos"			"-5"
			"wide"			"62"
			"tall"			"f0"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"bgcolor_override"	"77 72 68 255"
			"PaintBackgroundType"	"2"
			"proportionalToParent"	"1"
		}

		"PlayerTab4"
		{
			"ControlName"	"CMvMVictoryMannUpPlayerTab"
			"fieldName"		"PlayerTab4"
			"xpos"			"202"
			"ypos"			"0"
			"zpos"			"-5"
			"wide"			"62"
			"tall"			"f0"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"bgcolor_override"	"77 72 68 255"
			"PaintBackgroundType"	"2"
			"proportionalToParent"	"1"
		}

		"PlayerTab5"
		{
			"ControlName"	"CMvMVictoryMannUpPlayerTab"
			"fieldName"		"PlayerTab5"
			"xpos"			"269"
			"ypos"			"0"
			"zpos"			"-5"
			"wide"			"62"
			"tall"			"f0"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"bgcolor_override"	"77 72 68 255"
			"PaintBackgroundType"	"2"
			"proportionalToParent"	"1"
		}

		"PlayerTab6"
		{
			"ControlName"	"CMvMVictoryMannUpPlayerTab"
			"fieldName"		"PlayerTab6"
			"xpos"			"336"
			"ypos"			"0"
			"zpos"			"-5"
			"wide"			"62"
			"tall"			"f0"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"		"0"
			"bgcolor_override"	"77 72 68 255"
			"PaintBackgroundType"	"2"
			"proportionalToParent"	"1"
		}
	}
		
	"MainPanelContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"MainPanelContainer"
		"xpos"			"c-230"
		"ypos"			"140"
		"zpos"			"-10"
		"wide"			"f0"
		"tall"			"480"
		"visible"		"1"


			
		"NoItemServerContainer"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"NoItemServerContainer"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"530"
			"tall"			"480"
			"visible"		"1"
			
			"NoItemServerHeader"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"NoItemServer"
				"font"			"HudFontMediumSmallBold"
				"labelText"		"#TF_PVE_Server_GCDownHeader"
				"textAlignment" "center"
				"xpos"			"0"
				"ypos"			"100"
				"wide"			"530"
				"fgcolor"		"tanlight"
			}
			
			"NoItemServerMessage"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"NoItemServer"
				"font"			"HudFontSmall"
				"labelText"		"#TF_PVE_Server_GCDownMessage"
				"textAlignment" "center"
				"xpos"			"0"
				"ypos"			"120"
				"wide"			"530"
				"tall"			"40"
				"centerwrap"		"1"
				"fgcolor"		"tanlight"
			}
		}
	}
}
