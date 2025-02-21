"Resource/UI/AnnotationsPanelCallout.res"
{
	"AnnotationsPanelCallout"
	{
		"ArrowIcons"
		{
			"left"		"../hud/freezecam_callout_arrow_left"
			"right"		"../hud/freezecam_callout_arrow_right"
			"down"		"../hud/freezecam_callout_arrow"
		}
		
//		"bgcolor_override"	"255 0 0 255"
//		"PaintBackgroundType"	"1"
	}
	
	"CalloutBG"
	{
		"ControlName"	"CTFImagePanel"
		"fieldName"		"CalloutBG"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"
		"wide"			"76"
		"tall"			"24"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../hud/color_panel_brown"
		"scaleImage"	"1"	
		"teambg_2"		"../hud/color_panel_red"	// Reversed, due to showing killer's name
		"teambg_3"		"../hud/color_panel_blu"	// Reversed, due to showing killer's name
		
		"src_corner_height"		"23"				// pixels inside the image
		"src_corner_width"		"23"
			
		"draw_corner_width"		"5"				// screen size of the corners ( and sides ), proportional
		"draw_corner_height" 	"5"		
	}
	"CalloutLabel"
	{	
		"ControlName"	"CExLabel"
		"fieldName"		"CalloutLabel"
		"font"			"HudFontSmall"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"2"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		"#AnnotationPanel_Callout"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
	}
	"ArrowIcon"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"ArrowIcon"
		"xpos"			"20"
		"ypos"			"35"
		"zpos"			"3"
		"wide"			"20"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../hud/freezecam_callout_arrow"
		"scaleImage"	"1"	
	}
	"DistanceLabel"
	{	
		"ControlName"		"CExLabel"
		"fieldName"		"DistanceLabel"
		"font"			"HudFontSmallest"
		"xpos"			"0"
		"ypos"			"28"
		"zpos"			"2"
		"wide"			"70"
		"tall"			"10"
		"autoResize"		"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"center"
		"dulltext"		"0"
		"brighttext"		"0"
	}
}
