
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "Gstring/gstring_cvars.h"
#include "Gstring/vgui/vgui_gstringMain.h"
#include "ienginevgui.h"

#include "vgui_controls/panel.h"
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include "materialsystem/itexture.h"

#include "vgui_controls/checkbutton.h"
#include "vgui_controls/combobox.h"
#include "vgui_controls/slider.h"
#include "vgui_controls/button.h"
#include "ShaderEditor/ShaderEditorSystem.h"

static ConVar layer_x( "layer_x", "0" );
static ConVar layer_y( "layer_y", "0" );
static ConVar layer_w( "layer_w", "100" );
static ConVar layer_t( "layer_t", "100" );
static ConVar layer_p( "layer_p", "0" );
CON_COMMAND( layer_flush, "" )
{
	GetGstringMain()->ReloadLayout();
}
void LayerDbg( CVGUIMenuLayer *l )
{
	l->SetRelativeBounds( layer_x.GetInt(),
		layer_y.GetInt(),
		layer_w.GetInt(),
		layer_t.GetInt(),
		(CVGUIMenuLayer::PinCornerLayer_t)layer_p.GetInt() );
}


using namespace vgui;

static CVGUIGstringMain *g_pGstringMainMenu;

CVGUIGstringMain *GetGstringMain()
{
	return g_pGstringMainMenu;
}

CVGUIGstringMain::CVGUIGstringMain( VPANEL parent, const char *pName ) : BaseClass( NULL, pName )
{
	SetParent( parent );
	SetVisible( true );
	MakePopup();

	CMatRenderContextPtr renderContext( materials );

	renderContext->ClearColor4ub( 0, 0, 0, 255 );
	//ITexture *pTex = materials->FindTexture( "_rt_fullframefb", TEXTURE_GROUP_OTHER );
	//if ( !IsErrorTexture( pTex ) )
	//	renderContext->SetFrameBufferCopyTexture( pTex );

	CFrameTimeHelper::RandomStart();
	//m_pWrappedMenu = new CVGUIMenu3DWrapper( new CVGUIMenuEmbedded(), this );
	m_pWrappedMenu = new CVGUIMenuEmbedded( this );
	InitializeLayout();

	m_flNextDepth = 0.8f;
}

CVGUIGstringMain::~CVGUIGstringMain()
{
}

void CVGUIGstringMain::Create()
{
	vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );

	g_pGstringMainMenu = new CVGUIGstringMain( GameUIRoot, "GstringMain" );
}

void CVGUIGstringMain::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( true );

	SetPaintBackgroundEnabled( false );
	SetPaintBackgroundType( 0 );
	SetBgColor( Color( 0, 0, 255, 255 ) );
}

void CVGUIGstringMain::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, t;
	engine->GetScreenSize( w, t );
	SetBounds( 0, 0, w, t );

	UpdateLayoutVisibility();
}

void CVGUIGstringMain::OnThink()
{
	surface()->MovePopupToBack( GetVPanel() );

	if ( engine->IsInGame() && !engine->IsLevelMainMenuBackground() )
		SetParent( enginevgui->GetPanel( PANEL_GAMEUIDLL ) );
	else
		SetParent( enginevgui->GetPanel( PANEL_ROOT ) );

	static bool bWasIngame = false;
	bool bIsIngame = engine->IsInGame();
	if ( bWasIngame != bIsIngame )
	{
		bWasIngame = bIsIngame;
		UpdateLayoutVisibility();
	}
}

void CVGUIGstringMain::Paint()
{
	return;

	CMatRenderContextPtr renderContext( materials );
	renderContext->DepthRange( 0.81f, 0.81f );
	BaseClass::Paint();
}

void CVGUIGstringMain::Paint3DDepthReset()
{
	return;

	m_flNextDepth = 0.8f;
	CMatRenderContextPtr renderContext( materials );
	renderContext->DepthRange( 0, 1 );
}

void CVGUIGstringMain::Paint3DDepthAdvance()
{
	return;

	float m_flCurrentDepthStep = m_flNextDepth - 0.01f;
	Assert( m_flCurrentDepthStep >= 0.0f );
	CMatRenderContextPtr renderContext( materials );
	renderContext->DepthRange( m_flCurrentDepthStep, m_flNextDepth );
	m_flNextDepth = m_flCurrentDepthStep;
}

