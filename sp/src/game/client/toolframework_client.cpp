//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "toolframework_client.h"
#include "igamesystem.h"
#include "tier1/KeyValues.h"
#include "toolframework/iclientenginetools.h"
#include "client_factorylist.h"
#include "iviewrender.h"
#include "materialsystem/imaterialvar.h"

extern IViewRender *view;

class CToolFrameworkClient : public CBaseGameSystemPerFrame
{
public:
	// Methods of IGameSystem
	virtual bool	Init();
	virtual void	LevelInitPreEntity();
	virtual void	LevelInitPostEntity();
	virtual void	LevelShutdownPreEntity();
	virtual void	LevelShutdownPostEntity();
	virtual void	PreRender();
	virtual void	PostRender();

public:
	// Other public methods
	void PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg );
	void AdjustEngineViewport( int& x, int& y, int& width, int& height );
	bool SetupEngineView( Vector &origin, QAngle &angles, float &fov );
	bool SetupAudioState( AudioState_t &audioState );
	bool IsThirdPersonCamera();

	IClientEngineTools	*m_pTools;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CToolFrameworkClient g_ToolFrameworkClient;

#ifndef NO_TOOLFRAMEWORK

bool ToolsEnabled()
{
	return g_ToolFrameworkClient.m_pTools && g_ToolFrameworkClient.m_pTools->InToolMode();
}

#endif

IGameSystem *ToolFrameworkClientSystem()
{
	return &g_ToolFrameworkClient;
}


bool CToolFrameworkClient::Init()
{
	factorylist_t list;
	FactoryList_Retrieve( list );

	m_pTools = ( IClientEngineTools * )list.appSystemFactory( VCLIENTENGINETOOLS_INTERFACE_VERSION, NULL );
	return ( m_pTools != NULL );
}

void CToolFrameworkClient::LevelInitPreEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelInitPreEntityAllTools();
	}
}

void CToolFrameworkClient::LevelInitPostEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelInitPostEntityAllTools();
	}
}

void CToolFrameworkClient::LevelShutdownPreEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelShutdownPreEntityAllTools();
	}
}

void CToolFrameworkClient::LevelShutdownPostEntity()
{
	if ( m_pTools )
	{
		m_pTools->LevelShutdownPostEntityAllTools();
	}
}

void CToolFrameworkClient::PreRender()
{
	if ( m_pTools )
	{
		m_pTools->PreRenderAllTools();
	}
}

void CToolFrameworkClient::PostRender()
{
	if ( m_pTools )
	{
		m_pTools->PostRenderAllTools();
	}
}


//-----------------------------------------------------------------------------
// Should we render with a 3rd person camera?
//-----------------------------------------------------------------------------
bool CToolFrameworkClient::IsThirdPersonCamera()
{
	if ( !m_pTools )
		return false;
	return m_pTools->IsThirdPersonCamera( );
}

bool ToolFramework_IsThirdPersonCamera( )
{
	return g_ToolFrameworkClient.IsThirdPersonCamera( );
}


//-----------------------------------------------------------------------------
// Posts a message to all tools
//-----------------------------------------------------------------------------
void CToolFrameworkClient::PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg )
{
	if ( m_pTools )
	{
		m_pTools->PostToolMessage( hEntity, msg );
	}
}

void ToolFramework_PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg )
{
	g_ToolFrameworkClient.PostToolMessage( hEntity, msg );
}


//-----------------------------------------------------------------------------
// View manipulation
//-----------------------------------------------------------------------------
void CToolFrameworkClient::AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
	if ( m_pTools )
	{
		m_pTools->AdjustEngineViewport( x, y, width, height );
	}
}

void ToolFramework_AdjustEngineViewport( int& x, int& y, int& width, int& height )
{
	g_ToolFrameworkClient.AdjustEngineViewport( x, y, width, height );
}


//-----------------------------------------------------------------------------
// View manipulation
//-----------------------------------------------------------------------------
bool CToolFrameworkClient::SetupEngineView( Vector &origin, QAngle &angles, float &fov )
{
	if ( !m_pTools )
		return false;

	return m_pTools->SetupEngineView( origin, angles, fov );
}

bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov )
{
	return g_ToolFrameworkClient.SetupEngineView( origin, angles, fov );
}

//-----------------------------------------------------------------------------
// microphone manipulation
//-----------------------------------------------------------------------------
bool CToolFrameworkClient::SetupAudioState( AudioState_t &audioState )
{
	if ( !m_pTools )
		return false;

	return m_pTools->SetupAudioState( audioState );
}

bool ToolFramework_SetupAudioState( AudioState_t &audioState )
{
	return g_ToolFrameworkClient.SetupAudioState( audioState );
}


