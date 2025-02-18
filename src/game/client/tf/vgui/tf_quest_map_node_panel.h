//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TF_QUEST_NODE_MAP_PANEL_H
#define TF_QUEST_NODE_MAP_PANEL_H


#include "vgui_controls/EditablePanel.h"
#include "local_steam_shared_object_listener.h"
#include "tf_gcmessages.h"
#include "tf_controls.h"

using namespace vgui;
using namespace GCSDK;

class CQuestMapNode;

extern const float node_medium_radius;
extern const float node_large_radius;

void DrawAmbientActiveCirlce( float flXPos, float flYPos, const Color& color );

class CCircleDrawingHelper
{
public:
	struct CircleAnimData_t
	{
		double flStartTime;
		double flEndTime;
		float flX;
		float flY;
		float flStartRadius;
		float flEndRadius;
		Color colorStart;
		Color colorEnd;
		bool bFilled;
	};


	void PaintCircles();

	void AddCircle( const CircleAnimData_t animData );
	void ClearAllCircles();

private:
	CUtlVector< CircleAnimData_t > m_vecCircles;
};

//-----------------------------------------------------------------------------
// Purpose: A node on the quest map
//-----------------------------------------------------------------------------
class CQuestMapNodePanel : public vgui::EditablePanel
{
public:

	enum EMapState
	{
		NEUTRAL,
		MOUSE_OVER,
		SELECTED,
	};

	DECLARE_CLASS_SIMPLE( CQuestMapNodePanel, vgui::EditablePanel );
	CQuestMapNodePanel( uint32 nDefIndex, Panel* pParent, const char *pszPanelName );
	~CQuestMapNodePanel();

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void OnCommand( const char *pCommand ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void Paint() OVERRIDE;
	virtual void OnCursorEntered() OVERRIDE;
	virtual void OnCursorExited() OVERRIDE;
	virtual void OnMousePressed( MouseCode code ) OVERRIDE;
	virtual void OnMouseDoublePressed( MouseCode code ) OVERRIDE;

	MESSAGE_FUNC_PARAMS( UpdateStateVisuals, "UpdateStateVisuals", pKVParams );
	void UpdateFromSObject( const CQuestMapNode* pMapNode );
	const CSOQuestMapNode& GetLocalState() const { return m_msgLocalState; }
	const CQuestMapNodeDefinition* GetNodeDef() const;

	EMapState GetState() const { return m_eMapState; }
	void EnterMapState( EMapState eMapState );
	bool BRequirementsMet() const { return m_bRequirementsMet; }

	void DrawNode( float flXPos,
						  float flYPos,
						  bool bPurchased,
						  const Color& colorActive,
						  const Color& colorBonus,
						  const Color& colorInactive,
						  float flScale ) const;
private:
	static uint32 m_nDraggingID;

	EMapState m_eMapState;
	float m_flMapStateEnterTime;

	CSOQuestMapNode m_msgLocalState;

	CExButton* m_pSelectButton;
	Label* m_pNameLabel;
	ImagePanel* m_pStarCostImage;
	bool m_bOverSelected;
	bool m_bRequirementsMet;
	bool m_bBaselineSet = false;

	int m_nStartWide;
	int m_nStartTall;
};

//-----------------------------------------------------------------------------
// Purpose: Tooltip to hold the tooltip panel
//-----------------------------------------------------------------------------
class CQuestNodeTooltip : public vgui::BaseTooltip, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CQuestNodeTooltip, vgui::EditablePanel );
public:
	CQuestNodeTooltip( vgui::Panel *pParent );

	virtual void ShowTooltip( Panel *pCurrentPanel ) OVERRIDE;
	virtual void HideTooltip() OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	virtual void PositionWindow( Panel *pTipPanel ) OVERRIDE;	
	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;

private:

	CQuestMapNodePanel *m_pFocusedNode;
	CSOQuestMapNode m_msgLocalState;
};

#endif //TF_QUEST_MAP_NODE_PANEL_H
