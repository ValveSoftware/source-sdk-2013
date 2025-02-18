//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Competitive ranks tooltip panel
//
//=============================================================================//


#include "cbase.h"
#include <vgui_controls/EditablePanel.h>
#include "econ_controls.h"
#include "tf_controls.h"
#include "tf_matchmaking_dashboard.h"
#include "ienginevgui.h"

using namespace vgui;
using namespace GCSDK;

EditablePanel* GetCompRanksTooltipPanel()
{
	// Special tooltip panel
	class CRanksTooltipPanel : public EditablePanel
	{
		DECLARE_CLASS_SIMPLE( CRanksTooltipPanel, EditablePanel );
	public: 
		CRanksTooltipPanel()
			: EditablePanel( nullptr, "CompRanksTooltip" )
		{
			vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
			SetProportional( true );
			SetScheme(scheme);
		}

		void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
		{
			EditablePanel::ApplySchemeSettings( pScheme );
			LoadControlSettings( "resource/UI/CompRanksTooltip.res" );
		}

		void SetVisible(bool state) OVERRIDE
		{
			EditablePanel::SetVisible( state );
		}
	};

	return new CRanksTooltipPanel();
}

BaseTooltip* GetCompRanksTooltip()
{
	// Special tooltip that uses PositionTooltip to position itself
	class CPositionedSimplePanelToolTip : public CSimplePanelToolTip
	{
	public:
		CPositionedSimplePanelToolTip() 
			: CSimplePanelToolTip( nullptr, (const char*)nullptr )
		{}

		virtual void ShowTooltip( vgui::Panel *currentPanel ) 
		{ 
			CSimplePanelToolTip::ShowTooltip( currentPanel );

			PositionTooltip( TTP_RIGHT_CENTERED, currentPanel, m_pControlledPanel );
		}
	};

	static CPositionedSimplePanelToolTip tooltip;
	tooltip.SetControlledPanel( GetDashboardPanel().GetTypedPanel< EditablePanel >( k_eToolTipCompRanks ) );

	return &tooltip;
}

// Register with the dashboard for comp ranks tooltip
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( []()->Panel* { return GetCompRanksTooltipPanel(); }, k_eToolTipCompRanks );