void CVGUIGstringMain::ReloadLayout()
{
	for ( int i = 0; i < m_hLayer_Background.Count(); i++ )
		m_hLayer_Background[i]->MarkForDeletion();
	m_hLayer_Background.Purge();
	for ( int i = 0; i < m_hLayer_Top.Count(); i++ )
		m_hLayer_Top[i]->MarkForDeletion();
	m_hLayer_Top.Purge();

	InitializeLayout();
}

void CVGUIGstringMain::InitializeLayout()
{
	Vector viewOrigin;
	QAngle viewAngles;
	float viewFOV;

	LoadBackground00( viewOrigin, viewAngles, viewFOV );

	//m_pWrappedMenu->GetWrappedPanel()->SetViewXForms( viewOrigin, viewAngles, viewFOV );

	//CVGUIMenuLayer *pLayer = new CVGUIMenuLayer( this, true );
	//pLayer->SetBackgroundImage( "vgui/menu/menu_front" );
	//pLayer->SetBackgroundColor( 0, 0, 0, 200 );
	//pLayer->AddOperator( new vParticleOperator_AlphaFade( 0 ) );
	//pLayer->AddOperator( new vParticleOperator_SizeMultiplyXY( 1, 0 ) );
	//m_hLayer_Top.AddToTail( pLayer );
}

void CVGUIGstringMain::UpdateLayoutVisibility()
{
	int w, t;
	engine->GetScreenSize( w, t );

	int menuHeight = t * 0.7f;
	int menuWidth = menuHeight * 0.7f;

	m_pWrappedMenu->SetBounds( w / 2 - menuWidth / 2,
		t * 0.38f,
		menuWidth,
		menuHeight );
	m_pWrappedMenu->InvalidateLayout( false, true );

	//m_pWrappedMenu->SetBounds( 0, 0, GetWide(), GetTall() );
	//m_pWrappedMenu->GetWrappedPanel()->InvalidateLayout();
}

static const char *g_pszSmokeMaterials[] = {
	"vgui/menu/smoke_00",
	"vgui/menu/smoke_01",
};
static const int g_iNumSmokeMaterials = ARRAYSIZE( g_pszSmokeMaterials );
static const char * GetSmokeMaterial()
{
	return g_pszSmokeMaterials[ RandomInt( 0, g_iNumSmokeMaterials - 1 ) ];
}

pFnLayerRoutine( _00_titleglow )
{
	static float ranTimer = 0;
	static float alpha = 255;

	if ( alpha < 240 )
		alpha = Approach( 240, alpha, CFrameTimeHelper::GetFrameTime() * 1000 );

	if ( ranTimer < CFrameTimeHelper::GetCurrentTime() )
	{
		ranTimer = CFrameTimeHelper::GetCurrentTime() + RandomFloat( 0.1f, 5.0f );
		alpha = RandomFloat( 0, 96 );
	}

	Color c = l->GetBackgroundColor();
	c[3] = alpha + sin( CFrameTimeHelper::GetCurrentTime() * 40.0f ) * 15.0f;
	l->SetBackgroundColor( c );
}

pFnLayerRoutine( _rain_screen )
{
	static const char *g_pszDropMaterials[] = {
	"effects/water_drop_0",
	"effects/water_drop_1",
	"effects/water_drop_2",
	};
	static const int g_iNumDropMaterials = ARRAYSIZE( g_pszDropMaterials );

	if ( bInit )
	{
		vParticleOperatorBase *fadeAlpha = new vParticleOperator_AlphaFade(0);
		fadeAlpha->GetImpulseGenerator()->impulse_bias = 0.05f;

		l->AddOperator( new vParticleOperator_Movement() );
		l->AddOperator( fadeAlpha );
		l->AddOperator( new vParticleOperator_GravityRotational( 1.5f, Vector2D( 0.35f, -0.5f ) ) );
		l->AddOperator( new vParticleOperator_Movement_SpeedClamp( 3000.0f ) );
	}
	else
	{
		static float nextSpawn = 0;

		if ( nextSpawn < CFrameTimeHelper::GetCurrentTime() )
		{
			nextSpawn = CFrameTimeHelper::GetCurrentTime() + 0.1f;

			int w, t;
			l->GetSize( w, t );

			extern ConVar gstring_vgui_rain_size;
			extern ConVar gstring_vgui_rain_length;

			int amt = RandomInt( 1, 2 );
			for ( int i = 0; i < amt; i++ )
			{
				vParticle *p = new vParticle();
				p->SetStartSize_Relative( RandomFloat( 5.0f, 10.0f ) * gstring_vgui_rain_size.GetFloat() );
				p->vecPos.Init( RandomFloat(0,1) * w, RandomFloat(0,0.5f) * t );
				p->SetLifeDuration( RandomFloat( 4.0f, 6.0f ) );
				p->SetRenderer(
					new vParticleRenderer_Trail( g_pszDropMaterials[RandomInt(0,g_iNumDropMaterials-1)],
					RandomInt( 5, 20 ) * gstring_vgui_rain_length.GetFloat() )
					);
				p->AddParticleOperator( new vParticleOperator_RainSimulation() );

				l->AddParticle( p );
			}
		}
	}
}

