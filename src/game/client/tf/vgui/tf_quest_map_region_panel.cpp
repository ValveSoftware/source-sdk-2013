//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tf_quest_map_region_panel.h"
#include "tf_quest_map_node_panel.h"
#include "tf_quest_map.h"
#include "tf_gc_client.h"
#include "tf_quest_map_node.h"
#include "tf_quest_map_panel.h"
#include "tf_quest_map_utils.h"
#include <vgui/ISurface.h>
#include "softline.h"
#include "tf_quest_map_controller.h"
#include "tf_quest_map_editor_panel.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include "tf_quest_map_node_view_panel.h"
#include "econ_gcmessages.h"
#include "vgui/IInput.h"

static const float tf_quest_map_zoom_in_scale( 4 );
static const float tf_quest_map_zoom_out_scale( 2.0f );
static const float tf_quest_map_zoom_rest_scale( 2.5f );
const float tf_quest_map_zoom_transition_in_time( 0.2f );
static const float tf_quest_map_grid_size( 30 );
static const float tf_quest_map_region_line_dash_length( 10 );
static const float tf_quest_map_region_line_dash_gap_length( 3 );
static const float tf_quest_map_direction_arrow_length( 7 );
static const float tf_quest_map_arrow_time( 5 );

void GetRegionLinkDotSpot( const EditablePanel* pRegionLink, int& x, int& y )
{
	// Yea we kinda lied about pRegionLink being const, but vgui's GetPos() is not const...
	EditablePanel* pNonConstRegionLink = const_cast< EditablePanel* >( pRegionLink );
	pNonConstRegionLink->GetPos( x, y );
	x += YRES( 13 );
	y += YRES( 13 ); 
}

CQuestMapPathsPanel::CQuestMapPathsPanel( Panel *pParent, const char *pszPanelname, float flZoomScale )
	: Panel( pParent, pszPanelname )
	, m_mapQuestNodes( DefLessFunc( uint32 ) )
	, m_mapRegionLinkPanels( DefLessFunc( uint32 ) )
	, m_flZoomScale( Max( 0.1f, flZoomScale ) )
{
	m_nWhiteTexture = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( m_nWhiteTexture, "vgui/white", true, false );
}

template< typename F >
void PaintDirectionArrow( Vector2D vecStart, Vector2D vecDir, float flLength, F && lambdaPreDraw )
{
	// Now we want to draw a direction indicator halfway along the line
	float flPercent = fmod( Plat_FloatTime(), tf_quest_map_arrow_time ) / tf_quest_map_arrow_time;
	float flDist = ( flLength * flPercent );
	Vector2D vecTip( vecStart + ( vecDir * flDist ) );
	Vector2D vecReverse( vecDir.y, -vecDir.x );
	float flArrowLength = YRES( tf_quest_map_direction_arrow_length );
	vecReverse *= flArrowLength;
	Vector2D vecLeftTip( vecTip + vecReverse - ( vecDir * flArrowLength ) );
	Vector2D vecRightTip( vecTip - vecReverse - ( vecDir * flArrowLength ) );

	lambdaPreDraw( flPercent );

	vgui::Vertex_t arArrowVtx[ 3 ];
	arArrowVtx[ 0 ].Init( vecTip );
	arArrowVtx[ 1 ].Init( vecRightTip );
	arArrowVtx[ 2 ].Init( vecLeftTip );

	vgui::surface()->DrawTexturedPolygon( 3, arArrowVtx );
}

void DrawGrid( float flXOffset, float flYOffset, float flHorizWide, float flVertTall, float flSpan, int nAlpha )
{
	// How far apart each line is
	vgui::surface()->DrawSetColor( Color( 255, 255, 255, nAlpha ) );

	vgui::Vertex_t start, end;
	// We're wider than we are tall, so go to the width
	for( float nLinePos = 0; nLinePos < flHorizWide; nLinePos += flSpan )
	{
		float flLineXPos = nLinePos + flXOffset;
		float flLineYPos = nLinePos + flYOffset;

		// Vertical
		start.Init( Vector2D( flLineXPos, 0 ), Vector2D( 0, 0 ) );
		end.Init( Vector2D( flLineXPos, flVertTall ), Vector2D( 0, 0 ) );
		SoftLine::DrawPolygonLine( start, end, 2 );

		// Horizontal
		start.Init( Vector2D( 0, flLineYPos ), Vector2D( 0, 0 ) );
		end.Init( Vector2D( flHorizWide, flLineYPos ), Vector2D( 0, 0 ) );
		SoftLine::DrawPolygonLine( start, end, 2 );	
	}
}

