"Resource/UI/StaticBadgePanel.res"
{
	"BadgePanel"	
	{
		"fieldName"		"BadgePanel"
		"xpos"			"cs-0.5"
		"ypos"			"0"
		"zpos"			"0"		
		"wide"			"o1"
		"tall"			"f0"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fov"			"70"
		"start_framed"	"0"
		"proportionaltoparent"	"1"
		"render_texture"	"0"
		
		"model"
		{
			"force_pos"		"1"
			"modelname"	""
			"skin"		"0"
			"angles_x"	"0"
			"angles_y"	"180"
			"angles_z"	"0"
			"origin_x"		"40"
			"origin_y"		"0"
			"origin_z"		"2"
			"spotlight"	"1"
			
			"animation"
			{
				"sequence"	"idle"
				"default"	"1"
			}
		}

		"lights"
		{
			"default"
			{
				"name"			"directional"
				"color"			"0.5 0.5 0.5"
				"direction"		"1 0 -1"
			}
		}
	}

	"Shadow"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"Shadow"
		"xpos"			"cs-0.5"
		"ypos"			"cs-0.5"
		"zpos"			"-1"
		"wide"			"p0.8"
		"tall"			"o1"
		"visible"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"1"
		"image"			"gradient_radial_pure_black"
		"scaleImage"	"1"	
		"alpha"			"200"
	}

	"RankLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"RankLabel"
		"xpos"			"cs-0.5"
		"ypos"			"rs1-12"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"20"
		"visible"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
		"wrap"			"0"
		"textinsetx"	"0"
		"textinsety"	"0"

		"font"			"HudFontSmallest"
		"fgcolor_override"	"HUDBlueTeamSolid"
		"labelText"		"%rank%"
		"textAlignment"	"south"
		"use_proportional_insets"	"1"
	}

	"NameLabel"
	{
		"ControlName"	"Label"
		"fieldName"		"NameLabel"
		"xpos"			"cs-0.5"
		"ypos"			"rs1-4"
		"zpos"			"0"
		"wide"			"f0"
		"tall"			"20"
		"visible"		"1"
		"proportionaltoparent"	"1"
		"mouseinputenabled"	"0"
		"wrap"			"0"
		"textinsetx"	"0"
		"textinsety"	"0"

		"font"			"FontStorePrice"
		"fgcolor_override"	"TanLight"
		"labelText"		"%name%"
		"textAlignment"	"south"
		"use_proportional_insets"	"1"
	}
}