pFnLayerRoutine( _rain_scene )
{
	const char *pszParticleName = "vgui/menu/bg_rain_opaque";

	if ( bInit )
	{
		l->AddOperator( new vParticleOperator_Movement() );
		l->AddOperator( new vParticleOperator_GravityRotational( 10.0f, Vector2D( 0.3f, 0 ) ) );
		l->AddOperator( new vParticleOperator_Movement_SpeedClamp( 3000.0f ) );
	}
	else
	{
		static float nextSpawn = 0;

		if ( nextSpawn < CFrameTimeHelper::GetCurrentTime() )
		{
			nextSpawn = CFrameTimeHelper::GetCurrentTime() + 0.1f;

			int w, t;
			l->GetSize( w, t );

			int amt = RandomInt( 20, 400 );
			for ( int i = 0; i < amt; i++ )
			{
				vParticle *p = new vParticle();
				p->SetStartSize_Relative( RandomFloat( 1.0f, 1.5f ) );
				p->vecPos.Init( RandomFloat(0,0.5f) * w, RandomFloat(0,0.5f) * t );
				p->SetLifeDuration( RandomFloat( 1.0f, 2.0f ) );
				p->SetRenderer( new vParticleRenderer_Trail( pszParticleName, 3, 0.04f ) );
				//p->SetStartColor( Vector( 0.5f, 0.9f, 1.0f ) );
				p->SetStartColor( Vector( 1.0f, 0.65f, 0.2f ) );
				p->SetStartAlpha( 0.1f );

				l->AddParticle( p );
			}
		}
	}
}