void CQuestMapPathsPanel::Paint()
{
	BaseClass::Paint();

	vgui::surface()->DrawSetTexture( m_nWhiteTexture );
	const Color& colorActive =  vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
	const Color colorInactive( 0, 0, 0, 255 );

	//
	// Draw the grid behind the nodes
	//
	{
		const float flSpan = (float)GetWide() / tf_quest_map_grid_size * m_flZoomScale;
		DrawGrid( 0.f, 0.f, GetWide(), GetTall(), flSpan, 10 );
	}

	// Paint the ambient circle for any active region links
	if ( m_pActiveRegionLinkPanel )
	{
		int nX, nY;
		GetRegionLinkDotSpot( m_pActiveRegionLinkPanel, nX, nY );
		DrawAmbientActiveCirlce( nX, nY, colorActive );
	}

	CUtlVector< uint64 > vecDrawnPaths;

	const float flDashLength = tf_quest_map_region_line_dash_length / ( 2000.f / GetWide() ) * m_flZoomScale;
	const float flDashGapLength = tf_quest_map_region_line_dash_gap_length / ( 2000.f / GetWide() ) * m_flZoomScale;

	//
	// Go through each node within this region, then go through each of that node's connected nodes, and draw
	// a line between them to illustrate their logical connection.  Some nodes are in different regions and we
	// represent that relationship as a dashed line going from node to region link (or off the side if it's "above")
	//
	FOR_EACH_MAP_FAST( m_mapQuestNodes, i )
	{
		CUtlMap< uint32, bool > mapOutConnectedNodeCondition( DefLessFunc( uint32 ) );

		CQuestMapNodePanel* pNodePanel = m_mapQuestNodes[ i ];

		auto lambdaBrightenIfSelected = [&]( CQuestMapNodePanel* pStartNodepanel, Color& colorToModify )
		{
			if ( pStartNodepanel->GetState() == CQuestMapNodePanel::SELECTED || 
				 pStartNodepanel->GetState() == CQuestMapNodePanel::MOUSE_OVER )
			{
				BrigthenColor( colorToModify, 100 );
			}
		};

		// Draw the ambient active circle if this is the active node
		const CQuest* pQuest = GetQuestMapHelper().GetQuestForNode( pNodePanel->GetLocalState().node_id() );
		const CQuestMapNodeDefinition* pNodeDef = pNodePanel->GetNodeDef();
		if ( GetQuestMapHelper().BCanNodeBeTurnedIn( pNodeDef->GetDefIndex() ) )
		{
			const Color& colorTurnIn =  vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "CreditsGreen", Color( 255, 255, 255, 255 ) );
			DrawAmbientActiveCirlce( pNodePanel->GetXPos() + pNodePanel->GetWide() * 0.5f,
									 pNodePanel->GetYPos() + pNodePanel->GetTall() * 0.5f,
									 colorTurnIn );
		}
		else if ( pQuest && pQuest->Obj().active() )
		{
			DrawAmbientActiveCirlce( pNodePanel->GetXPos() + pNodePanel->GetWide() * 0.5f,
									 pNodePanel->GetYPos() + pNodePanel->GetTall() * 0.5f,
									 colorActive );

		}

	
	
		const EdgeVec_t& vecEdges = pNodeDef->GetLinkedNodes();

		FOR_EACH_VEC( vecEdges, j )
		{
			const Edges_t& edge = vecEdges[ j ];
			const CQuestMapNodeDefinition* pLinkedNodeDef = edge.m_pNode;

			// We don't want to double-draw any lines, so check if this node-pair has been drawn already
			uint64 key = GetNodeDefPairKey( pNodeDef->GetDefIndex(), pLinkedNodeDef->GetDefIndex() );
			auto drawnidx = vecDrawnPaths.Find( key );
			if ( drawnidx != vecDrawnPaths.InvalidIndex() )
				continue;

			// Not drawn yet, but we're about to.  Mark it for future iteration
			vecDrawnPaths.AddToTail( key );

			bool bAvailable = GetQuestMapHelper().BIsNodeConnectionFulfilled( pNodeDef, pLinkedNodeDef );

			auto idx = m_mapQuestNodes.Find( pLinkedNodeDef->GetDefIndex() );
			if ( idx != m_mapQuestNodes.InvalidIndex() )
			{
				//
				// This is the easy case.  The node is within this region so we'll just draw a line to it
				//
				CQuestMapNodePanel* pRelatedPanel = m_mapQuestNodes[ idx ];

				CQuestMapNodePanel* pStartNode = edge.m_eConnection == CONNECTS_TO ? pNodePanel : pRelatedPanel;
				CQuestMapNodePanel* pEndNode = edge.m_eConnection == CONNECTS_TO ? pRelatedPanel : pNodePanel;

				Color colorToDraw = bAvailable ? colorActive : colorInactive;

				// Brighten up if the start node is selected
				lambdaBrightenIfSelected( pStartNode, colorToDraw );

				Assert( pStartNode != pEndNode );

				// Different color if the connection is fulfilled
				vgui::Vertex_t start, end;
				vgui::surface()->DrawSetColor( colorToDraw );

				// Draw a line from the center of the starting node to the center of the related node
				start.Init( Vector2D( pStartNode->GetXPos() + ( pStartNode->GetWide() / 2 ), pStartNode->GetYPos() + ( pStartNode->GetTall() / 2 ) ), Vector2D( 0, 0 ) );
				end.Init(   Vector2D( pEndNode->GetXPos()   + ( pEndNode->GetWide()   / 2 ), pEndNode->GetYPos()   + ( pEndNode->GetTall()   / 2 ) ), Vector2D( 1, 1 ) );
				SoftLine::DrawPolygonLine( start, end, 5 );

				// Draw directional arrows
				Vector2D vecDir( end.m_Position - start.m_Position );
				vec_t flLength = vecDir.NormalizeInPlace();
				PaintDirectionArrow( start.m_Position, vecDir, flLength, [](float){} );
			}
			else
			{
				//
				// The not-so-easy case.  The node is in a different region.  It could be "above" or "below" us.
				// Either way, we need to figure out the relation of the region link to the node and draw a dashed
				// line between
				//

				Vector2D vecStart( Vector2D( pNodePanel->GetXPos() + ( pNodePanel->GetWide() / 2 ), pNodePanel->GetYPos() + ( pNodePanel->GetTall() / 2 ) ) );
				Vector2D vecEnd;
				Vector2D vecRegionLinkCenter( -1000, -1000 );

				bool bDrawArrow = true;

				auto regionidx = m_mapRegionLinkPanels.Find( pLinkedNodeDef->GetRegionDefIndex() );
				if ( regionidx == m_mapRegionLinkPanels.InvalidIndex() )
				{
					//
					// The very not-so-easy case.  The node is above us, which means we don't have a link to view that
					// region.  So what we're going to do is go to the region that owns the other node, then get the vector
					// from the other node to the link of our current region, then draw THAT line off the edge of the map
					// with a dash to represent that it's beyond the scope of this region.
					//

					// Our region containing the other node
					const CQuestMapRegionPanel* pOuterRegionPanel = GetQuestMapPanel()->GetRegionPanel( pLinkedNodeDef->GetRegionDefIndex() );
					if ( !pOuterRegionPanel )
						continue;

					// The other node's panel
					const CQuestMapNodePanel* pOuterNodePanel = pOuterRegionPanel->GetNodePanel( pLinkedNodeDef->GetDefIndex() );
					if ( !pOuterNodePanel )
						continue;

					// The current region's region link panel
					const EditablePanel* pLinkPanel = pOuterRegionPanel->GetRegionLinkPanel( pNodeDef->GetRegionDefIndex() );
					if ( !pLinkPanel )
						continue;

					int x, y;
					// Get the node's center
					CQuestMapNodePanel* pNonConstOuterNodePanel = const_cast< CQuestMapNodePanel* >( pOuterNodePanel );
					pNonConstOuterNodePanel->GetPos( x, y );
					x += pNonConstOuterNodePanel->GetWide() / 2;
					y += pNonConstOuterNodePanel->GetTall() / 2;
					Vector2D vecLinkPos( x, y );

					// Get the region link's dot spot
					GetRegionLinkDotSpot( pLinkPanel, x, y );
					Vector2D vecLinkNodePos( x, y );

					// Get the vector between
					Vector2D vecLinkDir = vecLinkPos - vecLinkNodePos;
					
					// Make a generous target along that path
					vecEnd = vecStart + ( vecLinkDir * 10 );
					bDrawArrow = false;
				}
				else
				{
					// Not too tricky.  The other node is within a region that we have a link to.  Just draw a dashed
					// line to the region link.
					EditablePanel* pRelatedPanel = m_mapRegionLinkPanels[ regionidx ];
					int x, y;
					GetRegionLinkDotSpot( pRelatedPanel, x, y );
					vecEnd.Init( x, y );
					vecRegionLinkCenter.Init( pRelatedPanel->GetXPos() + ( pRelatedPanel->GetWide() / 2 ), pRelatedPanel->GetYPos() + ( pRelatedPanel->GetTall() / 2 ) );
				}

				vgui::Vertex_t start, end;

				// Draw a line from the center of the starting node to the center of the related node
				Vector2D vecDir = vecEnd - vecStart;
				float flLength = vecDir.NormalizeInPlace();

				Color colorDashLine = bAvailable ? colorActive : colorInactive;
			

				if ( bDrawArrow )
				{
					// Brighten up if we're highlighted
					lambdaBrightenIfSelected( pNodePanel, colorDashLine );

					// Now we want to draw a direction indicator
					PaintDirectionArrow( vecStart, vecDir, flLength, [&]( float flPercent )
					{ 
						colorDashLine.SetColor( colorDashLine.r(), colorDashLine.g(), colorDashLine.b(), RemapValClamped( flPercent, 0.5, 1, 255, 0 ) );
						vgui::surface()->DrawSetColor( colorDashLine );
					} );
				}

				bool bContinue = true;

				// Draw dashes from the start to the end
				while( bContinue )
				{
					// Where the dash ends
					Vector2D vecDashEnd = vecStart + vecDir * flDashLength;
					float flDistRemaining = ( vecEnd - vecDashEnd ).Length();

					// We want to fade the dash line out if we're passing through a region link panel (helps the link panel real better)
					float flDistToLinkCenter = ( vecDashEnd - vecRegionLinkCenter ).Length();

					colorDashLine.SetColor( colorDashLine.r(), colorDashLine.g(), colorDashLine.b(), RemapValClamped( flDistToLinkCenter, 20, 120, 0, 255 ) );
					vgui::surface()->DrawSetColor( colorDashLine );

					// Draw the dash
					start.Init( vecStart, Vector2D( 0, 0 ) );
					end.Init( vecDashEnd, Vector2D( 1, 1 ) );
					SoftLine::DrawPolygonLine( start, end, 5 );

					// If we're close enough, or went of the edge of the screen, then we can stop
					if ( flDistRemaining < flDashLength || vecDashEnd.x < 0 || vecDashEnd.y < 0 || vecDashEnd.x > GetWide() || vecDashEnd.y > GetTall() )
					{
						bContinue = false;
					}
					else
					{
						// Step towards our goal
						vecStart += vecDir * flDashLength; // The dash
						vecStart += vecDir * flDashGapLength;// The gap
					}
				}
			}
		}
	}

	m_circleDrawer.PaintCircles();
}

