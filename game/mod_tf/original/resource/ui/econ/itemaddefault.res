"Resource/UI/econ/CyclingAdContainer.res"
{
	"ad"
	{
		"fieldname"		"ad"
		"proportionaltoparent"	"1"
		"wide"					"f0"
		"tall"					"f0"
	}

	"Background"
	{
		"ControlName"	"EditablePanel"
		"fieldname"		"Background"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"PaintBackgroundType"	"0"
		"proportionaltoparent"	"1"

		"paintborder"	"1"
		"border"		"ReplayDefaultBorder"
	}

	"ItemName"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ItemName"
		"xpos"			"2"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"30"
		"zpos"			"10"
		"visible"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"

		"font"			"AdFont_ItemName"
		"fgcolor"	"235 227 203 255"
		"use_proportional_insets"	"1"
		"textinsetx"	"5"
		"textinsety"	"5"
		"textAlignment"	"north-west"
		"labelText"		"%item_name%"
	}

	"ScrollableItemText"
	{
		"ControlName"			"CExScrollingEditablePanel"
		"fieldName"				"ScrollableItemText"
		"xpos"					"5"
		"ypos"					"12"
		"zpos"					"100"
		"wide"					"p0.67"
		"tall"					"p1"
		"proportionaltoparent"	"1"

		"allow_mouse_wheel_to_scroll" "0"
		"mouseinputenabled"		"1"
		"bottom_buffer"			"0"

		"ScrollBar"
		{
			"ControlName"	"ScrollBar"
			"FieldName"		"ScrollBar"
			"xpos"			"0"
			"ypos"			"0"
			"tall"			"f0"
			"wide"			"5" // This gets slammed from client schme.  GG.
			"zpos"			"1000"
			"nobuttons"		"1"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"1"

			"Slider"
			{
				"fgcolor_override"	"TanDark"
				"mouseinputenabled"	"1"
			}
		
			"UpButton"
			{
				"ControlName"	"Button"
				"FieldName"		"UpButton"
				"visible"		"0"
			}
		
			"DownButton"
			{
				"ControlName"	"Button"
				"FieldName"		"DownButton"
				"visible"		"0"
			}
		}

		"ItemAdText"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"ItemAdText"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"f0"
			"tall"			"100"
			"zpos"			"10"
			"visible"		"1"
			"proportionaltoparent"	"1"

			"font"			"AdFont_AdText"
			"fgcolor_override"	"163 163 163 255"
			"textinsetx"	"10"
			"textinsety"	"0"
			"textAlignment"	"north-west"
			"labelText"		"%item_ad_text%"
			"wrap"			"1"
		}
	}

	"ItemIcon"
	{
		"ControlName"	"CItemModelPanel"
		"fieldName"		"ItemIcon"
		"xpos"			"rs1.05"
		"ypos"			"3"
		"zpos"			"9"
		"wide"			"o1.5"
		"tall"			"p.9"
		"visible"		"1"
		"PaintBackgroundType"	"2"
		"paintborder"	"0"
		"proportionaltoparent"	"1"
			
		"model_xpos"	"0"
		"model_ypos"	"0"
		"model_wide"	"0"
		"model_tall"	"0"
		"text_ypos"		"60"
		"text_center"	"1"
		"model_only"	"1"

		"use_item_sounds"	"1"

		"itemmodelpanel"
		{
			"inventory_image_type"	"1"
			"use_item_rendertarget"	"0"
			"allow_rot"	"0"
		}
	}

	"BuyButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"BuyButton"
		"xpos"		"r70"
		"ypos"		"r15"
		"zpos"		"100"
		"wide"		"40"
		"tall"		"10"
		"labelText"			"%price%"
		"font"			"AdFont_PurchaseButton"
		"textinsetx"		"0"
		"textAlignment"	"center"
		
		"proportionaltoparent"	"1"
		"paintbackground"	"1"
		"command"	"purchase"
		"actionsignallevel" "2"

		"defaultFgColor_override"	"White"
		"armedFgColor_override"		"White"

		"border_default"	"NoBorder"

		"defaultBgColor_override"	"72 103 32 255"
		"armedBgColor_override"		"72 133 32 255"

		"sound_armed"		"ui/item_info_mouseover.wav"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}

	"MarketButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"MarketButton"
		"xpos"		"r70"
		"ypos"		"r15"
		"zpos"		"100"
		"wide"		"60"
		"tall"		"10"
		"labelText"			"#TF_ItemAd_ViewOnMarket"
		"font"			"AdFont_PurchaseButton"
		"textinsetx"		"0"
		"textAlignment"	"center"
		
		"proportionaltoparent"	"1"
		"paintbackground"	"1"
		"command"	"market"
		"actionsignallevel" "2"

		"defaultFgColor_override"	"White"
		"armedFgColor_override"		"White"

		"border_default"	"NoBorder"

		"defaultBgColor_override"	"72 103 32 255"
		"armedBgColor_override"		"72 133 32 255"

		"sound_armed"		"ui/item_info_mouseover.wav"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}
}
