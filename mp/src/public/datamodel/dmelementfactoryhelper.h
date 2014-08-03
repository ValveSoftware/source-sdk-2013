//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef DMELEMENTFACTORYHELPER_H
#define DMELEMENTFACTORYHELPER_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/idatamodel.h"
#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "tier1/utlvector.h"
#include "tier1/utlsymbol.h"


//-----------------------------------------------------------------------------
// Internal interface for IDmElementFactory
//-----------------------------------------------------------------------------
class IDmElementFactoryInternal : public IDmElementFactory
{
public:
	virtual void SetElementTypeSymbol( CUtlSymbol sym ) = 0;
	virtual bool IsAbstract() const = 0;
};


//-----------------------------------------------------------------------------
// Class used to register factories into a global list
//-----------------------------------------------------------------------------
class CDmElementFactoryHelper
{
public:
	// Static list of helpers
	static CDmElementFactoryHelper *s_pHelpers[2];

	// Create all the hud elements
	static void InstallFactories( );

public:
	// Construction
	CDmElementFactoryHelper( const char *pClassName, IDmElementFactoryInternal *pFactory, bool bIsStandardFactory );

	// Accessors
	CDmElementFactoryHelper *GetNext( void );

	const char *GetClassname();
	IDmElementFactoryInternal *GetFactory();

private:
	// Next factory in list
	CDmElementFactoryHelper	*m_pNext;
	// Creation function to use for this technology
	IDmElementFactoryInternal *m_pFactory;
	const char				*m_pszClassname;
};


//-----------------------------------------------------------------------------
// Inline methods 
//-----------------------------------------------------------------------------
inline const char *CDmElementFactoryHelper::GetClassname() 
{ 
	return m_pszClassname; 
}

inline IDmElementFactoryInternal *CDmElementFactoryHelper::GetFactory() 
{ 
	return m_pFactory; 
}


//-----------------------------------------------------------------------------
// Helper Template factory for simple creation of factories
//-----------------------------------------------------------------------------
template <class T >
class CDmElementFactory : public IDmElementFactoryInternal
{
public:
	CDmElementFactory( const char *pLookupName ) : m_pLookupName( pLookupName ) {}

	// Creation, destruction
	virtual CDmElement* Create( DmElementHandle_t handle, const char *pElementType, const char *pElementName, DmFileId_t fileid, const DmObjectId_t &id )
	{
		return new T( handle, m_pLookupName, id, pElementName, fileid );
	}

	virtual void Destroy( DmElementHandle_t hElement )
	{
		CDmElement *pElement = g_pDataModel->GetElement( hElement );
		if ( pElement )
		{
			T *pActualElement = static_cast< T* >( pElement );
			delete pActualElement;
		}
	}

	// Sets the type symbol, used for "isa" implementation
	virtual void SetElementTypeSymbol( CUtlSymbol sym )
	{
		T::SetTypeSymbol( sym );
	}

	virtual bool IsAbstract() const { return false; }

private:
	const char *m_pLookupName;
};


template < class T >
class CDmAbstractElementFactory : public IDmElementFactoryInternal
{
public:
	CDmAbstractElementFactory() {}

	// Creation, destruction
	virtual CDmElement* Create( DmElementHandle_t handle, const char *pElementType, const char *pElementName, DmFileId_t fileid, const DmObjectId_t &id )
	{
		return NULL;
	}

	virtual void Destroy( DmElementHandle_t hElement )
	{
	}

	// Sets the type symbol, used for "isa" implementation
	virtual void SetElementTypeSymbol( CUtlSymbol sym )
	{
		T::SetTypeSymbol( sym );
	}

	virtual bool IsAbstract() const { return true; }

private:
};


//-----------------------------------------------------------------------------
// Helper macro to create the class factory 
//-----------------------------------------------------------------------------
#if defined( MOVIEOBJECTS_LIB ) || defined ( DATAMODEL_LIB ) || defined ( DMECONTROLS_LIB )

#define IMPLEMENT_ELEMENT_FACTORY( lookupName, className )	\
	IMPLEMENT_ELEMENT( className )							\
	CDmElementFactory< className > g_##className##_Factory( #lookupName );							\
	CDmElementFactoryHelper g_##className##_Helper( #lookupName, &g_##className##_Factory, true );	\
	className *g_##className##LinkerHack = NULL;

#define IMPLEMENT_ABSTRACT_ELEMENT( lookupName, className )	\
	IMPLEMENT_ELEMENT( className )							\
	CDmAbstractElementFactory< className > g_##className##_Factory;									\
	CDmElementFactoryHelper g_##className##_Helper( #lookupName, &g_##className##_Factory, true );	\
	className *g_##className##LinkerHack = NULL;

#else

#define IMPLEMENT_ELEMENT_FACTORY( lookupName, className )	\
	IMPLEMENT_ELEMENT( className )							\
	CDmElementFactory< className > g_##className##_Factory( #lookupName );						\
	CDmElementFactoryHelper g_##className##_Helper( #lookupName, &g_##className##_Factory, false );	\
	className *g_##className##LinkerHack = NULL;

#define IMPLEMENT_ABSTRACT_ELEMENT( lookupName, className )	\
	IMPLEMENT_ELEMENT( className )							\
	CDmAbstractElementFactory< className > g_##className##_Factory;									\
	CDmElementFactoryHelper g_##className##_Helper( #lookupName, &g_##className##_Factory, false );	\
	className *g_##className##LinkerHack = NULL;

#endif


// Used by classes defined in movieobjects or scenedatabase that must be explicitly installed
#define IMPLEMENT_ELEMENT_FACTORY_INSTALL_EXPLICITLY( lookupName, className )	\
	IMPLEMENT_ELEMENT( className )							\
	CDmElementFactory< className > g_##className##_Factory( #lookupName );						\
	CDmElementFactoryHelper g_##className##_Helper( #lookupName, &g_##className##_Factory, false );	\
	className *g_##className##LinkerHack = NULL;

//-----------------------------------------------------------------------------
// Installs dm element factories
//-----------------------------------------------------------------------------
void InstallDmElementFactories( );


#endif // DMELEMENTFACTORYHELPER_H
