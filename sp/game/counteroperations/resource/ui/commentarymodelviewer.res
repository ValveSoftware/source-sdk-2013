"Resource/UI/CommentaryModelViewer.res"
{
	"commentary_modelviewer"
	{
		"ControlName"		"Frame"
		"fieldName"		"commentary_modelviewer"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"f0"
		"tall"			"480"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"settitlebarvisible"	"0"
		"PaintBackgroundType"	"2"
		"bgcolor_override"	"0 0 0 255"
		"infocus_bgcolor_override" "0 0 0 255"
		"outoffocus_bgcolor_override" "0 0 0 255"
	}

	"modelpanel"
	{
		"ControlName"		"CCommentaryModelPanel"
		"fieldName"		"modelpanel"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"		
		"wide"			"f0"
		"tall"			"480"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"

		"fov"			"75"
		"start_framed"		"1"
		
		"model"
		{
			"angles_x" "0"
			"angles_y" "180"
			"angles_z" "0"
			"origin_x" "220"
			"origin_y" "0"
			"origin_z" "0"
			"spotlight" "1"
		
			"modelname"	"models/player/engineer.mdl"
			"modelname_hwm"	"models/player/hwm/engineer.mdl"
			"skin"		"1"
			"vcd"		"scenes/Player/Engineer/low/class_select.vcd"		

			"animation"
			{
				"sequence"		"SelectionMenu_StartPose"
			}

			"attached_model"
			{
				"modelname" "models/weapons/w_models/w_wrench.mdl"
				"skin"		"1"
			}
		}
	}		
}
