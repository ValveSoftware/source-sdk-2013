//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MAPENTITIES_SHARED_H
#define MAPENTITIES_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#define MAPKEY_MAXLENGTH	2048


//-----------------------------------------------------------------------------
// Purpose: encapsulates the data string in the map file 
//			that is used to initialise entities.  The data
//			string contains a set of key/value pairs.
//-----------------------------------------------------------------------------
class CEntityMapData
{
private:
	char	*m_pEntData;
	int		m_nEntDataSize;
	char	*m_pCurrentKey;

public:
	explicit CEntityMapData( char *entBlock, int nEntBlockSize = -1 ) : 
		m_pEntData(entBlock), m_nEntDataSize(nEntBlockSize), m_pCurrentKey(entBlock) {}

	// find the keyName in the entdata and puts it's value into Value.  returns false if key is not found
	bool ExtractValue( const char *keyName, char *Value );

	// find the nth keyName in the endata and change its value to specified one
	// where n == nKeyInstance
	bool SetValue( const char *keyName, char *NewValue, int nKeyInstance = 0 );
	
	bool GetFirstKey( char *keyName, char *Value );
	bool GetNextKey( char *keyName, char *Value );

	const char *CurrentBufferPosition( void );
};

const char *MapEntity_ParseToken( const char *data, char *newToken );
const char *MapEntity_SkipToNextEntity( const char *pMapData, char *pWorkBuffer );
bool MapEntity_ExtractValue( const char *pEntData, const char *keyName, char Value[MAPKEY_MAXLENGTH] );


#endif // MAPENTITIES_SHARED_H
