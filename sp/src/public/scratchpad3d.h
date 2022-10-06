//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SCRATCHPAD3D_H
#define SCRATCHPAD3D_H
#ifdef _WIN32
#pragma once
#endif


#include <stdio.h>
#include "iscratchpad3d.h"
#include "mathlib/vmatrix.h"
#include "filesystem.h"

class CFileRead;


class CScratchPad3D : public IScratchPad3D
{
// Commands that can go in the file.
public:

	enum
	{
		COMMAND_POINT=0,
		COMMAND_LINE,
		COMMAND_POLYGON,
		COMMAND_MATRIX,
		COMMAND_RENDERSTATE,
		COMMAND_TEXT,
		COMMAND_NUMCOMMANDS
	};
	
	class ICachedRenderData
	{
	public:
		virtual void Release() = 0;
	};

	class CBaseCommand
	{
	public:
						CBaseCommand( unsigned char iCommand )
						{
							m_iCommand = (unsigned char)iCommand;
							m_pCachedRenderData = NULL;
						}

						virtual ~CBaseCommand()
						{
							ReleaseCachedRenderData();
						}
		
		virtual void	Read( CFileRead *pFile ) = 0;
		virtual void	Write( IFileSystem* pFileSystem, FileHandle_t fp ) = 0;
	
		// Release cached render data. Usually used for releasing things like textures when
		// the app is resizing..
		void			ReleaseCachedRenderData()
		{
			if ( m_pCachedRenderData )
			{
				m_pCachedRenderData->Release();
				m_pCachedRenderData = NULL;
			}
		}

	public:
		
		unsigned char	m_iCommand; // One of the COMMAND_ defines.

		// The renderer can cache data with the commands to speedup the rendering after
		// the first time (text uses this big time).
		ICachedRenderData	*m_pCachedRenderData;
	};

	class CCommand_Point : public CBaseCommand
	{
	public:
						CCommand_Point() : CBaseCommand( COMMAND_POINT ) {}

		virtual void	Read( CFileRead *pFile );
		virtual void	Write( IFileSystem* pFileSystem, FileHandle_t fp );

		float					m_flPointSize;
		CSPVert	m_Vert;
	};

	class CCommand_Line : public CBaseCommand
	{
	public:
						CCommand_Line() : CBaseCommand( COMMAND_LINE ) {}

		virtual void	Read( CFileRead *pFile );
		virtual void	Write( IFileSystem* pFileSystem, FileHandle_t fp );

		CSPVert	m_Verts[2];
	};

	class CCommand_Polygon : public CBaseCommand
	{
	public:
						CCommand_Polygon() : CBaseCommand( COMMAND_POLYGON ) {}

		virtual void	Read( CFileRead *pFile );
		virtual void	Write( IFileSystem* pFileSystem, FileHandle_t fp );

		CUtlVector<CSPVert>	m_Verts;
	};

	class CCommand_Matrix : public CBaseCommand
	{
	public:
						CCommand_Matrix() : CBaseCommand( COMMAND_MATRIX ) {}

		virtual void	Read( CFileRead *pFile );
		virtual void	Write( IFileSystem* pFileSystem, FileHandle_t fp );

		VMatrix			m_mMatrix;
	};

	class CCommand_RenderState : public CBaseCommand
	{
	public:
						CCommand_RenderState() : CBaseCommand( COMMAND_RENDERSTATE ) {}

		virtual void	Read( CFileRead *pFile );
		virtual void	Write( IFileSystem* pFileSystem, FileHandle_t fp );

		unsigned long	m_State;		// One of the RS_ enums.
		unsigned long	m_Val;
	};

	class CCommand_Text : public CBaseCommand
	{
	public:
						CCommand_Text() : CBaseCommand( COMMAND_TEXT ) {}

		virtual void	Read( CFileRead *pFile );
		virtual void	Write( IFileSystem* pFileSystem, FileHandle_t fp );

		CUtlVector<char>	m_String;				// The string to render.
		CTextParams m_TextParams;
	};


public:
	
						CScratchPad3D( char const *pFilename, IFileSystem *pFileSystem, bool bAutoClear );

	void				AutoFlush();
	void				DrawRectGeneric( int iPlane, int otherDim1, int otherDim2, float planeDist, Vector2D const &vMin, Vector2D const &vMax, CSPColor const &vColor );
	void				DeleteCommands();

	// Load a file...
	bool				LoadCommandsFromFile( );


public:

	virtual void		Release();

	virtual void		SetMapping( 
		Vector const &vInputMin, 
		Vector const &vInputMax,
		Vector const &vOutputMin,
		Vector const &vOutputMax );
	virtual bool		GetAutoFlush();
	virtual void		SetAutoFlush( bool bAutoFlush );
	virtual void		DrawPoint( CSPVert const &v, float flPointSize );
	virtual void		DrawLine( CSPVert const &v1, CSPVert const &v2 );
	virtual void		DrawPolygon( CSPVertList const &verts );
	virtual void		DrawRectYZ( float xPos, Vector2D const &vMin, Vector2D const &vMax, CSPColor const &vColor );
	virtual void		DrawRectXZ( float yPos, Vector2D const &vMin, Vector2D const &vMax, CSPColor const &vColor );
	virtual void		DrawRectXY( float zPos, Vector2D const &vMin, Vector2D const &vMax, CSPColor const &vColor );
	virtual void		DrawWireframeBox( Vector const &vMin, Vector const &vMax, Vector const &vColor );
	virtual void		DrawText( const char *pStr, const CTextParams &params );
	virtual void		SetRenderState( RenderState state, unsigned long val );
	virtual void		Clear();
	virtual void		Flush();
	virtual void		DrawImageBW( 
		unsigned char const *pData, 
		int width, 
		int height, 
		int pitchInBytes, 
		bool bOutlinePixels=true, 
		bool bOutlineImage=false,
		Vector *vCorners=NULL );
	
	// Draw an RGBA image.
	// Corners are in this order: bottom-left, top-left, top-right, bottom-right.
	virtual void		DrawImageRGBA( 
		SPRGBA *pData, 
		int width, 
		int height, 
		int pitchInBytes, 
		bool bOutlinePixels=true,
		bool bOutlineImage=false,
		Vector *vCorners=NULL );

	void DrawPolygonsForPixels(
			SPRGBA *pData, 
			int width, 
			int height, 
			int pitchInBytes, 
			Vector *vCorners );

public:
	IFileSystem*				m_pFileSystem;
	char const					*m_pFilename;
	CUtlVector<CBaseCommand*>	m_Commands;
	bool						m_bAutoFlush;
};


IFileSystem* ScratchPad3D_SetupFileSystem();


#endif // SCRATCHPAD3D_H
