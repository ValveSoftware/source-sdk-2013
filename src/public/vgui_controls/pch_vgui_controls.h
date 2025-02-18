//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PCH_VGUI_CONTROLS_H
#define PCH_VGUI_CONTROLS_H

#ifdef _WIN32
#pragma once
#endif

// general includes
#include <ctype.h>
#include <stdlib.h>
#include "tier0/dbg.h"
#include "tier0/valve_off.h"
#include "tier1/KeyValues.h"

#include "tier0/valve_on.h"
#include "tier0/memdbgon.h"

#include "filesystem.h"
#include "tier0/validator.h"

// vgui includes
#include "vgui/IBorder.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "vgui/KeyCode.h"
#include "vgui/Cursor.h"
#include "vgui/MouseCode.h"

// vgui controls includes
#include "vgui_controls/Controls.h"

#include "vgui_controls/AnimatingImagePanel.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/BitmapImagePanel.h"
#include "vgui_controls/BuildGroup.h"
#include "vgui_controls/BuildModeDialog.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/CheckButtonList.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/DialogManager.h"
#include "vgui_controls/DirectorySelectDialog.h"
#include "vgui_controls/Divider.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/FocusNavGroup.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/GraphPanel.h"
#include "vgui_controls/HTML.h"
#include "vgui_controls/Image.h"
#include "vgui_controls/ImageList.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/ListViewPanel.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/MenuBar.h"
#include "vgui_controls/MenuButton.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/MessageBox.h"
#include "vgui_controls/Panel.h"
#ifndef HL1
#include "vgui_controls/PanelAnimationVar.h"
#endif
#include "vgui_controls/PanelListPanel.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/ProgressBox.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/QueryBox.h"
#include "vgui_controls/RadioButton.h"
#include "vgui_controls/RichText.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ScrollBarSlider.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_controls/Slider.h"
#ifndef HL1
#include "vgui_controls/Splitter.h"
#endif
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/ToggleButton.h"
#include "vgui_controls/Tooltip.h"
#ifndef HL1
#include "vgui_controls/ToolWindow.h"
#endif
#include "vgui_controls/TreeView.h"
#ifndef HL1
#include "vgui_controls/TreeViewListControl.h"
#endif
#include "vgui_controls/URLLabel.h"
#include "vgui_controls/WizardPanel.h"
#include "vgui_controls/WizardSubPanel.h"

#ifndef HL1
#include "vgui_controls/KeyBoardEditorDialog.h"
#include "vgui_controls/InputDialog.h"
#endif

#endif // PCH_VGUI_CONTROLS_H