void CQuestMapPathsPanel::AddNode( CQuestMapNodePanel* pNodePanel )
{
	m_mapQuestNodes.Insert( pNodePanel->GetLocalState().defindex(), pNodePanel );
}

void CQuestMapPathsPanel::RemoveNode( uint32 nDefindex )
{
	m_mapQuestNodes.RemoveAt( m_mapQuestNodes.Find( nDefindex ) );
}

void CQuestMapPathsPanel::AddRegion( EditablePanel* pRegionPanel, uint32 nDefIndex )
{
	m_mapRegionLinkPanels.Insert( nDefIndex, pRegionPanel );
}

void CQuestMapPathsPanel::RemoveRegion( uint32 nDefIndex )
{
	m_mapRegionLinkPanels.RemoveAt( m_mapRegionLinkPanels.Find( nDefIndex ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CQuestMapRegionPanel::CQuestMapRegionPanel( Panel *pParent, const char *pszPanelName, const CMsgProtoDefID& msgIDCurrentRegion, CQuestNodeViewPanel* pNodeViewPanel )
	: BaseClass( pParent, pszPanelName )
	, m_mapNodePanels( DefLessFunc( uint32 ) )
	, m_mapRegionLinkPanels( DefLessFunc( uint32 ) )
	, m_msgIDCurrentRegion( msgIDCurrentRegion )
	, m_pQuestMapNodeView( pNodeViewPanel )
	, m_flLastClickTime( 0.f )
	, m_flClickScale( 1.f )
	, m_flZoomTime( 0.f )
	, m_flZoomStartTime( 0.f )
{
	const CQuestMapRegion* pRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( m_msgIDCurrentRegion.defindex() );

	Assert( m_pQuestMapNodeView );
	m_pQuestMapNodeView->AddActionSignalTarget( this );
	m_pZoomPanel = new CDraggableScrollingPanel( this, "ZoomPanel" );
	m_pZoomPanel->InstallMouseHandler( this );
	m_pPathsPanel = new CQuestMapPathsPanel( m_pZoomPanel, "PathsPanel", pRegion ? pRegion->GetZoomScale() : 1.f );
	m_pZoomPanel->AddOrUpdateChild( m_pPathsPanel, true, false, CDraggableScrollingPanel::PIN_CENTER );
	m_pBGImage = new ImagePanel( m_pZoomPanel, "BGImage" );
	m_pZoomPanel->AddOrUpdateChild( m_pBGImage, true, true, CDraggableScrollingPanel::PIN_CENTER );
	m_pDimmer = new Panel( this, "Dimmer" );

	ListenForGameEvent( "proto_def_changed" );
	ListenForGameEvent( "quest_response" );
	ListenForGameEvent( "quest_map_data_changed" );
	ListenForGameEvent( "quest_turn_in_state" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );
}

CQuestMapRegionPanel::~CQuestMapRegionPanel()
{}

void CQuestMapRegionPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	const CQuestMapRegion* pRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( m_msgIDCurrentRegion.defindex() );
	if ( !pRegion )
	{
		LoadControlSettings( "Resource/ui/quests/CYOA/regions/region_base.res" );
	}
	else
	{
		LoadControlSettings( pRegion->GetResFile() );
	}

	UpdateControls();
}


void CQuestMapRegionPanel::OnCommand( const char *pCommand )
{
	if ( Q_strnicmp( "link", pCommand, 4 ) == 0 )
	{
		pCommand = strchr( pCommand, ' ' );
		int nRegionDefIndex = atoi( pCommand + 1 );

		PostActionSignal( new KeyValues( "RegionSelected", "defindex", nRegionDefIndex ) );
	}
	else if ( FStrEq( "back", pCommand ) ) 
	{
		PostActionSignal( new KeyValues( "RegionBackout" ) );
	}
}

void CQuestMapRegionPanel::OnTick()
{
	BaseClass::OnTick();

	if ( IsLayoutInvalid() )
		return;

	if ( !IsVisible() )
	{
		return;
	}

	m_pZoomPanel->SetZoomAmount( m_flZoomScale, m_flZoomX * GetWide(), m_flZoomY * GetTall() );

	int nX, nY, nWide, nTall;
	m_pZoomPanel->GetBounds( nX, nY, nWide, nTall );

	int nXPadding = abs( nWide - GetWide() );
	int nYPadding = abs( nTall - GetTall() );

	float flZoomPercent = 1.f - ( abs( m_flZoomScale - tf_quest_map_zoom_rest_scale ) / ( tf_quest_map_zoom_in_scale - tf_quest_map_zoom_rest_scale ) );

	float flZoomX = m_flZoomX;
	float flZoomY = m_flZoomY;

	if ( m_flZoomScale < tf_quest_map_zoom_rest_scale )
	{
		flZoomX = 1.f - flZoomX;
		flZoomY = 1.f - flZoomY;
	}

	int nDesiredX = Lerp( flZoomPercent, -( ( flZoomX * nWide ) - ( GetWide() * flZoomX ) ), -nXPadding / 2.f);
	int nDesiredY = Lerp( flZoomPercent, -( ( flZoomY * nTall ) - ( GetTall() * flZoomY ) ), -nYPadding / 2.f);

	nX = Clamp( nDesiredX, -nXPadding, 0);
	nY = Clamp( nDesiredY, -nYPadding, 0);

	m_pZoomPanel->SetPos( nDesiredX, nDesiredY );
}

void CQuestMapRegionPanel::SOChangeEvent()
{
	UpdateControls();
}

void CQuestMapRegionPanel::CloseNodeView()
{
	m_pQuestMapNodeView->SetVisible( false );
}

void CQuestMapRegionPanel::CreateActivationCircle( KeyValues *pParams )
{
	uint32 nNodeDefindex = pParams->GetInt( "defindex" );
	const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNode( nNodeDefindex );
	if ( !pNode )
		return;

	if ( pNode->GetNodeDefinition()->GetRegionDefIndex() != m_msgIDCurrentRegion.defindex() )
		return;

	unsigned short nKey = pNode->GetNodeDefinition()->GetDefIndex();
	auto idx = m_mapNodePanels.Find( nKey );
	if ( idx == m_mapNodePanels.InvalidIndex() )
	{
		Assert( false );
		return;
	}

	CQuestMapNodePanel* pNodePanel = m_mapNodePanels[ idx ];

	Color colorStart = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
	Color colorEnd( colorStart.r(), colorStart.g(), colorStart.b(), 0.f );

	m_pPathsPanel->GetCircleDrawer().AddCircle( { Plat_FloatTime()
							  , Plat_FloatTime() + 1.f
							  , pNodePanel->GetXPos() + pNodePanel->GetWide() * 0.5f
							  , pNodePanel->GetYPos() + pNodePanel->GetTall() * 0.5f
							  , 0.f
							  , 200.f
							  , colorStart
							  , colorEnd
							  , true } );
}

void CQuestMapRegionPanel::CreateCircle( KeyValues* pParams )
{
	m_pPathsPanel->GetCircleDrawer().AddCircle( { pParams->GetFloat( "start_time", Plat_FloatTime() )
												, pParams->GetFloat( "end_time", Plat_FloatTime() + 1.f )
												, pParams->GetFloat( "xpos", 0.f )
												, pParams->GetFloat( "ypos", 0.f )
												, pParams->GetFloat( "start_radius", 0.f )
												, pParams->GetFloat( "end_radius", 200.f )
												, pParams->GetColor( "start_color" )
												, pParams->GetColor( "end_color" )
												, pParams->GetBool( "filled", true ) } );
}

void CQuestMapRegionPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( "quest_map_data_changed", event->GetName() ) )
	{
		UpdateControls();
		return;
	}
	
	if ( FStrEq( "quest_response", event->GetName() ) )
	{
		int nRequest = event->GetInt( "request" );
		bool bSuccess = event->GetBool( "success" );

		if ( nRequest == k_EMsgGC_QuestMapUnlockNode && bSuccess )
		{
			//
			// The request to select a quest came back as success.  Draw an expanding ring
			// 

			CMsgGCQuestMapUnlockNode msg;
			msg.ParseFromString( event->GetString( "msg" ) );

			PostMessage( this, new KeyValues( "CreateActivationCircle", "defindex", msg.node_defindex() ) );
			PlaySoundEntry( "CYOA.NodeActivate" );
		}
	}
	else if ( FStrEq( event->GetName(), "quest_turn_in_state" ) )
	{
		EQuestTurnInState eState = (EQuestTurnInState)event->GetInt( "state" );

		switch( eState )
		{
		case TURN_IN_BEGIN:
		{
			// This is to prevent our state from updating due to SOCache changes coming from turning in.
			m_bTurningIn = true;
			break;
		}

		case TURN_IN_SHOW_NODE_UNLOCKS:
		{
			auto& mapNodeDefs = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_NODE );
			FOR_EACH_MAP_FAST( mapNodeDefs, i )
			{
				const CQuestMapNodeDefinition* pNodeDef = (const CQuestMapNodeDefinition*)mapNodeDefs[ i ];
				auto idx = m_mapNodePanels.Find( pNodeDef->GetDefIndex() );

				//
				// Check if we need to create/update/destroy a node based on regions
				//
				if ( pNodeDef->GetRegionDefIndex() == m_msgIDCurrentRegion.defindex() )
				{
					if ( idx != m_mapNodePanels.InvalidIndex() )
					{
						// Node already exists. Just update it.
						m_mapNodePanels[ idx ]->UpdateFromSObject( GetQuestMapHelper().GetQuestMapNode( pNodeDef->GetDefIndex() ) );
					}
				}
			}

			// Have the nodes update
			FOR_EACH_MAP_FAST( m_mapNodePanels, i )
			{
				PostMessage( m_mapNodePanels[ i ]->GetVPanel(), new KeyValues( "UpdateStateVisuals", "effects", true ), i * 0.2f );
			}
			break;
		}

		case TURN_IN_COMPLETE:
		{
			m_bTurningIn = false;
			break;
		}

		// Nothing to do for these
		case TURN_IN_SHOW_SUCCESS:
		case TURN_IN_HIDE_SUCCESS:
		case TURN_IN_SHOW_STARS_EARNED:
		case TURN_IN_SHOW_BLOOD_MONEY_EARNED:
		case TURN_IN_SHOW_ITEM_PICKUP_SCREEN:
		case TURN_IN_SHOW_FAILURE:
		case TURN_IN_HIDE_FAILURE:
		case TURN_IN_HIDE_NODE_VIEW:
			break;
		};
	}

	bool bReload = false;
	if ( FStrEq( "proto_def_changed", event->GetName() ) && 
		 ( event->GetInt( "type" ) == DEF_TYPE_QUEST_MAP_NODE || event->GetInt( "type" ) == DEF_TYPE_QUEST_MAP_REGION ) )
	{
		bReload = true;
	}

	if ( bReload )
	{
		FOR_EACH_MAP_FAST( m_mapNodePanels, i )
		{
			m_pPathsPanel->RemoveNode( m_mapNodePanels.Key( i ) );
			m_mapNodePanels[ i ]->MarkForDeletion();
			m_pZoomPanel->OnChildRemoved( m_mapNodePanels[ i ] );
		}
		m_mapNodePanels.Purge();

		FOR_EACH_MAP_FAST( m_mapRegionLinkPanels, i )
		{
			m_pPathsPanel->RemoveRegion( m_mapRegionLinkPanels.Key( i )  );
			m_mapRegionLinkPanels[ i ]->MarkForDeletion();
			m_pZoomPanel->OnChildRemoved( m_mapRegionLinkPanels[ i ] );
		}
		m_mapRegionLinkPanels.Purge();

		UpdateControls();
	}
}

