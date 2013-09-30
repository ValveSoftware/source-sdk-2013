
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "Gstring/gstring_cvars.h"
#include "Gstring/vgui/vUtil.h"
#include "Gstring/vgui/vgui_menuLayer.h"
#include "Gstring/vgui/vParticleContainer.h"
#include "ienginevgui.h"

#include "vgui_controls/panel.h"
#include <vgui/ISurface.h>
#include "view_scene.h"

#include "vgui_controls/checkbutton.h"
#include "vgui_controls/combobox.h"
#include "vgui_controls/slider.h"
#include "vgui_controls/button.h"

using namespace vgui;

CVGUIMenuLayer::CVGUIMenuLayer( Panel *parent, bool bAutoScale, bool bRefractive ) : BaseClass( parent )
{
	m_iBackgroundImage = -1;
	m_bAutoSize = bAutoScale;
	m_colBackgroundColor = Color(255,255,255,255);

	m_pParticleParent = new CVGUIParticleContainer( this );

	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );

	rx = ry = 0;
	rw = 640;
	rt = 480;
	m_bRequireInit = true;
	m_bRefractive = bRefractive;
	m_bIngame = true;

	m_vecBGBounds_min.Init( 0, 0 );
	m_vecBGBounds_max.Init( 1, 1 );

	m_pRoutine = NULL;
	m_pPaintHook = NULL;
}

CVGUIMenuLayer::~CVGUIMenuLayer()
{
}

void CVGUIMenuLayer::SetIngameRenderingEnabled( bool bEnabled )
{
	m_bIngame = bEnabled;
}

void CVGUIMenuLayer::AddEmissionLayer( CVGUIMenuLayer *l )
{
	m_hEmissionLayers.AddToTail( l );
}

int CVGUIMenuLayer::GetNumEmissionLayers()
{
	return m_hEmissionLayers.Count();
}

CVGUIMenuLayer *CVGUIMenuLayer::GetEmissionLayer( int index )
{
	return m_hEmissionLayers[ index ];
}

void CVGUIMenuLayer::SetBackgroundImage( const char *pszFile )
{
	SetupVGUITex( pszFile, m_iBackgroundImage );
}

void CVGUIMenuLayer::SetBackgroundImage( int i )
{
	m_iBackgroundImage = i;
}

void CVGUIMenuLayer::SetBackgroundColor( int r, int g, int b, int a )
{
	m_colBackgroundColor = Color( r, g, b, a );
}

void CVGUIMenuLayer::SetBackgroundColor( Color col )
{
	m_colBackgroundColor = col;
}

Color CVGUIMenuLayer::GetBackgroundColor()
{
	return m_colBackgroundColor;
}

void CVGUIMenuLayer::SetRelativeBounds( int x, int y, int w, int t, PinCornerLayer_t pin )
{
	rx = x;
	ry = y;
	rw = w;
	rt = t;
	m_pin = pin;
}

void CVGUIMenuLayer::SetRoutine( void(* func )( CVGUIMenuLayer *l, bool bInit ) )
{
	m_pRoutine = func;
}

void CVGUIMenuLayer::SetPaintHook( void(* func )( CVGUIMenuLayer *l, bool bInit ) )
{
	m_pPaintHook = func;
}

void CVGUIMenuLayer::SetNormalizedBackgroundBounds( Vector2D p00, Vector2D p11 )
{
	m_vecBGBounds_min = p00;
	m_vecBGBounds_max = p11;
}

void CVGUIMenuLayer::AddParticle( vParticle * p )
{
	m_pParticleParent->AddParticle( p );
}

void CVGUIMenuLayer::AddOperator( vParticleOperatorBase * pOp )
{
	m_pParticleParent->AddGlobalOperator( pOp );
}

void CVGUIMenuLayer::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, t;
	engine->GetScreenSize( w, t );

	if ( m_bAutoSize )
	{
		SetBounds( 0, 0, w, t );
	}
	else
	{
		int cx = scheme()->GetProportionalScaledValue( rx );
		int cy = scheme()->GetProportionalScaledValue( ry );
		int cw = scheme()->GetProportionalScaledValue( rw );
		int ct = scheme()->GetProportionalScaledValue( rt );

		switch ( m_pin )
		{
		case PINCL_10:
				cx += w;
			break;
		case PINCL_11:
				cx += w;
				cy += t;
			break;
		case PINCL_01:
				cy += t;
			break;
		case PINCL_C0:
				cx += w / 2;
			break;
		case PINCL_0C:
				cy += t / 2;
			break;
		case PINCL_CC:
				cx += w / 2;
				cy += t / 2;
			break;
		}

		SetBounds( cx, cy, cw, ct );
	}

	m_pParticleParent->SetBounds( 0, 0, GetWide(), GetTall() );
}

void CVGUIMenuLayer::OnThink()
{
	if ( m_bRequireInit )
	{
		m_bRequireInit = false;

		if ( m_pRoutine != NULL )
			m_pRoutine( this, true );
		if ( m_pPaintHook != NULL )
			m_pPaintHook( this, true );
	}

	if ( m_pRoutine != NULL )
	{
		m_pRoutine( this, false );
	}
}

void CVGUIMenuLayer::Paint()
{
	BaseClass::Paint();

	if ( m_iBackgroundImage >= 0 &&
		( !engine->IsInGame() || m_bIngame ) )
	{
		int x, y, x1, y1;
		x = y = 0;
		GetSize( x1, y1 );

		if ( m_bAutoSize )
		{
			int halfx = x1 / 2;
			x1 = y1 * 16.0f/9.0f;

			x = halfx - x1/2;

			x1 += x;
		}
		else
		{
			x += m_vecBGBounds_min.x * GetWide();
			y += m_vecBGBounds_min.y * GetTall();
			x1 *= m_vecBGBounds_max.x;
			y1 *= m_vecBGBounds_max.y;
		}

		surface()->DrawSetColor( m_colBackgroundColor );
		surface()->DrawSetTexture( m_iBackgroundImage );
		surface()->DrawTexturedRect( x, y, x1, y1 );
	}

	if ( m_bRefractive && gpGlobals->framecount > 0 )
	{
		CViewSetup setup;
		setup.x = setup.y = 0;
		engine->GetScreenSize( setup.width, setup.height );
		//setup.context = 0;
		setup.origin.Init();
		setup.angles.Init();
		setup.m_bOrtho = false;
		setup.zNear = 7;
		setup.zFar = 30000;
		//setup.m_vUnreflectedOrigin = setup.origin;

		CMatRenderContextPtr renderContext( materials );

		Frustum frustum;
		render->Push3DView( setup, 0, NULL, frustum );
		renderContext->PushRenderTargetAndViewport( );

		UpdateScreenEffectTexture( 0, setup.x, setup.y, setup.width, setup.height);
		//UpdateScreenEffectTexture();

		renderContext->PopRenderTargetAndViewport();
		render->PopView( frustum );
	}

	if ( m_pPaintHook != NULL )
		m_pPaintHook( this, false );
}
