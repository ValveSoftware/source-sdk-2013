"Resource/UI/MatchmakingCategoryPanel.res"
{
	"MatchmakingCategoryPanel"
	{
		"fieldName"				"MatchmakingCategoryPanel"
		"xpos"					"0"
		"ypos"					"0"
		"wide"					"f0"
		"tall"					"50"
		"proportionaltoparent"	"1"

		"collapsed_height"	"57"
		"resize_time"	"0.2"
	
	}

	"TopContainer"
	{
		"Controlname"	"EditablePanel"
		"fieldName"		"TopContainer"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"f0"
		"tall"			"p1.17"
		"visible"		"1"
		"enabled"		"1"
		"proportionaltoparent"	"1"

		"BGColor"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"BGColor"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"-1"
			"wide"			"f0"
			"tall"			"f0"
			"visible"		"1"
			"enabled"		"1"
			"mouseinputenabled"	"0"
		
			"proportionaltoparent"	"1"
			"bgcolor_override"	"0 0 0 255"
		}

		"BGImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"BGImage"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"0"
			"wide"			"o4"
			"tall"			"f0"
			"visible"		"1"
			"enabled"		"1"
			"scaleImage"	"1"	
			"proportionaltoparent"	"1"
		}

		"EntryToggleButton"
		{
			"ControlName"	"CExImageButton"
			"fieldName"		"EntryToggleButton"
			"xpos"			"0"
			"ypos"			"rs1"
			"zpos"			"100"
			"wide"			"f0"
			"tall"			"9"
			"proportionaltoparent"	"1"

			"actionsignallevel"	"2"
			"command"		"toggle_collapse"
			"labeltext"		"#TF_Casual_ViewMaps"
			"textAlignment"	"west"
			"font"	"MMenuPlayListDesc"
			"textinsetx"	"40"

			"stay_armed_on_click"	"1"

			"border_default"	"NoBorder"
			"defaultBgColor_override"	"235 226 202 20"

			"border_armed"		"NoBorder"
			"armedBgColor_override"	"LightOrange"
			"selectedBGColor_override" "Orange"

			"sound_armed"		"ui/item_info_mouseover.wav"
			"sound_depressed"	"UI/buttonclick.wav"
			"sound_released"	"UI/buttonclickrelease.wav"

			"image_default"	"glyph_expand"

			"button_activation_type"	"1"

			"SubImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"SubImage"
				"xpos"			"6"
				"ypos"			"cs-0.5"
				"zpos"			"1"
				"wide"			"o1"
				"tall"			"p.9"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"

				"proportionaltoparent"	"1"
			}	
		}

		"Shade"
		{
			"fieldName"			"Shade"
			"ControlName"		"EditablePanel"
			"xpos"				"cs-0.5"
			"ypos"				"0"
			"zpos"				"0"
			"wide"				"f0"
			"tall"				"57"
			"mouseinputenabled"	"0"

			"proportionaltoparent"	"1"
			"bgcolor_override"	"0 0 0 0"
		}

		"Checkbutton"
		{
			"ControlName"		"CExCheckButton"
			"fieldName"		"Checkbutton"
			"xpos"		"r27"
			"ypos"		"-1"
			"zpos"		"3"
			"wide"		"25"
			"tall"		"20"
			"proportionaltoparent"	"1"
			"labeltext"		""
			"smallcheckimage"	"1"

			"sound_depressed"	"UI/buttonclickrelease.wav"	
			"button_activation_type"	"1"
		}

		"Title"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Title"
			"xpos"			"rs1-24"
			"ypos"			"2"
			"zpos"			"3"
			"wide"			"200"
			"tall"			"15"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		"%title_token%"
			"textinsetx"	"5"
			"use_proportional_insets" "1"
			"font"			"HudFontSmallestBold"
			"textAlignment"	"east"
			"dulltext"		"0"
			"brighttext"	"0"
			"default"		"1"
			"proportionaltoparent" "1"
			"mouseinputenabled"	"0"

			"fgcolor"		"TanLight"
		}	

		"TitleShadow"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"TitleShadow"
			"xpos"			"rs1-23"
			"ypos"			"3"
			"zpos"			"2"
			"wide"			"200"
			"tall"			"15"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		"%title_token%"
			"textinsetx"	"5"
			"use_proportional_insets" "1"
			"font"			"HudFontSmallestBold"
			"textAlignment"	"east"
			"dulltext"		"0"
			"brighttext"	"0"
			"default"		"1"
			"proportionaltoparent" "1"
			"mouseinputenabled"	"0"

			"fgcolor_override"		"Black"
		}

		"DescLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"DescLabel"
			"font"			"MMenuPlayListDesc"
			"labelText"		"%desc_token%"
			"textAlignment"	"north-west"
			"xpos"			"rs1-5"
			"ypos"			"18"
			"zpos"			"2"
			"wide"			"120"
			"tall"			"40"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"

			"wrap"			"1"
			"fgcolor_override" "TanLight"
			"proportionaltoparent" "1"
			"mouseinputenabled"	"0"
		}

		"DescLabelShadow"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"DescLabelShadow"
			"font"			"MMenuPlayListDesc"
			"labelText"		"%desc_token%"
			"textAlignment"	"north-west"
			"xpos"			"rs1-4"
			"ypos"			"19"
			"zpos"			"1"
			"wide"			"120"
			"tall"			"40"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"

			"wrap"			"1"
			"fgcolor_override" "Black"
			"proportionaltoparent" "1"
			"mouseinputenabled"	"0"
		}
	}

	"PlayListDropShadow"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"PlaylistBGPanel"
		"xpos"			"cs-0.5"
		"ypos"			"p1.17-2"
		"zpos"			"1"
		"wide"			"p1.5"
		"tall"			"1000"
		"visible"		"1"
		"PaintBackgroundType"	"2"
		"border"		"InnerShadowBorder"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
	}

	"MapsContainer"
	{
		"Controlname"	"EditablePanel"
		"fieldName"		"MapsContainer"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"0"
		"visible"		"1"
		"enabled"		"1"
		"proportionaltoparent"	"1"

		"border"		"InnerShadowBorder"

		"pinCorner"		"2"
		"autoResize"	"1"

		"skip_autoresize"	"1"

		"bgcolor_override"	"0 0 0 150"
	}
}