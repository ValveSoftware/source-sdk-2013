//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_quest_map_node_panel.h"
#include "econ_controls.h"
#include "econ_item_inventory.h"
#include "tf_quest_map.h"
#include <vgui/IInput.h>
#include <vgui_controls/Slider.h>
#include "tf_gc_client.h"
#include "tf_quest_map_panel.h"
#include "tf_quest_map_node.h"
#include "tf_quest_map_utils.h"
#include "tf_item_schema.h"
#include "clientmode_tf.h"
#include "econ_quests.h"
#include "vgui/ISurface.h"
#include "tf_quest_map_editor_panel.h"
#include "vgui_controls/CircularProgressBar.h"
#include "tf_quest_map_panel.h"

const float node_cycle_time( 3.f );
const float node_alpha_start( 255 );
const float node_alpha_end( 0 );
const float node_anim_time( 3 );
const float node_small_radius( 5 );
const float node_medium_radius( 17 );
const float node_large_radius( 30 );
const float node_pulse_time( 2 );
const float node_pulse_scale( 1.3f );
const float node_selected_brightness( 70 );
const float node_mouseover_brightness( 50 );
const float node_pulse_alpha( 50 );
const float node_pulse_bias( 0.2 );

void CCircleDrawingHelper::AddCircle( CircleAnimData_t animData )
{
	m_vecCircles.AddToTail( animData );
}

void CCircleDrawingHelper::ClearAllCircles()
{
	m_vecCircles.Purge();
}

void CCircleDrawingHelper::PaintCircles()
{
	// 
	// Draw extra circles
	//
	FOR_EACH_VEC_BACK( m_vecCircles, i )
	{
		// Skip circles that will happen in the future
		const CircleAnimData_t& circle = m_vecCircles[ i ];
		if ( circle.flStartTime > Plat_FloatTime() )
			continue;

		// Prune old circles
		if ( circle.flEndTime < Plat_FloatTime() )
		{
			m_vecCircles.Remove( i );
			continue;
		}

		// Lerp the radius and color
		float flT = (float)RemapValClamped( Plat_FloatTime(), circle.flStartTime, circle.flEndTime, 0.0, 1.0 );
		float flRadius = Lerp( flT, circle.flStartRadius, circle.flEndRadius );
		Color lerpColor = LerpColor( circle.colorStart, circle.colorEnd, flT );

		if ( circle.bFilled )
		{
			DrawFilledColoredCircle( circle.flX, circle.flY, flRadius, lerpColor);
		}
		else
		{
			DrawColoredCircle( circle.flX, circle.flY, flRadius, lerpColor );
		}
	}
}

uint32 CQuestMapNodePanel::m_nDraggingID = (uint32)-1;

CQuestMapNodePanel::CQuestMapNodePanel( uint32 nDefIndex, Panel* pParent, const char *pszPanelName )
	: EditablePanel( pParent, pszPanelName )
	, m_flMapStateEnterTime( 0.f )
	, m_eMapState( NEUTRAL )
	, m_bOverSelected( false )
	, m_bRequirementsMet( false )
{
	UpdateFromSObject( GetQuestMapHelper().GetQuestMapNode( nDefIndex ) );
	m_msgLocalState.set_defindex( nDefIndex );
	m_pSelectButton = new CExButton( this, "SelectButton", (char*)NULL );
	m_pSelectButton->PassMouseTicksTo( this, true );
	m_pNameLabel = new Label( this, "NodeNameLabel", (const char*)NULL );
	m_pStarCostImage = new ImagePanel( this, "StarCostImage" );
}

CQuestMapNodePanel::~CQuestMapNodePanel()
{}

void CQuestMapNodePanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "Resource/UI/econ/QuestMapNodePanel.res" );
}

void CQuestMapNodePanel::ApplySettings( KeyValues *inResourceData )
{
	// Inject our position into the KVs that are about to go down into
	// Panel::ApplySettings which will do all the positioning calculations
	auto pDef = GetNodeDef();
	Assert( pDef );
	if ( pDef )
	{
		inResourceData->SetFloat( "xpos", pDef->GetXPos() - ( inResourceData->GetFloat( "wide" ) / 2.f ) );
		inResourceData->SetFloat( "ypos", pDef->GetYPos() - ( inResourceData->GetFloat( "tall" ) / 2.f ) );
	}

	BaseClass::ApplySettings( inResourceData );

	m_nStartWide = GetWide();
	m_nStartTall = GetTall();
}

void CQuestMapNodePanel::PerformLayout()
{
	if ( !m_bBaselineSet )
	{
		UpdateStateVisuals( NULL );
	}
}