void CQuestMapRegionPanel::NodeSelected( KeyValues *pParams )
{
	if ( BIsZooming() )
		return;

	unsigned short nKey = pParams->GetInt( "node" );
	auto idx = m_mapNodePanels.Find( nKey );

	if ( m_mapNodePanels.InvalidIndex() == idx )
		return;

	FOR_EACH_MAP_FAST( m_mapNodePanels, i )
	{
		if ( i != idx )
		{
			m_mapNodePanels[ i ]->EnterMapState( CQuestMapNodePanel::NEUTRAL );
		}
	}

	CQuestMapNodePanel* pNodePanel = m_mapNodePanels[ idx ];
	pNodePanel->EnterMapState( CQuestMapNodePanel::SELECTED );
	m_pQuestMapNodeView->UpdateQuestViewPanelForNode( pNodePanel );
	m_pQuestMapNodeView->MakeReadyForUse();
	m_pQuestMapNodeView->SetVisible( true );
	
	// Bring up the dimmer so it's a bit easier to read the node view panel
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pDimmer, "alpha", 200, 0.0f, 0.2f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, true, false );
}

void CQuestMapRegionPanel::NodeCursorEntered( KeyValues *pParams )
{
	if ( BIsZooming() )
		return;

	unsigned short nKey = pParams->GetInt( "node" );
	auto idx = m_mapNodePanels.Find( nKey );
	if ( idx == m_mapNodePanels.InvalidIndex() )
	{
		Assert( false );
		return;
	}

	CQuestMapNodePanel* pNodePanel = m_mapNodePanels[ idx ];

	// Draw an expanding ring when the mouse enters a node
	Color colorActive = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
	Color colorInactive = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_InactiveGrey", Color( 255, 255, 255, 255 ) );
	Color& colorStart = pNodePanel->BRequirementsMet() ? colorActive : colorInactive;

	if ( pNodePanel->BRequirementsMet() )
	{
		colorStart[ 3 ] = 100;
	}

	Color colorEnd( colorStart );
	colorEnd[ 3 ] = 0;

	m_pPathsPanel->GetCircleDrawer().AddCircle( { Plat_FloatTime()
							  , Plat_FloatTime() + 0.4
							  , (float)pNodePanel->GetXPos() + ( pNodePanel->GetWide() * 0.5f ) + float( YRES( 0.5f ) )
							  , (float)pNodePanel->GetYPos() + ( pNodePanel->GetTall() * 0.5f ) + float( YRES( 0.5f ) )
							  , (float)YRES( node_medium_radius ) + (float)YRES( 5 )
							  , (float)YRES( node_large_radius )
							  , colorStart
							  , colorEnd
							  , false } );

	m_pPathsPanel->GetCircleDrawer().AddCircle( { Plat_FloatTime()
							  , Plat_FloatTime() + 0.4
							  , (float)pNodePanel->GetXPos() + ( pNodePanel->GetWide() * 0.5f ) + float( YRES( 0.5f ) )
							  , (float)pNodePanel->GetYPos() + ( pNodePanel->GetTall() * 0.5f ) + float( YRES( 0.5f ) )
							  , (float)YRES( node_medium_radius )
							  , (float)YRES( node_large_radius )
							  , colorStart
							  , colorEnd
							  , true } );
}

