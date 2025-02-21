"Resource/UI/ItemSlotPanel.res"
{
	"item_slot_panel"
	{
		"ControlName"	"Frame"
		"fieldName"		"item_slot_panel"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"501"
		"wide"			"f0"
		"tall"			"f0"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"settitlebarvisible"	"0"
		"PaintBackgroundType"	"0"
		"bgcolor_override"	"46 43 42 255"
		"infocus_bgcolor_override" "46 43 42 255"
		"outoffocus_bgcolor_override" "46 43 42 255"
		
		"item_xpos_offcenter_a"	"-270"
		"item_xpos_offcenter_b"	"165"
		"item_ypos"		"60"
		"item_ydelta"	"60"
		"item_mod_wide"	"40"
		
		"item_backpack_offcenter_x"		"-288"
		"item_backpack_xdelta"			"4"
		"item_backpack_ydelta"			"3"

		"button_xpos_offcenter"	"175"		
		"button_ypos"	"85"
		"button_ydelta"	"80"
		"button_override_delete_xpos" "0"
		
		"modelpanels_kv"
		{
			"ControlName"	"CItemModelPanel"
			"xpos"			"c-70"
			"ypos"			"270"
			"wide"			"100"
			"tall"			"55"
			"visible"		"0"
			"bgcolor_override"		"0 0 0 0"
			"noitem_textcolor"		"117 107 94 255"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"3"
			"model_tall"	"49"
			"text_ypos"		"46"
			"text_center"	"1"
			"name_only"		"1"
			
			"attriblabel"
			{
				"font"			"ItemFontAttribLarge"
				"visible"		"0"
			}
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"allow_rot"				"0"
			}
		}
	}

	"mouseoveritempanel"
	{
		"ControlName"	"CItemModelPanel"
		"fieldName"		"mouseoveritempanel"
		"xpos"			"c-70"
		"ypos"			"270"
		"zpos"			"100"
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
		"padding_height"	"30"
	}

	"CaratLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"		"CaratLabel"
		"font"			"HudFontSmallestBold"
		"labelText"		">>"
		"textAlignment"	"west"
		"xpos"			"c-300"
		"ypos"			"20"
		"zpos"			"1"
		"wide"			"20"
		"tall"			"15"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "200 80 60 255"
	}

	"ClassLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ClassLabel"
		"font"			"HudFontMediumSmallBold"
		"labelText"		"#EditSlots"
		"textAlignment"	"west"
		"xpos"			"c-280"
		"ypos"			"15"
		"zpos"			"1"
		"wide"			"210"
		"tall"			"25"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
	}

	"OKButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"OKButton"
		"xpos"			"c-50"
		"ypos"			"323"
		"zpos"			"20"
		"wide"			"100"
		"tall"			"25"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#TF_OK"
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"ok"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}
}