void CQuestMapNodePanel::UpdateStateVisuals( KeyValues *pKVParams )
{
	bool bEffects = pKVParams && pKVParams->GetBool( "effects", false );

	auto pDef = GetNodeDef();
	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNode( m_msgLocalState.defindex() );

	Color colorActive =  vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
	ImagePanel* pIcon = FindControl< ImagePanel >( "NodeIcon" );

	// The node will have a different icon than the definition based on which
	// contract the user selects
	if ( pNode )
	{
		pIcon->SetImage( pNode->GetIconName() );
	}
	else
	{
		pIcon->SetImage( pDef->GetIconName() );
	}

	bool bNewRequirementsMetState = pNode || pDef->BCanUnlock( GetQuestMapHelper() );
	if ( bNewRequirementsMetState && !m_bRequirementsMet && bEffects )
	{
		PlaySoundEntry( "CYOA.ObjectivePanelExpand" );
		Color colorStart = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
		colorStart.SetColor( colorStart.r(), colorStart.g(), colorStart.b(), 200 );
		Color colorEnd( colorStart.r(), colorStart.g(), colorStart.b(), 0 );

		KeyValues* pKVCreateCircle = new KeyValues( "CreateCircle" );
		pKVCreateCircle->SetColor( "start_color", colorStart );
		pKVCreateCircle->SetColor( "end_color", colorEnd );
		pKVCreateCircle->SetFloat( "start_time", Plat_FloatTime() );
		pKVCreateCircle->SetFloat( "end_time", Plat_FloatTime() + 1.f );
		pKVCreateCircle->SetFloat( "xpos", GetXPos() + GetWide() * 0.5f );
		pKVCreateCircle->SetFloat( "ypos", GetYPos() + GetTall() * 0.5f );
		pKVCreateCircle->SetFloat( "start_radius", 120.f );
		pKVCreateCircle->SetFloat( "end_radius", 115.f );
		pKVCreateCircle->SetBool( "filled", true );
		PostActionSignal( pKVCreateCircle );

	}
	m_bRequirementsMet = bNewRequirementsMetState;

	pIcon->SetDrawColor( pNode ? colorActive : Color( 100, 100, 100, 255 ) );
	SetControlVisible( "LockedIcon", !pNode && !pDef->BCanUnlock( GetQuestMapHelper() ) );
	SetControlVisible( "ItemIcon", ( pDef->GetRewardItem() ) && ( !pNode || !pNode->BHasLootBeenClaimed() ) );
	SetControlVisible( "CashIcon", !pNode || !pNode->BHasLootBeenClaimed() );
	SetControlVisible( "StarCount", !pNode, true );
	
	{
		if ( pNode )
		{
			m_pNameLabel->SetText( g_pVGuiLocalize->Find( pDef->GetNameLocToken() ) );
		}
		else
		{
			locchar_t locName[ MAX_ITEM_NAME_LENGTH ];
			loc_sprintf_safe( locName, LOCCHAR( "%ls: %dx" ), g_pVGuiLocalize->Find( pDef->GetNameLocToken() ), pDef->GetNumStarsToUnlock() );
			m_pNameLabel->SetText( locName );
			int nWide, nTall;
			m_pNameLabel->GetContentSize( nWide, nTall );
			m_pStarCostImage->SetPos( m_pNameLabel->GetXPos() + ( m_pNameLabel->GetWide() + nWide ) / 2, m_pStarCostImage->GetYPos() );
		}
		
		m_pStarCostImage->SetVisible( !pNode );
	}

	switch ( pDef->GetCashRewardType() )
	{
	case CASH_REWARD_LARGE: SetDialogVariable( "cash", "$$$" ); break;
	case CASH_REWARD_MEDIUM: SetDialogVariable( "cash", "$$" ); break;
	case CASH_REWARD_SMALL: SetDialogVariable( "cash", "$" ); break;
	case CASH_REWARD_NONE: SetDialogVariable( "cash", "" ); break;
	}

	Label* pCashIcon = FindControl< Label >( "CashIcon" );
	Panel* pItemIcon = FindChildByName( "ItemIcon" );
	if ( pCashIcon && pItemIcon )
	{
		pCashIcon->SizeToContents();
		int nRewardsWide = pCashIcon->GetWide() + ( pItemIcon->IsVisible() ? pItemIcon->GetWide() : 0 );
		//int nCashWide = pDef->GetCashRewardType() == CASH_REWARD_NONE ? 0 : pCashIcon->GetWide();
		pCashIcon->SetPos( GetWide() * 0.5f - nRewardsWide * 0.5f, pCashIcon->GetYPos() );
		pItemIcon->SetPos( pCashIcon->GetXPos() + pCashIcon->GetWide(), pItemIcon->GetYPos() );
	}

	EditablePanel* pTooltipRegion = FindControl< EditablePanel >( "ToolTipRegion" );
	if ( pTooltipRegion )
	{
		pTooltipRegion->InstallMouseHandler( m_pSelectButton, true, true );
		pTooltipRegion->SetTooltip( m_bRequirementsMet ? NULL : GetQuestMapPanel()->GetTextTooltip(), NULL );

		// Show the reason why they can't unlock
		if( !m_bRequirementsMet )
		{
			wchar_t wszBuff[ 1024 ];
			memset( wszBuff, 0, sizeof( wszBuff ) );
			pDef->GetCantUnlockReason( wszBuff, sizeof( wszBuff ) );
			pTooltipRegion->SetDialogVariable( "tiptext", wszBuff );
		}
	}
}