void CQuestMapRegionPanel::NodeViewClosed()
{
	FOR_EACH_MAP_FAST( m_mapNodePanels, i )
	{
		m_mapNodePanels[ i ]->EnterMapState( CQuestMapNodePanel::NEUTRAL );
	}

	// Lower the dimmer
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pDimmer, "alpha", 0, 0.0f, 0.2f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, true, false );
}

void CQuestMapRegionPanel::CreateClickCircle()
{
	int nMouseX, nMouseY;
	g_pVGuiInput->GetCursorPosition( nMouseX, nMouseY );
	m_pPathsPanel->ScreenToLocal( nMouseX, nMouseY );

	Color colorStart = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
	Color colorEnd( colorStart.r(), colorStart.g(), colorStart.b(), 0.f );

	PlaySoundEntry( "CYOA.NodeAbsent" );

	// Decay the click based on time passed. If they click fast enough the ring will grow. 
	// Why? Because it's fun.
	float flTimeSinceClick = Plat_FloatTime() - m_flLastClickTime;
	m_flClickScale = Max( 1.f, m_flClickScale - ( flTimeSinceClick * 5.f ) );

	m_pPathsPanel->GetCircleDrawer().AddCircle( { Plat_FloatTime()
							  , Plat_FloatTime() + ( 0.3f * m_flClickScale )
							  , (float)nMouseX
							  , (float)nMouseY
							  , 5.f
							  , 30.f * m_flClickScale
							  , colorStart
							  , colorEnd
							  , false } );

	// Expand the radius for every click
	m_flLastClickTime = Plat_FloatTime();
	m_flClickScale = Min( 500.f, m_flClickScale + 1.f );
}

