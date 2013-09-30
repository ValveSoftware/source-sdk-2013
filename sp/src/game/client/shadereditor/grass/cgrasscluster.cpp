
#include "cbase.h"
#include "ShaderEditor/Grass/CGrassCluster.h"
#include "view.h"
#include "viewrender.h"
#include "idebugoverlaypanel.h"
#include "engine/IVDebugOverlay.h"
#include "iclientshadowmgr.h"
#include "clientleafsystem.h"
#include "materialsystem/ITexture.h"
#include "materialsystem/IMaterialVar.h"
#include "collisionutils.h"
#include "fasttimer.h"

#define CONSTBOXEXTENT_LIGHT 1000
#define CONSTBOXEXTENT_COL 500

static ConVar gcluster_objectsPerHint( "grasscluster_objects_per_hint", "8" );
static ConVar gcluster_debug( "grasscluster_debug", "0" );
static ConVar gcluster_enable( "grasscluster_enable", "1" );
static ConVar gcluster_enable_flashlight( "grasscluster_enable_flashlightSupport", "1" );
static ConVar gcluster_enable_morph( "grasscluster_enable_morph", "1" );
static ConVar gcluster_cullDist( "grasscluster_cullDist", "4096" );

static ConVar gcluster_LOD_enable( "grasscluster_LOD_enable", "1" );
static ConVar gcluster_LOD_transitionDist( "grasscluster_LOD_transitionDist", "2048" );
static ConVar gcluster_LOD_objects_per_hint( "grasscluster_LOD_objects_per_hint", "1" );

static ConVar gcluster_grass_height_small_min( "grasscluster_grass_height_small_min", "20" );
static ConVar gcluster_grass_height_small_max( "grasscluster_grass_height_small_max", "30" );
static ConVar gcluster_grass_height_med_min( "grasscluster_grass_height_med_min", "30" );
static ConVar gcluster_grass_height_med_max( "grasscluster_grass_height_med_max", "50" );
static ConVar gcluster_grass_height_huge_min( "grasscluster_grass_height_huge_min", "50" );
static ConVar gcluster_grass_height_huge_max( "grasscluster_grass_height_huge_max", "70" );

static ConVar gcluster_grass_width_small_min( "grasscluster_grass_width_small_min", "25" );
static ConVar gcluster_grass_width_small_max( "grasscluster_grass_width_small_max", "35" );
static ConVar gcluster_grass_width_med_min( "grasscluster_grass_width_med_min", "40" );
static ConVar gcluster_grass_width_med_max( "grasscluster_grass_width_med_max", "60" );
static ConVar gcluster_grass_width_huge_min( "grasscluster_grass_width_huge_min", "60" );
static ConVar gcluster_grass_width_huge_max( "grasscluster_grass_width_huge_max", "80" );

static ConVar gcluster_clusterMaxQuads( "grasscluster_grass_clusterMaxQuads", "-1" );

static ConVar gcluster_grass_type_huge_oddness( "grasscluster_grass_type_huge_oddness", "20" );
static ConVar gcluster_grass_type_small_oddness( "grasscluster_grass_type_small_oddness", "2" );

static ConVar gcluster_grass_meadow_scale( "grasscluster_grass_meadow_scale", "1" );

static ConVar gcluster_grass_wind_angle( "grasscluster_grass_wind_angle", "70" );
static ConVar gcluster_grass_wind_strength( "grasscluster_grass_wind_strength", "1" );

static ConVar gcluster_sprite_index( "grasscluster_sprite_index", "0" );

static ConVar gcluster_grass_morph_delay( "grasscluster_grass_meadow_morph_delay", "5" );
static ConVar gcluster_grass_morph_framelag( "grasscluster_grass_meadow_morph_framelag", "0.1" );
static ConVar gcluster_grass_morph_speed( "grasscluster_grass_meadow_morph_speed", "1" );

static ConVar gcluster_grass_terrain_offset_min( "grasscluster_grass_terrain_offset_min", "20" );
static ConVar gcluster_grass_terrain_offset_exp( "grasscluster_grass_terrain_offset_exp", "0.5" );
static ConVar gcluster_grass_terrain_offset_multi( "grasscluster_grass_terrain_offset_multi", "2" );

extern ConVar r_DrawDetailProps;

const char *szSpriteMaterials[] = {
	"detail/detailsprites_editor",
	"detail/grass_lawn_cut",
	"detail/grass_lawn_cut_dark",
	"detail/grass_lawn_cut_lite",
};
const int iSpriteMaterialCount = ARRAYSIZE( szSpriteMaterials );

CON_COMMAND( grasscluster_flush, "" )
{
	CGrassClusterManager::GetInstance()->ClearClusterData();
}

CON_COMMAND( grasscluster_preset_density_high, "" )
{
	gcluster_grass_type_huge_oddness.Revert();
	gcluster_objectsPerHint.Revert();
	gcluster_LOD_objects_per_hint.Revert();
	gcluster_cullDist.Revert();
	CGrassClusterManager::GetInstance()->ClearClusterData();
}
CON_COMMAND( grasscluster_preset_density_med, "" )
{
	gcluster_grass_type_huge_oddness.SetValue( 17 );
	gcluster_objectsPerHint.SetValue( "5" );
	gcluster_LOD_objects_per_hint.Revert();
	gcluster_cullDist.SetValue( gcluster_cullDist.GetDefault() );
	CGrassClusterManager::GetInstance()->ClearClusterData();
}
CON_COMMAND( grasscluster_preset_density_low, "" )
{
	gcluster_grass_type_huge_oddness.SetValue( 10 );
	gcluster_objectsPerHint.SetValue( "2" );
	gcluster_LOD_objects_per_hint.Revert();
	gcluster_cullDist.SetValue( gcluster_cullDist.GetDefault() );
	CGrassClusterManager::GetInstance()->ClearClusterData();
}