pFnLayerRoutine( _scanner_00 )
{
	static vParticle *pParticle_star = NULL;
	//static vParticle *pParticle_lightshaft = NULL;

	int sw, st;
	engine->GetScreenSize( sw, st );

	int w, t;
	l->GetSize( w, t );

	static float x_offset = 0.66f;
	static float y_offset = 0.6f;
	static float flLastFraction = 1.0f;

	if ( bInit )
	{
		if ( l->GetNumEmissionLayers() > 0 )
		{
			l->GetEmissionLayer( 0 )->AddOperator( new vParticleOperator_Movement() );
			vParticleOperatorBase *pOp = new vParticleOperator_RotationAnimate( 10 );
			pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_FRAMETIME;
			l->GetEmissionLayer( 0 )->AddOperator( pOp );
		}

		pParticle_star = new vParticle();
		pParticle_star->CreateQuadRenderer( "vgui/menu/glow_star" );
		pParticle_star->SetStartColor( Vector( 1, 0.6f, 0.5f ) );

		vParticleOperatorBase *pOp = new vParticleOperator_AlphaFade(0);
		pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_CURTIME_SINE;
		pParticle_star->AddParticleOperator( pOp );

		pOp = new vParticleOperator_RotationAnimate( 100 );
		pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_FRAMETIME;
		pParticle_star->AddParticleOperator( pOp );

		l->AddParticle( pParticle_star );

		//pParticle_lightshaft = new vParticle();
		//pParticle_lightshaft->CreateRectRenderer( "vgui/menu/glow_dir_00", 1, 0.7f );
		//pParticle_lightshaft->flAngle = 15.0f;
		//l->AddParticle( pParticle_lightshaft );

		l->SetPos( w, t );
	}
	else
	{
		pParticle_star->vecPos.Init( w * 0.87f, t * 0.545f );
		pParticle_star->SetStartSize_Relative( sin( CFrameTimeHelper::GetCurrentTime() * 10.0f ) * 40 + 40 );

		//pParticle_lightshaft->vecPos.Init( w * 0.5f, t * 0.45f );
		//pParticle_lightshaft->SetStartSize_Absolute( w * 0.9f );

		float frac = fmod( CFrameTimeHelper::GetCurrentTime() * 0.005f, 1.0 );
		if ( flLastFraction > frac )
		{
			x_offset = RandomFloat( 0.20f, 0.66f );
			y_offset = RandomFloat( 0.6f, 0.0f );
		}
		flLastFraction = frac;

		frac = 1.0f - frac;

		float path_x = sw * 3.0f;
		float move_y = sin( CFrameTimeHelper::GetCurrentTime() ) * st * 0.02f;

		l->SetPos( path_x * frac - path_x * x_offset, st * frac * y_offset + move_y );

		static float nextDustEmission = 0.0f;

		if ( nextDustEmission < CFrameTimeHelper::GetCurrentTime() && l->GetNumEmissionLayers() > 0 )
		{
			int px, py;
			l->GetPos( px, py );
			nextDustEmission = CFrameTimeHelper::GetCurrentTime() + RandomFloat( 0.05f, 0.15f );

			vParticle *p = new vParticle();
			p->CreateQuadRenderer( GetSmokeMaterial() );
			p->vecPos.Init( px + w * 0.95f, py + t * 0.5f );
			p->AddParticleOperator( new vParticleOperator_SizeMultiply( RandomFloat( 1.5f, 2.5f ) ) );
			p->AddParticleOperator( new vParticleOperator_Gravity( Vector2D( RandomFloat( 3, 6 ), RandomFloat( -2, -4 ) ) ) );
			p->SetLifeDuration( Bias( RandomFloat( 0, 1 ), 0.3f ) * 5.0f + 1.0f );
			p->SetStartSize_Relative( RandomFloat( 10, 40 ) );
			p->SetStartAlpha( 0 );
			float c = RandomFloat( 0.4f, 0.5f );
			p->SetStartColor( Vector( c, c, c ) );
			p->flAngle = RandomFloat( 0, 360.0f );
			vParticleOperatorBase *pOp = new vParticleOperator_AlphaFade(RandomFloat( 0.2f, 0.5f ) );
			pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_LIFETIME_SINE;
			p->AddParticleOperator( pOp );

			l->GetEmissionLayer( 0 )->AddParticle( p );
		}
	}
}

pFnLayerRoutine( _clouds_00 )
{
	if ( bInit )
	{
		l->AddOperator( new vParticleOperator_Movement() );
	}
	else
	{
		static float flNextSpawn = 0.0f;

		if ( flNextSpawn < CFrameTimeHelper::GetCurrentTime() )
		{
			flNextSpawn = CFrameTimeHelper::GetCurrentTime() + RandomFloat( 0.1f, 1.0f );

			int w, t;
			l->GetSize( w, t );

			vParticle *p = new vParticle();

			p->CreateQuadRenderer( GetSmokeMaterial() );

			p->vecPos.Init( w * RandomFloat( 0, 1 ), t * RandomFloat( 0.9f, 1.3f ) );
			p->vecVelocity.Init( w * RandomFloat( 0.01f, 0.1f ), 0.0f );
			p->flAngle = RandomFloat( 0, 360.0f );

			p->SetLifeDuration( RandomFloat( 4.0f, 6.0f ) );
			p->SetStartSize_Relative( RandomFloat( 450, 600 ) );
			p->SetStartAlpha( 0 );
			float c = RandomFloat( 0.2f, 0.3f );
			p->SetStartColor( Vector( c, c, c * 1.5f ) );

			vParticleOperatorBase *pOp = new vParticleOperator_AlphaFade(RandomFloat( 0.3f, 0.6f ) );
			pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_LIFETIME_SINE;
			p->AddParticleOperator( pOp );

			l->AddParticle( p );
		}
	}
}