void CQuestMapRegionPanel::OnMousePressed( MouseCode code )
{
	m_pQuestMapNodeView->SetVisible( false );

	FOR_EACH_MAP_FAST( m_mapNodePanels, i )
	{
		m_mapNodePanels[ i ]->EnterMapState( CQuestMapNodePanel::NEUTRAL );
	}

	if ( code == MOUSE_LEFT ) 
	{
		CreateClickCircle();
	} 
}

void CQuestMapRegionPanel::OnMouseDoublePressed( MouseCode code )
{
	m_pQuestMapNodeView->SetVisible( false );

	if ( code == MOUSE_LEFT ) 
	{
		CreateClickCircle();
	} 
}

bool BRecursiveGetRegionNodeState( const CQuestMapRegion* pRegion,
								  int& nNumNodesAvailable,
								  int& nNumTotalNodes,
								  int& nNumNodesCompleted,
								  bool& bHasActiveNode )
{
	bool bHasAnyActiveNodes = false;
	const DefinitionMap_t& mapNodeDefs = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_NODE );
	FOR_EACH_MAP_FAST( mapNodeDefs, nNodeIdx )
	{
		auto pNodeDef = assert_cast< const CQuestMapNodeDefinition* >( mapNodeDefs[ nNodeIdx ] );
		const CQuestMapNode* pNode = GetQuestMapHelper().GetQuestMapNode( pNodeDef->GetDefIndex() );

		if ( pNodeDef->GetRegionDefIndex() != pRegion->GetDefIndex() || pNodeDef->GetHeader().prefab_only() )
			continue;

		if ( pNodeDef->BIsActive() )
			bHasAnyActiveNodes = true;

		for( int j=0; j < EQuestPoints_ARRAYSIZE; ++j )
		{
			if ( !pNodeDef->BIsMedalOffered( (EQuestPoints)j ) )
				continue;

			// Tally up how many stars they COULD go earn right now
			if ( pNode )
			{
				if ( !pNode->BIsMedalEarned( (EQuestPoints)j ) )
					nNumNodesAvailable += 1;		
				else
					nNumNodesCompleted += 1;

				const CQuest* pQuest = GetQuestMapHelper().GetQuestForNode( pNode->GetID() );
				if ( pQuest && pQuest->Obj().active() )
					bHasActiveNode = true;
			}
			else if ( !pNode )
			{
				nNumNodesAvailable += 1; 
			}
		}
	}

	for( int i=0; i < pRegion->GetNumLinks(); ++i )
	{
		const CQuestMapRegion* pSubRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( pRegion->GetLink( i ).target_region_defid().defindex() );
		bHasAnyActiveNodes |= BRecursiveGetRegionNodeState( pSubRegion, nNumNodesAvailable, nNumTotalNodes, nNumNodesCompleted, bHasActiveNode );
	}

	return bHasAnyActiveNodes;
}

void ConcatRegionName( const CQuestMapRegion* pRegion,
					   wchar_t* pwszRegionPathName,
					   size_t nStrLen )
{
	V_wcsncat( pwszRegionPathName, L"\\", nStrLen );
	wchar_t* wpszRegionName = g_pVGuiLocalize->Find( pRegion->GetRegionNameLocToken() );

	// This can fail while editing regions and nodes
	if ( !wpszRegionName )
	{
		Assert( wpszRegionName );
		return;
	}
	
	V_wcsncat( pwszRegionPathName, wpszRegionName, nStrLen );
}

void RecursiveGenerateRegionPathName( const CQuestMapRegion* pRegion,
									  wchar_t* pwszRegionPathName,
									  size_t nStrLen )
{
	const DefinitionMap_t& mapRegions = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_REGION );
	FOR_EACH_MAP_FAST( mapRegions, nRegionIdx )
	{
		auto pRegionDef = assert_cast< const CQuestMapRegion* >( mapRegions[ nRegionIdx ] );

		for( int i=0; i < pRegionDef->GetNumLinks(); ++i )
		{
			const CQuestMapRegion* pSubRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( pRegionDef->GetLink( i ).target_region_defid().defindex() );
			if ( pSubRegion == pRegion )
			{
				RecursiveGenerateRegionPathName( pRegionDef, pwszRegionPathName, nStrLen );
				ConcatRegionName( pRegionDef, pwszRegionPathName, nStrLen );
				return;
			}
		}
	}
}

