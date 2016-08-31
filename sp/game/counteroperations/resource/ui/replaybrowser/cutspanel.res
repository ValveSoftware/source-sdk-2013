"Resource/UI/replaybrowser/cutspanel.res"
{
	"CutsPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"CutsPanel"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"paintbackground"	"0"
		
		"cut_button_width"	"70"
		"cut_button_height"	"53"
		"cut_button_buffer"	"3"
		"cut_button_space"	"4"
		"cut_button_space_wide"	"10"
		"top_margin_height"	"5"
		"button_start_y"	"18"
		"name_label_top_margin"	"5"
		
		"button_settings"
		{
			"paintbackground"		"1"
			"image_drawcolor"		"255 255 255 255"
			"image_armedcolor"		"255 255 255 255"
			"image_selectedcolor"	"255 255 255 255"
			
			"sound_depressed"		"UI/buttonclick.wav"
			"sound_released"		"UI/buttonclickrelease.wav"
		
			"SubImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SubImage"
				"zpos"			"1"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"
			}				
			
			"addtorenderqueuebutton_settings"
			{
				"xpos"				"4"
				"ypos"				"37"
				"wide"				"12"
				"tall"				"12"
				"zpos"				"3"
				"textAlignment"		"center"
				"visible"			"0"
			}
		}
	}
	
	"VerticalLine"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"VerticalLine"
		"wide"				"1"
		"PaintBackground"	"1"
		"bgcolor_override" "122 111 98 255"
	}
	
	"NoCutsLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"NoCutsLabel"
		"font"			"ReplayMediumSmall"
		"labelText"		"#Replay_NoCuts"
		"wide"			"200"
		"tall"			"58"
		"zpos"			"1"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"centerwrap"	"1"
		"fgcolor_override"	"117 107 94 255"
	}
	
	"PrevButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"PrevButton"
		"visible"		"0"
		"wide"			"15"
		"tall"			"15"
		"labelText"		"<"
		"font"			"EconFontSmall"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"prevpage"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}
	
	"NextButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"NextButton"
		"visible"		"0"
		"wide"			"15"
		"tall"			"15"
		"labelText"		">"
		"font"			"EconFontSmall"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"nextpage"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}
	
	"OriginalLabel"
	{
		"ControlName"	"CExLabel"
		"FieldName"		"OriginalLabel"
		"Font"			"ReplayMediumSmall"
		"TextAlignment"	"west"
		"fgcolor_override"	"117 107 94 255"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"65"
		"tall"			"10"
		"labelText"		"#Replay_Original"
	}
	
	"CutsLabel"
	{
		"ControlName"	"CExLabel"
		"FieldName"		"CutsLabel"
		"Font"			"ReplayMediumSmall"
		"TextAlignment"	"west"
		"fgcolor_override"	"117 107 94 255"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"120"
		"tall"			"10"
		"labelText"		"#Replay_Performances"
	}
	
	"NameLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"NameLabel"
		"font"			"ReplayBrowserSmallest"
		"labelText"		""
		"zpos"			"1"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"centerwrap"	"0"
		"fgcolor_override"	"117 107 94 255"
	}
	
}
