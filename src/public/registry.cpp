//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h>
#endif
#include "tier0/platform.h"
#include "tier0/vcrmode.h"
#include "iregistry.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include <stdio.h>
#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Exposes registry interface to rest of launcher
//-----------------------------------------------------------------------------
class CRegistry : public IRegistry
{
public:
							CRegistry( void );
	virtual					~CRegistry( void );

	virtual bool			Init( const char *platformName );
	virtual bool			DirectInit( const char *subDirectoryUnderValve );
	virtual void			Shutdown( void );
	
	virtual int				ReadInt( const char *key, int defaultValue = 0);
	virtual void			WriteInt( const char *key, int value );

	virtual const char		*ReadString( const char *key, const char *defaultValue = NULL );
	virtual void			WriteString( const char *key, const char *value );

	// Read/write helper methods
	virtual int				ReadInt( const char *pKeyBase, const char *pKey, int defaultValue = 0 );
	virtual void			WriteInt( const char *pKeyBase, const char *key, int value );
	virtual const char		*ReadString( const char *pKeyBase, const char *key, const char *defaultValue );
	virtual void			WriteString( const char *pKeyBase, const char *key, const char *value );

private:
	bool			m_bValid;
#ifdef WIN32
	HKEY			m_hKey;
#endif
};

// Creates it and calls Init
IRegistry *InstanceRegistry( char const *subDirectoryUnderValve )
{
	CRegistry *instance = new CRegistry();
	instance->DirectInit( subDirectoryUnderValve );
	return instance;
}

// Calls Shutdown and deletes it
void ReleaseInstancedRegistry( IRegistry *reg )
{
	if ( !reg )
	{
		Assert( !"ReleaseInstancedRegistry( reg == NULL )!" );
		return;
	}

	reg->Shutdown();
	delete reg;
}

// Expose to launcher
static CRegistry g_Registry;
IRegistry *registry = ( IRegistry * )&g_Registry;

//-----------------------------------------------------------------------------
// Read/write helper methods
//-----------------------------------------------------------------------------
int CRegistry::ReadInt( const char *pKeyBase, const char *pKey, int defaultValue )
{
	int nLen = V_strlen( pKeyBase );
	int nKeyLen = V_strlen( pKey );
	char *pFullKey = (char*)_alloca( nLen + nKeyLen + 2 );
	Q_snprintf( pFullKey, nLen + nKeyLen + 2, "%s\\%s", pKeyBase, pKey );
	return ReadInt( pFullKey, defaultValue );
}

void CRegistry::WriteInt( const char *pKeyBase, const char *pKey, int value )
{
	int nLen = V_strlen( pKeyBase );
	int nKeyLen = V_strlen( pKey );
	char *pFullKey = (char*)_alloca( nLen + nKeyLen + 2 );
	Q_snprintf( pFullKey, nLen + nKeyLen + 2, "%s\\%s", pKeyBase, pKey );
	WriteInt( pFullKey, value );
}

const char *CRegistry::ReadString( const char *pKeyBase, const char *pKey, const char *defaultValue )
{
	int nLen = V_strlen( pKeyBase );
	int nKeyLen = V_strlen( pKey );
	char *pFullKey = (char*)_alloca( nLen + nKeyLen + 2 );
	Q_snprintf( pFullKey, nLen + nKeyLen + 2, "%s\\%s", pKeyBase, pKey );
	return ReadString( pFullKey, defaultValue );
}

void CRegistry::WriteString( const char *pKeyBase, const char *pKey, const char *value )
{
	int nLen = V_strlen( pKeyBase );
	int nKeyLen = V_strlen( pKey );
	char *pFullKey = (char*)_alloca( nLen + nKeyLen + 2 );
	Q_snprintf( pFullKey, nLen + nKeyLen + 2, "%s\\%s", pKeyBase, pKey );
	WriteString( pFullKey, value );
}