CON_COMMAND( grasscluster_preset_height_monstrous, "" )
{
	gcluster_grass_height_small_min.SetValue( 50 );
	gcluster_grass_height_small_max.SetValue( 60 );
	gcluster_grass_height_med_min.SetValue( 80 );
	gcluster_grass_height_med_max.SetValue( 95 );
	gcluster_grass_height_huge_min.SetValue( 100 );
	gcluster_grass_height_huge_max.SetValue( 130 );

	gcluster_grass_width_small_min.SetValue( 60 );
	gcluster_grass_width_small_max.SetValue( 70 );
	gcluster_grass_width_med_min.SetValue( 85 );
	gcluster_grass_width_med_max.SetValue( 100 );
	gcluster_grass_width_huge_min.SetValue( 110 );
	gcluster_grass_width_huge_max.SetValue( 140 );
	CGrassClusterManager::GetInstance()->ClearClusterData();
}
CON_COMMAND( grasscluster_preset_height_high, "" )
{
	gcluster_grass_height_small_min.SetValue( gcluster_grass_height_small_min.GetDefault() );
	gcluster_grass_height_small_max.SetValue( gcluster_grass_height_small_max.GetDefault() );
	gcluster_grass_height_med_min.SetValue( gcluster_grass_height_med_min.GetDefault() );
	gcluster_grass_height_med_max.SetValue( gcluster_grass_height_med_max.GetDefault() );
	gcluster_grass_height_huge_min.SetValue( gcluster_grass_height_huge_min.GetDefault() );
	gcluster_grass_height_huge_max.SetValue( gcluster_grass_height_huge_max.GetDefault() );

	gcluster_grass_width_small_min.SetValue( gcluster_grass_width_small_min.GetDefault() );
	gcluster_grass_width_small_max.SetValue( gcluster_grass_width_small_max.GetDefault() );
	gcluster_grass_width_med_min.SetValue( gcluster_grass_width_med_min.GetDefault() );
	gcluster_grass_width_med_max.SetValue( gcluster_grass_width_med_max.GetDefault() );
	gcluster_grass_width_huge_min.SetValue( gcluster_grass_width_huge_min.GetDefault() );
	gcluster_grass_width_huge_max.SetValue( gcluster_grass_width_huge_max.GetDefault() );
	CGrassClusterManager::GetInstance()->ClearClusterData();
}
CON_COMMAND( grasscluster_preset_height_med, "" )
{
	gcluster_grass_height_small_min.SetValue( 10 );
	gcluster_grass_height_small_max.SetValue( 15 );
	gcluster_grass_height_med_min.SetValue( 15 );
	gcluster_grass_height_med_max.SetValue( 30 );
	gcluster_grass_height_huge_min.SetValue( 25 );
	gcluster_grass_height_huge_max.SetValue( 45 );

	gcluster_grass_width_small_min.SetValue( 15 );
	gcluster_grass_width_small_max.SetValue( 20 );
	gcluster_grass_width_med_min.SetValue( 20 );
	gcluster_grass_width_med_max.SetValue( 35 );
	gcluster_grass_width_huge_min.SetValue( 30 );
	gcluster_grass_width_huge_max.SetValue( 50 );
	CGrassClusterManager::GetInstance()->ClearClusterData();
}
CON_COMMAND( grasscluster_preset_height_low, "" )
{
	gcluster_grass_height_small_min.SetValue( 8 );
	gcluster_grass_height_small_max.SetValue( 12 );
	gcluster_grass_height_med_min.SetValue( 10 );
	gcluster_grass_height_med_max.SetValue( 15 );
	gcluster_grass_height_huge_min.SetValue( 12 );
	gcluster_grass_height_huge_max.SetValue( 25 );

	gcluster_grass_width_small_min.SetValue( 10 );
	gcluster_grass_width_small_max.SetValue( 15 );
	gcluster_grass_width_med_min.SetValue( 15 );
	gcluster_grass_width_med_max.SetValue( 20 );
	gcluster_grass_width_huge_min.SetValue( 20 );
	gcluster_grass_width_huge_max.SetValue( 30 );
	CGrassClusterManager::GetInstance()->ClearClusterData();
}

CON_COMMAND( grasscluster_preset_meadow_ultra, "" )
{
	gcluster_grass_height_small_min.SetValue( 8 );
	gcluster_grass_height_small_max.SetValue( 12 );
	gcluster_grass_height_med_min.SetValue( 10 );
	gcluster_grass_height_med_max.SetValue( 15 );
	gcluster_grass_height_huge_min.SetValue( 12 );
	gcluster_grass_height_huge_max.SetValue( 25 );

	gcluster_grass_width_small_min.SetValue( 10 );
	gcluster_grass_width_small_max.SetValue( 15 );
	gcluster_grass_width_med_min.SetValue( 15 );
	gcluster_grass_width_med_max.SetValue( 20 );
	gcluster_grass_width_huge_min.SetValue( 20 );
	gcluster_grass_width_huge_max.SetValue( 30 );

	gcluster_grass_type_huge_oddness.SetValue( 100 );
	gcluster_cullDist.SetValue( 4096 );
	gcluster_objectsPerHint.SetValue( 40 );
	gcluster_LOD_objects_per_hint.SetValue( 5 );
	//gcluster_sprite_index.SetValue( 2 );
	CGrassClusterManager::GetInstance()->ClearClusterData();
}

#ifdef SHADEREDITOR_FORCE_ENABLED
#endif

Vector FetchLightSamples( Vector pos )
{
	//return engine->GetLightForPoint( pos, true );
	Vector dir;
	QAngle ang;
	Vector res( vec3_origin );
	for ( int y = 0; y < 8; y++ )
	{
		for ( int x = 0; x < 4; x++ )
		{
			ang.Init( x * (-90.0f/7.0f) - (90.0f/3.0f), y * (360.0f/9.0f), 0 );
			AngleVectors( ang, &dir );
			dir = dir * 32.0f + pos;
			res += engine->GetLightForPoint( dir, true );
		}
	}
	res /= 32.0f;

	for ( int i = 0; i < 3; i++ )
		res[i] = RemapValClamped( res[i], 0, 0.5f, 0, 1 );

	return res;
}

int GrassInfoSort( _grassClusterInfo const *c1, _grassClusterInfo const *c2 )
{
	return ( c1->flSortDist < c2->flSortDist ) ? -1 : 1;
}

_grassPressureData::_grassPressureData()
{
	iNumGrassObjects = 0;

	vecDir = NULL;
	vecPos = NULL;
	flAmt = NULL;
	flHeight = NULL;
	flLastMoveTime = NULL;
	flLastUpdateTime = NULL;
	bDirty = NULL;
}
_grassPressureData::~_grassPressureData()
{
	Assert( flAmt && vecDir && bDirty && vecPos );

	delete [] vecDir;
	delete [] vecPos;
	delete [] flAmt;
	delete [] flHeight;
	delete [] flLastMoveTime;
	delete [] flLastUpdateTime;
	delete [] bDirty;
}
void _grassPressureData::Init( int num )
{
	delete [] vecDir;
	delete [] vecPos;
	delete [] flAmt;
	delete [] flHeight;
	delete [] flLastMoveTime;
	delete [] flLastUpdateTime;
	delete [] bDirty;

	iNumGrassObjects = num;

	vecDir = new Vector[ iNumGrassObjects ];
	vecPos = new Vector[ iNumGrassObjects ];
	flAmt = new float[ iNumGrassObjects ];
	flHeight = new float[ iNumGrassObjects ];
	flLastMoveTime = new float[ iNumGrassObjects ];
	flLastUpdateTime = new float[ iNumGrassObjects ];
	bDirty = new bool[ iNumGrassObjects ];

	Q_memset( vecDir, 0, sizeof( Vector ) * iNumGrassObjects );
	Q_memset( vecPos, 0, sizeof( Vector ) * iNumGrassObjects );
	Q_memset( flAmt, 0, sizeof( float ) * iNumGrassObjects );
	Q_memset( flHeight, 0, sizeof( float ) * iNumGrassObjects );
	Q_memset( flLastMoveTime, 0, sizeof( float ) * iNumGrassObjects );
	Q_memset( flLastUpdateTime, 0, sizeof( float ) * iNumGrassObjects );
	Q_memset( bDirty, 0, sizeof( bool ) * iNumGrassObjects );
}


_grassClusterInfo::_grassClusterInfo()
{
	flSortDist = -1;
}


