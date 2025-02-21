"Resource/UI/QuestItemTrackerPanel_Base.res"
{	
	"ItemName"
	{
		"ControlName"	"Label"
		"fieldName"		"ItemName"
		"labeltext"		"%itemname%"
		"xpos"			"rs1"
		"ypos"			"8"
		"wide"			"190"
		"tall"			"18"
		"zpos"			"4"
		"textinsetx"	"5"
		"fgcolor_override"		"235 226 202 255"
		"font"			"AchievementTracker_Name"
		//"wrap"			"1"
		"TextAlignment"		"north-east"
		"proportionaltoparent" "1"
	}

	"NotYetCommittedContainer"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"NotYetCommittedContainer"
		"xpos"			"rs1"		
		"ypos"			"0"		
		"zpos"			"3"
		"wide"			"f0"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"	

		"proportionaltoparent" "1"

		"PendingText"
		{
			"ControlName"	"CExLabel"
			"fieldName"		"PendingText"
			"labeltext"		"#QuestTracker_Pending"
			"xpos"			"rs1"
			"ypos"			"0"
			"wide"			"190"
			"tall"			"f0"
			"zpos"			"4"
			"textinsetx"	"5"
			"fgcolor_override"		"235 226 202 255"
			"font"			"QuestObjectiveTracker_Desc"
			//"wrap"			"1"
			"TextAlignment"		"north-east"
			"proportionaltoparent" "1"
		}
	}

}