//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================
#ifndef TF_PLAYERCLASS_SHARED_H
#define TF_PLAYERCLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "tf_classdata.h"

// Client specific.
#ifdef CLIENT_DLL

EXTERN_RECV_TABLE( DT_TFPlayerClassShared );

// Server specific.
#else

EXTERN_SEND_TABLE( DT_TFPlayerClassShared );

#endif

//-----------------------------------------------------------------------------
// TF Player Class Shared
//-----------------------------------------------------------------------------
class CTFPlayerClassShared
{
public:

	CTFPlayerClassShared();

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerClassShared );

	bool		Init( int iClass );
	bool		IsClass( int iClass ) const						{ return ( m_iClass == iClass ); }
	int			GetClassIndex( void ) const						{ return m_iClass; }
	void		Reset( void );

#ifdef CLIENT_DLL
	string_t	GetClassIconName( void ) const					{ return MAKE_STRING( m_iszClassIcon ); }
	bool		HasCustomModel( void ) const					{ return m_iszCustomModel[0] != '\0'; }
#else
	string_t	GetClassIconName( void ) const					{ return m_iszClassIcon.Get(); }
	void		SetClassIconName( string_t iszClassIcon )		{ m_iszClassIcon = iszClassIcon; }
	bool		HasCustomModel( void ) const					{ return (m_iszCustomModel.Get() != NULL_STRING); }
#endif

#ifndef CLIENT_DLL
	#define USE_CLASS_ANIMATIONS true
	void		SetCustomModel( const char *pszModelName, bool isUsingClassAnimations = false );
	void		SetCustomModelOffset( const Vector &vecOffset )		{ m_vecCustomModelOffset = vecOffset; }
	void		SetCustomModelRotates( bool bRotates )			{ m_bCustomModelRotates = bRotates; }
	void		SetCustomModelRotation( const QAngle &vecOffset )		{ m_angCustomModelRotation = vecOffset; m_bCustomModelRotationSet = true; }
	void		ClearCustomModelRotation( void )				{ m_bCustomModelRotationSet = false; }
	void		SetCustomModelVisibleToSelf( bool bVisible )	{ m_bCustomModelVisibleToSelf = bVisible; }
#endif

	const char	*GetName( void ) const							{ return GetPlayerClassData( m_iClass )->m_szClassName; }
	const char	*GetModelName( void ) const;
	const char	*GetHandModelName( int iHandIndex ) const;
	float		GetMaxSpeed( void )								{ return GetPlayerClassData( m_iClass )->m_flMaxSpeed; }
	int			GetMaxHealth( void ) const						{ return GetPlayerClassData( m_iClass )->m_nMaxHealth; }
	int			GetMaxArmor( void )	const						{ return GetPlayerClassData( m_iClass )->m_nMaxArmor; }
	Vector		GetCustomModelOffset( void ) const				{ return m_vecCustomModelOffset.Get(); }
	QAngle		GetCustomModelRotation( void ) const			{ return m_angCustomModelRotation.Get(); }
	bool		CustomModelRotationSet( void )					{ return m_bCustomModelRotationSet.Get(); }
	bool		CustomModelRotates( void ) const				{ return m_bCustomModelRotates.Get(); }
	bool		CustomModelIsVisibleToSelf( void ) const		{ return m_bCustomModelVisibleToSelf.Get(); }
	bool		CustomModelUsesClassAnimations( void ) const	{ return m_bUseClassAnimations.Get(); }
	bool		CustomModelHasChanged( void );

	TFPlayerClassData_t  *GetData( void ) const					{ return GetPlayerClassData( m_iClass ); }

	// If needed, put this into playerclass scripts
	bool CanBuildObject( int iObjectType );

protected:

	CNetworkVar( int,	m_iClass );

#ifdef CLIENT_DLL
	char		m_iszClassIcon[MAX_PATH];
	char		m_iszCustomModel[MAX_PATH];
#else
	CNetworkVar( string_t, m_iszClassIcon );
	CNetworkVar( string_t, m_iszCustomModel );
#endif
	CNetworkVar( Vector, m_vecCustomModelOffset );
	CNetworkVar( QAngle, m_angCustomModelRotation );
	CNetworkVar( bool, m_bCustomModelRotates );
	CNetworkVar( bool, m_bCustomModelRotationSet );
	CNetworkVar( bool, m_bCustomModelVisibleToSelf );
	CNetworkVar( bool, m_bUseClassAnimations );
	CNetworkVar( int,  m_iClassModelParity );
	int			m_iOldClassModelParity;
};

#endif // TF_PLAYERCLASS_SHARED_H
