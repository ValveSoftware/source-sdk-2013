//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A panel that display particle systems
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/EditablePanel.h>
#include "vgui/IVGui.h"

#include "tf_particlepanel.h"
#include "matsys_controls/matsyscontrols.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "tier2/renderutils.h"
#include "renderparm.h"

using namespace vgui;


CTFParticlePanel::ParticleEffect_t::ParticleEffect_t()
	: m_bLoop( true )
	, m_pParticleSystem( NULL )
	, m_flLastTime( FLT_MAX )
	, m_ParticleSystemName( NULL )
	, m_bStartActivated( true )
	, m_flScale( 1.f )
	, m_flEndTime( FLT_MAX )
	, m_nXPos( 0 )
	, m_nYPos( 0 )
	, m_Angles( 0.f, 0.f, 0.f )
	, m_pParent( NULL )
	, m_bForceStopped( false )
	, m_bAutoDelete( false )
	, m_bStarted( false )
{}

CTFParticlePanel::ParticleEffect_t::~ParticleEffect_t()
{
	if ( m_pParticleSystem )
	{
		delete m_pParticleSystem;
		m_pParticleSystem = NULL;
	}
}


DECLARE_BUILD_FACTORY( CTFParticlePanel );
//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CTFParticlePanel::CTFParticlePanel( vgui::Panel *pParent, const char *pName ) 
	: BaseClass( pParent, pName )
{
	m_Camera.m_flZNear = 3.0f;
	m_Camera.m_flZFar = 16384.0f * 1.73205080757f;
	m_Camera.m_flFOV = 30.0f;
	m_Camera.m_origin = Vector(0,0,0);
	m_Camera.m_angles = QAngle(0,0,0);

	m_pLightmapTexture.Init( "//platform/materials/debug/defaultlightmap", "editor" );
	m_DefaultEnvCubemap.Init( "editor/cubemap", "editor", true );
}

CTFParticlePanel::~CTFParticlePanel()
{
	m_pLightmapTexture.Shutdown();
	m_DefaultEnvCubemap.Shutdown();
	m_vecParticleEffects.PurgeAndDeleteElements();

	if ( m_pKVParticles )
	{
		m_pKVParticles->deleteThis();
		m_pKVParticles = NULL;
	}
}


void CTFParticlePanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pKVParticleEffects = inResourceData->FindKey( "ParticleEffects" );
	if ( pKVParticleEffects )
	{
		if ( m_pKVParticles )
		{
			m_pKVParticles->deleteThis();
			m_pKVParticles = NULL;
		}

		m_pKVParticles = pKVParticleEffects->MakeCopy();

		UpdateParticlesFromKV();
	}
}

//-----------------------------------------------------------------------------
// Scheme
//-----------------------------------------------------------------------------
void CTFParticlePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );
}

void CTFParticlePanel::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "start", ARRAYSIZE("start") - 1 ) )
	{
		// Check if they specified a particular one to turn on
		const char* pszNum = command + ARRAYSIZE( "start" ) - 1;
		int nIndex = m_vecParticleEffects.InvalidIndex();

		if( pszNum && pszNum[0] )
		{
			nIndex = atoi(pszNum);
			Assert( nIndex >= 0 && nIndex < m_vecParticleEffects.Count() );
		}

		FOR_EACH_VEC( m_vecParticleEffects, i )
		{
			if ( nIndex != m_vecParticleEffects.InvalidIndex() && i != nIndex )
				continue;

			ParticleEffect_t* pEffect = m_vecParticleEffects[ i ];

			if( !pEffect->m_pParticleSystem )
			{
				pEffect->SetParticleSystem( pEffect->m_ParticleSystemName );
			}

			if( !pEffect->m_pParticleSystem )
				continue;
	
			pEffect->StartupParticleCollection();
			pEffect->m_pParticleSystem->StartEmission();
			pEffect->m_bForceStopped = false;
		}
	}
	else if ( !Q_strnicmp( command, "stop", ARRAYSIZE("stop") - 1 ) )
	{
		// Check if they specified a specific one to turn off
		const char* pszNum = command + ARRAYSIZE( "start" ) - 1;
		if( pszNum && pszNum[0] )
		{
			int iIndex = atoi(pszNum);
			Assert( iIndex >= 0 && iIndex < m_vecParticleEffects.Count() );

			ParticleEffect_t* pEffect = m_vecParticleEffects[iIndex];
			if( pEffect->m_pParticleSystem )
			{
				pEffect->m_pParticleSystem->StopEmission();
				pEffect->m_bForceStopped = true;
			}
		}
		else
		{
			// Turn them ALL off
			FOR_EACH_VEC( m_vecParticleEffects, i )
			{
				ParticleEffect_t* pEffect = m_vecParticleEffects[ i ];
				if( pEffect->m_pParticleSystem )
				{
					pEffect->m_pParticleSystem->StopEmission();
					pEffect->m_bForceStopped = true;
				}
			}
		}
	}
}

