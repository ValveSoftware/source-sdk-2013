//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TF_QUEST_MAP_REGION_PANEL_H
#define TF_QUEST_MAP_REGION_PANEL_H


#include "vgui_controls/EditablePanel.h"
#include "local_steam_shared_object_listener.h"
#include "tf_proto_def_messages.h"
#include "tf_controls.h"
#include "tf_quest_map_node_panel.h"

using namespace vgui;
using namespace GCSDK;

class CQuestMapNode;
class CQuestMapNodePanel;
class CQuestObjectiveTextPanel;
class CExScrollingEditablePanel;
class CQuestProgressTrackerPanel;
class CItemModelPanel;
class CItemModelPanelToolTip;
class CQuestNodeViewPanel;

extern const float tf_quest_map_zoom_transition_in_time;

namespace vgui
{
	class Slider;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CQuestMapPathsPanel : public Panel
{
public:
	DECLARE_CLASS_SIMPLE( CQuestMapPathsPanel, Panel );
	CQuestMapPathsPanel( Panel *pParent, const char *pszPanelname, float flZoomScale );

	virtual void Paint() OVERRIDE;
	
	void AddNode( CQuestMapNodePanel* pNodePanel );
	void RemoveNode( uint32 nDefindex );

	void AddRegion( EditablePanel* pRegionPanel, uint32 nDefIndex );
	void RemoveRegion( uint32 nDefIndex );
	void SetActiveRegion( EditablePanel* pRegionPanel ) { m_pActiveRegionLinkPanel = pRegionPanel; }

	CCircleDrawingHelper& GetCircleDrawer() { return m_circleDrawer; }

private:

	CCircleDrawingHelper m_circleDrawer;
	CUtlMap< uint32, EditablePanel* > m_mapRegionLinkPanels;
	EditablePanel* m_pActiveRegionLinkPanel = nullptr;
	CUtlMap< uint32, CQuestMapNodePanel* > m_mapQuestNodes;
	int m_nWhiteTexture;
	float m_flZoomScale;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CQuestMapRegionPanel : public EditablePanel
						   , public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CQuestMapRegionPanel, EditablePanel );
public:
	CQuestMapRegionPanel( Panel *pParent, const char *pszPanelName, const CMsgProtoDefID& msgIDCurrentRegion, CQuestNodeViewPanel* pNodeViewPanel );
	~CQuestMapRegionPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *pCommand ) OVERRIDE;

	virtual void OnTick() OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	MESSAGE_FUNC_PARAMS( NodeSelected, "NodeSelected", pParams );
	MESSAGE_FUNC_PARAMS( NodeCursorEntered, "NodeCursorEntered", pParams );
	MESSAGE_FUNC( NodeViewClosed, "NodeViewClosed" );

	MESSAGE_FUNC( CloseNodeView, "CloseNodeView" );
	MESSAGE_FUNC_PARAMS( CreateActivationCircle, "CreateActivationCircle", pParams );
	MESSAGE_FUNC_PARAMS( CreateCircle, "CreateCircle", pParams );

	virtual void OnMousePressed( MouseCode code );
	virtual void OnMouseDoublePressed(MouseCode code);

	const EditablePanel* GetRegionLinkPanel( uint32 nDefIndex ) const;
	const CQuestMapNodePanel* GetNodePanel( uint32 nDefIndex ) const;
	const CDraggableScrollingPanel* GetZoomPanel() const { return m_pZoomPanel; }

	void StartZoomTo( float flX, float flY, bool bZoomingIn );
	void StartZoomAway( float flX, float flY, bool bZoomingIn );
	bool BIsZooming() const;
	void UpdateControls();
private: 

	void SOChangeEvent();
	void CreateClickCircle();

	CDraggableScrollingPanel*	m_pZoomPanel;
	CQuestMapPathsPanel*		m_pPathsPanel;
	CQuestNodeViewPanel*		m_pQuestMapNodeView;
	CUtlMap< uint32, EditablePanel* > m_mapRegionLinkPanels;
	CUtlMap< uint32, CQuestMapNodePanel* >	m_mapNodePanels;
	CMsgProtoDefID m_msgIDCurrentRegion;
	ImagePanel* m_pBGImage;
	Panel* m_pDimmer = NULL;
	float m_flLastClickTime;
	float m_flClickScale;
	
	CPanelAnimationVar( float, m_flZoomScale, "zoom_scale", "1.1" );
	CPanelAnimationVar( float, m_flZoomX, "zoom_x", "0.5" );
	CPanelAnimationVar( float, m_flZoomY, "zoom_y", "0.5" );
	float m_flZoomStartTime;
	float m_flZoomTime;
	bool m_bTurningIn = false;
};

#endif //TF_QUEST_MAP_REGION_PANEL_H
