#base "MainMenuPlayListEntry.res"

"Resource/UI/MainMenuEventPlayListEntry.res"
{
	"TimeImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"TimeImage"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"101"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"image"			"viewmode_loaner"
		"scaleImage"	"1"
		"proportionaltoparent" "1"
	}

	"Gradient"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"Gradient"
		"xpos"		"rs1-50"
		"ypos"		"0"
		"wide"		"100"
		"tall"		"f0"
		"zpos"	"1"
		"visible"		"1"
		"enabled"		"1"
		"rotation"		"1"
		"proportionaltoparent"	"1"
		"scaleimage"	"1"
		"mouseinputenabled"	"0"
		"alpha"		"255"

		"image"		"gradient_pure_black"
	}

	"Gradient2"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"Gradient2"
		"xpos"		"rs1-50"
		"ypos"		"0"
		"wide"		"100"
		"tall"		"f0"
		"zpos"	"1"
		"visible"		"1"
		"enabled"		"1"
		"rotation"		"1"
		"proportionaltoparent"	"1"
		"scaleimage"	"1"
		"mouseinputenabled"	"0"
		"alpha"		"255"

		"image"		"gradient_pure_black"
	}

	"Gradient3"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"Gradient3"
		"xpos"		"rs1-50"
		"ypos"		"0"
		"wide"		"100"
		"tall"		"f0"
		"zpos"	"1"
		"visible"		"1"
		"enabled"		"1"
		"rotation"		"1"
		"proportionaltoparent"	"1"
		"scaleimage"	"1"
		"mouseinputenabled"	"0"
		"alpha"		"255"

		"image"		"gradient_pure_black"
	}

	"GradientHoriz"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"GradientHoriz"
		"xpos"		"rs1-50"
		"ypos"		"0"
		"wide"		"f0"
		"tall"		"30"
		"zpos"	"1"
		"visible"		"1"
		"enabled"		"1"
		"rotation"		"3"
		"proportionaltoparent"	"1"
		"scaleimage"	"1"
		"mouseinputenabled"	"0"
		"alpha"		"255"

		"image"		"gradient_pure_black"
	}

	"ExpireLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ExpireLabel"
		"font"			"StorePromotionsTitle"
		"fgcolor_override"	"TanLight"
		"labelText"		"%expire%"
		"textalignment"	"north-west"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"2"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"enabled"		"1"
		"autoResize"	"1"
		"pinCorner"		"0"
		"proportionaltoparent"	"1"
		"textinsetx"	"23"
		"textinsety"	"9"

		"paintbackground"	"0"
		"use_proportional_insets"	"1"
	}

	"FlashColor"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"FlashColor"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"999"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"enabled"		"1"
		
		"mouseinputenabled"		"0"
		"keyboardinputenabled"	"0"
		"proportionaltoparent"	"1"
		"bgcolor_override"	"TanLight"
	}

	"BorderOverlay"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"BorderOverlay"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"1001"
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"enabled"		"1"
		
		"mouseinputenabled"		"0"
		"keyboardinputenabled"	"0"
		"proportionaltoparent"	"1"
		"border"	"CYOAPopupBorder"
	}
}