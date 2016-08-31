//------------------------------------
// Achievement Group
//------------------------------------
"AchievementGroup.res"
{	
	"AchievementDialogGroupPanel"
	{
		"ControlName"	        "CAchievementDialogGroupPanel"
		"fieldName"		        "AchievementDialogGroupPanel"
		"xpos"					"50"	
		"ypos"					"0"
		"wide"					"260"
		"tall"					"64"
		"autoResize"			"0"
		"visible"			    "1"
		"enabled"               "1"
		"tabPosition"           "0"
		"settitlebarvisible"    "0"
		"pinCorner"			    "0"
	}

	"GroupIcon"
	{
		"ControlName"	        "ImagePanel"
		"fieldName"		        "GroupIcon"
		"xpos"			        "0"
		"ypos"			        "0"
		"wide"			        "256"
		"tall"			        "64"
		"visible"		        "1"
		"scaleImage"            "0"
	}
			
	"GroupName"
	{
		"ControlName"	        "label"
		"fieldName"		        "GroupName"
		"labeltext"		        ""
		"xpos"			        "10"
		"ypos"			        "10"
		"wide"			        "256"
		"tall"			        "20"
		"font"			        "AchievementItemTitle"
		"textAlignment"		    "west"
	}

	"GroupPercentageBarBackground"
	{
		"ControlName"		    "ImagePanel"
		"fieldName"	        	"GroupPercentageBarBackground"
		"xpos"		            "20"
		"ypos"		            "40"
		"zpos"                  "2"
		"wide"		            "170"
		"tall"		            "12"
		"fillcolor"	            "32 32 32 255"
		//"zpos"	                "-1"
		"visible"	        	"1"
		"enabled"	        	"1"
	}

	"GroupPercentageBar" // current completed
	{
		"ControlName"	        "ImagePanel"
		"fieldName"		        "GroupPercentageBar"
		"xpos"			        "20"
		"ypos"			        "40"
		"zpos"                  "3"
		"wide"			        "0"
		"tall"			        "12"
		//"fillcolor"		        "200 184 148 255"	// overwritten by code
		"zpos"			        "0"
		"visible"		        "0"
		"enabled"		        "1"
	}

	"GroupPercentageText" //Percent Text inside the percentage field
	{
		"ControlName"	    	"Label"
		"fieldName"		        "GroupPercentageText"
		"xpos"		            "200"
		"ypos"		            "40"
		"wide"		            "50"
		"tall"		            "12"
		"autoResize"	    	"0"
		"pinCorner"		        "0"
		"visible"		        "1"
		"enabled"		        "1"
		"tabPosition"	    	"0"
		"labelText"		        ""
		"textAlignment"	    	"west"
		"dulltext"		        "0"
		"brighttext"	    	"0"
		"wrap"		            "0"
		"font"			        "AchievementItemDescription"
	}
}
