"Resource/UI/replaybrowser/replaylistpanel.res"
{
	"ReplayList"
	{
		"ControlName"		"CReplayListPanel"
		"fieldName"			"ReplayList"
		
		"PanelListEmbedded"
		{
			"ControlName"			"EditablePanel"
			"fieldName"				"PanelListEmbedded"
		}
		
		"PanelListPanelVScroll"
		{
			"ControlName"			"ScrollBar"
			"FieldName"				"PanelListPanelVScroll"
			"paintborder"			"0"
			"nobuttons"				"1"
			
			"Slider"
			{
				"ControlName"			"Panel"
				"FieldName"				"Slider"
				
				"paintborder"			"0"
				"paintbackground"		"1"
				"bgcolor_override"		"ReplayBrowser.BgColor"
				"fgcolor_override"		"ReplayBrowser.ScrollBar.SliderButton.FgColor"
				
				"ButtonBorder"			"ReplayBrowser.ScrollBar.SliderButton.Border"
			}
			
			"UpButton"
			{
				"ControlName"			"Button"
				"FieldName"				"UpButton"
				"visible"				"0"
			}
			
			"DownButton"
			{
				"ControlName"			"Button"
				"FieldName"				"DownButton"
				"visible"				"0"
			}
		}
	}
}
