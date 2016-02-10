"resource/ui/DialogMapSelector.res"
{
	"CMapSelectorDialog"
	{
		"ControlName"		"Frame"
		"fieldName"		"CMapSelectorDialog"
		"xpos"		"1"
		"ypos"		"1"
		"wide"		"602"
		"tall"		"387"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
	}
	"MapTabs"
	{
		"ControlName"		"PropertySheet"
		"fieldName"		"MapTabs"
		"xpos"		"1"
		"ypos"		"24"
		"wide"		"638"
		"tall"		"338"
		"autoResize"	"3"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"1"
	}
	"StatusLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"StatusLabel"
		"xpos"		"1"
		"ypos"		"362"
		"wide"		"5000"
		"tall"		"24"
		"autoResize"		"1"
		"pinCorner"		"2"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"textAlignment"		"west"
		"dulltext"		"0"
		"zpos"	"-1"
		style="status"
	}
	
	styles
	{
		status
		{
			bgcolor="PropertySheetBG"
			inset="8 0 0 0"
		}
	}
}
