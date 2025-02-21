#base "ConfirmApplyStrangifierDialog.res"

"Resource/UI/ConfirmApplyPaintkitDialog.res"
{
	"ConfirmApplyStrangifierDialog"
	{
		"xpos"					"c-200"
		"ypos"					"c-200"
		"wide"					"400"
		"tall"					"300"
	}

	"ConfirmLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ConfirmLabel"
		"font"			"HudFontSmall"
		"labelText"		"#ToolStrangifierConfirm"
		"textAlignment"	"center"
		"xpos"			"20"
		"ypos"			"40"
		"zpos"			"0"
		"wide"			"360"
		"tall"			"100"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "200 80 60 255"
		"centerwrap"	"1"
	}

	"PreviewLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"PreviewLabel"
		"font"			"HudFontSmall"
		"labelText"		"#ToolPaintKitPreview"
		"textAlignment"	"north"
		"xpos"			"20"
		"ypos"			"240"
		"zpos"			"6"
		"wide"			"360"
		"tall"			"100"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "TanLight"
		"centerwrap"	"1"
		"mouseinputenabled"	"0"
	}

	"ModelInspectionPanel"
	{
		"fieldName"		"ModelInspectionPanel"
		"xpos"			"50"
		"ypos"			"105"
		"zpos"			"5"
		"wide"			"300"
		"tall"			"150"
		"visible"		"1"
		"border"		"StorePreviewBorder"
		"paintborder"	"1"

		"proportionaltoparent"	"1"

		"force_use_model"		"1"
		"use_item_rendertarget" "0"
		"allow_rot"				"1"
		"allow_pitch"			"1"
		"max_pitch"				"30"
		"use_pedestal"			"1"
		"use_particle"			"1"
		"fov"					"75"

		"model"
		{
			"force_pos"	"1"

			"angles_x" "7"
			"angles_y" "130"
			"angles_z" "0"
			"origin_x" "175"
			"origin_y" "0"
			"origin_z" "0"
			"frame_origin_x"	"0"
			"frame_origin_y"	"0"
			"frame_origin_z"	"0"
			"spotlight" "1"
		
			"modelname"		""
		}

		"lights"
		{
			"default"
			{
				"name"			"directional"
				"color"			"1 1 1"
				"direction"		"0 0 -1"
			}
			"spot light"
			{
				"name"				"spot"
				"color"				"1 .9 .9"
				"attenuation"		"4.5 0 0"
				"origin"			"0 0 100"
				"direction"			"1 0 -0.5"
				"inner_cone_angle"	"1"
				"outer_cone_angle"	"90"
				"maxDistance"		"1000"
				"exponent"			"25"
			}
			"point light"
			{
				"name"				"point"
				"color"				".7 .8 1"
				"attenuation"		"15 0 0"
				"origin"			"15 -50 -200"
				"maxDistance"		"1000"
			}
		}
	}

	"CancelButton"
	{
		"ypos"			"r30"
		"proportionaltoparent"	"1"
	}
	
	"OkButton"
	{
		"ypos"			"r30"
		"proportionaltoparent"	"1"

	}
}
