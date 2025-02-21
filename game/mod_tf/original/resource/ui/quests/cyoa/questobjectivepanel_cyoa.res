#base "..\QuestObjectivePanel_InGame_Base.res"

"Resource/UI/HudAchievementTrackerItem.res"
{	
	"QuestObjectiveTextPanel"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"QuestObjectiveTextPanel"
		"xpos"			"cs-0.5"		
		"ypos"			"0"		
		"zpos"			"3"
		"wide"			"148"
		"tall"			"18"
		"visible"		"1"
		"enabled"		"1"	
		"proportionaltoparent"	"1"
		"map_view"	"1"
		
		"PaintBackgroundType"	"2"

		"enabled_text_color_override"		"TanLight"
		"disabled_text_color_override"		"TanDark"

		"normal_token"		"#QuestObjective_Required"
		"advanced_token"	"#QuestObjective_Optional"
	}

	"AttribGlow"
	{
		"ControlName"	"Label"
		"fieldName"		"AttribGlow"
		"labeltext"		"%attr_desc%"
		"xpos"			"rs1"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"18"
		"zpos"			"5"
		"textinsetx"	"0"
		"font"			"QuestMap_Small_Blur"
		"alpha"			"0"

		"TextAlignment"		"north-west"
		"proportionaltoparent" "1"
		"wrap"			"1"
	}
	
	"AttribDesc"
	{
		"ControlName"	"Label"
		"fieldName"		"AttribDesc"
		"labeltext"		"%attr_desc%"
		"xpos"			"rs1"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"18"
		"zpos"			"4"
		"textinsetx"	"0"
		"font"			"QuestMap_Small"

		"TextAlignment"		"north-west"
		"proportionaltoparent" "1"
		"wrap"			"1"
	}

	"AttribBlur"
	{
		"ControlName"	"Label"
		"fieldName"		"AttribBlur"
		"labeltext"		"%attr_desc%"
		"xpos"			"rs1"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"10"
		"zpos"			"3"
		"visible"		"1"
		"alpha"			"0"
		"font"			"QuestMap_Small_Blur"
		"textinsetx"	"0"
		"textAlignment"		"north-west"
		"proportionaltoparent" "1"
		"fgcolor_override"	"White"
		"wrap"			"1"
	}
}