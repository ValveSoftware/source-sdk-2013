//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IVP_H
#define IVP_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class CPhysCollide;
class CTextBuffer;
class IPhysicsCollision;
extern IPhysicsCollision *physcollision;

// a list of all of the materials in the world model
extern int RemapWorldMaterial( int materialIndexIn );

class CPhysCollisionEntry
{
public:
	CPhysCollisionEntry( CPhysCollide *pCollide );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex ) = 0;
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex ) = 0;
	
	unsigned int GetCollisionBinarySize();
	unsigned int WriteCollisionBinary( char *pDest );

protected:
	void DumpCollideFileName( const char *pName, int modelIndex, CTextBuffer *pTextBuffer );

protected:
	CPhysCollide	*m_pCollide;
};

class CPhysCollisionEntryStaticMesh : public CPhysCollisionEntry
{
public:
	CPhysCollisionEntryStaticMesh( CPhysCollide *pCollide, const char *pMaterialName );

	virtual void WriteToTextBuffer( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );
	virtual void DumpCollide( CTextBuffer *pTextBuffer, int modelIndex, int collideIndex );

private:
	const char *m_pMaterial;
};


class CTextBuffer
{
public:
	CTextBuffer( void );
	~CTextBuffer( void );
	inline int GetSize( void ) { return m_buffer.Count(); }
	inline char *GetData( void ) { return m_buffer.Base(); }
	
	void WriteText( const char *pText );
	void WriteIntKey( const char *pKeyName, int outputData );
	void WriteStringKey( const char *pKeyName, const char *outputData );
	void WriteFloatKey( const char *pKeyName, float outputData );
	void WriteFloatArrayKey( const char *pKeyName, const float *outputData, int count );

	void CopyStringQuotes( const char *pString );
	void Terminate( void );
private:
	void CopyData( const char *pData, int len );
	CUtlVector<char> m_buffer;
};

#endif // IVP_H