void CQuestMapRegionPanel::UpdateControls()
{
	if ( m_bTurningIn )
		return;

	if ( !steamapicontext || !steamapicontext->SteamUser() )
	{
		return;
	}

	CSteamID steamIDOwner = steamapicontext->SteamUser()->GetSteamID();

	const CQuestMapRegion* pRegion = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( m_msgIDCurrentRegion.defindex() );
	Assert( pRegion );
	if ( !pRegion )
		return;

	m_pPathsPanel->SetActiveRegion( NULL );

	// Create link panels from all the links specified in the region def
	for ( int i=0; i < pRegion->GetNumLinks(); ++i )
	{
		const CMsgQuestMapRegionDef_RegionLink& msgRegionLink = pRegion->GetLink( i );
		const CQuestMapRegion* pLinkedRegionDef = GetProtoScriptObjDefManager()->GetTypedDefinition< CQuestMapRegion >( msgRegionLink.target_region_defid().defindex() );

		// Gather some data about the region
		int nNumAvailable = 0;
		int nNumTotal = 0;
		int nNumCompleted = 0;
		bool bHasActive = false;
		if ( !BRecursiveGetRegionNodeState( pLinkedRegionDef, nNumAvailable, nNumTotal, nNumCompleted, bHasActive ) )
			continue;

		auto linkIdx = m_mapRegionLinkPanels.Find( pLinkedRegionDef->GetDefIndex() );
		if ( linkIdx == m_mapRegionLinkPanels.InvalidIndex() )
		{
			linkIdx = m_mapRegionLinkPanels.Insert( pLinkedRegionDef->GetDefIndex(), new EditablePanel( m_pZoomPanel, "Link" ) );
			m_pPathsPanel->AddRegion( m_mapRegionLinkPanels[ linkIdx ], pLinkedRegionDef->GetDefIndex() );
		}

		// The link panel is just an editable panel that we do a LoadControlSettings on.
		// It has a button in it that we'll point to ourselves
		EditablePanel* pRegionLink = m_mapRegionLinkPanels[ linkIdx ];
		pRegionLink->MakeReadyForUse();
		pRegionLink->LoadControlSettings( "Resource/UI/quests/cyoa/QuestMapRegionLink.res" );
		pRegionLink->SetPos( YRES( msgRegionLink.xpos() ) + YRES( 60 ), YRES( msgRegionLink.ypos() ) );
		m_pZoomPanel->AddOrUpdateChild( pRegionLink, false, true, CDraggableScrollingPanel::PIN_CENTER );

		// Have the button point to us and trigger a region change
		Button* pLinkButton = pRegionLink->FindControl< Button >( "LinkRegionNameButton", true );
		if ( pLinkButton )
		{
			pLinkButton->SetCommand( CFmtStr( "link %d", pLinkedRegionDef->GetDefIndex() ) );
			pLinkButton->AddActionSignalTarget( this );
		}
	
		// Setup labels
		pRegionLink->SetDialogVariable( "link_region_name", g_pVGuiLocalize->Find( pLinkedRegionDef->GetRegionNameLocToken() ) );
		pRegionLink->SetDialogVariable( "completed", LocalizeNumberWithToken( "#TF_QuestMap_Region_Completed", nNumCompleted ) );
		pRegionLink->SetDialogVariable( "available", LocalizeNumberWithToken( "#TF_QuestMap_Region_Available", nNumAvailable ) );
		pRegionLink->SetControlVisible( "ActiveLabel", bHasActive );

		Panel* pWhiteLine = pRegionLink->FindChildByName( "WhiteLine" );
		Label* pNameLabel = pRegionLink->FindControl< Label >( "LinkRegionNameButton" );
		if ( pWhiteLine && pNameLabel )
		{
			int nXinset = 0;
			int nTall = pNameLabel->GetTall();
			pNameLabel->SizeToContents();
			pNameLabel->SetTall( nTall );
			pNameLabel->GetTextInset( &nXinset, NULL );

			pWhiteLine->SetWide( Max( pNameLabel->GetWide() - nXinset, (int)YRES( 80 ) ) );
			pNameLabel->SetWide( pWhiteLine->GetWide() + pWhiteLine->GetXPos() );
		}

		// It's a tad expensive to look up which panel is the active one, and we want to know every
		// Paint() call, so let's cache it off here where we're actually doing the work.
		if ( bHasActive )
		{
			m_pPathsPanel->SetActiveRegion( pRegionLink );
		}

		ImagePanel* pRegionIcon = pRegionLink->FindControl< ImagePanel >( "RegionIcon" );
		if ( pRegionIcon )
		{
			const Color& colorActive =  vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
			const Color& colorInactive = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_InactiveGrey", Color( 255, 255, 255, 255 ) );

			bool bAvailable = GetQuestMapHelper().BRegionHasAvailableContracts( pLinkedRegionDef->GetDefIndex() );

			pRegionIcon->SetDrawColor( bAvailable ? colorActive : colorInactive );
		}
	}

	SetControlVisible( "ReturnButton", pRegion->GetParent() );

	wchar_t wszRegionNameBuff[ 1024 ];
	memset( wszRegionNameBuff, 0, sizeof( wszRegionNameBuff ) );
	V_wcsncat( wszRegionNameBuff, L".", ARRAYSIZE( wszRegionNameBuff ) );
	RecursiveGenerateRegionPathName( pRegion, wszRegionNameBuff, ARRAYSIZE( wszRegionNameBuff ) );
	ConcatRegionName( pRegion, wszRegionNameBuff, ARRAYSIZE( wszRegionNameBuff ) );

	SetDialogVariable( "region_name", wszRegionNameBuff );

	//
	// Create or update all of the nodes
	//
	auto& mapNodeDefs = GetProtoScriptObjDefManager()->GetDefinitionMapForType( DEF_TYPE_QUEST_MAP_NODE );
	FOR_EACH_MAP_FAST( mapNodeDefs, i )
	{
		const CQuestMapNodeDefinition* pNodeDef = (const CQuestMapNodeDefinition*)mapNodeDefs[ i ];

		if ( pNodeDef->GetHeader().prefab_only() )
			continue;

		if ( !pNodeDef->BIsActive() )
			continue;

		auto idx = m_mapNodePanels.Find( pNodeDef->GetDefIndex() );

		//
		// Check if we need to create/update/destroy a node based on regions
		//
		if ( pNodeDef->GetRegionDefIndex() != m_msgIDCurrentRegion.defindex() )
		{
			// Node is not in this region.  Destroy its panel it if it exists.
			if ( idx != m_mapNodePanels.InvalidIndex() )
			{
				m_mapNodePanels[ idx ]->MarkForDeletion();
				m_mapNodePanels.Remove( idx );
				m_pPathsPanel->RemoveNode( pNodeDef->GetDefIndex() );
			}
		}
		else
		{
			if ( idx == m_mapNodePanels.InvalidIndex() )
			{
				// Panel for the node doesn't exist.  Create it now
				CQuestMapNodePanel* pNewNode = new CQuestMapNodePanel( pNodeDef->GetDefIndex(), m_pZoomPanel, "QuestMapNode" );
				pNewNode->MakeReadyForUse();
				pNewNode->SetAutoDelete( false );
				m_pPathsPanel->AddNode( pNewNode );
				pNewNode->AddActionSignalTarget( this );
				m_mapNodePanels.Insert( pNodeDef->GetDefIndex(), pNewNode );
				m_pZoomPanel->AddOrUpdateChild( pNewNode, false, true, CDraggableScrollingPanel::PIN_CENTER );

			}
			else
			{
				// Node already exists. Just update it.
				m_mapNodePanels[ idx ]->UpdateFromSObject( GetQuestMapHelper().GetQuestMapNode( pNodeDef->GetDefIndex() ) );
				m_mapNodePanels[ idx ]->InvalidateLayout();
			}
		}
	}
}