void CTFParticlePanel::OnSizeChanged( int wide, int tall )
{
	BaseClass::OnSizeChanged( wide, tall );
	UpdateParticlesFromKV();
}

void CTFParticlePanel::FireParticleEffect( const char *pszName, int xPos, int yPos, float flScale, bool bLoop, float flEndTime )
{
	m_vecParticleEffects[ m_vecParticleEffects.AddToTail() ] = new ParticleEffect_t();
	ParticleEffect_t* pEffect = m_vecParticleEffects.Tail();

	int iParentAbsX, iParentAbsY;
	vgui::ipanel()->GetAbsPos( GetParent()->GetVPanel(), iParentAbsX, iParentAbsY );

	pEffect->m_pParent = this;
	pEffect->m_nXPos = xPos - iParentAbsX;
	pEffect->m_nYPos = GetTall() - yPos - iParentAbsY;
	pEffect->m_flScale = flScale;
	pEffect->m_bLoop = bLoop;
	pEffect->m_bAutoDelete = true; // This will get automatically deleted once it stops
	pEffect->m_bStartActivated = true;
	pEffect->m_flEndTime = gpGlobals->curtime + flEndTime;

	if( IsProportional() )
	{
		int wide, tall;
		surface()->GetScreenSize( wide, tall );

		int proH, proW;
		surface()->GetProportionalBase( proW, proH );
		double scale = (double)tall / (double)proH;
		pEffect->m_flScale *= scale;
	}

	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		pEffect->SetControlPointValue( i, Vector( 0, 0, 10.0f * i ) );
	}
	pEffect->SetParticleSystem( pszName );
}


static bool IsValidHierarchy( CParticleCollection *pCollection )
{
	if ( !pCollection->IsValid() )
		return false;

	for( CParticleCollection *pChild = pCollection->m_Children.m_pHead; pChild; pChild = pChild->m_pNext )
	{
		if ( !IsValidHierarchy( pChild ) )
			return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Simulate the particle system
//-----------------------------------------------------------------------------
void CTFParticlePanel::OnTick()
{
	BaseClass::OnTick();
	
	float flTime = engine->Time();

	bool bAnyActive = false;
	// Update all particles
	FOR_EACH_VEC_BACK( m_vecParticleEffects, i )
	{
		bAnyActive |= m_vecParticleEffects[i]->Update( flTime );
		// If this effect is done and should auto-delete, then now is when we delete
		if( m_vecParticleEffects[i]->m_pParticleSystem == NULL && m_vecParticleEffects[i]->m_bAutoDelete )
		{
			delete m_vecParticleEffects[i];
			m_vecParticleEffects.FastRemove( i );
		}
	}

	if ( !bAnyActive )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
}


void CTFParticlePanel::Paint()
{
	// This needs calling to reset various counters.
	g_pParticleSystemMgr->SetLastSimulationTime( gpGlobals->curtime );

	// No particles?  Do nothing.
	if( m_vecParticleEffects.Count() == 0 )
		return;

	// See if anyone needs to even paint.  If not, let's not do anything else
	{
		bool bAnyNeedToPaint = false;
		FOR_EACH_VEC( m_vecParticleEffects, i )
		{
			bAnyNeedToPaint |= m_vecParticleEffects[i]->BNeedsToPaint();
		}

		if ( !bAnyNeedToPaint )
			return;
	}

	int iXPos, iYPos;
	GetPos( iXPos, iYPos );
	GetParent()->LocalToScreen( iXPos, iYPos );

	int nWide, nTall;
	GetSize( nWide, nTall );

	int screenW, screenH;
	vgui::surface()->GetScreenSize( screenW, screenH );

	nWide = Min( nWide, screenW );
	nTall = Min( nTall, screenH );

	int nTallClipped = GetTall() - nTall;

	vgui::MatSystemSurface()->Begin3DPaint( 0, 0, nWide, nTall, false );

	VMatrix view, projection;
	ComputeViewMatrix( &view, m_Camera );
	ComputeProjectionMatrix( &projection, m_Camera, nWide, nTall );

	CMatRenderContextPtr pRenderContext( g_pMaterialSystem );

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );
	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, false );

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->LoadIdentity( );

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->LoadMatrix( view );

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->LoadMatrix( projection );

	int clipRect[4];
	ipanel()->GetClipRect( GetVPanel(), clipRect[0], clipRect[1], clipRect[2], clipRect[3] );

	// Offset by where our edge is vs. where we wanted our edge to be
	int iXOffset = clipRect[ 0 ] - iXPos;
	int iYOffset = ( clipRect[ 1 ] - iYPos ) - nTallClipped;

	FOR_EACH_VEC( m_vecParticleEffects, i )
	{
		m_vecParticleEffects[i]->Paint( pRenderContext, iXOffset, iYOffset, nWide, nTall );
	}

	pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

	vgui::MatSystemSurface()->End3DPaint();
}

