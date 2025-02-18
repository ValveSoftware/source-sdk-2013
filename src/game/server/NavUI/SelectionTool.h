//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef SELECTIONTOOL_H
#define SELECTIONTOOL_H

#ifdef SERVER_USES_VGUI

#include "NavUI.h"
#include "nav.h"


//--------------------------------------------------------------------------------------------------------
class SelectionToolPanel : public CNavUIToolPanel
{
	DECLARE_CLASS_SIMPLE( SelectionToolPanel, CNavUIToolPanel );

public:
	SelectionToolPanel( vgui::Panel *parent, const char *toolName );

	virtual void Init( void );
	virtual void Shutdown( void );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );

	virtual void StartLeftClickAction( const char *actionName );
	virtual void FinishLeftClickAction( const char *actionName );
	virtual void StartRightClickAction( const char *actionName );
	virtual void OnCursorMoved( int x, int y );

	virtual bool IsFloodSelectable( CNavArea *area );

protected:
	void FloodSelect( void );
	CNavArea *m_floodStartArea;

	enum DragSelectType
	{
		DRAG_NONE,
		DRAG_SELECT,
		DRAG_UNSELECT
	};
	DragSelectType m_dragType;
};

#endif // SERVER_USES_VGUI

#endif // SELECTIONTOOL_H