_grassClusterData::_grassClusterData()
{
	pGrassMesh = NULL;
	pPressureInfo = NULL;
	pLOD = NULL;
	iNextLodThreshold = 0;
	iNumQuads = -1;
	lighting = NULL;
	iLPatchSize_x = iLPatchSize_y = 0;
	flLPatchStep_x = flLPatchStep_y = 0;
}
_grassClusterData::~_grassClusterData()
{
	// DestroyLightingPatch();
}
void _grassClusterData::Destroy()
{
	if ( pLOD )
		pLOD->Destroy();
	delete pLOD;

	if ( pGrassMesh )
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->DestroyStaticMesh( pGrassMesh );
		pGrassMesh = NULL;
	}

	if ( pPressureInfo )
	{
		delete pPressureInfo;
		pPressureInfo = NULL;
	}
}
int _grassClusterData::Draw()
{
	const bool bDrawLOD = pLOD != NULL &&
		(MainViewOrigin() - pos).LengthSqr() > iNextLodThreshold;

	if ( bDrawLOD )
		return pLOD->Draw();
	else
	{
		Assert( pGrassMesh );

		pGrassMesh->Draw();
		return iNumQuads;
	}
}
void _grassClusterData::CreateLightingPatch( const CUtlVector< _grassClusterInfo > &hints )
{
	Assert( !lighting );

	CUtlVector< _grassClusterInfo > localHints;
	localHints.AddVectorToTail( hints );

	for ( int i = 0; i < localHints.Count(); i++ )
	{
		if ( !IsPointInBox( localHints[i].orig,
			extents_min - Vector(1,1,1) * CONSTBOXEXTENT_LIGHT,
			extents_max + Vector(1,1,1) * CONSTBOXEXTENT_LIGHT ) )
		{
			localHints.Remove(i);
			i--;
		}
	}

	const float sizeScaling = 0.006f; //0.006f;
	float deltax = extents_max.x - extents_min.x;
	float deltay = extents_max.y - extents_min.y;
	iLPatchSize_x = deltax * sizeScaling + 1;
	iLPatchSize_y = deltay * sizeScaling + 1;
	flLPatchStep_x = (iLPatchSize_x > 0) ? deltax / (iLPatchSize_x-1) : 0;
	flLPatchStep_y = (iLPatchSize_y > 0) ? deltay / (iLPatchSize_y-1) : 0;

	lighting = new Vector[iLPatchSize_x * iLPatchSize_y];

	for ( int x = 0; x < iLPatchSize_x; x++ )
	{
		for ( int y = 0; y < iLPatchSize_y; y++ )
		{
			int slot = y * iLPatchSize_x + x;
			const Vector pos = extents_min +
				Vector( x * flLPatchStep_x, y * flLPatchStep_y, (extents_max.z-extents_min.z) * 0.5f );

			//DebugDrawLine( pos, pos+Vector(0,0,100), 0,0,255,false, 10.0f );

			Vector &light = lighting[ slot ];

			for ( int i = 0; i < localHints.Count(); i++ )
				localHints[i].flSortDist = (localHints[i].orig - pos).LengthSqr();
			localHints.Sort( GrassInfoSort );

			light.Init( 0, 0, 0 );
			int l = 0;
			for ( ; l < min( localHints.Count(), 20 ); l++ )
				light += localHints[l].color.AsVector3D();

			if ( l )
				light /= (float)l;

			//Vector sample = FetchLightSamples( pos + Vector( 0, 0, 40 ) );
			//for ( int i = 0; i < 3; i++ )
			//	light[i] = min( light[i], sample[i] );
			//light = ( light +  ) * 0.5f;

			float intensity = (light.x+light.y+light.z) * 0.333334f;
			for ( int i = 0; i < 3; i++ )
				light[i] *= intensity; // * intensity;

			//float intensity = light.Length();
			//intensity *= intensity;
			//light.NormalizeInPlace();
			//light *= intensity;

			Assert( IsFinite( light.x ) && IsFinite( light.y ) && IsFinite( light.z ) );
		}
	}

	localHints.Purge();
}
const Vector _grassClusterData::GetLightingForPoint( const Vector &pos )
{
	Assert( lighting );
	Assert( iLPatchSize_x && iLPatchSize_y );

	if ( iLPatchSize_x == 1 && iLPatchSize_y == 1 )
		return lighting[0];

	float delta_x = ( pos.x - extents_min.x );
	float delta_y = ( pos.y - extents_min.y );
	int x0 = delta_x / flLPatchStep_x;
	int y0 = delta_y / flLPatchStep_y;
	float interp_x = clamp( ( delta_x - x0 * flLPatchStep_x ) / flLPatchStep_x, 0, 1 );
	float interp_y = clamp( ( delta_y - y0 * flLPatchStep_y ) / flLPatchStep_y, 0, 1 );

	x0 = max( 0, min( x0, iLPatchSize_x - 2 ) );
	y0 = max( 0, min( y0, iLPatchSize_y - 2 ) );

	int x1 = min( x0 + 1, iLPatchSize_x - 1 );
	int y1 = min( y0 + 1, iLPatchSize_y - 1 );

	Assert( x1 + iLPatchSize_x * y1 < iLPatchSize_x * iLPatchSize_y );

	Vector samples[2][2];
	samples[0][0] = lighting[ x0 + iLPatchSize_x * y0 ];
	samples[0][1] = lighting[ x0 + iLPatchSize_x * y1 ];
	samples[1][1] = lighting[ x1 + iLPatchSize_x * y1 ];
	samples[1][0] = lighting[ x1 + iLPatchSize_x * y0 ];

	Assert( IsFinite( samples[0][0].x ) && IsFinite( samples[0][0].y ) && IsFinite( samples[0][0].z ) );
	Assert( IsFinite( samples[0][1].x ) && IsFinite( samples[0][1].y ) && IsFinite( samples[0][1].z ) );
	Assert( IsFinite( samples[1][0].x ) && IsFinite( samples[1][0].y ) && IsFinite( samples[1][0].z ) );
	Assert( IsFinite( samples[1][1].x ) && IsFinite( samples[1][1].y ) && IsFinite( samples[1][1].z ) );
	Assert( interp_x >= 0 && interp_x <= 1 );
	Assert( interp_y >= 0 && interp_y <= 1 );

	return Lerp( interp_y,
		Lerp( interp_x, samples[0][0], samples[1][0] ),
		Lerp( interp_x, samples[0][1], samples[1][1] ) );
}
void _grassClusterData::DestroyLightingPatch()
{
	Assert( lighting );

	delete [] lighting;
	lighting = NULL;
}


clusterMaterial::clusterMaterial()
{
	ivar_dir = ivar_ang = 0;
}
void clusterMaterial::Init( const char *pszMat )
{
	ivar_dir = ivar_ang = 0;
	mat.Init( pszMat, TEXTURE_GROUP_OTHER );

	if ( mat.IsValid() && IsErrorMaterial( mat ) )
		mat.Shutdown();
}
void clusterMaterial::Shutdown()
{
	ivar_dir = ivar_ang = 0;
	mat.Shutdown();
}
bool clusterMaterial::IsValid()
{
	return mat.IsValid();
}
IMaterial *clusterMaterial::GetMaterial()
{
	if ( mat.IsValid() )
		return mat;
	return NULL;
	//ivar_dir
}
IMaterialVar *clusterMaterial::GetVarDir()
{
	if ( !GetMaterial() )
		return NULL;
	return GetMaterial()->FindVarFast( "$MUTABLE_01", &ivar_dir );
}
IMaterialVar *clusterMaterial::GetVarAng()
{
	if ( !GetMaterial() )
		return NULL;
	return GetMaterial()->FindVarFast( "$MUTABLE_02", &ivar_ang );
}


void ReleaseGrassCluster()
{
	CGrassClusterManager::GetInstance()->ClearClusterData();
}

CGrassClusterManager::CGrassClusterManager() : CAutoGameSystemPerFrame( "grassclustersystem" )
{
	m_iDrawnQuads = 0;
	m_iDrawnCluster = 0;
	m_iDrawnPerDrawcall = 0;
	m_iDrawnEngineMax = 0;
	m_flMorphTime = 0;

	//m_refMaterial = NULL;
	m_refMaterials = NULL;
}

