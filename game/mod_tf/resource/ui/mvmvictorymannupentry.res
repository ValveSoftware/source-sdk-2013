"Resource/UI/MvMVictoryMannUpEntry.res"
{	
	"Divider"
	{
		"ControlName"	"Panel"
		"fieldName"		"Divider"
		"xpos"			"10"
		"ypos"			"0"
		"wide"			"515"
		"tall"			"1"
		"visible"		"1"
		"bgcolor_override"		"30 30 30 100"
	}
	
	"CompletedHighlight"
	{
		"ControlName"	"Panel"
		"fieldName"		"CompletedHighlight"
		"xpos"			"10"
		"ypos"			"7"
		"wide"			"516"
		"tall"			"20"
		"visible"		"0"
		"bgcolor_override"		"100 100 100 100"
	}
	
	"PlayerAvatar"
	{
		"ControlName"	"CAvatarImagePanel"
		"fieldName"		"PlayerAvatar"
		"xpos"			"10"
		"ypos"			"2"
		"wide"			"32"
		"tall"			"32"
		"visible"		"1"
		"enabled"		"1"
		"image"			""
		"scaleImage"	"1"	
		"color_outline"	"52 48 45 255"
	}
	
	"PlayerNameLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"PlayerNameLabel"
		"font"			"HudFontSmallest"
		"labelText"		"%name%"
		"textAlignment" "west"
		"xpos"			"50"
		"ypos"			"0"
		"wide"			"120"
		"tall"			"32"
		"fgcolor"		"tanlight"
	}
	
	"SquadSurplus"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"SquadSurplus"
		"xpos"		"135"
		"ypos"		"14"
		"wide"		"20"
		"tall"		"20"
		"image"			"../backpack/crafting/mvm_squad_surplus_ripped"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"		"1"
	}
	
	"CompletedCheckOn"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"CompletedCheckOn"
		"xpos"		"135"
		"ypos"		"0"
		"wide"		"20"
		"tall"		"20"
		"image"			"../backpack/crafting/mvm_ticket_ripped"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"		"1"
	}
	
	"LevelProgressBarBG"
	{
		"ControlName"	"Panel"
		"fieldName"		"LevelProgressBarBG"
		"xpos"			"160"
		"ypos"			"7"
		"wide"			"80"
		"tall"			"20"
		"visible"		"1"
		"bgcolor_override"		"TanDarker"
	}
	
	"LevelProgressBarFGAnim"
	{
		"ControlName"	"Panel"
		"fieldName"		"LevelProgressBarFGAnim"
		"xpos"			"160"
		"ypos"			"7"
		"wide"			"80"
		"tall"			"20"
		"visible"		"1"
		"bgcolor_override"	"CreditsGreen"
	}
	
	"LevelProgressBarFGStatic"
	{
		"ControlName"	"Panel"
		"fieldName"		"LevelProgressBarFGStatic"
		"xpos"			"160"
		"ypos"			"7"
		"wide"			"80"
		"tall"			"20"
		"visible"		"1"
		"bgcolor_override"	"TanDark"
	}
	
	"LevelLabelDropShadow"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"LevelLabelDropShadow"
		"font"			"HudFontSmallestBold"
		"labelText"		"%level%"
		"textAlignment" "center"
		"xpos"			"159"
		"ypos"			"3"
		"wide"			"80"
		"tall"			"32"
		"fgcolor"		"0 0 0 255"
	}
	
	"LevelLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"LevelLabel"
		"font"			"HudFontSmallestBold"
		"labelText"		"%level%"
		"textAlignment" "center"
		"xpos"			"160"
		"ypos"			"2"
		"wide"			"80"
		"tall"			"32"
		"fgcolor"		"tanlight"
	}
	
	"TourLoot"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"TourLoot"
		"xpos"			"250"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"
		
		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
	
	"MannUpLoot"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"MannUpLoot"
		"xpos"			"285"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"

		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
	
	"MissingVoucher"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"MissingVoucher"
		"font"			"HudFontSmallest"
		"labelText"		"#TF_PVE_Server_SquadVoucherMissing"
		"textAlignment" "center"
		"xpos"			"320"
		"ypos"			"5"
		"wide"			"200"
		"fgcolor"		"tanlight"
		"centerwrap"	"1"
		"visible"		"0"
	}
	
	"SquadSurplusLoot01"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"SquadSurplusLoot01"
		"xpos"			"320"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"
		
		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
	
	"SquadSurplusLoot02"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"SquadSurplusLoot02"
		"xpos"			"355"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"
		
		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
	
	"SquadSurplusLoot03"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"SquadSurplusLoot03"
		"xpos"			"390"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"
		
		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
	
	"SquadSurplusLoot04"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"SquadSurplusLoot04"
		"xpos"			"425"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"
		
		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
	
	"SquadSurplusLoot05"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"SquadSurplusLoot05"
		"xpos"			"460"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"
		
		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
	
	"SquadSurplusLoot06"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"SquadSurplusLoot06"
		"xpos"			"495"
		"ypos"			"0"
		"wide"			"35"
		"tall"			"35"
		
		"EconItemModel"
		{
			"ControlName"	"CItemModelPanel"
			"fieldName"		"EconItemModel"
			"xpos"			"0"
			"ypos"			"5"
			"wide"			"32"
			"tall"			"24"
					
			"visible"		"1"
			"bgcolor_override"		"0 0 0 50"
			"PaintBackgroundType"	"2"
			"paintborder"	"1"
			
			"model_ypos"	"1"
			"model_xpos"	"1"
			"model_tall"	"22"
			"model_wide"	"30"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"1"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"inventory_image_type" "1"
				"allow_rot"				"0"
			}
		}
	}
}
