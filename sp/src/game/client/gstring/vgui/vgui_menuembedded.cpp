
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "Gstring/vgui/vgui_gstringMain.h"

#include <view.h>
#include <view_shared.h>
#include "collisionutils.h"

#include <cdll_client_int.h>
#include "VGUIMatSurface/IMatSystemSurface.h"
#include <vgui/IVGUI.h>
#include <vgui/ISystem.h>
#include <vgui/IInputInternal.h>

#include <vgui_controls/Label.h>


extern vgui::IInputInternal *g_InputInternal;


using namespace vgui;


void ScreenToWorldLocal( int localx, int localy, float fov,
					int viewWidth, int viewHeight,
					const Vector& vecRenderOrigin,
					const QAngle& vecRenderAngles,
					Vector& vecPickingRay )
{
	float dx, dy;
	float c_x, c_y;
	float dist;
	Vector vpn, vup, vright;

	float scaled_fov = fov;

	c_x = viewWidth / 2;
	c_y = viewHeight / 2;

	dx = (float)localx - c_x;
	dy = c_y - (float)localy;

	float dist_denom = tan(M_PI * scaled_fov / 360.0f);
	dist = c_x / dist_denom;
	AngleVectors( vecRenderAngles, &vpn, &vright, &vup );
	vecPickingRay = vpn * dist + vright * ( dx ) + vup * ( dy );
	VectorNormalize( vecPickingRay );
}

static int MenuButtonSort( CVGUIMenuButton *const *p1, CVGUIMenuButton *const *p2 )
{
	return ( (*p1)->GetOrderIndex() < (*p2)->GetOrderIndex() ) ? -1 : 1;
}


CVGUIMenuEmbedded::CVGUIMenuEmbedded( Panel *parent ) : BaseClass( parent )
{
	m_hContext = ivgui()->CreateContext();
	//ivgui()->AssociatePanelWithContext( m_hContext, GetVPanel() );
	SetProportional( false );

	SetVisible( true );

	m_pParticleParent =  new CVGUIParticleContainer( this );
	m_pParticleParent->AddGlobalOperator( new vParticleOperator_SizeMultiplyXY( 2, 0 ) );
	m_pParticleParent->AddGlobalOperator( new vParticleOperator_AlphaFade( 0 ) );

	m_hButtons_InGame.AddToTail( new CVGUIMenuButton( this, "Resume", "game_resume", 0 ) );
	m_hButtons_General.AddToTail( new CVGUIMenuButton( this, "New", "game_new", 1 ) );
	m_hButtons_General.AddToTail( new CVGUIMenuButton( this, "Load", "game_load", 2 ) );
	m_hButtons_InGame.AddToTail( new CVGUIMenuButton( this, "Save", "game_save", 3 ) );
	m_hButtons_General.AddToTail( new CVGUIMenuButton( this, "Options", "game_options", 4 ) );
	m_hButtons_General.AddToTail( new CVGUIMenuButton( this, "Post-processing", "game_postprocessing", 5 ) );
	m_hButtons_General.AddToTail( new CVGUIMenuButton( this, "Quit", "game_quit", 6 ) );

	SetupVGUITex( "vgui/menu/menu_bg", m_iTexture_BG );
	SetupVGUITex( "vgui/menu/menu_line", m_iTexture_Line );

	m_pTitle_1 = new CVGUIMenuLabel( this, "Main menu" );
	m_pTitle_0 = new CVGUIMenuLabel( this, "Main menu" );

	SetAlpha( 255 );
	SetBounds( 0, 0, 400, 500 );

	_line_offset = 100;
	_line_size_y = 2;

	viewOrigin.Init( 400, -120, -100 );
	//viewAngles.Init( -20, 160, 0 );
	viewAngles.Init( -20, 180, 0 );
	viewFOV = 110;
	m_pViewport = NULL;

	SetActive( true );
}

CVGUIMenuEmbedded::~CVGUIMenuEmbedded()
{
	ivgui()->DestroyContext( m_hContext );
}

void CVGUIMenuEmbedded::SetViewport( Panel *vp )
{
	m_pViewport = vp;
}

void CVGUIMenuEmbedded::SetViewXForms( Vector origin, QAngle angles, float fov )
{
	viewOrigin = origin;
	viewAngles = angles;
	viewFOV = fov;
}

CViewSetup CVGUIMenuEmbedded::CalcView()
{
	Assert( m_pViewport != NULL );

	int x, y, w, t;
	m_pViewport->GetBounds( x, y, w, t );
	CViewSetup setup;

	//setup.context = 0;
	setup.origin = viewOrigin;
	setup.angles = viewAngles;
	setup.fov = ScaleFOVByWidthRatio( viewFOV, ( (float)w / (float)t ) / ( 4.0f / 3.0f ) );
	setup.x = x;
	setup.y = y;
	setup.width = w;
	setup.height = t;
	setup.zNear = 10;
	setup.zFar = 10000;
	setup.m_bOrtho = false;

	return setup;
}

void CVGUIMenuEmbedded::SetActive( bool bActive )
{
	m_bActive = bActive;
}