void CTFParticlePanel::UpdateParticlesFromKV()
{
	if ( !m_pKVParticles )
		return;

	m_vecParticleEffects.PurgeAndDeleteElements();

	FOR_EACH_SUBKEY( m_pKVParticles, pKVEffect )
	{
		m_vecParticleEffects[ m_vecParticleEffects.AddToTail() ] = new ParticleEffect_t();
		ParticleEffect_t* pEffect = m_vecParticleEffects.Tail();

		// get the position
		int alignScreenWide = GetWide(), alignScreenTall = GetTall();	// screen dimensions used for pinning in splitscreen

		int x, y;
		GetPos(x, y);
		const char *xstr = pKVEffect->GetString( "particle_xpos", NULL );
		const char *ystr = pKVEffect->GetString( "particle_ypos", NULL );

		if (xstr)
		{
			bool bRightAlign = false;
			bool bCenterAlign = false;
			// look for alignment flags
			if (xstr[0] == 'r' || xstr[0] == 'R')
			{
				bRightAlign = true;
				xstr++;
			}
			else if (xstr[0] == 'c' || xstr[0] == 'C')
			{
				bCenterAlign = true;
				xstr++;
			}

			// get the value
			x = atoi(xstr);
			// scale the x up to our screen co-ords
			if ( IsProportional() )
			{
				x = scheme()->GetProportionalScaledValueEx(GetScheme(), x);
			}
			// now correct the alignment
			if ( bRightAlign )
			{
				x = alignScreenWide - x;
			}
			else if ( bCenterAlign )
			{
				x = (alignScreenWide / 2) + x;
			}
		}

		if (ystr)
		{
			bool bBottomAlign = false;
			bool bCenterAlign = false;
			// look for alignment flags
			if (ystr[0] == 'r' || ystr[0] == 'R')
			{
				bBottomAlign = true;
				ystr++;
			}
			else if (ystr[0] == 'c' || ystr[0] == 'C')
			{
				bCenterAlign = true;
				ystr++;
			}
			y = atoi(ystr);
			if (IsProportional())
			{
				// scale the y up to our screen co-ords
				y = scheme()->GetProportionalScaledValueEx(GetScheme(), y);
			}
			// now correct the alignment
			if ( bBottomAlign )
			{
				y = alignScreenTall - y;
			}
			else if ( bCenterAlign )
			{
				y = (alignScreenTall / 2) + y;
			}
		}

		pEffect->m_nXPos = x;
		pEffect->m_nYPos = GetTall() - y;

		pEffect->m_flScale	= pKVEffect->GetFloat( "particle_scale", 1.f );
		// Scale the scale factor the same way we do the XY position coordinates
		if( IsProportional() )
		{
			int wide, tall;
			surface()->GetScreenSize( wide, tall );

			int proH, proW;
			surface()->GetProportionalBase( proW, proH );
			double scale = (double)tall / (double)proH;
			pEffect->m_flScale *= scale;
		}

		pEffect->m_pParent	= this;
		pEffect->m_bLoop		= pKVEffect->GetBool( "loop", true );
		pEffect->m_bStartActivated = pKVEffect->GetBool( "start_activated", true );
		pEffect->SetParticleSystem( pKVEffect->GetString( "particleName" ) );

		// Read angles for the particle system
		{
			float x1,y1,z1;
			const char* pszAngles = pKVEffect->GetString( "angles" );
			if( *pszAngles )
			{
				if( pEffect->m_pParticleSystem && sscanf( pszAngles, "%f %f %f", &x1, &y1, &z1 ) == 3 )
				{
					pEffect->m_Angles = QAngle( x1, y1, z1 );
					Quaternion q;
					AngleQuaternion( pEffect->m_Angles , q );
					pEffect->m_pParticleSystem->SetControlPointOrientation( 0, q );
				}
			}
		}

		pEffect->SetControlPointValue( 0, Vector(0,0,0) );
		// Read all control point values
		const char* pszControlPoint = NULL;
		int nControlPointNumber = 0;
		do
		{
			pszControlPoint = pKVEffect->GetString( VarArgs("control_point%d", nControlPointNumber), "" );
			if ( *pszControlPoint )
			{
				float x2,y2,z2;
				if (sscanf(pszControlPoint, "%f %f %f", &x2, &y2, &z2 ) == 3)
				{
					pEffect->SetControlPointValue( nControlPointNumber, Vector( x2, y2, z2 ) );
				}
			}

			++nControlPointNumber;
		}
		while( *pszControlPoint );
	}
}