void DrawAmbientActiveCirlce( float flXPos, float flYPos, const Color& color )
{
	const float flNow = Plat_FloatTime();
	const float flSmallRadius =		YRES( node_small_radius );
	const float flMediumRadius =	YRES( node_medium_radius );
	const float flLargeRadius =		YRES( node_large_radius );

	const float flRingInterval = 1.f;
	for ( float flStepBack = 0.f; flStepBack < node_cycle_time; flStepBack += flRingInterval )
	{
		float flCycle = node_cycle_time - fmod( flNow - flStepBack, node_cycle_time );
		// Start a full alpha and fade out
		float flAlphaScale = Bias( RemapValClamped( flCycle, node_anim_time, 0.f, 0.f, 1.f ), 0.2f );
		float flAlpha = RemapValClamped( flAlphaScale, 0.f, 1.f, node_alpha_start, node_alpha_end );
		// Start at small radius and grow to large
		float flRadius = RemapValClamped( flCycle, node_anim_time, 0.f, 0.f, YRES( node_large_radius ) );

		Color colorPartial = color;
		colorPartial.SetColor( colorPartial.r(), colorPartial.g(), colorPartial.b(), flAlpha );
		// A circle that grows from small to large while fading out
		DrawColoredCircle( flXPos, flYPos, flRadius, colorPartial );
		colorPartial[3] = colorPartial[3] * 0.4f;
		DrawFilledColoredCircle( flXPos, flYPos, flRadius, colorPartial );
	}
}

void CQuestMapNodePanel::DrawNode( float flXPos,
								   float flYPos,
								   bool bPurchased,
								   const Color& colorActive,
								   const Color& colorBonus,
								   const Color& colorInactive,
								   float flScale ) const
{
	Color colorBlack( 0, 0, 0, 255 );

	const float flNow = Plat_FloatTime();
	const float flSmallRadius = YRES( node_small_radius ) * flScale;
	const float flMediumRadius = YRES( node_medium_radius ) * flScale;
	const float flLargeRadius = YRES( node_large_radius ) * flScale;

	// Black background to start
	DrawFilledColoredCircle( flXPos, flYPos, flMediumRadius + YRES( 1 ), colorBlack );

	auto pDef = GetNodeDef();
	int nNumSegments = 0;
	for( int i=0; i < EQuestPoints_ARRAYSIZE; ++i )
	{
		if ( pDef->BIsMedalOffered( (EQuestPoints)i ) )
		{
			++nNumSegments;
		}
	}

	float flGap = nNumSegments == 1 ? 0.f : 2.f;		// Gap between segment

	auto lambdaPaintCompletionSegment = [ & ]( int nSegment, const Color& color )
	{
		float flStart = (float)nSegment / (float)nNumSegments * 360.f + flGap;
		float flEnd = (float)(nSegment+1) / (float)nNumSegments * 360.f - flGap;
		DrawFilledColoredCircleSegment( flXPos, flYPos, flMediumRadius, flMediumRadius - YRES( 2 ), color, flStart, flEnd );
	};

	

	// Paint all the grey slots first.  We'll paint over them with orange later
	for( int i=0; i < nNumSegments; ++i )
	{
		lambdaPaintCompletionSegment( i, colorInactive );
	}

	// Paint an orange segment for earned categories
	if ( m_msgLocalState.star_0_earned() )
	{
		lambdaPaintCompletionSegment( 1, colorActive );
	}

	if ( m_msgLocalState.star_1_earned() )
	{
		lambdaPaintCompletionSegment( 2, colorBonus );
	}

	if ( m_msgLocalState.star_2_earned() )
	{
		lambdaPaintCompletionSegment( 0, colorBonus );
	}
}

