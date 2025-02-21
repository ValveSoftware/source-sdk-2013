"Resource/UI/MvMVictoryMannUpPanel.res"
{			
	//Mouse over panel
	"mouseoveritempanel"
	{
		"ControlName"	"CItemModelPanel"
		"fieldName"		"mouseoveritempanel"
		"xpos"			"0"
		"ypos"			"600"
		"zpos"			"1000"
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
		"padding_height"	"15"
		
		"attriblabel"
		{
			"font"			"ItemFontAttribLarge"
			"xpos"			"0"
			"ypos"			"30"
			"zpos"			"2"
			"wide"			"140"
			"tall"			"60"
			"autoResize"	"0"
			"pinCorner"		"0"
			"visible"		"1"
			"enabled"		"1"
			"labelText"		"%attriblist%"
			"textAlignment"	"center"
			"fgcolor"		"117 107 94 255"
			"centerwrap"	"1"
		}
	}
		
	"MainPanelContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"MainPanelContainer"
		"xpos"			"c-265"
		"ypos"			"100"
		"wide"			"f0"
		"tall"			"480"
		"visible"		"1"

		"Background"
		{
			"ControlName"		"ScalableImagePanel"
			"fieldName"		"Background"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"530"
			"tall"			"230"
			"visible"		"1"
			"enabled"		"1"
			"image"			"../hud/color_panel_red"
			
			"scaleImage"		"1"
			
			"src_corner_height"	"22"				// pixels inside the image
			"src_corner_width"	"22"
		
			"draw_corner_width"	"5"				// screen size of the corners ( and sides ), proportional
			"draw_corner_height" 	"5"	
		}
			
		"NoItemServerContainer"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"NoItemServerContainer"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"530"
			"tall"			"480"
			"visible"		"1"
			
			"NoItemServerHeader"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"NoItemServer"
				"font"			"HudFontMediumSmallBold"
				"labelText"		"#TF_PVE_Server_GCDownHeader"
				"textAlignment" "center"
				"xpos"			"0"
				"ypos"			"100"
				"wide"			"530"
				"fgcolor"		"tanlight"
			}
			
			"NoItemServerMessage"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"NoItemServer"
				"font"			"HudFontSmall"
				"labelText"		"#TF_PVE_Server_GCDownMessage"
				"textAlignment" "center"
				"xpos"			"0"
				"ypos"			"120"
				"wide"			"530"
				"tall"			"40"
				"centerwrap"		"1"
				"fgcolor"		"tanlight"
			}
		}
		
		//Header
		"ItemTableHeader"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"ItemTableHeader"
			"xpos"			"0"
			"ypos"			"0"
			"wide"			"600"
			"tall"			"20"
			"visible"		"1"
			
			"LevelProgress"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"LevelLabel"
				"font"			"HudFontSmallestBold"
				"labelText"		"#TF_MVM_Victory_TourProgress"
				"textAlignment" "south-west"
				"xpos"			"160"
				"ypos"			"0"
				"wide"			"100"
				"tall"			"19"
				"fgcolor"		"tanlight"
			}
			
			"LootLabel"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"LootLabel"
				"font"			"HudFontSmallestBold"
				"labelText"		"#TF_MVM_Victory_Loot"
				"textAlignment" "south-west"
				"xpos"			"255"
				"ypos"			"0"
				"wide"			"100"
				"tall"			"19"
				"fgcolor"		"tanlight"
			}
			
			"SquadSurplusLabel"
			{
				"ControlName"	"CExLabel"
				"fieldName"		"LootLabel"
				"font"			"HudFontSmallestBold"
				"labelText"		"#TF_MVM_Victory_SquadSurplus"
				"textAlignment" "south-west"
				"xpos"			"325"
				"ypos"			"0"
				"wide"			"100"
				"tall"			"19"
				"fgcolor"		"tanlight"
			}
		}
		
		// 6 Entries
		"PlayerEntry01"
		{
			"ControlName"	"CMvMVictoryMannUpEntry"
			"fieldName"		"PlayerEntry01"
			"xpos"			"0"
			"ypos"			"20"
			"wide"			"600"
			"tall"			"32"
			"visible"		"1"
		}
		
		"PlayerEntry02"
		{
			"ControlName"	"CMvMVictoryMannUpEntry"
			"fieldName"		"PlayerEntry02"
			"xpos"			"0"
			"ypos"			"55"
			"wide"			"600"
			"tall"			"32"
			"visible"		"1"
		}
		
		"PlayerEntry03"
		{
			"ControlName"	"CMvMVictoryMannUpEntry"
			"fieldName"		"PlayerEntry03"
			"xpos"			"0"
			"ypos"			"90"
			"wide"			"600"
			"tall"			"32"
			"visible"		"1"
		}
		
		"PlayerEntry04"
		{
			"ControlName"	"CMvMVictoryMannUpEntry"
			"fieldName"		"PlayerEntry04"
			"xpos"			"0"
			"ypos"			"125"
			"wide"			"600"
			"tall"			"32"
			"visible"		"1"
		}
		
		"PlayerEntry05"
		{
			"ControlName"	"CMvMVictoryMannUpEntry"
			"fieldName"		"PlayerEntry05"
			"xpos"			"0"
			"ypos"			"160"
			"wide"			"600"
			"tall"			"32"
			"visible"		"1"
		}
		
		"PlayerEntry06"
		{
			"ControlName"	"CMvMVictoryMannUpEntry"
			"fieldName"		"PlayerEntry06"
			"xpos"			"0"
			"ypos"			"195"
			"wide"			"600"
			"tall"			"32"
			"visible"		"1"
		}
	}
}