//-----------------------------------------------------------------------------
// Helper class to indicate ownership of effects
//-----------------------------------------------------------------------------
CRecordEffectOwner::CRecordEffectOwner( C_BaseEntity *pEntity, bool bIsViewModel )
{
	m_bToolsEnabled = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( m_bToolsEnabled )
	{
		KeyValues *msg = new KeyValues( "EffectsOwner" );
		msg->SetInt( "viewModel", bIsViewModel );
		ToolFramework_PostToolMessage( pEntity ? pEntity->GetToolHandle() : HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}

CRecordEffectOwner::~CRecordEffectOwner()
{
	if ( m_bToolsEnabled )
	{
		KeyValues *msg = new KeyValues( "EffectsOwner" );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// material recording - primarily for proxy materials
//-----------------------------------------------------------------------------

void WriteFloat( char *&buf, float f)
{
	*( float* )buf = f;
	buf += sizeof( float );
}

void WriteInt( char *&buf, int i )
{
	*( int* )buf = i;
	buf += sizeof( int );
}

void WritePtr( char *&buf, void *p )
{
	*( void** )buf = p;
	buf += sizeof( void* );
}

void ToolFramework_RecordMaterialParams( IMaterial *pMaterial )
{
	Assert( pMaterial );
	if ( !pMaterial )
		return;

	if ( !clienttools->IsInRecordingMode() )
		return;

	C_BaseEntity *pEnt = view->GetCurrentlyDrawingEntity();
	if ( !pEnt || !pEnt->IsToolRecording() )
		return;

	KeyValues *msg = new KeyValues( "material_proxy_state" );
	msg->SetString( "mtlName", pMaterial->GetName() );
	msg->SetString( "groupName", pMaterial->GetTextureGroupName() );

	int nParams = pMaterial->ShaderParamCount();
	IMaterialVar **pParams = pMaterial->GetShaderParams();

	char str[ 256 ];

	for ( int i = 0; i < nParams; ++i )
	{
		IMaterialVar *pVar = pParams[ i ];
		const char *pVarName = pVar->GetName();
		MaterialVarType_t vartype = pVar->GetType();
		switch ( vartype )
		{
		case MATERIAL_VAR_TYPE_FLOAT:
			msg->SetFloat( pVarName, pVar->GetFloatValue() );
			break;

		case MATERIAL_VAR_TYPE_INT:
			msg->SetInt( pVarName, pVar->GetIntValue() );
			break;

		case MATERIAL_VAR_TYPE_STRING:
			msg->SetString( pVarName, pVar->GetStringValue() );
			break;

		case MATERIAL_VAR_TYPE_FOURCC:
			Assert( 0 ); // JDTODO
			break;

		case MATERIAL_VAR_TYPE_VECTOR:
			{
				const float *pVal = pVar->GetVecValue();
				int dim = pVar->VectorSize();
				switch ( dim )
				{
				case 2:
					V_snprintf( str, sizeof( str ), "vector2d: %f %f", pVal[ 0 ], pVal[ 1 ] );
					break;
				case 3:
					V_snprintf( str, sizeof( str ), "vector3d: %f %f %f", pVal[ 0 ], pVal[ 1 ], pVal[ 2 ] );
					break;
				case 4:
					V_snprintf( str, sizeof( str ), "vector4d: %f %f %f %f", pVal[ 0 ], pVal[ 1 ], pVal[ 2 ], pVal[ 3 ] );
					break;
				default:
					Assert( 0 );
					*str = 0;
				}
				msg->SetString( pVarName, str );
			}
			break;

		case MATERIAL_VAR_TYPE_MATRIX:
			{
				const VMatrix &matrix = pVar->GetMatrixValue();
				const float *pVal = matrix.Base();
				V_snprintf( str, sizeof( str ),
					"matrix: %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
					pVal[ 0 ],  pVal[ 1 ],  pVal[ 2 ],  pVal[ 3 ],
					pVal[ 4 ],  pVal[ 5 ],  pVal[ 6 ],  pVal[ 7 ],
					pVal[ 8 ],  pVal[ 9 ],  pVal[ 10 ], pVal[ 11 ],
					pVal[ 12 ], pVal[ 13 ], pVal[ 14 ], pVal[ 15 ] );
				msg->SetString( pVarName, str );
			}
			break;

		case MATERIAL_VAR_TYPE_TEXTURE:
			//			V_snprintf( str, sizeof( str ), "texture: %x", pVar->GetTextureValue() );
			//			msg->SetString( pVarName, str );
			break;

		case MATERIAL_VAR_TYPE_MATERIAL:
			//			V_snprintf( str, sizeof( str ), "material: %x", pVar->GetMaterialValue() );
			//			msg->SetString( pVarName, str );
			break;

		case MATERIAL_VAR_TYPE_UNDEFINED:
			//			Assert( 0 ); // these appear to be (mostly? all?) textures, although I don't know why they're not caught by the texture case above...
			break; // JDTODO

		default:
			Assert( 0 );
		}
	}

	Assert( pEnt->GetToolHandle() );
	ToolFramework_PostToolMessage( pEnt->GetToolHandle(), msg );

	msg->deleteThis();
}
