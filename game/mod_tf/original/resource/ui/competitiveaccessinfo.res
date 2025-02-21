"resource/ui/CompetitiveAccessInfo.res"
{
	"MainContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"MainContainer"
		"xpos"			"-15"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"300"
		"tall"			"350"
		"visible"		"1"
		"enabled"		"1"
		"PaintBackgroundType"	"2"
		"paintbackground"	"0"
		"paintborder"		"0"

		"TitleLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"TitleLabel"
			"font"			"HudFontSmallBold"
			"labelText"		"#MMenu_CompetitiveAccess"
			"textAlignment"	"center"
			"xpos"			"5"
			"ypos"			"10"
			"wide"			"f10"
			"tall"			"20"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"fgcolor_override" "TanLight"
			"proportionaltoparent"	"1"
		}

		"TextLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"TextLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#MMenu_CompetitiveAccess_Desc"
			"textAlignment"	"north-west"
			"xpos"			"20"
			"ypos"			"35"
			"wide"			"f40"
			"tall"			"40"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"wrap"			"1"
			"fgcolor_override" "TanLight"
			"proportionaltoparent"	"1"
		}

		"Option1Label"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Option1Label"
			"font"			"HudFontSmallestBold"
			"labelText"		"#MMenu_CompetitiveAccessOption1"
			"textAlignment"	"center"
			"xpos"			"10"
			"ypos"			"70"
			"wide"			"f20"
			"tall"			"15"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"fgcolor_override" "TanLight"
			"proportionaltoparent"	"1"
		}

		"Option1DescLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Option1DescLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#MMenu_CompetitiveAccessOption1_Desc"
			"textAlignment"	"north-west"
			"xpos"			"20"
			"ypos"			"85"
			"wide"			"f40"
			"tall"			"60"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"wrap"			"1"
			"fgcolor_override" "TanLight"
			"proportionaltoparent"	"1"
		}

		"Option1Panel"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"Option1Panel"
			"xpos"			"20"
			"ypos"			"135"
			"zpos"			"100"
			"wide"			"f40"
			"tall"			"p0.2"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"tabPosition"	"0"
			"proportionaltoparent"	"1"

			"Frame"
			{
				"Controlname"	"EditablePanel"
				"fieldName"		"Frame"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"f0"
				"zpos"			"5"
				"proportionaltoparent"	"1"
				"border"		"InnerShadowBorder"
				"mouseinputenabled"	"0"
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

			"PremiumButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"PremiumButton"
				"xpos"			"0"
				"ypos"			"0"
				"zpos"			"10"
				"wide"			"p0.3"
				"tall"			"f0"
				"autoResize"	"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"default"		"1"
				"labelText"		""
				"command"		"addpremium"
				"proportionaltoparent"	"1"
				"actionsignallevel"	"3"

				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
			
				"paintbackground"	"1"

				"defaultBgColor_override" "0 0 0 64"
				"armedBgColor_override" "0 0 0 0"

				"image_drawcolor"	"200 200 200 255"
				"image_armedcolor"	"255 255 255 255"

				"SubImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SubImage"
					"xpos"			"cs-0.5"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"o1"
					"tall"			"f0"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"proportionaltoparent"	"1"
					"image"			"../backpack/crafting/gift_premium_large"
				}
			}

			"PremiumCheckImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"PremiumCheckImage"
				"xpos"			"p0.15-s0.5"
				"ypos"			"p0.5-s0.5"
				"zpos"			"100"
				"wide"			"20"
				"tall"			"20"
				"visible"		"0"
				"enabled"		"1"
				"scaleImage"	"1"
				"proportionaltoparent"	"1"
				"image"			"checkmark"
				"drawcolor_override"	"0 255 0 255"
			}

			"AndLabel1"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"AndLabel1"
				"font"			"HudFontMediumBigBold"
				"labelText"		"+"
				"textAlignment"	"center"
				"xpos"			"p0.3"
				"ypos"			"0"
				"wide"			"p0.05"
				"tall"			"f0"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"fgcolor_override" "TanLight"
				"proportionaltoparent"	"1"
			}

			"PhoneButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"PhoneButton"
				"xpos"			"p0.35"
				"ypos"			"0"
				"zpos"			"10"
				"wide"			"p0.3"
				"tall"			"f0"
				"autoResize"	"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"default"		"1"
				"labelText"		""
				"command"		"addphone"
				"proportionaltoparent"	"1"
				"actionsignallevel"	"3"

				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
			
				"paintbackground"	"1"

				"defaultBgColor_override" "0 0 0 64"
				"armedBgColor_override" "0 0 0 0"

				"image_drawcolor"	"200 200 200 255"
				"image_armedcolor"	"255 255 255 255"

				"SubImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"SubImage"
					"xpos"			"cs-0.5"
					"ypos"			"0"
					"zpos"			"1"
					"wide"			"o1"
					"tall"			"f0"
					"visible"		"1"
					"enabled"		"1"
					"scaleImage"	"1"
					"proportionaltoparent"	"1"
					"image"			"mobile_phone"
				}
			}

			"PhoneCheckImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"PhoneCheckImage"
				"xpos"			"p0.5-s0.5"
				"ypos"			"p0.5-s0.5"
				"zpos"			"100"
				"wide"			"20"
				"tall"			"20"
				"visible"		"0"
				"enabled"		"1"
				"scaleImage"	"1"
				"proportionaltoparent"	"1"
				"image"			"checkmark"
				"drawcolor_override"	"0 255 0 255"
			}

			"AndLabel2"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"AndLabel2"
				"font"			"HudFontMediumBigBold"
				"labelText"		"+"
				"textAlignment"	"center"
				"xpos"			"p0.65"
				"ypos"			"0"
				"wide"			"p0.05"
				"tall"			"f0"
				"autoResize"	"0"
				"pinCorner"		"0"
				"visible"		"1"
				"enabled"		"1"
				"fgcolor_override" "TanLight"
				"proportionaltoparent"	"1"
			}

			"RankImage"
			{
				"ControlName"	"CTFBadgePanel"
				"fieldName"		"RankImage"
				"xpos"			"p0.7"
				"ypos"			"5"
				"zpos"			"10"
				"wide"			"p0.3"
				"tall"			"f0"
				"visible"		"1"
				"enabled"		"1"
				"proportionaltoparent"	"1"

				"BadgePanel"
				{
					"ypos"		"10"
				}
			}

			"RankButton"
			{
				"ControlName"	"CExImageButton"
				"fieldName"		"RankButton"
				"xpos"			"p0.7"
				"ypos"			"0"
				"zpos"			"11"
				"wide"			"p0.3"
				"tall"			"f0"
				"autoResize"	"0"
				"visible"		"1"
				"enabled"		"1"
				"tabPosition"	"0"
				"default"		"1"
				"labelText"		""
				"command"		"open_casual"
				"proportionaltoparent"	"1"
				"actionsignallevel"	"3"
			
				"sound_depressed"	"UI/buttonclick.wav"
				"sound_released"	"UI/buttonclickrelease.wav"
			
				"paintbackground"	"1"
			
				"defaultBgColor_override" "0 0 0 64"
				"armedBgColor_override" "0 0 0 0"
			
				"image_drawcolor"	"200 200 200 255"
				"image_armedcolor"	"255 255 255 255"
			}

			"RankCheckImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"RankCheckImage"
				"xpos"			"p0.85-s0.5"
				"ypos"			"p0.5-s0.5"
				"zpos"			"100"
				"wide"			"20"
				"tall"			"20"
				"visible"		"0"
				"enabled"		"1"
				"scaleImage"	"1"
				"proportionaltoparent"	"1"
				"image"			"checkmark"
				"drawcolor_override"	"0 255 0 255"
			}
		}

		"Option2Label"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Option2Label"
			"font"			"HudFontSmallestBold"
			"labelText"		"#MMenu_CompetitiveAccessOption2"
			"textAlignment"	"center"
			"xpos"			"10"
			"ypos"			"140+p0.2"
			"wide"			"f10"
			"tall"			"15"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"fgcolor_override" "TanLight"
			"proportionaltoparent"	"1"
		}

		"Option2DescLabel"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"Option2DescLabel"
			"font"			"HudFontSmallest"
			"labelText"		"#MMenu_CompetitiveAccessOption2_Desc"
			"textAlignment"	"north-west"
			"xpos"			"20"
			"ypos"			"140+p0.2+15"
			"wide"			"f40"
			"tall"			"40"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"wrap"			"1"
			"fgcolor_override" "TanLight"
			"proportionaltoparent"	"1"
		}

		"CyclingAd"
		{
			"ControlName"			"CCyclingAdContainerPanel"
			"fieldName"				"CyclingAd"
			"xpos"					"20"
			"ypos"					"140+p0.2+15+40"
			"zpos"					"100"
			"wide"					"f40"
			"tall"					"p0.2"
			"visible"				"1"
			"enabled"				"1"
			"scaleImage"			"1"
			"proportionaltoparent"	"1"

			"bgcolor_override"		"0 0 0 255"

			"items"
			{
				"0"
				{
					"item"		"Competitive Matchmaking Official"
					"show_market"	"0"
				}
			}
		}
	}
}