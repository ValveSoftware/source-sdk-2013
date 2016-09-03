// ******************************************************
//
// Purpose:
//		-	Handles model rendering requests from the
//			shader editor library
// 
// ******************************************************

#include "cbase.h"

#include "vgui/iinput.h"
#include "vgui_controls/controls.h"

#include "ShaderEditor/SEdit_ModelRender.h"
#include "model_types.h"

#ifndef SOURCE_2006
#include "viewpostprocess.h"
#endif

#include "view.h"
#include "input.h"

#include "beamdraw.h"

#ifdef SOURCE_2006
void ScreenToWorld( int mousex, int mousey, float fov,
					const Vector& vecRenderOrigin,
					const QAngle& vecRenderAngles,
					Vector& vecPickingRay )
{
	float dx, dy;
	float c_x, c_y;
	float dist;
	Vector vpn, vup, vright;

	float scaled_fov = ScaleFOVByWidthRatio( fov, engine->GetScreenAspectRatio() * 0.75f );

	c_x = ScreenWidth() / 2;
	c_y = ScreenHeight() / 2;

	dx = (float)mousex - c_x;
	dy = c_y - (float)mousey;

	float dist_denom = tan(M_PI * scaled_fov / 360.0f); 
	dist = c_x / dist_denom;
	AngleVectors( vecRenderAngles, &vpn, &vright, &vup );
	vecPickingRay = vpn * dist + vright * ( dx ) + vup * ( dy );
	VectorNormalize( vecPickingRay );
}
#else
extern void ScreenToWorld( int mousex, int mousey, float fov,
					const Vector& vecRenderOrigin,
					const QAngle& vecRenderAngles,
					Vector& vecPickingRay );
#endif

SEditModelRender __g_ShaderEditorMReder( "ShEditMRender" );
SEditModelRender *sEditMRender = &__g_ShaderEditorMReder;

SEditModelRender::SEditModelRender( char const *name ) : CAutoGameSystemPerFrame( name )
{
	pModelInstance = NULL;
	m_iNumPoseParams = 0;
	DestroyModel();
}
SEditModelRender::~SEditModelRender()
{
	DestroyModel();
}

bool SEditModelRender::Init()
{
	return true;
}
void SEditModelRender::Shutdown()
{
}
void SEditModelRender::Update( float frametime )
{
	if ( !IsModelReady() )
		return;

	pModelInstance->StudioFrameAdvance();
	if ( pModelInstance->GetCycle() >= 1.0f )
		pModelInstance->SetCycle( pModelInstance->GetCycle() - 1.0f );
}
void SEditModelRender::LevelInitPostEntity()
{
	ResetModel();
}
void SEditModelRender::LevelShutdownPostEntity()
{
	ResetModel();
}
void SEditModelRender::ResetModel()
{
	if ( !IsModelReady() )
		return;
	pModelInstance->m_flAnimTime = gpGlobals->curtime;
	pModelInstance->m_flOldAnimTime = gpGlobals->curtime;
}
bool SEditModelRender::IsModelReady()
{
	if ( !pModelInstance )
		return false;

	bool bValid = !!pModelInstance->GetModel();

	if ( bValid && Q_strlen( m_szModelPath ) )
	{
		const model_t *pMdl = modelinfo ? modelinfo->FindOrLoadModel( m_szModelPath ) : NULL;
		if ( pMdl )
			pModelInstance->SetModelPointer( pMdl );
		bValid = !!pMdl;
	}

	if ( !bValid )
		DestroyModel();

	return bValid;
}
bool SEditModelRender::LoadModel( const char *localPath )
{
	DestroyModel();

	const model_t *mdl = modelinfo->FindOrLoadModel( localPath );
	if ( !mdl )
		return false;

	Q_strcpy( m_szModelPath, localPath );

	C_BaseFlex *pEnt = new C_BaseFlex();
	pEnt->InitializeAsClientEntity( NULL,
#if SWARM_DLL
		false
#else
		RENDER_GROUP_OPAQUE_ENTITY
#endif
		);
	MDLCACHE_CRITICAL_SECTION();
	pEnt->SetModelPointer( mdl );
	pEnt->Spawn();

	pEnt->SetAbsAngles( vec3_angle );
	pEnt->SetAbsOrigin( vec3_origin );
	
	pEnt->AddEffects( EF_NODRAW | EF_NOINTERP );
	pEnt->m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;

	// leave it alone.
	pEnt->RemoveFromLeafSystem();
	cl_entitylist->RemoveEntity( pEnt->GetRefEHandle() );
	pEnt->CollisionProp()->DestroyPartitionHandle();

	CStudioHdr *pHdr = pEnt->GetModelPtr();
	m_iNumPoseParams = pHdr ? pHdr->GetNumPoseParameters() : 0;

	pModelInstance = pEnt;
	return true;
}
void SEditModelRender::DestroyModel()
{
	if ( pModelInstance )
		pModelInstance->Remove();

	pModelInstance = NULL;
	m_szModelPath[0] = '\0';
	m_iNumPoseParams = 0;
}
void SEditModelRender::GetModelCenter( float *pFl3_ViewOffset )
{
	Q_memset( pFl3_ViewOffset, 0, sizeof(float) * 3 );
	if ( IsModelReady() )
	{
		MDLCACHE_CRITICAL_SECTION();
		if ( pModelInstance->GetModelPtr() )
		{
			const Vector &vecMin = pModelInstance->GetModelPtr()->hull_min();
			const Vector &vecMax = pModelInstance->GetModelPtr()->hull_max();
			Vector vecPos = ( vecMin + ( vecMax - vecMin ) * 0.5f );
			if ( pFl3_ViewOffset )
				Q_memcpy( pFl3_ViewOffset, vecPos.Base(), sizeof(float) * 3 );
		}
	}
}
void SEditModelRender::DestroyCharPtrList( char ***szList )
{
	Assert( szList );
	if ( *szList )
	{
		delete [] (**szList);
		delete [] (*szList);
		*szList = NULL;
	}
}