pFnLayerRoutine( _clouds_01 )
{
	if ( bInit )
	{
		l->AddOperator( new vParticleOperator_Movement() );
	}
	else
	{
		static float flNextSpawn = 0.0f;

		if ( flNextSpawn < CFrameTimeHelper::GetCurrentTime() )
		{
			flNextSpawn = CFrameTimeHelper::GetCurrentTime(); // + RandomFloat( 0.1f, 1.0f );

			int w, t;
			l->GetSize( w, t );

			vParticle *p = new vParticle();

			p->CreateQuadRenderer( GetSmokeMaterial() );

			p->vecPos.Init( w * RandomFloat( 0, 1 ), t * RandomFloat( 0.0f, 0.5f ) );
			p->vecVelocity.Init( w * RandomFloat( 0.01f, 0.1f ), 0.0f );
			p->flAngle = RandomFloat( 0, 360.0f );

			p->SetLifeDuration( RandomFloat( 4.0f, 6.0f ) );
			p->SetStartSize_Relative( RandomFloat( 50, 100 ) );
			p->SetStartAlpha( 0 );
			float c = RandomFloat( 0.2f, 0.3f );
			p->SetStartColor( Vector( c * 2.0f, c * 1.6f, c ) );

			vParticleOperatorBase *pOp = new vParticleOperator_AlphaFade(RandomFloat( 0.3f, 0.6f ) );
			pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_LIFETIME_SINE;
			p->AddParticleOperator( pOp );

			l->AddParticle( p );
		}
	}
}

pFnLayerRoutine( _lightShafts_00 )
{
	if ( bInit )
	{
		int w, t;
		l->GetSize( w, t );

		int amt = RandomInt( 2, 5 );
		for ( int i = 0; i < amt; i++ )
		{
			vParticle *p = new vParticle();
			vParticle *pGlow = new vParticle();
			p->CreateRectRenderer( "vgui/menu/glow_dir_01", RandomFloat( 1.2f, 1.5f ), RandomFloat( 0.1f, 0.2f ) );
			pGlow->CreateQuadRenderer( "vgui/menu/glow_simple_00" );
			p->flAngle = 90.0f + RandomFloat( -30, 30 );
			p->vecPos.Init( RandomFloat( 0, 1 ) * w, RandomFloat( 0.9f, 1 ) * t );
			pGlow->vecPos = p->vecPos;
			p->SetStartSize_Relative( RandomFloat( 600, 1000 ) );
			pGlow->SetStartSize_Relative( RandomFloat( 900, 1200 ) );

			vParticleOperatorBase *pOp = new vParticleOperator_RotationFade( 90,
				RandomFloat( 90, 100 ) );
			pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_CURTIME_SINE_SIGNED;
			pOp->GetImpulseGenerator()->impulse_multiplier = RandomFloat( 0.02f, 0.08f );
			p->AddParticleOperator( pOp );

			pOp = new vParticleOperator_AlphaFade(0);
			vParticleOperatorBase *pOp2 = new vParticleOperator_AlphaFade(0);
			pOp2->GetImpulseGenerator()->mode = pOp->GetImpulseGenerator()->mode = vParticleImpulseGen::PARTICLEIMPULSEGENERATOR_CURTIME_SINE;
			pOp2->GetImpulseGenerator()->impulse_multiplier = pOp->GetImpulseGenerator()->impulse_multiplier = RandomFloat( 0.05f, 0.2f );
			p->AddParticleOperator( pOp );
			pGlow->AddParticleOperator( pOp2 );

			p->SetStartAlpha( RandomFloat( 0.6f, 0.8f ) );
			p->SetStartColor( Vector( 1, 0.7f, 0.5f ) );
			pGlow->SetStartAlpha( RandomFloat( 0.6f, 0.8f ) );
			pGlow->SetStartColor( Vector( 1, 0.7f, 0.5f ) );
			l->AddParticle( p );
			l->AddParticle( pGlow );
		}
	}
	else
	{
	}
}

