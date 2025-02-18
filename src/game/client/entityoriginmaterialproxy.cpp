//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A base class for all material proxies in the client dll
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
// identifier was truncated to '255' characters in the debug information
//#pragma warning(disable: 4786)

#include "proxyentity.h"
#include "materialsystem/imaterialvar.h"

class CEntityOriginMaterialProxy : public CEntityMaterialProxy
{
public:
	CEntityOriginMaterialProxy()
	{
		m_pMaterial = NULL;
		m_pOriginVar = NULL;
	}
	virtual ~CEntityOriginMaterialProxy()
	{
	}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		m_pMaterial = pMaterial;
		bool found;
		m_pOriginVar = m_pMaterial->FindVar( "$entityorigin", &found );
		if( !found )
		{
			m_pOriginVar = NULL;
			return false;
		}
		return true;
	}
	virtual void OnBind( C_BaseEntity *pC_BaseEntity )
	{
		const Vector &origin = pC_BaseEntity->GetAbsOrigin();
		m_pOriginVar->SetVecValue( origin.x, origin.y, origin.z );
	}

	virtual IMaterial *GetMaterial()
	{
		return m_pMaterial;
	}

protected:
	IMaterial *m_pMaterial;
	IMaterialVar *m_pOriginVar;
};

EXPOSE_INTERFACE( CEntityOriginMaterialProxy, IMaterialProxy, "EntityOrigin" IMATERIAL_PROXY_INTERFACE_VERSION );

//=================================================================================================================
// This is a last-minute hack to ship Orange Box on the 360!
//=================================================================================================================
class CEntityOriginAlyxMaterialProxy : public CEntityMaterialProxy
{
public:
	CEntityOriginAlyxMaterialProxy()
	{
		m_pMaterial = NULL;
		m_pOriginVar = NULL;
	}
	virtual ~CEntityOriginAlyxMaterialProxy()
	{
	}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		m_pMaterial = pMaterial;
		bool found;
		m_pOriginVar = m_pMaterial->FindVar( "$entityorigin", &found );
		if( !found )
		{
			m_pOriginVar = NULL;
			return false;
		}
		return true;
	}
	virtual void OnBind( C_BaseEntity *pC_BaseEntity )
	{
		const Vector &origin = pC_BaseEntity->GetAbsOrigin();
		m_pOriginVar->SetVecValue( origin.x - 15.0f, origin.y, origin.z );
	}

	virtual IMaterial *GetMaterial()
	{
		return m_pMaterial;
	}

protected:
	IMaterial *m_pMaterial;
	IMaterialVar *m_pOriginVar;
};

EXPOSE_INTERFACE( CEntityOriginAlyxMaterialProxy, IMaterialProxy, "EntityOriginAlyx" IMATERIAL_PROXY_INTERFACE_VERSION );

//=================================================================================================================
// This is a last-minute hack to ship Orange Box on the 360!
//=================================================================================================================
class CEp1IntroVortRefractMaterialProxy : public CEntityMaterialProxy
{
public:
	CEp1IntroVortRefractMaterialProxy()
	{
		m_pMaterial = NULL;
		m_pOriginVar = NULL;
	}
	virtual ~CEp1IntroVortRefractMaterialProxy()
	{
	}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		m_pMaterial = pMaterial;
		bool found;
		m_pOriginVar = m_pMaterial->FindVar( "$refractamount", &found );
		if( !found )
		{
			m_pOriginVar = NULL;
			return false;
		}
		return true;
	}
	virtual void OnBind( C_BaseEntity *pC_BaseEntity )
	{
		if ( m_pOriginVar != NULL)
		{
			float flTmp = ( 1.0f - m_pOriginVar->GetFloatValue() );
			flTmp *= flTmp;
			flTmp *= flTmp;
			flTmp = ( 1.0f - flTmp ) * 0.25f;
			m_pOriginVar->SetFloatValue( flTmp );
		}
	}

	virtual IMaterial *GetMaterial()
	{
		return m_pMaterial;
	}

protected:
	IMaterial *m_pMaterial;
	IMaterialVar *m_pOriginVar;
};

EXPOSE_INTERFACE( CEp1IntroVortRefractMaterialProxy, IMaterialProxy, "Ep1IntroVortRefract" IMATERIAL_PROXY_INTERFACE_VERSION );
