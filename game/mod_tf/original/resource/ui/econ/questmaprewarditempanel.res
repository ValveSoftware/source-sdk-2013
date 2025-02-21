#base "ItemAdDefault.res"

"Resource/UI/QuestMapRewardItemPanel.res"
{
	"CashImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"CashImage"
		"xpos"			"rs1-26"
		"ypos"			"rs1-3"
		"zpos"			"101"
		"wide"			"15"
		"tall"			"15"
		"visible"		"1"
		"enabled"		"1"
		"image"			"cyoa/cyoa_cash_large"
		"scaleImage"	"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
	}		

	"BGBorder"
	{
		"ControlName"	"Panel"
		"fieldName"		"BGBorder"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-2"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"PaintBackgroundType"	"0"
		"proportionaltoparent"	"1"
		"mouseinputenabled"		"0"
		"keyboardinputenabled"	"0"

		"border"		"ReplayDefaultBorder"
	}	

	"Background"
	{
	//	"border"		"CYOAPopupBorder"
	}

	"ItemName"
	{
		"font"			"QuestMap_Medium"
	}

	"ScrollableItemText"
	{
		"ItemAdText"
		{
			"font"			"QuestMap_Small"
		}
	}

	"BuyButton"
	{
		"font"			"QuestMap_Small"
	}

	"LimitLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"LimitLabel"
		"xpos"		"5"
		"ypos"		"r15"
		"zpos"		"100"
		"wide"		"200"
		"tall"		"10"
		"labelText"			"%limit%"
		"font"			"QuestMap_Medium"
		"textinsetx"		"0"
		"textAlignment"	"west"
		"mouseinputenabled"		"0"
		"proportionaltoparent"	"1"
		"paintbackground"	"1"

		"fgcolor_override"	"Orange"

		"border_default"	"NoBorder"
	}

	"SoldOutImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"SoldOutImage"
		"xpos"			"rs1-5"
		"ypos"			"cs-0.5"
		"zpos"			"100"
		"wide"			"o1"
		"tall"			"p1.5"
		"visible"		"1"
		"enabled"		"1"
		"image"			"cyoa/sold_out"
		"scaleImage"	"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
	}
}