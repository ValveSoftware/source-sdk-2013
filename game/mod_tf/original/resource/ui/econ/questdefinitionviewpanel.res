"Resource/UI/QuestDefinitionViewPanel.res"
{
	"DebugButton"
	{
		"ControlName"	"CExImageButton"
		"fieldName"		"DebugButton"
		"xpos"			"5"
		"ypos"			"0"
		"zpos"			"1005"
		"wide"			"8"
		"tall"			"8"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labeltext"		""
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"default"		"0"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		"Command"		"debug_menu"
		"proportionaltoparent" "1"
		"actionsignallevel"	"1"
			
		"paintbackground"	"0"
			
		"defaultFgColor_override" "0 0 0 0"
		"armedFgColor_override" "200 80 60 255"
		"depressedFgColor_override" "46 43 42 255"
			
		"image_drawcolor"	"0 0 0 0"
		"image_armedcolor"	"200 80 60 255"
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"1"
			"wide"			"8"
			"tall"			"8"
			"visible"		"1"
			"enabled"		"1"
			"image"			"glyph_workshop_edit"
			"scaleImage"	"1"
		}				
	} // Debug button

	"CloseButton"
	{
		"ControlName"	"CExImageButton"
		"fieldName"		"CloseButton"
		"xpos"			"183"
		"ypos"			"3"
		"zpos"			"1000"
		"wide"			"12"
		"tall"			"o1"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labeltext"		""
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"default"		"1"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
		"Command"		"close"
		"proportionaltoparent"	"1"
		"actionsignallevel"	"1"
			
		"paintbackground"	"0"
			
		"defaultFgColor_override" "46 43 42 255"
		"armedFgColor_override" "235 226 202 255"
		"depressedFgColor_override" "46 43 42 255"
			
		"image_drawcolor"	"117 107 94 255"
		"image_armedcolor"	"200 80 60 255"
		"SubImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"SubImage"
			"xpos"			"0"
			"ypos"			"0"
			"zpos"			"1"
			"wide"			"f0"
			"tall"			"f0"
			"visible"		"1"
			"enabled"		"1"
			"image"			"close_button"
			"scaleImage"	"1"
			"proportionaltoparent"	"1"
		}				
	}		

	"TitleLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"TitleLabel"
		"labeltext"		"%name%"
		"xpos"			"rs1+1"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"15"
		"zpos"			"1000"
		"font"			"QuestMap_Medium"
		"fgcolor_override"		"QuestMap_ActiveOrange"
		"textinsety"	"5"
		"TextAlignment"		"center"
		"proportionaltoparent" "1"
		"mouseinputenabled"		"0"
		"autoresize"	"1"	// Stick to the right.  It's going to resize and we want to resize with it
	}

	"StateBorderOverlay"
	{
		"ControlName"	"Panel"
		"fieldName"		"StateBorderOverlay"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"999"
		"wide"			"f0"
		"tall"			"f0"
		"autoresize"	"3"	// Stick to the bottom and right.  It's going to resize and we want to resize with it
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
		"border"		"CYOANodeViewBorder_Inactive"
	}

	"ExpireLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"ExpireLabel"
		"labeltext"		"%expire_time%"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"199"
		"tall"			"10"
		"zpos"			"1000"
		"font"			"QuestMap_Small"
		"fgcolor_override"		"RedSolid"
		"TextAlignment"		"North"
		"proportionaltoparent" "1"
		"mouseinputenabled"		"0"
	}

	"ContentContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"ContentContainer"
		"xpos"			"1"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"199"
		"tall"			"f3"
		"autoresize"	"2"	// Stick to the bottom.  It's going to resize and we want to resize with it
		"proportionaltoparent"	"1"

		"ContractsLabel"
		{
			"ControlName"	"Label"
			"fieldName"		"ContractsLabel"
			"labeltext"		"#Quests"
			"xpos"			"0"
			"ypos"			"17"
			"wide"			"f0"
			"tall"			"15"
			"zpos"			"1"
			"font"			"QuestMap_Medium"
			"TextAlignment"		"east"
			"textinsetx"	"40"
			"proportionaltoparent" "1"
			"mouseinputenabled"		"0"
			"bgcolor_override"	"0 0 0 255"
			"fgcolor_override"		"QuestMap_ActiveOrange"
		}

		"ContractsInfoImage"
		{
			"ControlName"	"ImagePanel"
			"fieldName"		"ContractsInfoImage"
			"xpos"			"rs1-5"
			"ypos"			"19"
			"zpos"			"2"
			"wide"			"10"
			"tall"			"10"
			"visible"		"1"
			"enabled"		"1"
			"scaleImage"	"1"	
			"proportionaltoparent"	"1"
			"image"			"info"
		}

		"QuestOption1"
		{
			"fieldName"		"QuestOption1"
			"xpos"			"rs1"
			"ypos"			"31"
			"zpos"			"0"
			"wide"			"f0"
			"tall"			"o0.25"
			"visible"		"1"
			"enabled"		"1"
			"proportionaltoparent"	"1"

			"collapsed_height"	"45"
		}

		"QuestOption2"
		{
			"fieldName"		"QuestOption2"
			"xpos"			"rs1"
			"ypos"			"3"
			"zpos"			"0"
			"wide"			"f0"
			"tall"			"o0.25"
			"visible"		"1"
			"enabled"		"1"
			"proportionaltoparent"	"1"

			"collapsed_height"	"45"

			"pin_to_sibling"		"QuestOption1"
			"pin_corner_to_sibling"	"PIN_TOPLEFT"
			"pin_to_sibling_corner" "PIN_BOTTOMLEFT"
		}

		"QuestOption3"
		{
			"fieldName"		"QuestOption3"
			"xpos"			"rs1"
			"ypos"			"3"
			"zpos"			"0"
			"wide"			"f0"
			"tall"			"o0.25"
			"visible"		"1"
			"enabled"		"1"
			"proportionaltoparent"	"1"

			"collapsed_height"	"45"

			"pin_to_sibling"		"QuestOption2"
			"pin_corner_to_sibling"	"PIN_TOPLEFT"
			"pin_to_sibling_corner" "PIN_BOTTOMLEFT"
		}

		

		"RewardContainer"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"RewardContainer"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"f0"
			"tall"			"75"
			"proportionaltoparent"	"1"
			"mouseinputenabled"	"1"

			"RewardsLabel"
			{
				"ControlName"	"Label"
				"fieldName"		"RewardsLabel"
				"labeltext"		"#TF_QuestView_Rewards"
				"xpos"			"0"
				"ypos"			"0"
				"wide"			"f0"
				"tall"			"15"
				"zpos"			"1"
				"font"			"QuestMap_Medium"
				"TextAlignment"		"east"
				"textinsetx"	"40"
				"proportionaltoparent" "1"
				"mouseinputenabled"		"0"
				"bgcolor_override"	"0 0 0 255"
				"fgcolor_override"		"QuestMap_ActiveOrange"
			}

			"RewardsInfoImage"
			{
				"ControlName"	"ImagePanel"
				"fieldName"		"RewardsInfoImage"
				"xpos"			"rs1-5"
				"ypos"			"2"
				"zpos"			"2"
				"wide"			"10"
				"tall"			"10"
				"visible"		"1"
				"enabled"		"1"
				"scaleImage"	"1"	
				"proportionaltoparent"	"1"
				"image"			"info"
			}

			"BloodMoneyContainer"
			{
				"ControlName"	"EditablePanel"
				"fieldName"		"BloodMoneyContainer"
				"xpos"			"5"
				"ypos"			"rs1"
				"wide"			"80"
				"tall"			"50"
				"PaintBackgroundType"	"0"
				"paintborder"	"1"
				"proportionaltoparent"	"1"
				"mouseinputenabled"	"1"
				"border"	"EconItemBorder"

				"BloodMoneyObtainedIndicator"
				{
					"ControlName"	"Label"
					"fieldName"		"BloodMoneyObtainedIndicator"
					"xpos"			"0"
					"ypos"			"cs-0.5"
					"zpos"			"101"
					"wide"			"f0"
					"tall"			"20"
					"visible"		"1"
					"enabled"		"1"
					"proportionaltoparent"	"1"
					"mouseinputenabled"	"0"	
					"labeltext"	"#TF_QuestView_Reward_Claimed"
					"textAlignment"	"center"
					"font"	"QuestMap_Large"
					"bgcolor_override"	"CreditsGreen"
					"fgcolor_override"	"TanLight"
					"alpha"		"200"
				}

				"AmountLabel"
				{
					"ControlName"	"Label"
					"fieldName"		"AmountLabel"
					"labeltext"		"%cash%"
					"xpos"			"cs-0.5"
					"ypos"			"rs1-5"
					"zpos"			"2"
					"wide"			"40"
					"tall"			"15"
					"zpos"			"1"
					"font"			"QuestMap_Large"
					"fgcolor_override"		"CreditsGreen"
					"TextAlignment"		"center"
					"proportionaltoparent" "1"
					"mouseinputenabled"		"0"
					"bgcolor_override"	"0 0 0 240"
				}

				"CashImage"
				{
					"ControlName"	"ImagePanel"
					"fieldName"		"CashImage"
					"xpos"			"0"
					"ypos"			"-7"
					"zpos"			"1"
					"wide"			"f0"
					"tall"			"f0"
					"visible"		"1"
					"enabled"		"1"
					"image"			"cyoa/cyoa_cash_large"
					"scaleImage"	"1"
					"proportionaltoparent"	"1"
					"mouseinputenabled"	"0"
				}			
			}

			"RewardItemModelPanel"
			{		
				"ControlName"	"CItemModelPanel"
				"fieldName"		"RewardItemModelPanel"
				"xpos"			"rs1-5"
				"ypos"			"rs1"
				"zpos"			"2"
				"wide"			"80"
				"tall"			"50"
				"visible"		"0"
				"PaintBackgroundType"	"0"
				"paintborder"	"1"
				"proportionaltoparent"	"1"
				"mouseinputenabled"	"1"
				"border"		"EconItemBorder"
		
				"model_xpos"	"5"
				"model_ypos"	"2"
				"model_wide"	"70"
				"model_tall"	"45"
				"name_only"		"0"
				"attrib_only"	"0"
				"model_only"	"1"
				"paint_icon_hide"	"0"
		
				"text_ypos"		"10"
		
				"itemmodelpanel"
				{

					"inventory_image_type"	"1"
					"allow_rot"				"1"
					"force_square_image"	"0"
				}

				"RewardObtainedIndicator"
				{
					"ControlName"	"Label"
					"fieldName"		"RewardObtainedIndicator"
					"xpos"			"0"
					"ypos"			"cs-0.5"
					"zpos"			"101"
					"wide"			"f0"
					"tall"			"20"
					"visible"		"1"
					"enabled"		"1"
					"proportionaltoparent"	"1"
					"mouseinputenabled"	"0"	
					"labeltext"	"#TF_QuestView_Reward_Claimed"
					"textAlignment"	"center"
					"font"	"QuestMap_Large"
					"bgcolor_override"	"CreditsGreen"
					"fgcolor_override"	"TanLight"
					"alpha"		"200"
				}
			}
		}
	} // ContentContainer
}