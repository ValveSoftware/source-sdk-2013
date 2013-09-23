
#include "cbase.h"
#include "Gstring/cscreenoverlay_multi.h"

#ifdef GAME_DLL

#define OVERLAYFLAGS_ENABLED 0x01

#else
#include "cdll_client_int.h"
#include "view_scene.h"
#endif

static CUtlVector< CScreenoverlayMulti* > g_hScreenoverlayList;

static int OverlaySort( CScreenoverlayMulti *const *p1, CScreenoverlayMulti *const *p2 )
{
	if ( (*p1)->GetOverlayRenderMode() != (*p2)->GetOverlayRenderMode() )
		return ( (*p1)->GetOverlayRenderMode() < (*p2)->GetOverlayRenderMode() ) ? -1 : 1;

	return ( (*p1)->GetRenderIndex() < (*p2)->GetRenderIndex() ) ? -1 : 1;
}

static void AddOverlay( CScreenoverlayMulti *o )
{
	Assert( !g_hScreenoverlayList.HasElement( o ) );
	g_hScreenoverlayList.AddToTail( o );
	g_hScreenoverlayList.Sort( OverlaySort );
}

static void RemoveOverlay( CScreenoverlayMulti *o )
{
	Assert( g_hScreenoverlayList.HasElement( o ) );
	g_hScreenoverlayList.FindAndRemove( o );
}

#ifdef CLIENT_DLL
void DrawOverlaysForMode( CScreenoverlayMulti::RENDERMODE mode, int x, int y, int w, int h )
{
	for ( int i = 0; i < g_hScreenoverlayList.Count(); i++ )
	{
		CScreenoverlayMulti *o = g_hScreenoverlayList[ i ];

		if ( mode != o->GetOverlayRenderMode() )
			continue;

		if ( !o->IsEnabled() )
			continue;

		o->RenderOverlay( x, y, w, h );
	}
}
#endif

#ifdef GAME_DLL
static void ValidateEntityData()
{
	for ( int a = 0; a < g_hScreenoverlayList.Count(); a++ )
	{
		CScreenoverlayMulti *overlay_0 = g_hScreenoverlayList[a];

		for ( int b = 0; b < g_hScreenoverlayList.Count(); b++ )
		{
			if ( a == b )
				continue;

			CScreenoverlayMulti *overlay_1 = g_hScreenoverlayList[b];

			if ( overlay_0->GetOverlayRenderMode() != overlay_1->GetOverlayRenderMode() )
				continue;

			if ( overlay_0->GetRenderIndex() != overlay_1->GetRenderIndex() )
				continue;

			Warning( "WARNING: two env_screenoverlay_multi entities are using the same render mode and index!\n" );
			const char *name_0 = STRING( overlay_0->GetEntityName() );
			const char *name_1 = STRING( overlay_1->GetEntityName() );
			Warning( "    mode: %i, index: %i, entity name A: %s, name B: %s\n",
				overlay_0->GetOverlayRenderMode(), overlay_0->GetRenderIndex(),
				(name_0 ? name_0 : "NONAME"), (name_1 ? name_1 : "NONAME") );
		}
	}
};
#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CScreenoverlayMulti )

	DEFINE_KEYFIELD( m_strOverlayMaterial, FIELD_STRING, "overlayname" ),
	DEFINE_KEYFIELD( m_iRenderMode, FIELD_INTEGER, "rendermode" ),
	DEFINE_KEYFIELD( m_iRenderIndex, FIELD_INTEGER, "renderindex" ),

	DEFINE_FIELD( m_iMaterialIndex, FIELD_INTEGER ),

	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "toggle", InputToggle ),

END_DATADESC()
#endif


IMPLEMENT_NETWORKCLASS_DT( CScreenoverlayMulti, CScreenoverlayMulti_DT )

#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_iMaterialIndex ) ),

	SendPropInt( SENDINFO( m_iRenderMode ) ),
	SendPropInt( SENDINFO( m_iRenderIndex ) ),

	SendPropBool( SENDINFO( m_bEnabled ) ),
#else
	RecvPropInt( RECVINFO( m_iMaterialIndex ) ),

	RecvPropInt( RECVINFO( m_iRenderMode ) ),
	RecvPropInt( RECVINFO( m_iRenderIndex ) ),

	RecvPropBool( RECVINFO( m_bEnabled ) ),
#endif

END_NETWORK_TABLE();


LINK_ENTITY_TO_CLASS( env_screenoverlay_multi, CScreenoverlayMulti );

CScreenoverlayMulti::CScreenoverlayMulti()
{
#ifdef GAME_DLL
	AddOverlay( this );
#endif
}

CScreenoverlayMulti::~CScreenoverlayMulti()
{
#ifdef GAME_DLL
	RemoveOverlay( this );
#endif
}

int CScreenoverlayMulti::GetOverlayRenderMode()
{
	return m_iRenderMode;
}

int CScreenoverlayMulti::GetRenderIndex()
{
	return m_iRenderIndex;
}

bool CScreenoverlayMulti::IsEnabled()
{
	return m_bEnabled;
}

#ifdef GAME_DLL
void CScreenoverlayMulti::Spawn()
{
	BaseClass::Spawn();

	SetEnabled( HasSpawnFlags( OVERLAYFLAGS_ENABLED ) );
}

void CScreenoverlayMulti::Activate()
{
	m_iMaterialIndex = -1;

	BaseClass::Activate();

	Assert( m_strOverlayMaterial.ToCStr() && m_strOverlayMaterial.ToCStr()[0] != '\0' );

	m_iMaterialIndex = PrecacheMaterialGetIndex( m_strOverlayMaterial.ToCStr() );
	
	Assert( m_iMaterialIndex >= 0 );

	ValidateEntityData();
}

int CScreenoverlayMulti::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CScreenoverlayMulti::SetEnabled( bool bEnabled )
{
	m_bEnabled = bEnabled;
}

void CScreenoverlayMulti::InputEnable( inputdata_t &inputdata )
{
	SetEnabled( true );
}

void CScreenoverlayMulti::InputDisable( inputdata_t &inputdata )
{
	SetEnabled( false );
}

void CScreenoverlayMulti::InputToggle( inputdata_t &inputdata )
{
	SetEnabled( !IsEnabled() );
}
#else
void CScreenoverlayMulti::OnDataChanged( DataUpdateType_t t )
{
	BaseClass::OnDataChanged( t );

	if ( t == DATA_UPDATE_CREATED )
	{
		Assert( m_iMaterialIndex >= 0 );

		const char *pszMat = GetMaterialNameFromIndex( m_iMaterialIndex );

		Assert( pszMat != NULL );

		if ( pszMat != NULL )
		{
			m_matOverlay.Init( pszMat, TEXTURE_GROUP_CLIENT_EFFECTS );
		}

		AddOverlay( this );
	}
}

void CScreenoverlayMulti::RenderOverlay( int x, int y, int w, int h )
{
	DrawScreenEffectMaterial( m_matOverlay, x, y, w, h );
}

void CScreenoverlayMulti::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	RemoveOverlay( this );
}
#endif