pFnLayerRoutine( _sparks_00 )
{
	if ( bInit )
	{
		l->AddOperator( new vParticleOperator_Movement() );
	}
	else
	{
		static float flNextSpark = RandomFloat( 10, 15 );

		if ( flNextSpark < CFrameTimeHelper::GetCurrentTime() )
		{
			flNextSpark = CFrameTimeHelper::GetCurrentTime() + RandomFloat( 10, 20 );

			int w, t;
			l->GetSize( w, t );

			Vector2D center( RandomFloat( 0.3f, 0.7f ) * w, RandomFloat( 0.24f, 0.28f ) * t );

			int amt = RandomInt( 1, 3 );
			for ( int i = 0; i < amt; i++ )
			{
				vParticle *p = new vParticle();
				p->CreateQuadRenderer( "vgui/menu/glow_star" );
				p->SetStartColor( Vector( RandomFloat( 0.8f, 1.0f ), RandomFloat( 0.7f, 0.8f ), RandomFloat( 0.5f, 0.7f ) ) );
				p->vecPos = center;
				p->SetStartSize_Relative( RandomFloat( 300, 500 ) );
				p->AddParticleOperator( new vParticleOperator_SizeMultiply( 0 ) );
				p->SetLifeDuration( RandomFloat( 0.1f, 0.2f ) );
				p->flAngle = RandomFloat( 0, 360 );
				p->AddParticleOperator( new vParticleOperator_RotationAnimate( RandomFloat( -2, 2 ) ) );
				l->AddParticle( p );
			}

			amt = RandomInt( 1, 3 );
			for ( int i = 0; i < amt; i++ )
			{
				vParticle *p = new vParticle();
				p->CreateQuadRenderer( "vgui/menu/glow_simple_00" );
				p->SetStartColor( Vector( RandomFloat( 0.8f, 1.0f ), RandomFloat( 0.7f, 0.8f ), RandomFloat( 0.5f, 0.7f ) ) );
				p->vecPos = center;
				p->SetStartSize_Relative( RandomFloat( 300, 600 ) );
				p->AddParticleOperator( new vParticleOperator_SizeMultiply( 0 ) );
				p->SetLifeDuration( RandomFloat( 0.2f, 0.3f ) );
				p->flAngle = RandomFloat( 0, 360 );
				l->AddParticle( p );
			}

			amt = RandomInt( 20, 30 );
			for ( int i = 0; i < amt; i++ )
			{
				vParticle *p = new vParticle();
				p->SetRenderer( new vParticleRenderer_Trail( "vgui/menu/bg_rain_opaque", 4, 0.03f ) );
				p->SetStartColor( Vector( RandomFloat( 0.8f, 1.0f ), RandomFloat( 0.7f, 0.8f ), RandomFloat( 0.5f, 0.7f ) ) );
				p->vecPos = center;
				p->SetStartSize_Relative( RandomFloat( 1, 2 ) );
				p->AddParticleOperator( new vParticleOperator_SizeMultiply( 0 ) );
				p->SetLifeDuration( RandomFloat( 1.0f, 2.0f ) );
				p->AddParticleOperator( new vParticleOperator_Gravity( Vector2D( 0, 5000 ) ) );
				p->SetSafeVelocity( Vector2D( RandomFloat( -1000, 1000 ), RandomFloat( -1000, 0 ) ) );
				l->AddParticle( p );
			}
		}
	}
}

pFnLayerRoutine( _paint_ppe_menu_fx )
{
	static int tex_vignette = -1;
	//static int tex_blur = -1;

	if ( bInit )
	{
		//SetupVGUITex( "vgui/menu/pp_blur", tex_blur );
		SetupVGUITex( "vgui/menu/pp_vignette", tex_vignette );
		return;
	}

	//if ( !g_ShaderEditorSystem->IsReady() )
	//	return;

	//int idx = shaderEdit->GetPPEIndex( "ppe_menu_fx" );
	//if ( idx < 0 )
	//	return;

	if ( tex_vignette < 0 ) //||
		//tex_blur < 0 )
		return;

	//CViewSetup setup;
	//setup.x = setup.y = 0;
	//engine->GetScreenSize( setup.width, setup.height );
	//setup.context = 0;
	//setup.origin.Init();
	//setup.angles.Init();
	//setup.m_bOrtho = false;
	//setup.zNear = 7;
	//setup.zFar = 30000;
	//setup.m_vUnreflectedOrigin = setup.origin;

	//Frustum frustum;
	//materials->PushRenderTargetAndViewport();
	//render->Push3DView( setup, 0, false, NULL, frustum );

	//shaderEdit->DrawPPEOnDemand( idx );

	//render->PopView( frustum );
	//materials->PopRenderTargetAndViewport();

	surface()->DrawSetColor( 255, 255, 255, 255 );
	//surface()->DrawSetTexture( tex_blur );
	//surface()->DrawTexturedRect( 0, 0, l->GetWide(), l->GetTall() );
	surface()->DrawSetTexture( tex_vignette );
	surface()->DrawTexturedRect( 0, 0, l->GetWide(), l->GetTall() );
}