CGrassClusterManager::~CGrassClusterManager()
{
	m_hClusterInfo.Purge();
	m_hClusterData.Purge();

	//delete [] m_refMaterial;
	delete [] m_refMaterials;
}

static CGrassClusterManager g_pGrassClusterManager;

CGrassClusterManager *CGrassClusterManager::GetInstance()
{
	return &g_pGrassClusterManager;
}

bool CGrassClusterManager::Init()
{
	materials->AddReleaseFunc( &ReleaseGrassCluster );

	return true;
}

void CGrassClusterManager::Shutdown()
{
	materials->RemoveReleaseFunc( &ReleaseGrassCluster );
}

IMaterial *CGrassClusterManager::GetActiveMaterial()
{
	if ( !m_refMaterials )
		return NULL;

	if ( m_iCurrentMaterial < 0 || m_iCurrentMaterial >= iSpriteMaterialCount )
		return NULL;

	return m_refMaterials[ m_iCurrentMaterial ].GetMaterial();
}

void CGrassClusterManager::LevelInitPostEntity()
{
	delete [] m_refMaterials;
	m_refMaterials = new clusterMaterial[ iSpriteMaterialCount ];

	for ( int i = 0; i < iSpriteMaterialCount; i++ )
	{
		m_refMaterials[i].Init( szSpriteMaterials[i] );

		if ( !m_refMaterials[i].IsValid() )
			Warning( "unable to initialize grass material: %s\n", szSpriteMaterials[i] );
	}


	//m_refMaterial.Init( "detail/detailsprites_editor", TEXTURE_GROUP_OTHER );
	//m_refMaterial.Init( "debug/debugspritewireframe", TEXTURE_GROUP_OTHER );

}

void CGrassClusterManager::LevelShutdownPostEntity()
{
	m_hClusterInfo.Purge();

	ClearClusterData();

	for ( int i = 0; i < iSpriteMaterialCount; i++ )
		m_refMaterials[i].Shutdown();
}

void CGrassClusterManager::Update( float frametime )
{
	if ( !gcluster_enable.GetInt() )
		return;

	m_iCurrentMaterial = clamp( gcluster_sprite_index.GetInt(), 0, iSpriteMaterialCount - 1 );

	const bool matValid = GetActiveMaterial() != NULL;

	if ( matValid )
	{
		QAngle wAng( 0, gcluster_grass_wind_angle.GetFloat(), 0 );
		Vector wVec;
		AngleVectors( wAng, &wVec );
		wVec.y *= -1.0f;
		wVec *= gcluster_grass_wind_strength.GetFloat();

		m_refMaterials[ m_iCurrentMaterial ].GetVarDir()->SetVecValue( wVec.Base(), 3 );
		m_refMaterials[ m_iCurrentMaterial ].GetVarAng()->SetFloatValue( DEG2RAD( wAng.y + 225.0f ) );
	}

	if ( m_hClusterInfo.Count() && !m_hClusterData.Count() && matValid )
	{
		GenerateClusterData();
		Assert( m_hClusterInfo.Count() >= m_hClusterData.Count() );
	}

	m_iDrawnQuads = 0;
	m_iDrawnCluster = 0;
	m_flMorphTime = 0;
	UpdateMorphInfo();
}

class EnumFlashlights : public IClientLeafShadowEnum
{
public:
	virtual void EnumShadow( ClientShadowHandle_t clienthandle )
	{
		const ShadowType_t type = g_pClientShadowMgr->GetActualShadowCastType( clienthandle );
		if ( type != SHADOWS_RENDER_TO_DEPTH_TEXTURE )
			return;

		ShadowHandle_t handle = g_pClientShadowMgr->GetShadowHandle( clienthandle );

		shadowList.AddToTail( handle );
	};

	CUtlVector< ShadowHandle_t >shadowList;
};

void CGrassClusterManager::PreRender()
{
}

void CGrassClusterManager::PostRender()
{
	if ( gcluster_debug.GetInt() )
	{
		int numQuads = 0;
		for ( int i = 0; i < m_hClusterData.Count(); i++ )
			numQuads += m_hClusterData[i].iNumQuads;

		engine->Con_NPrintf( 9, "grass cluster debugging:" );
		engine->Con_NPrintf( 10, "quads drawn: %i // clusters drawn: %i", m_iDrawnQuads, m_iDrawnCluster );
		engine->Con_NPrintf( 11, "quads generated: %i // clusters generated: %i", numQuads, m_hClusterData.Count() );

		float efficiency_cur = 100.0f;
		float efficiency_avg = 100.0f;
		if ( m_iDrawnCluster && m_iDrawnPerDrawcall )
			efficiency_cur = m_iDrawnQuads / (float)m_iDrawnCluster / m_iDrawnPerDrawcall * 100.0f;
		if ( numQuads && m_iDrawnPerDrawcall )
			efficiency_avg = numQuads / (float)m_hClusterData.Count() / m_iDrawnPerDrawcall * 100.0f;

		engine->Con_NPrintf( 13, "current efficiency: %3.1f%% // average efficiency: %3.1f%%", efficiency_cur, efficiency_avg );
		engine->Con_NPrintf( 14, "engine max quads: %i // cluster max quads: %i", m_iDrawnEngineMax, m_iDrawnPerDrawcall );

		engine->Con_NPrintf( 16, "morphing took: %3.3f msec", m_flMorphTime );
	}
}

