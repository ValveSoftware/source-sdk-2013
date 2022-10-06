//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Material proxy to stuff a convar into a material var.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
// identifier was truncated to '255' characters in the debug information
//#pragma warning(disable: 4786)

#include "convar.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterialvar.h"
//#include "imaterialproxydict.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


class CConVarMaterialProxy: public IMaterialProxy
{
public:
	CConVarMaterialProxy()
	: m_pResult( NULL ),
	  m_conVarRef( "", true )
	{
	}

	virtual ~CConVarMaterialProxy()
	{
	}

	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		const char *pResult = pKeyValues->GetString( "resultVar" );
		if ( !pResult )
			return false;

		bool found;
		m_pResult = pMaterial->FindVar( pResult, &found );
		if ( !found )
		{
			m_pResult = NULL;
			return false;
		}

		/*
		if ( !Q_stricmp( pResult, "$alpha" ) )
		{
			pMaterial->SetMaterialVarFlag( MATERIAL_VAR_ALPHA_MODIFIED_BY_PROXY, true );
		}
		*/

		pResult = pKeyValues->GetString( "convar" );
		if( !pResult )
		{
			return false;
		}

		m_conVarRef.Init( pResult, false );
		if ( !m_conVarRef.IsValid() )
		{
			return false;
		}

		return true;
	}

	virtual void OnBind( void* )
	{
		switch( m_pResult->GetType() )
		{
		case MATERIAL_VAR_TYPE_VECTOR:
			{
				float f = m_conVarRef.GetFloat();
				Vector4D vec( f, f, f, f );
				m_pResult->SetVecValue( vec.Base(), m_pResult->VectorSize() );
			}
			break;

#ifdef MAPBASE
		case MATERIAL_VAR_TYPE_STRING:
			m_pResult->SetStringValue( m_conVarRef.GetString() );
			break;
#endif

		case MATERIAL_VAR_TYPE_INT:
			m_pResult->SetIntValue( m_conVarRef.GetInt() );
			break;

		case MATERIAL_VAR_TYPE_FLOAT:
		default:
			m_pResult->SetFloatValue( m_conVarRef.GetFloat() );
			break;
		}
	}

	virtual IMaterial *GetMaterial()
	{
		return m_pResult->GetOwningMaterial();
	}

	virtual void Release()
	{
	}

protected:
	IMaterialVar *m_pResult;
	ConVarRef m_conVarRef;
};

EXPOSE_INTERFACE( CConVarMaterialProxy, IMaterialProxy, "ConVar" IMATERIAL_PROXY_INTERFACE_VERSION );
