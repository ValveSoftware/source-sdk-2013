//------------------------------------
// Achievement Item
//------------------------------------
"AchievementItem.res"
{	
	"AchievementDialogItemPanel"
	{
		"ControlName"	"CAchievementDialogItemPanel"
		"fieldName"		"AchievementDialogItemPanel"
		"xpos"						"0"	
		"ypos"						"0"
		"wide"						"594"
		"tall"						"64"
		"autoResize"				"0"
		"visible"					"1"
		"enabled"					"1"
		"tabPosition"				"0"
		"settitlebarvisible"		"0"
		"pinCorner"					"0"
		"ProgressBarColor" 	"200 184 148 255" [$WIN32]
		"PaintBackgroundType"	"2"
	}
			
	"AchievementIcon"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"AchievementIcon"
		"xpos"			"4"
		"ypos"			"4"
		"wide"			"56"
		"tall"			"56"
		"visible"		"1"
		"scaleImage"		"1"
	}
			
	"AchievementName"
	{
		"ControlName"	"label"
		"fieldName"		"AchievementName"
		"labeltext"		"name"
		"xpos"			"70"
		"ypos"			"2"
		"wide"			"400"
		"tall"			"20"
		"font"			"AchievementItemTitle"
		"textAlignment"		"west"
	}
	
	"AchievementDesc"
	{
		"ControlName"	"label"
		"fieldName"		"AchievementDesc"
		"labeltext"		"desc"
		"xpos"			"71"
		"ypos"			"22"
		"wide"			"490"
		"tall"			"40"
		"font"			"AchievementItemDescription"
		"wrap"			"1"
		"textAlignment"		"north-west"
	}
	
	"PercentageBarBackground"
	{
		"ControlName"		"ImagePanel"
		"fieldName"		"PercentageBarBackground"
		"xpos"		"70"
		"ypos"		"42"
		"wide"		"350"
		"tall"		"12"
		"fillcolor"	"32 32 32 255"
		"zpos"	"-1"
		"visible"		"0"
		"enabled"		"1"
	}
	
	"PercentageBar" // current completed
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"PercentageBar"
		"xpos"			"70"
		"ypos"			"42"
		"wide"			"0"
		"tall"			"12"
		//"fillcolor"		"200 184 148 255"	// overwritten by code
		"zpos"			"0"
		"visible"		"0"
		"enabled"		"1"
	}
	
	"PercentageText" //Percent Text inside the percentage field
	{
		"ControlName"		"Label"
		"fieldName"		"PercentageText"
		"xpos"		"430"
		"ypos"		"41"
		"wide"		"150"
		"tall"		"12"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"0%"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"			"AchievementItemDescription"
	}
	"ShowOnHUD"
	{
		"ControlName"		"CheckButton"
		"fieldName"		"ShowOnHUD"
		"xpos"		"450"
		"ypos"		"7"
		"wide"		"150"
		"tall"		"12"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#GameUI_Achievement_Show_HUD"
		"textAlignment"		"west"
		"dulltext"		"0"
		"brighttext"		"0"
		"wrap"		"0"
		"font"			"AchievementItemDescription"
	}
}