void CGrassClusterManager::RenderClusters( bool bShadowDepth )
{
	const bool bSupportFlashlight = gcluster_enable_flashlight.GetBool();
	const bool bFullDbg = gcluster_debug.GetInt() > 1;

	if ( !gcluster_enable.GetInt() )
		return;

	if ( bShadowDepth && !bSupportFlashlight )
		return;

	if ( view->GetDrawFlags() & DF_DRAWSKYBOX )
		return;

	IMaterial *pMat = GetActiveMaterial();

	if ( !pMat )
		return;

	CUtlVector< Frustum_t > flashlightFrusta;
	EnumFlashlights enumList;

	if ( !bShadowDepth && bSupportFlashlight )
	{
		IWorldRenderList *pWorldRenderList = render->CreateWorldList();
		WorldListInfo_t *pListInfo = new WorldListInfo_t();
		VisOverrideData_t vOverride;
		vOverride.m_vecVisOrigin = CurrentViewOrigin();
		vOverride.m_fDistToAreaPortalTolerance = FLT_MAX;
		render->BuildWorldLists( pWorldRenderList, pListInfo, -1, &vOverride );

		ClientLeafSystem()->EnumerateShadowsInLeaves( pListInfo->m_LeafCount, pListInfo->m_pLeafList, &enumList );
		for ( int i = 0; i < enumList.shadowList.Count(); i++ )
			flashlightFrusta.AddToTail( shadowmgr->GetFlashlightFrustum( enumList.shadowList[i] ) );

		Assert( enumList.shadowList.Count() == flashlightFrusta.Count() );

		SafeRelease( pWorldRenderList );
		delete pListInfo;
	}

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( pMat );

	const float flCullDist = gcluster_cullDist.GetFloat();
	const float flCullDistSqr = flCullDist * flCullDist;

	for ( int i = 0; i < m_hClusterData.Count(); i++ )
	{
		Assert( m_hClusterData[ i ].pGrassMesh );

		Vector pos = m_hClusterData[i].pos;
		//DebugDrawLine( pos, pos + Vector( 0, 0, 150 ), 255, 0, 0, false, 0.1f );

		const float distSqr = ( CurrentViewOrigin() - m_hClusterData[ i ].pos ).LengthSqr();

		if ( distSqr > flCullDistSqr )
			continue;

		if ( !engine->IsBoxInViewCluster( m_hClusterData[ i ].extents_min, m_hClusterData[ i ].extents_max ) )
			continue;

		if ( engine->CullBox( m_hClusterData[ i ].extents_min, m_hClusterData[ i ].extents_max ) )
			continue;

		//m_hClusterData[ i ].pGrassMesh->Draw();
		//m_iDrawnQuads += m_hClusterData[ i ].iNumQuads;
		m_iDrawnQuads += m_hClusterData[ i ].Draw();
		m_iDrawnCluster++;

		if ( bFullDbg )
			debugoverlay->AddBoxOverlay( m_hClusterData[ i ].extents_min, vec3_origin, m_hClusterData[ i ].extents_max - m_hClusterData[ i ].extents_min, vec3_angle, 0, 255, 0, 20, -1 );
	}

	if ( !bShadowDepth && bSupportFlashlight && flashlightFrusta.Count() )
	{
		Assert( g_pClientShadowMgr->GetNumShadowDepthtextures() >= flashlightFrusta.Count() );
		pRenderContext->SetFlashlightMode( true );

		for ( int i = 0; i < flashlightFrusta.Count(); i++ )
		{
			const FlashlightState_t &flState = shadowmgr->GetFlashlightState( enumList.shadowList[i] );
			ITexture *pTex = g_pClientShadowMgr->GetShadowDepthTex( i );

			VMatrix a,b,c,d;
			CViewSetup shadowView;
			shadowView.m_flAspectRatio = 1.0f;
			shadowView.x = shadowView.y = 0;
			shadowView.width = pTex->GetActualWidth();
			shadowView.height = pTex->GetActualHeight();
			shadowView.m_bOrtho = false;
			shadowView.origin = flState.m_vecLightOrigin;
			QuaternionAngles( flState.m_quatOrientation, shadowView.angles );
			shadowView.fov = flState.m_fHorizontalFOVDegrees;
			shadowView.zFar = shadowView.zFarViewmodel = flState.m_FarZ;
			shadowView.zNear = shadowView.zNearViewmodel = flState.m_NearZ;
			render->GetMatricesForView( shadowView, &a, &b, &c, &d );

			VMatrix shadowToUnit;
			MatrixBuildScale( shadowToUnit, 1.0f / 2, 1.0f / -2, 1.0f );
			shadowToUnit[0][3] = shadowToUnit[1][3] = 0.5f;
			MatrixMultiply( shadowToUnit, c, d );

			pRenderContext->SetFlashlightStateEx( flState, d, pTex );

			for ( int c = 0; c < m_hClusterData.Count(); c++ )
			{
				if ( R_CullBox( m_hClusterData[c].extents_min, m_hClusterData[c].extents_max, flashlightFrusta[i] ) )
					continue;

				//m_hClusterData[ c ].pGrassMesh->Draw();
				//m_iDrawnQuads += m_hClusterData[ c ].iNumQuads;
				m_iDrawnQuads += m_hClusterData[ c ].Draw();
				m_iDrawnCluster++;
			}
		}

		pRenderContext->SetFlashlightMode( false );
	}

	pRenderContext->Flush();
}