bool CVGUIMenuEmbedded::IsActive()
{
	return m_bActive;
}

vgui::HContext &CVGUIMenuEmbedded::GetContext()
{
	return m_hContext;
}

void CVGUIMenuEmbedded::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pTitle_0->SetFont( pScheme->GetFont( "Menu3DTitle", true ) );
	m_pTitle_1->SetFont( pScheme->GetFont( "Menu3DTitleBlur", true ) );

	m_pTitle_0->SetContentAlignment( Label::a_center );
	m_pTitle_1->SetContentAlignment( Label::a_center );

	SetPaintBackgroundEnabled( false );
	SetBgColor( Color( 0, 0, 32, 255 ) );

	m_pTitle_0->SetFgColor( Color( 255, 148, 32, 127 ) );
	m_pTitle_1->SetFgColor( Color( 255, 148, 32, 127 ) );

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
}

void CVGUIMenuEmbedded::PerformLayout()
{
	bool bIngame = engine->IsInGame();
	for ( int i = 0; i < m_hButtons_InGame.Count(); i++ )
	{
		m_hButtons_InGame[i]->SetVisible( bIngame );
	}

	BaseClass::PerformLayout();

	int w, t;
	GetSize( w, t );

	_line_offset = scheme()->GetProportionalScaledValue( 50 );

	m_pTitle_0->SetBounds( 0, 0, w, _line_offset );
	m_pTitle_1->SetBounds( 0, 0, w, _line_offset );

	CUtlVector< CVGUIMenuButton* > activeButtons;
	if ( bIngame )
		activeButtons.AddVectorToTail( m_hButtons_InGame );
	activeButtons.AddVectorToTail( m_hButtons_General );
	activeButtons.Sort( MenuButtonSort );

	int button_size_y = scheme()->GetProportionalScaledValue( 19 );
	int button_size_x = w * 0.75f;
	int button_offset_y = scheme()->GetProportionalScaledValue( 7 );
	int cur_y = _line_offset + _line_size_y +
		button_size_y;

	for ( int i = 0; i < activeButtons.Count(); i++ )
	{
		activeButtons[i]->SetBounds( (w - button_size_x) * 0.5f, cur_y,
			button_size_x, button_size_y );

		cur_y += button_offset_y + button_size_y;
	}

	m_pParticleParent->SetBounds( 0, 0, w, t );
}

void CVGUIMenuEmbedded::OnCommand( const char *cmd )
{
	const char *consoleCmd = NULL;

	if ( !Q_stricmp( cmd, "game_quit" ) )
		consoleCmd = "gamemenucommand quit";
	else if ( !Q_stricmp( cmd, "game_new" ) )
		consoleCmd = "gamemenucommand OpenNewGameDialog";
	else if ( !Q_stricmp( cmd, "game_load" ) )
		consoleCmd = "gamemenucommand OpenLoadGameDialog";
	else if ( !Q_stricmp( cmd, "game_save" ) )
		consoleCmd = "gamemenucommand OpenSaveGameDialog";
	else if ( !Q_stricmp( cmd, "game_resume" ) )
		consoleCmd = "gamemenucommand ResumeGame";
	else if ( !Q_stricmp( cmd, "game_options" ) )
		consoleCmd = "gamemenucommand OpenOptionsDialog";
	else if ( !Q_stricmp( cmd, "game_postprocessing" ) )
		consoleCmd = "vgui_showGstringOptions";

	if ( consoleCmd == NULL )
		return;

	engine->ClientCmd( consoleCmd );
}

void CVGUIMenuEmbedded::OnButtonArmed( KeyValues *pKV )
{
	Panel *panel = (Panel*)pKV->GetPtr( "panel" );

	bool bState = pKV->GetInt( "state" ) == 1;
	if ( !bState )
		return;

	int x, y, w, t;
	panel->GetBounds( x, y, w, t );

	vParticle *p = new vParticle();
	p->CreateRectRenderer( "vgui/menu/button_effect", 1, 1 );
	p->vecPos.Init( x + w * 0.5f, y + t * 0.5f );
	p->SetStartSize_Absolute( w * 0.5f, t );
	p->SetStartAlpha( 0.5f );
	p->SetLifeDuration( 0.1f );
	p->SetStartColor( Vector( 1, 0.8f, 0.6f ) );
	m_pParticleParent->AddParticle( p );
}

void CVGUIMenuEmbedded::OnThink()
{
	return;

	CViewSetup setup = CalcView();

	UpdateCursor( setup );
}

