"Resource/UI/Quests/QuestObjectiveScorer.res"
{	
	"Scorer"
	{
		"fieldName"		"Scorer"
		"zpos"			"9999"
		"wide"			"200"
		"tall"			"18"
		"visible"		"1"
		"enabled"		"1"	
	}

	"ScorerLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"ScorerLabel"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"f0"
				"proportionaltoparent"	"1"

		"textAlignment"	"north-east"
		"labelText"	"%scorer%"
		"font"			"QuestObjectiveTracker_Desc"
	//	"fgcolor_override"		"QuestMap_ActiveOrange"
	}

	"ScorerLabelBlur"
	{
		"ControlName"	"Label"
		"fieldName"		"ScorerLabelBlur"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"f0"
		"proportionaltoparent"	"1"

		"textAlignment"	"north-east"
		"labelText"	"%scorer%"
		"font"			"QuestObjectiveTracker_DescBlur"
	//	"fgcolor_override"		"QuestMap_ActiveOrange"
	}
}