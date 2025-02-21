#base "StoreHome_Base.res"

"Resource/UI/StoreHome_Premium.res"
{
	"PremiumCallout"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"PremiumCallout"
		"xpos"			"c-323"
		"ypos"			"c-45"
		"zpos"			"-1"
		"wide"			"300"
		"tall"			"300"
		"visible"		"1"
		"enabled"		"1"
	
		"MannCoImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"MannCoImage"
			"wide"			"420"
			"tall"			"220"
			"visible"		"0"
			"enabled"		"1"
			"scaleImage"	"1"
			"xpos"			"15"
			"ypos"			"0"
			"zpos"			"0"
			
//			if_halloween
//			{
//				"visible"		"1"
//				"image"		"store/store_halloween_premium_callout"
//			}
			if_christmas
			{
				"visible"		"1"
				"ypos"			"15"
				"image"		"store/store_australian_christmas_callout_sale"
			}
		}
	}
}