void CVGUIGstringMain::LoadBackground00( Vector &vOrigin, QAngle &vAng, float &vFOV )
{
	float flTime = CFrameTimeHelper::GetCurrentTime();
	bool bRain = ( int(flTime * 10000.0f) % 2 ) != 0; //RandomInt( 0, 3 ) != 0;

	CVGUIMenuLayer *pLayer = new CVGUIMenuLayer( this, true );
	pLayer->SetBackgroundImage( "vgui/menu/bg_00_layer_00" );
	//pLayer->SetIngameRenderingEnabled( false );
	m_hLayer_Background.AddToTail( pLayer );

	pLayer = new CVGUIMenuLayer( this, true );
	pLayer->SetRoutine( _lightShafts_00 );
	m_hLayer_Background.AddToTail( pLayer );

	pLayer = new CVGUIMenuLayer( this, true );
	pLayer->SetBackgroundImage( "vgui/menu/bg_00_layer_01" );
	m_hLayer_Background.AddToTail( pLayer );

	CVGUIMenuLayer *pScannerEmissive = new CVGUIMenuLayer( this, true );
	m_hLayer_Background.AddToTail( pScannerEmissive );

	pLayer = new CVGUIMenuLayer( this );
	pLayer->SetBackgroundImage( "vgui/menu/scanner_00" );
	pLayer->SetRoutine( _scanner_00 );
	pLayer->SetRelativeBounds( 0, 0, 200, 200 );
	pLayer->SetNormalizedBackgroundBounds( Vector2D( 0.85f, 0.425f ), Vector2D( 1, 0.575f ) );
	pLayer->AddEmissionLayer( pScannerEmissive );
	m_hLayer_Background.AddToTail( pLayer );

	pLayer = new CVGUIMenuLayer( this, true );
	pLayer->SetRoutine( _sparks_00 );
	m_hLayer_Background.AddToTail( pLayer );

	if ( bRain )
	{
		pLayer = new CVGUIMenuLayer( this, true );
		pLayer->SetRoutine( _rain_scene );
		m_hLayer_Background.AddToTail( pLayer );
	}

	pLayer = new CVGUIMenuLayer( this );
	pLayer->SetBackgroundImage( "vgui/menu/bg_00_layer_02" );
	pLayer->SetRelativeBounds( -320, -265, 550, 550, CVGUIMenuLayer::PINCL_CC );
	m_hLayer_Background.AddToTail( pLayer );

	pLayer = new CVGUIMenuLayer( this, true );
	pLayer->SetRoutine( _clouds_00 );
	m_hLayer_Background.AddToTail( pLayer );

	pLayer = new CVGUIMenuLayer( this, false );
	pLayer->SetBackgroundImage( "vgui/menu/bg_00_title" );
	pLayer->SetRelativeBounds( -192, 16, 345, 175, CVGUIMenuLayer::PINCL_C0 );
	m_hLayer_Background.AddToTail( pLayer );

	pLayer = new CVGUIMenuLayer( this, false );
	pLayer->SetBackgroundImage( "vgui/menu/bg_00_title_glow" );
	pLayer->SetRelativeBounds( -192, 16, 345, 175, CVGUIMenuLayer::PINCL_C0 );
	pLayer->SetRoutine( _00_titleglow );
	m_hLayer_Background.AddToTail( pLayer );

	//vOrigin.Init( 550, -127, -110 );
	//vAng.Init( -25, 166, 0 );

	vOrigin.Init( 550, 0, 150 );
	vAng.Init( 0, 180, 0 );
	vFOV = 96;

	pLayer = new CVGUIMenuLayer( this, true, false );
	pLayer->SetPaintHook( _paint_ppe_menu_fx );
	m_hLayer_Top.AddToTail( pLayer );

	m_pWrappedMenu->MoveToFront();

	if ( bRain )
	{
		pLayer = new CVGUIMenuLayer( this, true, true );
		pLayer->SetRoutine( _rain_screen );
		m_hLayer_Top.AddToTail( pLayer );
	}
}