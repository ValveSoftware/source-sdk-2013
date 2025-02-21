"Resource/UI/StorePreviewItemPanel.res"
{
	"storepreviewitem"
	{
		"ControlName"		"CStorePreviewItemPanel"
		"fieldName"		"storepreviewitem"
		"xpos"			"0"
		"ypos"			"0"
		"wide"			"305"
		"tall"			"255"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"settitlebarvisible"	"0"
	}
	
	"classmodelpanel"
	{
		"ControlName"	"CTFPlayerModelPanel"
		"fieldName"		"classmodelpanel"
		
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"0"		
		"wide"			"295"
		"tall"			"250"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		
		"render_texture"	"0"
		"fov"			"40"
		"allow_rot"		"0"

		"paintbackground" "1"		
		"paintbackgroundenabled" "1"
		"bgcolor_override" "255 255 255 0"
		
		"model"
		{
			"force_pos"	"1"

			"angles_x" "0"
			"angles_y" "170"
			"angles_z" "0"
			"origin_x" "190"
			"origin_y" "0"
			"origin_z" "-36"
			"frame_origin_x"	"0"
			"frame_origin_y"	"0"
			"frame_origin_z"	"0"
			"spotlight" "1"
		
			"modelname"		""
			
			"animation"
			{
				"name"		"PRIMARY"
				"activity"	"ACT_MP_STAND_PRIMARY"
				"default"	"1"
			}
			"animation"
			{
				"name"		"SECONDARY"
				"activity"	"ACT_MP_STAND_SECONDARY"
			}
			"animation"
			{
				"name"		"MELEE"
				"activity"	"ACT_MP_STAND_MELEE"
			}
			"animation"
			{
				"name"		"BUILDING"
				"activity"	"ACT_MP_STAND_BUILDING"
			}
			"animation"
			{
				"name"		"PDA"
				"activity"	"ACT_MP_STAND_PDA"
			}
			"animation"
			{
				"name"		"ITEM1"
				"activity"	"ACT_MP_STAND_ITEM1"
			}						
			"animation"
			{
				"name"		"ITEM2"
				"activity"	"ACT_MP_STAND_ITEM2"
			}	
			"animation"
			{
				"name"		"MELEE_ALLCLASS"
				"activity"	"ACT_MP_STAND_MELEE_ALLCLASS"
			}								
			"animation"
			{
				"name"		"PRIMARY2"
				"activity"	"ACT_MP_STAND_PRIMARY"
			}
			"animation"
			{
				"name"		"SECONDARY2"
				"activity"	"ACT_MP_STAND_SECONDARY2"
			}
		}
	}
		
	"BottomBGImage"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BottomBGImage"
		"xpos"			"40"
		"ypos"			"214"
		"zpos"			"0"
		"wide"			"225"
		"tall"			"67"
		"visible"		"1"
		"enabled"		"1"
		"image"			"store/store_inspector_bottom"
		"scaleImage"	"1"
	}
	
	"PreviewItemModelPanel"
	{		
		"ControlName"	"CItemModelPanel"
		"fieldName"		"PreviewItemModelPanel"
		"xpos"			"40"
		"ypos"			"0"
		"zpos"			"1"
		"wide"			"225"
		"tall"			"200"
		"visible"		"1"
		"bgcolor_override"		"0 0 0 255"
		"PaintBackgroundType"	"2"
		"paintborder"	"0"
		
		"model_xpos"	"25"
		"model_ypos"	"50"
		"model_wide"	"175"
		"model_tall"	"100"
		"name_only"		"0"
		"attrib_only"	"0"
		"model_only"	"1"
		"paint_icon_hide"	"0"
		
		"text_ypos"		"10"
		
		"itemmodelpanel"
		{
			"inventory_image_type" "1"
			"allow_rot"				"0"
		}
	}
	
	"ItemNameLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ItemNameLabel"
		"font"			"HudFontSmallBold"
		"labelText"		"%selectiontitle%"
		"textAlignment"	"center"
		"xpos"			"45"
		"ypos"			"5"
		"wide"			"205"
		"tall"			"33"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"centerwrap"	"1"
	}		
	
	"DetailsRichText"
	{
		"ControlName"	"CEconItemDetailsRichText"
		"fieldName"		"DetailsRichText"
		"font"			"ScoreboardSmall"
		"labelText"		"%datatext%"
		"textAlignment"	"north-west"
		"xpos"			"45"
		"ypos"			"40"
		"wide"			"205"
		"tall"			"165"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor"		"TanLight"
		"wrap"			"1"
		"highlight_color"	"177 168 149 255"
		"itemset_color"		"216 244 9 255"
		"link_color"		"252 191 27 255"
		"image_up_arrow"				"scroll_up_off"
		"image_up_arrow_mouseover"		"scroll_up_on"
		"image_down_arrow"				"scroll_down_off"
		"image_down_arrow_mouseover"	"scroll_down_on"
		"image_line"		"ArmoryScrollbarWell"
		"image_box"			"ArmoryScrollbarBox"
	}
	
	"ItemIcon1"
	{
		"ControlName"	"CStorePreviewItemIcon"
		"fieldName"		"ItemIcon1"
		
		"xpos"			"20"
		"ypos"			"225"
		"zpos"			"13"
		"wide"			"30"
		"tall"			"20"
		"visible"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"0"
		"PaintBackgroundType"	"2"
		
		"bgblockout"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"bgblockout"
			"xpos"			"2"
			"ypos"			"2"
			"zpos"			"-10"
			"wide"			"26"
			"tall"			"16"
			"visible"		"1"
			"enabled"		"0"
			"PaintBackgroundType"	"2"
			"bgcolor_override" "51 47 46 255"
		}
		
		"itempanel"
		{
			"fieldName"		"itempanel"
			
			"PaintBackgroundType"	"2"
			"paintborder"	"0"
			
			"model_xpos"	"1"
			"model_ypos"	"2"
			"model_wide"	"28"
			"model_tall"	"16"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"0"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"allow_rot"				"0"
			}
		}
	}
	"ItemIcon2"
	{
		"ControlName"	"CStorePreviewItemIcon"
		"fieldName"		"ItemIcon2"
		
		"xpos"			"20"
		"ypos"			"225"
		"zpos"			"13"
		"wide"			"30"
		"tall"			"20"
		"visible"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"0"
		"PaintBackgroundType"	"2"
		
		"bgblockout"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"bgblockout"
			"xpos"			"2"
			"ypos"			"2"
			"zpos"			"-10"
			"wide"			"26"
			"tall"			"16"
			"visible"		"1"
			"enabled"		"0"
			"PaintBackgroundType"	"2"
			"bgcolor_override" "51 47 46 255"
		}
		
		"itempanel"
		{
			"fieldName"		"itempanel"
			
			"PaintBackgroundType"	"2"
			"paintborder"	"0"
			
			"model_xpos"	"1"
			"model_ypos"	"2"
			"model_wide"	"28"
			"model_tall"	"16"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"0"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"allow_rot"				"0"
			}
		}
	}
	"ItemIcon3"
	{
		"ControlName"	"CStorePreviewItemIcon"
		"fieldName"		"ItemIcon3"
		
		"xpos"			"20"
		"ypos"			"225"
		"zpos"			"13"
		"wide"			"30"
		"tall"			"20"
		"visible"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"0"
		"PaintBackgroundType"	"2"
		
		"bgblockout"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"bgblockout"
			"xpos"			"2"
			"ypos"			"2"
			"zpos"			"-10"
			"wide"			"26"
			"tall"			"16"
			"visible"		"1"
			"enabled"		"0"
			"PaintBackgroundType"	"2"
			"bgcolor_override" "51 47 46 255"
		}
		
		"itempanel"
		{
			"fieldName"		"itempanel"
			
			"PaintBackgroundType"	"2"
			"paintborder"	"0"
			
			"model_xpos"	"1"
			"model_ypos"	"2"
			"model_wide"	"28"
			"model_tall"	"16"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"0"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"allow_rot"				"0"
			}
		}
	}
	"ItemIcon4"
	{
		"ControlName"	"CStorePreviewItemIcon"
		"fieldName"		"ItemIcon4"
		
		"xpos"			"20"
		"ypos"			"225"
		"zpos"			"13"
		"wide"			"30"
		"tall"			"20"
		"visible"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"0"
		"PaintBackgroundType"	"2"
		
		"bgblockout"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"bgblockout"
			"xpos"			"2"
			"ypos"			"2"
			"zpos"			"-10"
			"wide"			"26"
			"tall"			"16"
			"visible"		"1"
			"enabled"		"0"
			"PaintBackgroundType"	"2"
			"bgcolor_override" "51 47 46 255"
		}
		
		"itempanel"
		{
			"fieldName"		"itempanel"
			
			"PaintBackgroundType"	"2"
			"paintborder"	"0"
			
			"model_xpos"	"1"
			"model_ypos"	"2"
			"model_wide"	"28"
			"model_tall"	"16"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"0"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"allow_rot"				"0"
			}
		}
	}
	"ItemIcon5"
	{
		"ControlName"	"CStorePreviewItemIcon"
		"fieldName"		"ItemIcon5"
		
		"xpos"			"20"
		"ypos"			"225"
		"zpos"			"13"
		"wide"			"30"
		"tall"			"20"
		"visible"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"0"
		"PaintBackgroundType"	"2"
		
		"bgblockout"
		{
			"ControlName"	"EditablePanel"
			"fieldName"		"bgblockout"
			"xpos"			"2"
			"ypos"			"2"
			"zpos"			"-10"
			"wide"			"26"
			"tall"			"16"
			"visible"		"1"
			"enabled"		"0"
			"PaintBackgroundType"	"2"
			"bgcolor_override" "51 47 46 255"
		}
		
		"itempanel"
		{
			"fieldName"		"itempanel"
			
			"PaintBackgroundType"	"2"
			"paintborder"	"0"
			
			"model_xpos"	"1"
			"model_ypos"	"2"
			"model_wide"	"28"
			"model_tall"	"16"
			"name_only"		"0"
			"attrib_only"	"0"
			"model_only"	"1"
			"paint_icon_hide"	"0"
			
			"itemmodelpanel"
			{
				"use_item_rendertarget" "0"
				"allow_rot"				"0"
			}
		}
	}
	
	"ClassUsageImage1"
	{
		"ControlName"	"CStorePreviewClassIcon"
		"fieldName"		"ClassUsageImage1"
		"xpos"			"65"
		"ypos"			"225"
		"zpos"			"5"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"2"
		"PaintBackgroundType"	"2"
	}
	"ClassUsageImage2"
	{
		"ControlName"	"CStorePreviewClassIcon"
		"fieldName"		"ClassUsageImage2"
		"xpos"			"85"
		"ypos"			"225"
		"zpos"			"5"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"2"
		"PaintBackgroundType"	"2"
	}
	"ClassUsageImage3"
	{
		"ControlName"	"CStorePreviewClassIcon"
		"fieldName"		"ClassUsageImage3"
		"xpos"			"105"
		"ypos"			"225"
		"zpos"			"5"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"2"
		"PaintBackgroundType"	"2"
	}
	"ClassUsageImage4"
	{
		"ControlName"	"CStorePreviewClassIcon"
		"fieldName"		"ClassUsageImage4"
		"xpos"			"125"
		"ypos"			"225"
		"zpos"			"5"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"2"
		"PaintBackgroundType"	"2"
	}
	"ClassUsageImage5"
	{
		"ControlName"	"CStorePreviewClassIcon"
		"fieldName"		"ClassUsageImage5"
		"xpos"			"145"
		"ypos"			"225"
		"zpos"			"5"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"2"
		"PaintBackgroundType"	"2"
	}
	"ClassUsageImage6"
	{
		"ControlName"	"CStorePreviewClassIcon"
		"fieldName"		"ClassUsageImage6"
		"xpos"			"165"
		"ypos"			"225"
		"zpos"			"5"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"panel_bgcolor" "117 107 94 255"
		"panel_bgcolor_mouseover" "255 255 255 255"
		"image_indent"	"2"
		"PaintBackgroundType"	"2"
	}
	
	"IconsMoveLeftButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"IconsMoveLeftButton"
		"xpos"			"52"
		"ypos"			"227"
		"zpos"			"20"
		"wide"			"15"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"<"
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"command"		"icons_left"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}		
	"IconsMoveRightButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"IconsMoveRightButton"
		"xpos"			"238"
		"ypos"			"227"
		"zpos"			"20"
		"wide"			"15"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		">"
		"font"			"HudFontSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"command"		"icons_right"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}	
	
	"ClassUsageMouseoverLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"ClassUsageMouseoverLabel"
		"font"			"HudFontSmallest"
		"textAlignment"	"center"
		"xpos"			"36"
		"ypos"			"170"
		"zpos"			"100"
		"wide"			"230"
		"tall"			"55"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"bgcolor_override"		"0 0 0 0"
		"PaintBackgroundType"	"2"
		"centerwrap"	"1"
		"paintborder"	"1"
		"textinsetx"	"100"
		"border"		"LoadoutItemPopupBorder"
	}

	"RotLeftButton"
	{
		"ControlName"	"CPreviewRotButton"
		"fieldName"		"RotLeftButton"
		"xpos"			"60"
		"ypos"			"190"
		"zpos"			"20"
		"wide"			"20"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"<"
		"font"			"HudFontMediumSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"-1"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}		
	"RotRightButton"
	{
		"ControlName"	"CPreviewRotButton"
		"fieldName"		"RotRightButton"
		"xpos"			"220"
		"ypos"			"190"
		"zpos"			"20"
		"wide"			"20"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		">"
		"font"			"HudFontMediumSmallBold"
		"textAlignment"	"center"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"1"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}	
				
	"NextWeaponButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"NextWeaponButton"
		"xpos"			"100"
		"ypos"			"172"
		"zpos"			"20"
		"wide"			"95"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#Store_NextWeapon"
		"font"			"HudFontSmallest"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"next_weapon"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}					

	"ZoomButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"ZoomButton"
		"xpos"			"94"
		"ypos"			"192"
		"zpos"			"20"
		"wide"			"50"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#Store_Zoom"
		"font"			"HudFontSmallest"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"zoom_toggle"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}

	"TeamButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"TeamButton"
		"xpos"			"154"
		"ypos"			"192"
		"zpos"			"20"
		"wide"			"50"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#Store_Team"
		"font"			"HudFontSmallest"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"team_toggle"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}

	"PaintButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"PaintButton"
		"xpos"			"60"
		"ypos"			"172"
		"zpos"			"20"
		"wide"			"66"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#Store_Paint"
		"font"			"HudFontSmallest"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"paint_toggle"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}

	"PaintNameLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"PaintNameLabel"
		"font"			"FontStorePrice"
		"labelText"		"#Store_NoPaint"
		"textAlignment"	"left"
		"xpos"			"132"
		"ypos"			"168"
		"wide"			"110"
		"tall"			"25"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"centerwrap"	"1"
	}		

	"NextStyleButton"
	{
		"ControlName"	"CExButton"
		"fieldName"		"NextStyleButton"
		"xpos"			"60"
		"ypos"			"154"
		"zpos"			"20"
		"wide"			"66"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		"tabPosition"	"0"
		"labelText"		"#Store_NextStyle"
		"font"			"HudFontSmallest"
		"textAlignment"	"center"
		"textinsetx"	"50"
		"dulltext"		"0"
		"brighttext"	"0"
		"Command"		"next_style"
		"sound_depressed"	"UI/buttonclick.wav"
		"sound_released"	"UI/buttonclickrelease.wav"
	}	

	"StyleNameLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"StyleNameLabel"
		"font"			"FontStorePrice"
		"labelText"		"None"
		"textAlignment"	"left"
		"xpos"			"132"
		"ypos"			"148"
		"wide"			"110"
		"tall"			"25"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"wrap"			"1"
		"centerwrap"	"1"
	}		
}
