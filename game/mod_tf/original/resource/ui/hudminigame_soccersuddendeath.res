#base "HudMiniGame_Soccer.res"

"Resource/UI/HudMiniGame_SuddenDeath.res"
{
	"PlayingTo"
	{
		"labelText"		"#game_SuddenDeath"
	}

	"RedScore"
	{
		"visible"		"0"	
	}	

	"RedScoreShadow"
	{
		"visible"		"0"
	}

	"BlueScore"
	{
		"visible"		"0"	
	}	

	"BlueScoreShadow"
	{
		"visible"		"0"
	}

	"RedSkullImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"RedSkullImage"
		"xpos"			"c70"
		"ypos"			"r60"
		"zpos"			"5"
		"wide"			"50"
		"tall"			"50"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../effects/skull001_hud"
		"scaleImage"	"1"	
		"proportionaltoparent" "1"
	}

	"BlueSkullImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BlueSkullImage"
		"xpos"			"c-120"
		"ypos"			"r60"
		"zpos"			"5"
		"wide"			"50"
		"tall"			"50"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../effects/skull001_hud"
		"scaleImage"	"1"	
		"proportionaltoparent" "1"
	}
}