void CGrassClusterManager::UpdateMorphInfo()
{
	if ( gcluster_enable_morph.GetInt() == 0 )
	{
		for ( int i = 0; i < m_hClusterData.Count(); i++ )
		{
			for ( int m = 0; m <  m_hClusterData[i].pPressureInfo->iNumGrassObjects; m++ )
			{
				if ( m_hClusterData[i].pPressureInfo->flAmt[m] != 0 )
				{
					m_hClusterData[i].pPressureInfo->flAmt[m] = 0.0f;
					m_hClusterData[i].pPressureInfo->bDirty[m] = true;
				}
			}
		}

		for ( int i = 0; i < m_hClusterData.Count(); i++ )
			InjectMorph( i );
		gcluster_enable_morph.SetValue( -1 );
	}

	if ( gcluster_enable_morph.GetInt() <= 0 )
		return;

	const bool bDebugging = gcluster_debug.GetBool();

	CFastTimer timer;
	if ( bDebugging )
		timer.Start();

	for ( int i = 0; i < m_hClusterData.Count(); i++ )
	{
		for ( int m = 0; m <  m_hClusterData[i].pPressureInfo->iNumGrassObjects; m++ )
		{
			if ( m_hClusterData[i].pPressureInfo->flAmt[m] != 0 &&
				m_hClusterData[i].pPressureInfo->flLastMoveTime[m] < gpGlobals->curtime &&
				m_hClusterData[i].pPressureInfo->flLastUpdateTime[m] < gpGlobals->curtime )
			{
				m_hClusterData[i].pPressureInfo->flAmt[m] = Approach( 0,
					m_hClusterData[i].pPressureInfo->flAmt[m],
					gpGlobals->frametime * gcluster_grass_morph_speed.GetFloat() );
				m_hClusterData[i].pPressureInfo->bDirty[m] = true;
				m_hClusterData[i].pPressureInfo->flLastUpdateTime[m] = gpGlobals->curtime + gcluster_grass_morph_framelag.GetFloat();
			}
		}
	}

	float flGrassColScale = ( gcluster_grass_width_med_min.GetFloat() +
		( gcluster_grass_width_med_max.GetFloat() - gcluster_grass_width_med_min.GetFloat() ) * 0.5f ) / 50.0f;
	flGrassColScale = clamp( flGrassColScale, 0.3f, 1.6f );

	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
	for ( C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity( pEnt ) )
	{
		if ( !pEnt || !pEnt->IsVisible() && pEnt != pLocal )
			continue;

		const bool bPlayer = pEnt->IsPlayer();

		if ( bPlayer )
		{
			C_BasePlayer *player = (C_BasePlayer*)pEnt;
			if ( player->m_Local.m_bDucked )
				continue;
		}
		else if ( dynamic_cast< CBaseViewModel* >( pEnt ) != NULL )
			continue;

		CCollisionProperty *pCProp = pEnt->CollisionProp();

		if ( !pCProp )
			continue;

		//const bool bIsPhys = pEnt->GetMoveType() == MOVETYPE_VPHYSICS;
		Vector vPos = bPlayer ? pEnt->GetLocalOrigin() : pEnt->WorldSpaceCenter();

		Vector colMin = pCProp->OBBMins();
		Vector colMax = pCProp->OBBMaxs();
		colMax-=colMin;
		if ( !bPlayer )
			vPos.z -= colMax.z * 0.5f;
		colMax.z = 0;

		Vector vVel = pEnt->GetLocalVelocity();

		if ( vVel.IsZero() )
			pEnt->EstimateAbsVelocity( vVel );

		if ( vVel.IsZero() && pEnt->GetMoveType() == MOVETYPE_VPHYSICS && pEnt->VPhysicsGetObject() )
			pEnt->VPhysicsGetObject()->GetVelocity( &vVel, NULL );

		IPhysicsObject *dummyList[2];
		if ( pEnt->VPhysicsGetObjectList( dummyList, 2 ) > 1 ||
			pEnt->GetClientVehicle() != NULL )
			colMax *= 0.25f;

		const float flBoundsSqr = max( 1500, colMax.LengthSqr() * flGrassColScale );
		const float flLenSqr = vVel.LengthSqr();

		if ( flLenSqr < 10 )
			continue;

		vVel *= 1.0f / FastSqrt( flLenSqr );

		for ( int i = 0; i < m_hClusterData.Count(); i++ )
		{
			const _grassClusterData &data = m_hClusterData[i];
			if ( !IsPointInBox( vPos, data.extents_min - Vector(CONSTBOXEXTENT_COL,CONSTBOXEXTENT_COL,CONSTBOXEXTENT_COL),
				data.extents_max + Vector(CONSTBOXEXTENT_COL,CONSTBOXEXTENT_COL,CONSTBOXEXTENT_COL) ) )
				continue;

			_grassPressureData *morphData = data.pPressureInfo;
			Vector delta;

			for ( int o = 0; o < morphData->iNumGrassObjects; o++ )
			{
				delta = morphData->vecPos[o] - vPos;
				if ( delta.LengthSqr() > flBoundsSqr )
					continue;

				if ( morphData->flAmt[o] < 0.5f )
					Q_memcpy( morphData->vecDir[o].Base(), vVel.Base(), sizeof(Vector) );

				morphData->flAmt[o] = 1.0f; //morphData->flHeight[o];
				morphData->bDirty[o] = true;
				morphData->flLastMoveTime[o] = gpGlobals->curtime + gcluster_grass_morph_delay.GetFloat();

				//DebugDrawLine( morphData->vecPos[o], morphData->vecPos[o] + Vector( 0, 0, 100 ), 255, 0, 0, false, 0.1f );
			}
		}
	}

	for ( int i = 0; i < m_hClusterData.Count(); i++ )
		InjectMorph( i );

	if ( bDebugging )
	{
		timer.End();
		m_flMorphTime += timer.GetDuration().GetMillisecondsF();
	}
}
void CGrassClusterManager::InjectMorph( int i )
{
	Assert( i >= 0 && i < m_hClusterData.Count() );

	_grassClusterData *data = &m_hClusterData[ i ];
	_grassPressureData *morphInfo = data->pPressureInfo;

	Assert( data->pGrassMesh );
	Assert( morphInfo && morphInfo->iNumGrassObjects > 0 );

	CMeshBuilder pMeshBuilder;

	int grassObject, iPlane;
	int NumVerticesPerGrassObject = 4 * 3;

	for ( grassObject = 0; grassObject < morphInfo->iNumGrassObjects; grassObject++ )
	{
		if ( !morphInfo->bDirty[grassObject] )
			continue;

		int continuousObjects = 1;
		for ( int x = grassObject+1; x < morphInfo->iNumGrassObjects; x++ )
		{
			if ( !morphInfo->bDirty[x] )
				break;
			continuousObjects++;
		}

		pMeshBuilder.BeginModify( data->pGrassMesh,
			grassObject * NumVerticesPerGrassObject,
			NumVerticesPerGrassObject * continuousObjects );

		for ( int x = 0; x < continuousObjects; x++ )
		{
			const int curIndex = grassObject + x;
			const float &amt = morphInfo->flAmt[ curIndex ];
			const float amt_scaled = amt * morphInfo->flHeight[ curIndex ];
			const Vector &dir = morphInfo->vecDir[ curIndex ];

			morphInfo->bDirty[ curIndex ] = false;

			for ( iPlane = 0; iPlane < 3; iPlane++ )
			{
					pMeshBuilder.TexCoord3f( 1, 1, amt, amt_scaled );
					pMeshBuilder.TexCoord3f( 2, dir.x, dir.y, dir.z );
					pMeshBuilder.AdvanceVertex();
					pMeshBuilder.AdvanceVertex();
					pMeshBuilder.AdvanceVertex();
					pMeshBuilder.TexCoord3f( 1, 1, amt, amt_scaled );
					pMeshBuilder.TexCoord3f( 2, dir.x, dir.y, dir.z );
					pMeshBuilder.AdvanceVertex();
			}
		}

		pMeshBuilder.EndModify();

		grassObject += continuousObjects - 1;
	}
}

void CGrassClusterManager::AddClusterHint( _grassClusterInfo hint )
{
	m_hClusterInfo.AddToTail( hint );
}

void CGrassClusterManager::ClearClusterData()
{
	for ( int i = 0; i < m_hClusterData.Count(); i++ )
		m_hClusterData[i].Destroy();
	m_hClusterData.Purge();
}

void CGrassClusterManager::GenerateClusterData()
{
	m_iCurObjectsPerHint = gcluster_objectsPerHint.GetInt();

	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMeshDummy = pRenderContext->GetDynamicMesh( true, NULL, NULL, GetActiveMaterial() );
	int nMaxVerts, nMaxIndices;
	pRenderContext->GetMaxToRender( pMeshDummy, false, &nMaxVerts, &nMaxIndices );
	pMeshDummy->Draw();

	m_iDrawnEngineMax = nMaxIndices / 6;
	if ( m_iDrawnEngineMax > nMaxVerts / 4 )
		m_iDrawnEngineMax = nMaxVerts / 4;
	int maxQuads = m_iDrawnEngineMax;

	if ( gcluster_clusterMaxQuads.GetInt() > 0 )
		maxQuads = min( maxQuads, gcluster_clusterMaxQuads.GetInt() );

	int maxClusterHints = maxQuads / ( 3 * m_iCurObjectsPerHint );

	m_iDrawnPerDrawcall = maxClusterHints * 3 * m_iCurObjectsPerHint;

	if ( !maxClusterHints )
		return;

	CUtlVector< _grassClusterInfo >hClusterInfoLocal;
	hClusterInfoLocal.AddVectorToTail( m_hClusterInfo );

	CUtlVector< _grassClusterInfo >hClusterInfoSorted;

	//float step = 0;
	//for ( int i = 0; i < m_hClusterInfo.Count(); i += skipAmt )
	//const float skipDistSqr = 2500.0f * 2500.0f;
	const float skipDistSqr = 1500.0f * 1500.0f;

	while ( hClusterInfoLocal.Count() )
	{
		const Vector ref = hClusterInfoLocal[0].orig;
		hClusterInfoLocal[0].flSortDist = 0;

		for ( int i = 1; i < hClusterInfoLocal.Count(); i++ )
			hClusterInfoLocal[i].flSortDist = ( hClusterInfoLocal[i].orig - ref ).LengthSqr();

		hClusterInfoLocal.Sort( GrassInfoSort );

		hClusterInfoSorted.Purge();
		int count = min( maxClusterHints, hClusterInfoLocal.Count() );
		for ( int i = 0; i < count; i++ )
		{
			hClusterInfoSorted.AddToTail( hClusterInfoLocal[i] );
		}
		hClusterInfoLocal.RemoveMultiple( 0, count );

		bool bFinished = hClusterInfoLocal.Count() == 0;

		if ( hClusterInfoSorted.Count() < 5 )
			continue;

		for ( int i = 0; i < hClusterInfoSorted.Count(); i++ )
		{
			//const float dist = ( hClusterInfoSorted[i].orig - ref ).LengthSqr();
			//if ( dist > skipDistSqr )
			if ( hClusterInfoSorted[i].flSortDist > skipDistSqr )
			{
				//DebugDrawLine( hClusterInfoSorted[i].orig, hClusterInfoSorted[i].orig + Vector( 0, 0, 300 ), 255, 0, 0, false, 1 );
				hClusterInfoLocal.AddToHead( hClusterInfoSorted[i] );
				hClusterInfoSorted.Remove( i );
				i--;
			}
		}

		count = hClusterInfoSorted.Count();

		if ( !hClusterInfoSorted.Count() )
		{
			if ( bFinished )
				break;
			//Assert(0);
			continue;
		}

		Vector avgPos( vec3_origin );
		for ( int i = 0; i < count; i++ )
			avgPos += hClusterInfoSorted[i].orig;
		avgPos /= (float)count;

		//int r = ( step * 10 - floor( step * 10 ) ) * 255;
		//int g = ( step * 13 - floor( step * 13 ) ) * 255;
		//int b = ( step * 19 - floor( step * 19 ) ) * 255;

		//for ( int i = 0; i < hClusterInfoSorted.Count(); i++ )
		//	DebugDrawLine( hClusterInfoSorted[i].orig, hClusterInfoSorted[i].orig + Vector( 0, 0, 100 ), r, g, b, false, 1 );

		//DebugDrawLine( avgPos, avgPos + Vector( 0, 0, 300 ), 255, 0, 0, false, 1 );

		int numGrassObjects = hClusterInfoSorted.Count() * m_iCurObjectsPerHint;

		_grassPressureData *morphData = new _grassPressureData();
		morphData->Init( numGrassObjects );

		_grassClusterData data;

		BuildClusterMesh( data, hClusterInfoSorted, m_iCurObjectsPerHint, morphData );

		if ( gcluster_LOD_enable.GetInt() )
		{
			_grassClusterData *dataLOD = new _grassClusterData();
			BuildClusterMesh( *dataLOD, hClusterInfoSorted, min( m_iCurObjectsPerHint, gcluster_LOD_objects_per_hint.GetInt() ) );
			dataLOD->pos = avgPos;
			data.iNextLodThreshold = gcluster_LOD_transitionDist.GetInt() * gcluster_LOD_transitionDist.GetInt();
			data.pLOD = dataLOD;
		}

		Assert( data.pGrassMesh );
		data.pos = avgPos;
		data.pPressureInfo = morphData;

		m_hClusterData.AddToTail( data );

		//step+=0.1f;
	}

	hClusterInfoLocal.Purge();
	hClusterInfoSorted.Purge();

	r_DrawDetailProps.SetValue( "0" );
}