void CVGUIMenuEmbedded::UpdateCursor( const CViewSetup &setup )
{
	return;

	int mx, my;
	mx = my = -1;

	if ( IsActive() )
		input()->GetCursorPosition( mx, my );

	Vector dir;
	ScreenToWorldLocal( mx - setup.x, my - setup.y,
		setup.fov, setup.width, setup.height,
		setup.origin, setup.angles,
		dir );

	Vector v00 = Vector( 0, GetWide() * -0.5f, GetTall() * 0.5f );
	Vector v10 = Vector( 0, GetWide() * 0.5f, GetTall() * 0.5f );
	Vector v01 = Vector( 0, GetWide() * -0.5f, GetTall() * -0.5f );
	float u, v;

	Ray_t ray;
	ray.Init( setup.origin, setup.origin + dir * 5000 );

	if ( !ComputeIntersectionBarycentricCoordinates( ray, v00, v10, v01, u, v ) )
		return;

	u = u * GetWide();
	v = v * GetTall();

	//g_InputInternal->ActivateInputContext( m_hContext );

	//g_InputInternal->InternalCursorMoved( u, v );

	//g_InputInternal->ActivateInputContext( DEFAULT_VGUI_CONTEXT );
}

void CVGUIMenuEmbedded::Paint()
{
	BaseClass::Paint();

	int w, t;
	GetSize( w, t );
	Paint3DAdvanceDepth();
	surface()->DrawSetColor( 255, 255, 255, 164 );
	surface()->DrawSetTexture( m_iTexture_BG );
	surface()->DrawTexturedRect( 0, _line_offset + _line_size_y,
		w, t );

	Paint3DAdvanceDepth();
	//surface()->DrawSetColor( 255, 255, 255, 127 );
	surface()->DrawSetColor( 255, 148, 32, 127 );
	surface()->DrawSetTexture( m_iTexture_Line );
	surface()->DrawTexturedRect( 0, _line_offset,
		w, _line_offset + _line_size_y );
}

void CVGUIMenuEmbedded::Render3D()
{
	Frustum frustum;

	CViewSetup setup = CalcView();

	CMatRenderContextPtr renderContext( materials );

	renderContext->MatrixMode( MATERIAL_MODEL );
	renderContext->PushMatrix();
	renderContext->MatrixMode( MATERIAL_VIEW );
	renderContext->PushMatrix();
	renderContext->MatrixMode( MATERIAL_PROJECTION );
	renderContext->PushMatrix();
	
	//render->SceneBegin();
	render->Push3DView( setup, 0, NULL, frustum );
	renderContext->PushRenderTargetAndViewport(NULL, setup.x, setup.y, setup.width, setup.height);

	VMatrix ident;
	ident.Identity();
	ident.SetupMatrixOrgAngles( Vector( 0, GetWide() * -0.5f, GetTall() * 0.5f ),
		QAngle( 180, -90, -90 ) );

	g_pMatSystemSurface->DrawPanelIn3DSpace( GetVPanel(), ident,
		GetWide(), GetTall(), GetWide(), GetTall() );

	renderContext->PopRenderTargetAndViewport();
	render->PopView( frustum );
	//render->SceneEnd();

	renderContext->MatrixMode( MATERIAL_PROJECTION );
	renderContext->PopMatrix();
	renderContext->MatrixMode( MATERIAL_VIEW );
	renderContext->PopMatrix();
	renderContext->MatrixMode( MATERIAL_MODEL );
	renderContext->PopMatrix();
}


CVGUIMenu3DWrapper::CVGUIMenu3DWrapper( CVGUIMenuEmbedded *p3DPanel, vgui::Panel *parent )
	: BaseClass( parent )
{
	m_p3DPanel = p3DPanel;
	m_p3DPanel->SetViewport( this );
	//m_p3DPanel->SetParent( this );
	m_p3DPanel->MakeReadyForUse();
	m_p3DPanel->SetZPos( 0 );

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );
}

CVGUIMenuEmbedded *CVGUIMenu3DWrapper::GetWrappedPanel()
{
	return m_p3DPanel;
}

void CVGUIMenu3DWrapper::Paint()
{
	GetGstringMain()->Paint3DDepthAdvance();

	m_p3DPanel->Render3D();

	GetGstringMain()->Paint3DDepthReset();
}

void CVGUIMenu3DWrapper::OnCursorEntered()
{
	BaseClass::OnCursorEntered();

	m_p3DPanel->SetActive( true );
}

void CVGUIMenu3DWrapper::OnCursorExited()
{
	BaseClass::OnCursorExited();

	m_p3DPanel->SetActive( false );
}

void CVGUIMenu3DWrapper::OnMousePressed( vgui::MouseCode code )
{
	g_InputInternal->ActivateInputContext( m_p3DPanel->GetContext() );

	g_InputInternal->InternalMousePressed( code );

	g_InputInternal->ActivateInputContext( DEFAULT_VGUI_CONTEXT );
}

void CVGUIMenu3DWrapper::OnMouseDoublePressed( vgui::MouseCode code )
{
	g_InputInternal->ActivateInputContext( m_p3DPanel->GetContext() );

	g_InputInternal->InternalMouseDoublePressed( code );

	g_InputInternal->ActivateInputContext( DEFAULT_VGUI_CONTEXT );
}

void CVGUIMenu3DWrapper::OnMouseReleased( vgui::MouseCode code )
{
	g_InputInternal->ActivateInputContext( m_p3DPanel->GetContext() );

	g_InputInternal->InternalMouseReleased( code );

	g_InputInternal->ActivateInputContext( DEFAULT_VGUI_CONTEXT );
}