#ifndef POSIX

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRegistry::CRegistry( void )
{
	// Assume failure
	m_bValid	= false;
	m_hKey		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRegistry::~CRegistry( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Read integer from registry
// Input  : *key - 
//			defaultValue - 
// Output : int
//-----------------------------------------------------------------------------
int CRegistry::ReadInt( const char *key, int defaultValue /*= 0*/ )
{
	LONG lResult;           // Registry function result code
	DWORD dwType;           // Type of key
	DWORD dwSize;           // Size of element data

	int value;

	if ( !m_bValid )
	{
		return defaultValue;
	}

	dwSize = sizeof( DWORD );

	lResult = VCRHook_RegQueryValueEx(
		m_hKey,		// handle to key
		key,	// value name
		0,			// reserved
		&dwType,    // type buffer
		(LPBYTE)&value,    // data buffer
		&dwSize );  // size of data buffer

	if (lResult != ERROR_SUCCESS)  // Failure
		return defaultValue;

	if (dwType != REG_DWORD)
		return defaultValue;

	return value;
}

//-----------------------------------------------------------------------------
// Purpose: Save integer to registry
// Input  : *key - 
//			value - 
//-----------------------------------------------------------------------------
void CRegistry::WriteInt( const char *key, int value )
{
	// Size of element data
	DWORD dwSize;           

	if ( !m_bValid )
	{
		return;
	}

	dwSize = sizeof( DWORD );

	VCRHook_RegSetValueEx(
		m_hKey,		// handle to key
		key,	// value name
		0,			// reserved
		REG_DWORD,		// type buffer
		(LPBYTE)&value,    // data buffer
		dwSize );  // size of data buffer
}

//-----------------------------------------------------------------------------
// Purpose: Read string value from registry
// Input  : *key - 
//			*defaultValue - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CRegistry::ReadString( const char *key, const char *defaultValue /* = NULL */ )
{
	LONG lResult;        
	// Type of key
	DWORD dwType;        
	// Size of element data
	DWORD dwSize = 512;           

	static char value[ 512 ];

	value[0] = 0;

	if ( !m_bValid )
	{
		return defaultValue;
	}

	lResult = VCRHook_RegQueryValueEx(
		m_hKey,		// handle to key
		key,	// value name
		0,			// reserved
		&dwType,    // type buffer
		(unsigned char *)value,    // data buffer
		&dwSize );  // size of data buffer

	if ( lResult != ERROR_SUCCESS ) 
	{
		return defaultValue;
	}

	if ( dwType != REG_SZ )
	{
		return defaultValue;
	}

	return value;
}

//-----------------------------------------------------------------------------
// Purpose: Save string to registry
// Input  : *key - 
//			*value - 
//-----------------------------------------------------------------------------
void CRegistry::WriteString( const char *key, const char *value )
{
	DWORD dwSize;           // Size of element data

	if ( !m_bValid )
	{
		return;
	}

	dwSize = (DWORD)( V_strlen( value ) + 1 );

	VCRHook_RegSetValueEx(
		m_hKey,		// handle to key
		key,	// value name
		0,			// reserved
		REG_SZ,		// type buffer
		(LPBYTE)value,    // data buffer
		dwSize );  // size of data buffer
}




bool CRegistry::DirectInit( const char *subDirectoryUnderValve )
{
	LONG lResult;           // Registry function result code
	ULONG dwDisposition;    // Type of key opening event

	char szModelKey[ 1024 ];
	wsprintf( szModelKey, "Software\\Valve\\%s", subDirectoryUnderValve );

	lResult = VCRHook_RegCreateKeyEx(
		HKEY_CURRENT_USER,	// handle of open key 
		szModelKey,			// address of name of subkey to open 
		0ul,					// DWORD ulOptions,	  // reserved 
		NULL,			// Type of value
		REG_OPTION_NON_VOLATILE, // Store permanently in reg.
		KEY_ALL_ACCESS,		// REGSAM samDesired, // security access mask 
		NULL,
		&m_hKey,				// Key we are creating
		&dwDisposition );    // Type of creation

	if ( lResult != ERROR_SUCCESS )
	{
		m_bValid = false;
		return false;
	}
	
	// Success
	m_bValid = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Open default launcher key based on game directory
//-----------------------------------------------------------------------------
bool CRegistry::Init( const char *platformName )
{
	char subDir[ 512 ];
	wsprintf( subDir, "%s\\Settings", platformName );
	return DirectInit( subDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRegistry::Shutdown( void )
{
	if ( !m_bValid )
		return;

	// Make invalid
	m_bValid = false;
	VCRHook_RegCloseKey( m_hKey );
}

#else






//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRegistry::CRegistry( void )
{
	// Assume failure
	m_bValid	= false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRegistry::~CRegistry( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Read integer from registry
// Input  : *key - 
//			defaultValue - 
// Output : int
//-----------------------------------------------------------------------------
int CRegistry::ReadInt( const char *key, int defaultValue /*= 0*/ )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Save integer to registry
// Input  : *key - 
//			value - 
//-----------------------------------------------------------------------------
void CRegistry::WriteInt( const char *key, int value )
{
}

//-----------------------------------------------------------------------------
// Purpose: Read string value from registry
// Input  : *key - 
//			*defaultValue - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CRegistry::ReadString( const char *key, const char *defaultValue /* = NULL */ )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Save string to registry
// Input  : *key - 
//			*value - 
//-----------------------------------------------------------------------------
void CRegistry::WriteString( const char *key, const char *value )
{
}



bool CRegistry::DirectInit( const char *subDirectoryUnderValve )
{

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Open default launcher key based on game directory
//-----------------------------------------------------------------------------
bool CRegistry::Init( const char *platformName )
{
	char subDir[ 512 ];
	snprintf( subDir, sizeof(subDir), "%s\\Settings", platformName );
	return DirectInit( subDir );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRegistry::Shutdown( void )
{
	if ( !m_bValid )
		return;

	// Make invalid
	m_bValid = false;
}
#endif // POSIX

