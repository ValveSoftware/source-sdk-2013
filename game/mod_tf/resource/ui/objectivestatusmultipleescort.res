"Resource/UI/ObjectiveStatusMultipleEscort.res"
{	
	"ObjectiveStatusMultipleEscort"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"ObjectiveStatusMultipleEscort"
		"xpos"				"0"
		"ypos"				"0"
		"wide"				"f0"
		"tall"				"480"
		"visible"			"1"
		"enabled"			"1"
	}
	
	"BlueEscortPanel"
	{
		"ControlName"		"CTFHudEscort"
		"fieldName"			"BlueEscortPanel"
		"xpos"				"c-200"
		"ypos"				"r160"
		"zpos"				"1"
		"wide"				"400"
		"tall"				"200"
		"visible"			"1"
		"enabled"			"1"
		"progress_xpos"		"79"
		"progress_wide"		"270"
		
		"if_blue_is_top"
		{
			"ypos"				"r176"
		}
	}

	"RedEscortPanel"
	{
		"ControlName"		"CTFHudEscort"
		"fieldName"			"RedEscortPanel"
		"xpos"				"c-200"
		"ypos"				"r160"
		"zpos"				"1"
		"wide"				"400"
		"tall"				"200"
		"visible"			"1"
		"enabled"			"1"
		"progress_xpos"		"79"
		"progress_wide"		"270"
		
		"if_red_is_top"
		{
			"ypos"				"r176"
		}
	}
}
