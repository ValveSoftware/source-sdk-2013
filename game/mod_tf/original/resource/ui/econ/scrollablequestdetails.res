"Resource/UI/econ/ScrollableQuestDetails.res"
{
	"Container"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"Container"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"enabled"		"1"
		"proportionaltoparent" "1"
		"skip_autoresize"	"1"

		"RewardsLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"RewardsLabel"
			"font"			"HudFontSmallBold"
			"labelText"		"#QuestDetails_Reward"
			"textAlignment"	"west"
			"xpos"			"10"
			"ypos"			"20"
			"zpos"			"2"
			"wide"			"f0"
			"tall"			"f0"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"wrap"			"0"
			"proportionaltoparent"	"1"
			"fgcolor"		"QuestGold"
		}

		"ItemContainer"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"ItemContainer"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"10"
			"wide"			"80"
			"tall"			"60"
			"proportionaltoparent"	"1"
			"visible"		"1"
			"enabled"		"1"
			"paintborder"	"0"
	
			"ItemPanel"
			{
				"ControlName"	"CItemModelPanel"
				"fieldName"		"ItemPanel"
				"xpos"			"cs-0.5"
				"ypos"			"cs-0.5"
				"zpos"			"5"
				"wide"			"f0"
				"tall"			"f0"
				"visible"		"1"
				"bgcolor_override"		"0 0 0 0"
				"noitem_textcolor"		"117 107 94 255"
				"PaintBackgroundType"	"2"
				"paintborder"	"0"
				"proportionaltoparent"	"1"
			
				"model_xpos"	"4"
				"model_ypos"	"4"
				"model_wide"	"100"
				"model_tall"	"65"
				"text_ypos"		"100"
				"text_center"	"1"
				"name_only"		"1"
			
				"inset_eq_x"	"2"
				"inset_eq_y"	"2"

				"deferred_description"	"1"
				"deferred_icon"			"1"
			
				"itemmodelpanel"
				{
					"use_item_rendertarget" "0"
					"allow_rot"				"0"
					"inventory_image_type"	"1"
				}

				"use_item_sounds"	"1"
			}
		}
	}

	"ScrollingContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"ScrollingContainer"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"5"
		"wide"			"f0"
		"tall"			"f0"
		"proportionaltoparent" "1"
		"PaintBackgroundType"	"2"
		"fgcolor_override"	"59 54 48 255"
		"bgcolor_override"	"0 0 0 255"
		"autohide_buttons" "1"
		"paintborder"	"0"
	}
}