int SequenceSort( mstudioseqdesc_t *const *seq1, mstudioseqdesc_t *const *seq2 )
{
	return Q_stricmp( ( *seq1 )->pszLabel(), ( *seq2 )->pszLabel() );
}
int SEditModelRender::QuerySequences( char ***list )
{
	if ( !IsModelReady() )
		return 0;

	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr *pHdr = pModelInstance->GetModelPtr();
	if ( !pHdr )
		return 0;

	CUtlVector< mstudioseqdesc_t* >hSeqs;
	for ( int i = 0; i < pHdr->GetNumSeq(); i++ )
		if ( !( pHdr->pSeqdesc( i ).flags & STUDIO_HIDDEN ) )
			hSeqs.AddToTail( &pHdr->pSeqdesc( i ) );

	int numSequences = hSeqs.Count();

	if ( !numSequences )
		return 0;

	hSeqs.Sort( SequenceSort );

	CUtlVector< const char* >hNameList;
	for ( int i = 0; i < numSequences; i++ )
	{
		const char *seqName = NULL;
		const mstudioseqdesc_t &seqPtr = *hSeqs[ i ];
		if ( seqPtr.pszLabel() )
			seqName = seqPtr.pszLabel();
		else
			seqName = "Unknown Sequence";

		hNameList.AddToTail( seqName );
	}

	*list = new char*[numSequences];

	int iTotalLength = 0;
	for ( int i = 0; i < numSequences; i++ )
		iTotalLength += Q_strlen( hNameList[i] ) + 1;

	**list = new char[ iTotalLength ];

	int curpos = 0;
	for ( int i = 0; i < numSequences; i++ )
	{
		int curLength = Q_strlen( hNameList[i] ) + 1;
		(*list)[ i ] = **list + curpos;
		Q_strcpy( (*list)[ i ], hNameList[i] );
		curpos += curLength;
	}

	hNameList.Purge();
	hSeqs.Purge();
	return numSequences;
}
void SEditModelRender::SetSequence( const char *name )
{
	if ( !IsModelReady() )
		return;

	MDLCACHE_CRITICAL_SECTION();
	pModelInstance->ResetSequence( pModelInstance->LookupSequence( name ) );
}
void SEditModelRender::ExecRender()
{
	if ( !IsModelReady() )
		return;

	MDLCACHE_CRITICAL_SECTION();
	for ( int i = 0; i < m_iNumPoseParams; i++ )
		pModelInstance->SetPoseParameter( i, 0 );

#if SWARM_DLL
	RenderableInstance_t instance;
	instance.m_nAlpha = 255;
#endif
	pModelInstance->DrawModel( STUDIO_RENDER
#if SWARM_DLL
		, instance
#endif
		);
}
void SEditModelRender::DoPostProc( int x, int y, int w, int h )
{
#ifndef SOURCE_2006
	if ( view && view->GetPlayerViewSetup()->m_bDoBloomAndToneMapping )
		DoEnginePostProcessing( x, y, w, h, false, false );
#endif
}
int SEditModelRender::MaterialPicker( char ***szMat )
{
	int mx, my;
#ifdef SOURCE_2006
	vgui::input()->GetCursorPos( mx, my );
#else
	vgui::input()->GetCursorPosition( mx, my );
#endif

	Vector ray;
	const CViewSetup *pViewSetup = view->GetPlayerViewSetup();
	float ratio =engine->GetScreenAspectRatio(
#if SWARM_DLL
		pViewSetup->width, pViewSetup->height
#endif
		);

	ratio = ( 1.0f / ratio ) * (4.0f/3.0f);
	float flFov = ScaleFOVByWidthRatio( pViewSetup->fov, ratio );
	ScreenToWorld( mx, my, flFov, pViewSetup->origin, pViewSetup->angles, ray );

	Vector start = pViewSetup->origin;
	Vector end = start + ray * MAX_TRACE_LENGTH;
	trace_t tr;
	C_BaseEntity *pIgnore = input->CAM_IsThirdPerson() ? NULL : C_BasePlayer::GetLocalPlayer();
	UTIL_TraceLine( start, end, MASK_SOLID, pIgnore, COLLISION_GROUP_NONE, &tr );

	if ( !tr.DidHit() )
		return 0;

	int numMaterials = 0;
	IMaterial **MatList = NULL;
	studiohdr_t *pSHdr = NULL;

	if ( tr.DidHitWorld() )
	{
		if ( tr.hitbox == 0 )
		{
			Vector dummy;
			IMaterial *pMat = engine->TraceLineMaterialAndLighting( start, end, dummy, dummy );
			if ( pMat )
			{
				numMaterials = 1;
				MatList = new IMaterial*[1];
				MatList[0] = pMat;
			}
		}
		else
		{
			ICollideable *prop = staticpropmgr->GetStaticPropByIndex( tr.hitbox - 1 );
			if ( prop )
			{
				IClientRenderable *pRenderProp = prop->GetIClientUnknown()->GetClientRenderable();
				if ( pRenderProp )
				{
					const model_t *pModel = pRenderProp->GetModel();
					if ( pModel )
						pSHdr = modelinfo->GetStudiomodel( pModel );
				}
			}
		}
	}
	else if ( tr.m_pEnt )
	{
		const model_t *pModel = tr.m_pEnt->GetModel();
		if ( pModel )
			pSHdr = modelinfo->GetStudiomodel( pModel );
	}

	if ( pSHdr )
	{
		Assert( !numMaterials && !MatList );
		numMaterials = pSHdr->numtextures;
		const int numPaths = pSHdr->numcdtextures;

		if ( numMaterials )
		{
			CUtlVector< IMaterial* >hValidMaterials;
			for ( int i = 0; i < numMaterials; i++ )
			{
				mstudiotexture_t *pStudioTex = pSHdr->pTexture( i );
				const char *matName = pStudioTex->pszName();

				for ( int p = 0; p < numPaths; p++ )
				{
					char tmpPath[MAX_PATH];
					Q_snprintf( tmpPath, MAX_PATH, "%s%s\0", pSHdr->pCdtexture( p ), matName );
					Q_FixSlashes( tmpPath );
					IMaterial *pTempMat = materials->FindMaterial( tmpPath, TEXTURE_GROUP_MODEL );
					if ( !IsErrorMaterial( pTempMat ) )
					{
						hValidMaterials.AddToTail( pTempMat );
						break;
					}
				}
			}

			numMaterials = hValidMaterials.Count();
			if ( numMaterials )
			{
				MatList = new IMaterial*[ numMaterials ];
				for ( int i = 0; i < numMaterials; i++ )
					MatList[i] = hValidMaterials[i];
			}

			hValidMaterials.Purge();
		}
	}

	*szMat = new char*[ numMaterials ];

	int iTotalLength = 0;
	for ( int i = 0; i < numMaterials; i++ )
		iTotalLength += Q_strlen( MatList[i]->GetName() ) + 1;

	**szMat = new char[ iTotalLength ];

	int curpos = 0;
	for ( int i = 0; i < numMaterials; i++ )
	{
		const char *pszName = MatList[i]->GetName();

		int curLength = Q_strlen( pszName ) + 1;
		(*szMat)[ i ] = **szMat + curpos;
		Q_strcpy( (*szMat)[ i ], pszName );
		curpos += curLength;
	}

	if ( MatList )
		delete [] MatList;

	return numMaterials;
}