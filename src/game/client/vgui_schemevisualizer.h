//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef VGUI_CLIENTSCHEMEVISUALIZER_H
#define VGUI_CLIENTSCHEMEVISUALIZER_H

//----------------------------------------------------------------------------------------

#include "vgui_controls/Frame.h"
#include "vgui_controls/PanelListPanel.h"

//----------------------------------------------------------------------------------------

class CSchemeVisualizer : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CSchemeVisualizer, vgui::Frame );

public:
	CSchemeVisualizer( vgui::Panel *pParent, vgui::IScheme *pViewScheme, const char *pSchemeName );
	virtual ~CSchemeVisualizer();

private:
	virtual void	PerformLayout();

private:
	virtual void OnTick();

	enum ListDataType_t
	{
		LISTDATATYPE_INVALID = -1,
		LISTDATATYPE_BORDERS,
		LISTDATATYPE_FONTS,
		LISTDATATYPE_COLORS,
		NUM_TYPES
	};

	int m_aComboDataTypeToItemIDMap[NUM_TYPES];	// [ data type ] = < item id >

	void UpdateList( ListDataType_t nType );
	void AddBordersToList();
	void AddFontsToList();
	void AddColorsToList();

	vgui::PanelListPanel	*m_pList;
	ListDataType_t			m_nListDataType;	// Currently selected data type
	int						m_nSelectedComboItem;

	vgui::IScheme			*m_pViewScheme;		// The scheme we're visualizing
	vgui::ComboBox			*m_pListDataTypeCombo;
};

//----------------------------------------------------------------------------------------

#endif //  VGUI_CLIENTSCHEMEVISUALIZER_H