void CGrassClusterManager::BuildClusterMesh( _grassClusterData &data, const CUtlVector< _grassClusterInfo > &hints,
	int iObjectMultiplier, _grassPressureData *pMorphInfo )
{
	const int hintCount = hints.Count();
	const int numObjectsPerHint = iObjectMultiplier; //m_iCurObjectsPerHint;
	const int numQuads = numObjectsPerHint * 3 * hintCount;

	Assert( hintCount && numQuads && numObjectsPerHint );

	float flAverageMinDist = 0;
	for ( int i = 0; i < hintCount; i++ )
	{
		const Vector &ref = hints[i].orig;

		CUtlVector< _grassClusterInfo >hClusterInfoSortedSub;
		hClusterInfoSortedSub.AddVectorToTail( hints );

		for ( int s = 0; s < hintCount; s++ )
		{
			if ( s == i )
				continue;

			hClusterInfoSortedSub[s].flSortDist = ( hClusterInfoSortedSub[s].orig - ref ).LengthSqr();
		}

		hClusterInfoSortedSub.Sort( GrassInfoSort );

		for ( int s = 0; s < hintCount; s++ )
		{
			if ( s == i )
				continue;
			flAverageMinDist += hClusterInfoSortedSub[s].flSortDist;
			break;
		}

		hClusterInfoSortedSub.Purge();
	}
	flAverageMinDist = FastSqrt( flAverageMinDist/(float)hintCount );

	data.iNumQuads = numQuads;
	data.extents_min = hints[0].orig;
	data.extents_max = hints[0].orig;
	for ( int i = 1; i < hintCount; i++ )
		for ( int v = 0; v < 3; v++ )
			data.extents_min[v] = min( data.extents_min[v], hints[i].orig[v] );
	for ( int i = 1; i < hintCount; i++ )
		for ( int v = 0; v < 3; v++ )
			data.extents_max[v] = max( data.extents_max[v], hints[i].orig[v] );
	data.extents_max.z += 60;
	data.extents_min -= Vector( 40, 40, 40 );
	data.extents_max += Vector( 40, 40, 40 );

	CMatRenderContextPtr pRenderContext( materials );
	CMeshBuilder pMeshBuilder;

	VertexFormat_t format = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_COLOR |
		VERTEX_FORMAT_VERTEX_SHADER | VERTEX_TEXCOORD_SIZE( 0, 2 ) | VERTEX_TEXCOORD_SIZE( 1, 3 ) | VERTEX_TEXCOORD_SIZE( 2, 3 );
	IMesh *pMesh = pRenderContext->CreateStaticMesh( format, TEXTURE_GROUP_OTHER, GetActiveMaterial() );
	Assert( pMesh );

	pMeshBuilder.Begin( pMesh, MATERIAL_QUADS, numQuads );

	//data.CreateLightingPatch( hints );
	data.CreateLightingPatch( m_hClusterInfo );

	for ( int i = 0; i < hintCount; i++ )
		for ( int m = 0; m < numObjectsPerHint; m++ )
			BuildSingleGrassObject( pMeshBuilder, data, hints[i], flAverageMinDist, i * numObjectsPerHint + m, pMorphInfo );

	data.DestroyLightingPatch();

	pMeshBuilder.End();

	data.pGrassMesh = pMesh;
}

