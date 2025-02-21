#base "CollectionCraftingDialog_Base.res"

"Resource/UI/econ/HalloweenOfferingDialog.res"
{
	"CollectionCraftingPanel"
	{
		"fieldName"				"CollectionCraftingPanel"
		"visible"				"0"
		"enabled"				"1"
		"xpos"					"cs-0.5"
		"ypos"					"cs-0.5"
		"zpos"					"101"
		"wide"					"f0"
		"tall"					"f0"
		"paintbackground"		"0"

		"buttons_start_x"	"305"
		"buttons_start_y"	"140"
		"buttons_step_x"	"56"
		"buttons_step_y"	"56"

		"bg_target_y"		"10"
		"slide_in_time"		"1.f"

		"boxtops"
		{
			"boxtop"	"trade_ups/trade_ups_boxtop_01"
			"boxtop"	"trade_ups/trade_ups_boxtop_02"
			"boxtop"	"trade_ups/trade_ups_boxtop_03"
			"boxtop"	"trade_ups/trade_ups_boxtop_04"
			"boxtop"	"trade_ups/trade_ups_boxtop_05"
		}

		"stampimages"
		{
			"image"	"contracts\halloween\accepted"
		}

		"resultstring"
		{
			"string"	"#TF_HalloweenOffering_Result0"
		}

		"localizedpanels"
		{
			"0"
			{
				"panelname"	"Paragraph1"
				"show_for_english"	"0"
			}

			"1"
			{
				"panelname"	"Paragraph2"
				"show_for_english"	"0"
			}

			"2"
			{
				"panelname"	"Paragraph3"
				"show_for_english"	"0"
			}

			"3"
			{
				"panelname"	"Instructions"
				"show_for_english"	"0"
			}

			"4"
			{
				"panelname"	"LocalizedPaperImagePaperImage"
				"show_for_english"	"1"
			}

			"5"
			{
				"panelname"	"BlankPaperImage"
				"show_for_english"	"0"
			}
		}
	}

	"DrawingPanel"
	{
		"linecolor"		"QualityColorHaunted"
	}

	"LetterFront"
	{

		"image"			"trade_ups/trade_ups_letter_back_front_halloween"
	}

	"Stamp"
	{
		"image"			"trade_ups/trade_ups_stamp_02"
	}

	"TradeUpContainer"
	{
		"PaperContainer"
		{
			"BlankPaperImage"
			{
				"image"			"trade_ups/trade_ups_bg_halloween_blank"
			}

			"LocalizedPaperImagePaperImage"
			{
				"image"			"trade_ups/trade_ups_bg_halloween"
			}

			"Paragraph1"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"Paragraph1"
				"font"			"TradeUp_Text"
				"textAlignment"	"north-west"
				"xpos"			"20"
				"ypos"			"105"
				"zpos"			"1"
				"wide"			"p0.2"
				"tall"			"300"
				"pinCorner"		"0"	
				"visible"		"1"
				"enabled"		"1"
				"labelText"		"#TF_CollectionCrafting_Halloween_Paragraph1"
				"mouseinputenabled" "0"
				"paintbackground"	"0"
				"proportionaltoparent"	"1"
				"paintborder"		"0"
				"fgcolor"		"Black"
				"wrap"			"1"
			}

			"Paragraph2"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"Paragraph2"
				"font"			"TradeUp_Text"
				"textAlignment"	"north-west"
				"xpos"			"20"
				"ypos"			"160"
				"zpos"			"1"
				"wide"			"p0.2"
				"tall"			"300"
				"pinCorner"		"0"	
				"visible"		"1"
				"enabled"		"1"
				"labelText"		"#TF_CollectionCrafting_Halloween_Paragraph2"
				"mouseinputenabled" "0"
				"paintbackground"	"0"
				"proportionaltoparent"	"1"
				"paintborder"		"0"
				"fgcolor"		"Black"
				"wrap"			"1"
			}

			"Paragraph3"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"Paragraph3"
				"font"			"TradeUp_Text"
				"textAlignment"	"north-west"
				"xpos"			"20"
				"ypos"			"215"
				"zpos"			"1"
				"wide"			"p0.8"
				"tall"			"300"
				"pinCorner"		"0"	
				"visible"		"1"
				"enabled"		"1"
				"labelText"		"#TF_CollectionCrafting_Halloween_Paragraph3"
				"mouseinputenabled" "0"
				"paintbackground"	"0"
				"proportionaltoparent"	"1"
				"paintborder"		"0"
				"fgcolor"		"Black"
				"wrap"			"1"
			}

			"Instructions"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"Instructions"
				"font"			"TradeUp_Text"
				"textAlignment"	"north-west"
				"xpos"			"295"
				"ypos"			"105"
				"zpos"			"1"
				"wide"			"p0.4"
				"tall"			"300"
				"pinCorner"		"0"	
				"visible"		"1"
				"enabled"		"1"
				"labelText"		"#TF_CollectionCrafting_Instructions_Halloween"
				"mouseinputenabled" "0"
				"paintbackground"	"0"
				"proportionaltoparent"	"1"
				"paintborder"		"0"
				"fgcolor"		"RedSolid"
				"wrap"			"1"
			}
	
			"OkButton"
			{
				"xpos"			"p0.795"
				"ypos"			"p0.84"
				"wide"			"p0.18"
				"tall"			"p0.085"
				
				"defaultFgColor_override" "235 226 202 255"
				"armedFgColor_override" "200 80 60 255"
				"depressedFgColor_override" "200 100 80 255"
			}
		}
	}

	"StartExplanation"
	{
		"TitleLabel"
		{
			"labelText"		"#TF_CollectionCrafting_Halloween_Explanation_Overview_Title"
		}
		
		"TextLabel"
		{
			"labelText"		"#TF_CollectionCrafting_Halloween_Explanation_Overview_Text"
		}
	}	

	"ItemSlotsExplanation"
	{
		"force_close"	"1"
		"end_x"			"c-250"
		"end_y"			"100"
		"end_wide"		"300"
		"end_tall"		"120"
		"callout_inparents_x"	"c75"
		"callout_inparents_y"	"210"
		
		"TitleLabel"
		{
			"labelText"		"#TF_CollectionCrafting_Halloween_Explanation_ItemToTrade_Title"
		}
		
		"TextLabel"
		{
			"labelText"		"#TF_CollectionCrafting_Halloween_Explanation_ItemToTrade_Text"
		}
		
		"PrevButton"
		{
			"ypos"			"90"				
		}		

		"PositionLabel"
		{
			"ypos"			"90"
		}	
	}	
}