bool CTFParticlePanel::ParticleEffect_t::Update( float flTime )
{
	if ( !m_pParticleSystem || !m_bStarted )
		return false;

	if ( m_flLastTime == FLT_MAX )
	{
		m_flLastTime = flTime;
	}

	float flDt = flTime - m_flLastTime;
	m_flLastTime = flTime;

	Quaternion q;
	AngleQuaternion( m_Angles, q );

	for ( int i = 0; i < MAX_PARTICLE_CONTROL_POINTS; ++i )
	{
		if ( !m_pParticleSystem->ReadsControlPoint( i ) )
			continue;

		m_pParticleSystem->SetControlPoint( i, m_pControlPointValue[i] );
		m_pParticleSystem->SetControlPointOrientation( i, q );
		m_pParticleSystem->SetControlPointParent( i, i );
	}

	// Restart the particle system if it's finished
	bool bIsInvalid = !IsValidHierarchy( m_pParticleSystem );

	if ( !bIsInvalid )
	{
		m_pParticleSystem->Simulate( flDt, false );
	}

	// Past our end time?
	bool bEnd = gpGlobals->curtime >= m_flEndTime;

	if ( m_pParticleSystem->IsFinished() || bIsInvalid || bEnd )
	{
		delete m_pParticleSystem;
		m_pParticleSystem = NULL;

		// Loop if we're supposed to
		if ( m_bLoop && m_ParticleSystemName.Length() && !m_bForceStopped )
		{
			m_pParticleSystem = g_pParticleSystemMgr->CreateParticleCollection( m_ParticleSystemName );
		}

		if ( bIsInvalid && m_pParent )
		{
			m_pParent->PostActionSignal( new KeyValues( "ParticleSystemReconstructed" ) );
		}
		m_flLastTime = FLT_MAX;
	}

	return m_pParticleSystem != NULL;
}


//-----------------------------------------------------------------------------
// Startup, shutdown particle collection
//-----------------------------------------------------------------------------
void CTFParticlePanel::ParticleEffect_t::StartupParticleCollection()
{
	if ( m_pParticleSystem && m_pParent )
	{
		vgui::ivgui()->AddTickSignal( m_pParent->GetVPanel(), 0 );
	}
	m_flLastTime = FLT_MAX;
	m_bStarted = true;
}

void CTFParticlePanel::ParticleEffect_t::ShutdownParticleCollection()
{
	if ( m_pParticleSystem && m_pParent )
	{
		delete m_pParticleSystem;
		m_pParticleSystem = NULL;
	}
	m_bStarted = false;
}

//-----------------------------------------------------------------------------
// Set the particle system to draw
//-----------------------------------------------------------------------------
void CTFParticlePanel::ParticleEffect_t::SetParticleSystem( const char* pszParticleSystemName )
{
	ShutdownParticleCollection();

	if( !g_pParticleSystemMgr->IsParticleSystemDefined( pszParticleSystemName ) )
	{
		AssertMsg1( false, "%s is not a valid particle system name", pszParticleSystemName );
		return;
	}
	m_pParticleSystem = g_pParticleSystemMgr->CreateParticleCollection( pszParticleSystemName );
	m_ParticleSystemName = pszParticleSystemName;

	m_pParent->PostActionSignal( new KeyValues( "ParticleSystemReconstructed" ) );
	
	if( m_bStartActivated )
	{
		StartupParticleCollection();
	}
}


void CTFParticlePanel::ParticleEffect_t::Paint( CMatRenderContextPtr& pRenderContext,
												int iXOffset,
												int iYOffset, 
												int nWide,
												int nTall )
{
	if ( !m_pParticleSystem || !m_bStarted )
		return;

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	

	pRenderContext->Ortho( 0, 0, nWide, nTall, -9999, 9999 );

	int nX = m_nXPos - iXOffset;
	int nY = m_nYPos + iYOffset;
	pRenderContext->Translate( nX, nY, 0.f );
	pRenderContext->Scale( m_flScale, m_flScale, m_flScale );

	// Render Particles
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity( );

	m_pParticleSystem->Render( pRenderContext );

	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}