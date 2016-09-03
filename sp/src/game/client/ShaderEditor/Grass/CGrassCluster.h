#ifndef GRASS_CLUSTER_H
#define GRASS_CLUSTER_H

#include "cbase.h"

class CFastTimer;

struct _grassPressureData
{
	_grassPressureData();
	~_grassPressureData();

	void Init( int num );

	int iNumGrassObjects;

	Vector *vecPos;
	Vector *vecDir;
	float *flAmt;
	float *flHeight;
	float *flLastMoveTime;
	float *flLastUpdateTime;
	bool *bDirty;
};

struct _grassClusterInfo
{
public:
	_grassClusterInfo();

	Vector orig;
	Vector4D color;
	Vector2D uv_min, uv_max;
	float flSortDist;
};

struct _grassClusterData
{
public:
	_grassClusterData();
	~_grassClusterData();
	// flat copy on purpose!!!

	void Destroy();
	int Draw();

	IMesh *pGrassMesh;
	_grassPressureData *pPressureInfo;
	Vector pos;

	int iNumQuads;
	Vector extents_min, extents_max;

	int iNextLodThreshold;
	_grassClusterData *pLOD;


	void CreateLightingPatch( const CUtlVector< _grassClusterInfo > &hints );
	const Vector GetLightingForPoint( const Vector &pos );
	void DestroyLightingPatch();
	int iLPatchSize_x, iLPatchSize_y;
	float flLPatchStep_x, flLPatchStep_y;
	Vector *lighting;
};

struct clusterMaterial
{
	clusterMaterial();
	void Init( const char *pszMat );
	void Shutdown();
	bool IsValid();

	IMaterialVar *GetVarDir();
	IMaterialVar *GetVarAng();
	IMaterial *GetMaterial();

	CMaterialReference mat;
	unsigned int ivar_dir;
	unsigned int ivar_ang;
};

class CGrassClusterManager : public CAutoGameSystemPerFrame
{
public:

	CGrassClusterManager();
	~CGrassClusterManager();

	static CGrassClusterManager *GetInstance();

	bool Init();
	void Shutdown();
	void LevelInitPostEntity();
	void LevelShutdownPostEntity();

	void Update( float frametime );
	void PreRender();
	void PostRender();
	void RenderClusters( bool bShadowDepth );

	void AddClusterHint( _grassClusterInfo hint );
	void ClearClusterData();

private:

	void GenerateClusterData();
	void BuildClusterMesh( _grassClusterData &data, const CUtlVector< _grassClusterInfo > &hints, int iObjectMultiplier, _grassPressureData *pMorphInfo = NULL );
	void BuildSingleGrassObject( CMeshBuilder &builder, _grassClusterData &clusterData, const _grassClusterInfo &hint,
		const float avgDist, const int grassObjectIndex, _grassPressureData *pMorphInfo = NULL );

	void UpdateMorphInfo();
	void InjectMorph( int i );

	CUtlVector< _grassClusterInfo >m_hClusterInfo;
	CUtlVector< _grassClusterData >m_hClusterData;
	
	IMaterial *GetActiveMaterial();
	//CMaterialReference *m_refMaterial;
	clusterMaterial *m_refMaterials;
	int m_iCurrentMaterial;

	int m_iCurObjectsPerHint;
	int m_iDrawnQuads;
	int m_iDrawnCluster;
	int m_iDrawnPerDrawcall;
	int m_iDrawnEngineMax;

	double m_flMorphTime;
};


#endif