void CGrassClusterManager::BuildSingleGrassObject( CMeshBuilder &builder, _grassClusterData &clusterData, const _grassClusterInfo &hint,
	const float avgDist,
	const int grassObjectIndex, _grassPressureData *pMorphInfo )
{
	//bool bStarShape = !!RandomInt( 0, 1 );
	const _grassClusterInfo &uvData = m_hClusterInfo[ RandomInt( 0, m_hClusterInfo.Count() - 1 ) ];
	const Vector2D un_min = uvData.uv_min;
	const Vector2D un_max = uvData.uv_max;
	//engine->GetLightForPoint
	trace_t tr;
	//UTIL_TraceLine( hint.orig + Vector( 0, 0, 10 ), hint.orig - Vector( 0, 0, 10 ), MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &tr );
	CTraceFilterWorldOnly filter;
	UTIL_TraceLine( hint.orig + Vector( 0, 0, 10 ), hint.orig - Vector( 0, 0, 10 ), MASK_ALL, &filter, &tr );
	Assert( tr.DidHit() );

	Vector normal = tr.DidHitWorld() ? tr.plane.normal : Vector( 0, 0, 1 );
	Vector orig = hint.orig; //tr.endpos;
	if ( ( !tr.DidHit() && !tr.allsolid) || normal.IsZero() )
	{
		normal.Init( 0, 0, 1 );
		orig = hint.orig;

		//Assert( 0 );
	}
	Vector right, up;

	const float ranExp = gcluster_grass_terrain_offset_exp.GetFloat();

	const float min_Dist = gcluster_grass_terrain_offset_min.GetFloat();
	const float max_Dist = max( min_Dist, avgDist ) * gcluster_grass_terrain_offset_multi.GetFloat();
	const float maxAng = cos( DEG2RAD( 60.0f ) );

	int attempts = 0;
	const int maxAttempts = 24;

	for ( attempts = 0; attempts < maxAttempts; attempts++ )
	{
		QAngle orient;
		VectorAngles( normal, orient );
		orient.z += RandomFloat( 0, 360.0f );
		AngleVectors( orient, NULL, &right, &up );
		//VectorVectors( normal, right, up );
		right *= RandomFloatExp( min_Dist, max_Dist, ranExp ) * (RandomInt(0,1)?-1:1);
		up *= RandomFloatExp( min_Dist, max_Dist, ranExp ) * (RandomInt(0,1)?-1:1);

		Vector tr_start = hint.orig + normal, tr_end = hint.orig + normal * 50;
		UTIL_TraceLine( tr_start, tr_end, MASK_ALL, &filter, &tr );
		tr_end = tr.endpos + right + up;
		UTIL_TraceLine( tr.endpos, tr_end, MASK_ALL, &filter, &tr );
		tr_end = tr.endpos - normal * 100.0f;
		UTIL_TraceLine( tr.endpos, tr_end, MASK_ALL, &filter, &tr );

		if ( !tr.DidHit() )
			continue;

		const float dot = DotProduct( normal, tr.plane.normal );
		if ( dot < maxAng )
			continue;

		orig = tr.endpos;
		normal = tr.plane.normal;
		break;
	}

	Assert( attempts < maxAttempts );

	const bool bSmall = !RandomInt( 0, gcluster_grass_type_small_oddness.GetInt() );
	const bool bHuge = !RandomInt( 0, gcluster_grass_type_huge_oddness.GetInt() );
	const float flSize_up = bHuge ?		RandomFloat( gcluster_grass_height_huge_min.GetFloat(), gcluster_grass_height_huge_max.GetFloat() )	:	
		bSmall ?	RandomFloat( gcluster_grass_height_small_min.GetFloat(), gcluster_grass_height_small_max.GetFloat() ) : 
		RandomFloat( gcluster_grass_height_med_min.GetFloat(), gcluster_grass_height_med_max.GetFloat() );

	const float flSize_side = bHuge ?		RandomFloat( gcluster_grass_width_huge_min.GetFloat(), gcluster_grass_width_huge_max.GetFloat() )	:	
		bSmall ?	RandomFloat( gcluster_grass_width_small_min.GetFloat(), gcluster_grass_width_small_max.GetFloat() ) : 
		RandomFloat( gcluster_grass_width_med_min.GetFloat(), gcluster_grass_width_med_max.GetFloat() );

	if ( pMorphInfo != NULL )
	{
		pMorphInfo->vecPos[grassObjectIndex] = orig;
		pMorphInfo->flHeight[grassObjectIndex] = flSize_up;
	}

	Vector orig_top = orig + normal * flSize_up;

	QAngle orientation;
	VectorAngles( normal, orientation );

	orientation.z += RandomFloat( 0, 60 );

	Vector planePos[4];
	Vector2D uvs[4];

	//Vector4D AverageColor( hint.color );
	//Vector col, diff;

	//if ( tr.DidHitWorld() && tr.hitbox == 0 && engine->TraceLineMaterialAndLighting( orig, normal * 1.1f, col, diff ) )
	//if ( tr.DidHitWorld() && tr.hitbox == 0 )
	{
		//col = FetchLightSamples( orig ) * 1.4f; //engine->GetLightForPoint( orig, false );
		//for ( int i = 0; i < 3; i++ )
		//	col[i] = min( col[i], hint.color[i] );
		//col = ( col + hint.color.AsVector3D() * 1.5f ) * 0.5f;
		//col = clusterData.GetLightingForPoint( orig );
		//AverageColor.Init( col.x, col.y, col.z, 1 );
	}

	float meadowAccum = RandomFloat( 2, 3 );
	meadowAccum += sin( orig[0] * 0.005f ) * 0.5f + 0.5f;
	meadowAccum += cos( DotProduct(Vector(0.707f,0.707f,0), orig) * 0.007f ) * 0.5f + 0.5f;
	meadowAccum += sin( DotProduct(Vector(-0.638224f, -0.304417f, -0.707107), orig) * 0.0055f ) * 0.5f + 0.5f;
	meadowAccum += cos( orig[0] * 0.006f ) * 0.5f + 0.5f;
	meadowAccum = Bias( meadowAccum / 7.0f, 0.4f );

	meadowAccum = Lerp( gcluster_grass_meadow_scale.GetFloat(), 1.0f, meadowAccum );

	//for ( int i = 0; i < 3; i++ )
	//{
	//	AverageColor[i] *= meadowAccum;
	//	AverageColor[i] = clamp( AverageColor[i], 0, 1 );
	//}

	Vector4D vPosInfo( 1, 0, 0, 1 );

	//Vector col = clusterData.GetLightingForPoint( orig );
	//for ( int v = 0; v < 3; v++ )
	//	col[v] = clamp( col[v], 0, 1 );

	for ( int i = 0; i < 3; i++ )
	{
		AngleVectors( orientation, NULL, &right, &up );

		planePos[0] = orig_top + up * flSize_side;
		planePos[1] = orig + up * flSize_side;
		planePos[2] = orig - up * flSize_side;
		planePos[3] = orig_top - up * flSize_side;

		for ( int p = 0; p < 4; p++ )
		{
			float len = flSize_up * 0.2f * (i==1 ? -1 : 1);
			//float len = flSize_side * 0.15f * (i==1 ? -1 : 1);
			if ( p < 1 || p > 2 )
				len *= 3.0f;
			planePos[p] += right * len;
		}

		Vector colors[4];
		for ( int c = 0; c < 4; c++ )
		{
			colors[c] = clusterData.GetLightingForPoint( planePos[c] );
			for ( int v = 0; v < 3; v++ )
				colors[c][v] = clamp( colors[c][v], 0, 1 );
			colors[c] *= meadowAccum;
		}


		//uvs[0].Init( un_min.x, un_max.y );
		//uvs[1].Init( un_min.x, un_min.y );
		//uvs[2].Init( un_max.x, un_min.y );
		//uvs[3].Init( un_max.x, un_max.y );

		if ( !RandomInt( 0, 20 ) )
		{
			uvs[0].Init( 0, 0.0f );
			uvs[1].Init( 0, 0.5f );
			uvs[2].Init( 1, 0.5f );
			uvs[3].Init( 1, 0.0f );
		}
		else
		{
			uvs[0].Init( 0, 0.51f + (0.5f/128.0f) );
			uvs[1].Init( 0, 1 );
			uvs[2].Init( 1, 1 );
			uvs[3].Init( 1, 0.51f + (0.5f/128.0f) );
		}

		//uvs[0].Init( 0, 1 );
		//uvs[1].Init( 0, 0 );
		//uvs[2].Init( 1, 0 );
		//uvs[3].Init( 1, 1 );
		
		for ( int t = 0; t < 4; t++ )
		{
			builder.Position3fv( planePos[t].Base() );
			//builder.Color4fv( AverageColor.Base() );
			builder.Color4f( colors[t][0], colors[t][1], colors[t][2], 1 );
			//builder.Color4f( col.x, col.y, col.z, 1 );
			builder.TexCoord2f( 0, uvs[t][0], uvs[t][1] );
			builder.TexCoord3f( 1, vPosInfo[t], 0, 0 );
			builder.TexCoord3f( 2, 0, 0, 0 );
			builder.Normal3fv( normal.Base() );
			builder.AdvanceVertex();
		}

		orientation.z += 60.0f;
	}
}