const EditablePanel* CQuestMapRegionPanel::GetRegionLinkPanel( uint32 nDefIndex ) const
{
	auto idx = m_mapRegionLinkPanels.Find( nDefIndex );
	if ( idx == m_mapRegionLinkPanels.InvalidIndex() )
	{
		return NULL;
	}

	return m_mapRegionLinkPanels[ idx ];
}

const CQuestMapNodePanel* CQuestMapRegionPanel::GetNodePanel( uint32 nDefIndex ) const
{
	auto idx = m_mapNodePanels.Find( nDefIndex );
	if ( idx == m_mapNodePanels.InvalidIndex() )
	{
		return NULL;
	}

	return m_mapNodePanels[ idx ];
}

void CQuestMapRegionPanel::StartZoomTo( float flX, float flY, bool bZoomingIn )
{
	FOR_EACH_MAP_FAST( m_mapNodePanels, i )
	{
		m_mapNodePanels[ i ]->EnterMapState( CQuestMapNodePanel::NEUTRAL );
	}

	m_flZoomStartTime = Plat_FloatTime();
	m_flZoomTime = tf_quest_map_zoom_transition_in_time * 2;
	m_pPathsPanel->GetCircleDrawer().ClearAllCircles();
	m_pQuestMapNodeView->SetVisible( false );
	
	Panel* pRegionName = FindChildByName( "RegionName" );
	if ( pRegionName )
	{
		if ( bZoomingIn )
		{
			const Color& colorActive =  vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "QuestMap_ActiveOrange", Color( 255, 255, 255, 255 ) );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pRegionName, "ypos", GetTall() / 2 - pRegionName->GetTall() / 2, 0.0f, 0.0f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, true, false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pRegionName, "ypos", YRES(15), tf_quest_map_zoom_transition_in_time, tf_quest_map_zoom_transition_in_time, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, false, false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pRegionName, "fgcolor", colorActive, tf_quest_map_zoom_transition_in_time * 2.f, tf_quest_map_zoom_transition_in_time, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
		}

		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pRegionName, "alpha", 255, 0, 0, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
	}

	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "zoom_scale", bZoomingIn ? tf_quest_map_zoom_out_scale : tf_quest_map_zoom_in_scale, 0.0f, 0.0f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, true, false );

	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "zoom_x", flX, 0.f, 0.f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.35f );
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "zoom_y", flY, 0.f, 0.f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.35f );
	
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "zoom_scale", tf_quest_map_zoom_rest_scale, tf_quest_map_zoom_transition_in_time, tf_quest_map_zoom_transition_in_time, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, false, false );
}

void CQuestMapRegionPanel::StartZoomAway( float flX, float flY, bool bZoomingIn )
{
	FOR_EACH_MAP_FAST( m_mapNodePanels, i )
	{
		m_mapNodePanels[ i ]->EnterMapState( CQuestMapNodePanel::NEUTRAL );
	}

	m_flZoomStartTime = Plat_FloatTime();
	m_flZoomTime = tf_quest_map_zoom_transition_in_time;
	m_pPathsPanel->GetCircleDrawer().ClearAllCircles();
	m_pQuestMapNodeView->SetVisible( false );

	Panel* pRegionName = FindChildByName( "RegionName" );
	if ( pRegionName )
	{
		if ( bZoomingIn )
		{
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pRegionName, "alpha", 0, 0, tf_quest_map_zoom_transition_in_time, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
		}
		else
		{
			const Color& colorNeutral =  vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "TanLight", Color( 255, 255, 255, 255 ) );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pRegionName, "ypos", GetTall() / 2 - pRegionName->GetTall() / 2, 0.0f, tf_quest_map_zoom_transition_in_time, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.75f, true, false );
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pRegionName, "fgcolor", colorNeutral, 0.0f, tf_quest_map_zoom_transition_in_time, vgui::AnimationController::INTERPOLATOR_BIAS, 0.75f, true, false );
		}
	}

	// Tell the current region to animate
	{
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "zoom_scale", bZoomingIn ? tf_quest_map_zoom_in_scale : tf_quest_map_zoom_out_scale, 0.f, tf_quest_map_zoom_transition_in_time, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.35f, true, false );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "zoom_x", flX, 0.f, 0.f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.7f, true, false );
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "zoom_y", flY, 0.f, 0.f, vgui::AnimationController::INTERPOLATOR_LINEAR, 0.7f, true, false );
	}

}

bool CQuestMapRegionPanel::BIsZooming() const
{
	return ( m_flZoomStartTime + m_flZoomTime ) > Plat_FloatTime();
}