void CQuestMapNodePanel::Paint()
{
	Color colorActive =  vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
	Color colorInactive = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_InactiveGrey", Color( 255, 255, 255, 255 ) );

	bool bBrighten = m_eMapState == MOUSE_OVER || m_eMapState == SELECTED;

	if ( bBrighten )
	{
		// Brighten up a little when selected and moused over
		int nGlow = node_mouseover_brightness;
		BrigthenColor( colorActive, nGlow );
		BrigthenColor( colorInactive, nGlow );
	}

	const float x = ( GetWide() * 0.5f ) - 0.5f;
	const float y = ( GetTall() * 0.5f ) - 0.5f;

	const float flSmallRadius = node_small_radius;
	const float flMediumRadius = node_medium_radius;
	const float flLargeRadius = node_large_radius;

	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNode( m_msgLocalState.defindex() );
	if ( !pNode )
	{
		// Still draw the node in staging for content creation
		DrawNode( x, y, false, colorActive, colorActive, colorInactive, 1.f );

		return;
	}

	float flScale = 1.f;

	if ( m_eMapState == MOUSE_OVER )
	{
		const float flSelectScalePulse = 1.2f;
		flScale *= RemapValClamped( Plat_FloatTime() - m_flMapStateEnterTime, 0.2f, 0.0f, 1.0f, flSelectScalePulse );
	}

	// Draw the node
	DrawNode( x, y, true, colorActive, colorActive, colorInactive, flScale );
}

void CQuestMapNodePanel::OnCursorEntered()
{
	if ( m_pSelectButton->IsCursorOver() )
	{
		PostActionSignal( new KeyValues("NodeCursorEntered", "node", m_msgLocalState.defindex() ) );
		m_bOverSelected = true;

		EnterMapState( MOUSE_OVER );
	}
}

void CQuestMapNodePanel::OnCursorExited()
{
	if ( !m_pSelectButton->IsCursorOver() && m_eMapState == MOUSE_OVER )
	{
		m_bOverSelected = false;
		PostActionSignal( new KeyValues("NodeCursorExited", "node", m_msgLocalState.defindex() ) );

		EnterMapState( NEUTRAL );
	}
}


void CQuestMapNodePanel::OnMousePressed( MouseCode code )
{
	if ( !m_pSelectButton->IsCursorOver() )
	{
		CallParentFunction( new KeyValues( "MousePressed", "code", code ) );
		return;
	}
	
	BaseClass::OnMousePressed( code );
}

void CQuestMapNodePanel::OnMouseDoublePressed( MouseCode code )
{
	if ( !m_pSelectButton->IsCursorOver() )
	{
		CallParentFunction( new KeyValues( "MousePressed", "code", code ) );
		return;
	}

	BaseClass::OnMouseDoublePressed( code );
}

void CQuestMapNodePanel::OnThink()
{
	BaseClass::OnThink();

}


void CQuestMapNodePanel::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "selected" ) )
	{
		// Play a sound when the cursor enters us
		if ( input()->IsKeyDown( KEY_LCONTROL ) )
		{
			CMsgProtoDefID msgDefId;
			msgDefId.set_type( DEF_TYPE_QUEST_MAP_NODE );
			msgDefId.set_defindex( m_msgLocalState.defindex() );
		}
		else
		{
			PostActionSignal( new KeyValues("NodeSelected", "node", m_msgLocalState.defindex() ) );
			EnterMapState( SELECTED );
		}
		return;
	}
}

void CQuestMapNodePanel::UpdateFromSObject( const CQuestMapNode* pMapNode )
{
	if ( pMapNode )
	{
		m_msgLocalState = pMapNode->Obj();
	}
}

const CQuestMapNodeDefinition* CQuestMapNodePanel::GetNodeDef() const
{
	return (const CQuestMapNodeDefinition*)GetProtoScriptObjDefManager()->GetDefinition( ProtoDefID_t( DEF_TYPE_QUEST_MAP_NODE , m_msgLocalState.defindex() ) );
}

void CQuestMapNodePanel::EnterMapState( EMapState eMapState )
{
	if ( m_eMapState == eMapState )
		return;

	m_eMapState = eMapState;

	bool bUnlocked = false;

	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNode( m_msgLocalState.defindex() );
	if ( pNode )
	{
		bUnlocked = true;
	}

	switch( eMapState )
	{
		case MOUSE_OVER:
		{
			if ( bUnlocked )
			{
				PlaySoundEntry( "CYOA.PingInProgress" );
			}
			else if( GetNodeDef()->BCanUnlock( GetQuestMapHelper() ) )
			{
				PlaySoundEntry( "CYOA.PingAvailable" );
			}
			else
			{
				PlaySoundEntry( "CYOA.NodeLocked" );
			}
		}
		break;
	}

	m_flMapStateEnterTime = Plat_FloatTime();
}
