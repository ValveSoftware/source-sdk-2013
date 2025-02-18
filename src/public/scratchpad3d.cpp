//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>
#include "scratchpad3d.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef POSIX
// NOTE - linux doesn't need any of this code!

extern "C"
{
	extern void __stdcall Sleep( unsigned long ms );
};


class CFileRead
{
public:
				CFileRead( IFileSystem* pFileSystem, FileHandle_t fp )
				{
					m_pFileSystem = pFileSystem;
					m_fp = fp;
					m_Pos = 0;
				}

	bool		Read( void *pDest, int len )
	{
		int count = m_pFileSystem->Read( pDest, len, m_fp );
		m_Pos += count;
		return count == len;
	}

	IFileSystem*	m_pFileSystem;
	FileHandle_t	m_fp;
	int				m_Pos;
};


// ------------------------------------------------------------------------ //
// CCommand_Point.
// ------------------------------------------------------------------------ //

void CScratchPad3D::CCommand_Point::Read( CFileRead *pFile )
{
	pFile->Read( &m_flPointSize, sizeof(m_flPointSize) );
	pFile->Read( &m_Vert, sizeof(m_Vert) );
}

void CScratchPad3D::CCommand_Point::Write( IFileSystem* pFileSystem, FileHandle_t fp )
{
	pFileSystem->Write( &m_flPointSize, sizeof(m_flPointSize), fp );
	pFileSystem->Write( &m_Vert, sizeof(m_Vert), fp );
}


// ------------------------------------------------------------------------ //
// CCommand_Line.
// ------------------------------------------------------------------------ //

void CScratchPad3D::CCommand_Line::Read( CFileRead *pFile )
{
	pFile->Read( m_Verts, sizeof(m_Verts) );
}

void CScratchPad3D::CCommand_Line::Write( IFileSystem* pFileSystem, FileHandle_t fp )
{
	pFileSystem->Write( m_Verts, sizeof(m_Verts), fp );
}


// ------------------------------------------------------------------------ //
// CCommand_Polygon.
// ------------------------------------------------------------------------ //

void CScratchPad3D::CCommand_Polygon::Read( CFileRead *pFile )
{
	int count;
	pFile->Read( &count, sizeof(count) );
	m_Verts.RemoveAll();
	m_Verts.AddMultipleToTail( count );
	
	if( count )
		pFile->Read( &m_Verts[0], sizeof(CSPVert)*count );
}

void CScratchPad3D::CCommand_Polygon::Write( IFileSystem* pFileSystem, FileHandle_t fp )
{
	int count = m_Verts.Size();
	pFileSystem->Write( &count, sizeof(count), fp );
	
	if( count )
		pFileSystem->Write( &m_Verts[0], sizeof(CSPVert)*count, fp );
}


// ------------------------------------------------------------------------ //
// CCommand_Matrix.
// ------------------------------------------------------------------------ //

void CScratchPad3D::CCommand_Matrix::Read( CFileRead *pFile )
{
	pFile->Read( &m_mMatrix, sizeof(m_mMatrix) );
}

void CScratchPad3D::CCommand_Matrix::Write( IFileSystem* pFileSystem, FileHandle_t fp )
{
	pFileSystem->Write( &m_mMatrix, sizeof(m_mMatrix), fp );
}


// ------------------------------------------------------------------------ //
// CCommand_RenderState.
// ------------------------------------------------------------------------ //

void CScratchPad3D::CCommand_RenderState::Read( CFileRead *pFile )
{
	pFile->Read( &m_State, sizeof(m_State) );
	pFile->Read( &m_Val, sizeof(m_Val) );
}

void CScratchPad3D::CCommand_RenderState::Write( IFileSystem* pFileSystem, FileHandle_t fp )
{
	pFileSystem->Write( &m_State, sizeof(m_State), fp );
	pFileSystem->Write( &m_Val, sizeof(m_Val), fp );
}


// ------------------------------------------------------------------------ //
// CCommand_Text.
// ------------------------------------------------------------------------ //

void CScratchPad3D::CCommand_Text::Read( CFileRead *pFile )
{
	int strLen;
	pFile->Read( &strLen, sizeof( strLen ) );
	m_String.SetSize( strLen );
	pFile->Read( m_String.Base(), strLen );

	pFile->Read( &m_TextParams, sizeof( m_TextParams ) );
}


void CScratchPad3D::CCommand_Text::Write( IFileSystem* pFileSystem, FileHandle_t fp )
{
	int strLen = m_String.Count();
	pFileSystem->Write( &strLen, sizeof( strLen ), fp );
	pFileSystem->Write( m_String.Base(), strLen, fp );

	pFileSystem->Write( &m_TextParams, sizeof( m_TextParams ), fp );
}


// ------------------------------------------------------------------------ //
// CScratchPad3D internals.
// ------------------------------------------------------------------------ //

CScratchPad3D::CScratchPad3D( char const *pFilename, IFileSystem* pFileSystem, bool bAutoClear )
{
	m_pFileSystem = pFileSystem;
	m_pFilename = pFilename;
	m_bAutoFlush = true;

	if( bAutoClear )
		Clear();	// Clear whatever is in the file..
}

void CScratchPad3D::AutoFlush()
{
	if( m_bAutoFlush )
		Flush();
}

void CScratchPad3D::DrawRectGeneric( int iPlane, int otherDim1, int otherDim2, float planeDist, const Vector2D &vMin, const Vector2D &vMax, const CSPColor &vColor )
{
	Vector verts[4];

	verts[0][iPlane] = verts[1][iPlane] = verts[2][iPlane] = verts[3][iPlane] = planeDist;

	verts[0][otherDim1] = vMin.x;
	verts[0][otherDim2] = vMin.y;

	verts[1][otherDim1] = vMin.x;
	verts[1][otherDim2] = vMax.y;

	verts[2][otherDim1] = vMax.x;
	verts[2][otherDim2] = vMax.y;

	verts[3][otherDim1] = vMax.x;
	verts[3][otherDim2] = vMin.y;

	DrawPolygon( CSPVertList(verts, 4, vColor) );
}

void CScratchPad3D::DeleteCommands()
{
	for( int i=0; i < m_Commands.Size(); i++ )
		delete m_Commands[i];

	m_Commands.RemoveAll();
}

bool CScratchPad3D::LoadCommandsFromFile( )
{
	DeleteCommands();

	FileHandle_t fp = m_pFileSystem->Open( m_pFilename, "rb" );
	if( !fp )
		return false;

	long fileEndPos = m_pFileSystem->Size( fp );

	CFileRead fileRead( m_pFileSystem, fp );
	while( fileRead.m_Pos != fileEndPos )
	{
		unsigned char iCommand;
		fileRead.Read( &iCommand, sizeof(iCommand) );
		
		CBaseCommand *pCmd = NULL;
		if( iCommand == COMMAND_POINT )
			pCmd = new CCommand_Point;
		else if( iCommand == COMMAND_LINE )
			pCmd = new CCommand_Line;
		else if( iCommand == COMMAND_POLYGON )
			pCmd = new CCommand_Polygon;
		else if( iCommand == COMMAND_MATRIX )
			pCmd = new CCommand_Matrix;
		else if( iCommand == COMMAND_RENDERSTATE )
			pCmd = new CCommand_RenderState;
		else if ( iCommand == COMMAND_TEXT )
			pCmd = new CCommand_Text;

		if( !pCmd )
		{
			Assert( !"LoadCommandsFromFile: invalid file" );
			m_pFileSystem->Close( fp );
			return false;
		}

		pCmd->Read( &fileRead );
		m_Commands.AddToTail( pCmd );
	}	

	m_pFileSystem->Close( fp );
	return true;
}


// ------------------------------------------------------------------------ //
// CScratchPad3D's IScratchPad3D implementation.
// ------------------------------------------------------------------------ //

void CScratchPad3D::Release()
{
	Flush();
	delete this;
}

void CScratchPad3D::SetMapping( 
	const Vector &vInputMin, 
	const Vector &vInputMax,
	const Vector &vOutputMin,
	const Vector &vOutputMax )
{
	CCommand_Matrix *cmd = new CCommand_Matrix;
	m_Commands.AddToTail( cmd );
	
	Vector vDivisor(1,1,1);
	for( int i=0; i < 3; i++ )
		vDivisor[i] = fabs(vInputMax[i] - vInputMin[i]) < 0.0001f ? 0.001f : (vInputMax[i] - vInputMin[i]);

	Vector vScale = (vOutputMax - vOutputMin) / vDivisor; 
	Vector vShift = -vInputMin * vScale + vOutputMin;

	cmd->m_mMatrix.Init( 
		vScale.x,		0,			0,			vShift.x,
		0,				vScale.y,	0,			vShift.y,
		0,				0,			vScale.z,	vShift.z,
		0,				0,			0,			1 );

	
	AutoFlush();
}

bool CScratchPad3D::GetAutoFlush()
{
	return m_bAutoFlush;
}

void CScratchPad3D::SetAutoFlush( bool bAutoFlush )
{
	m_bAutoFlush = bAutoFlush;
	if( m_bAutoFlush )
		Flush();
}

void CScratchPad3D::DrawPoint( CSPVert const &v, float flPointSize )
{
	CCommand_Point *cmd = new CCommand_Point;
	m_Commands.AddToTail( cmd );

	cmd->m_Vert = v;
	cmd->m_flPointSize = flPointSize;

	AutoFlush();
}

void CScratchPad3D::DrawLine( CSPVert const &v1, CSPVert const &v2 )
{
	CCommand_Line *cmd = new CCommand_Line;
	m_Commands.AddToTail( cmd );

	cmd->m_Verts[0] = v1;
	cmd->m_Verts[1] = v2;

	AutoFlush();
}

void CScratchPad3D::DrawPolygon( CSPVertList const &verts )
{
	CCommand_Polygon *cmd = new CCommand_Polygon;
	m_Commands.AddToTail( cmd );

	cmd->m_Verts.AddVectorToTail( verts.m_Verts );
	
	AutoFlush();
}

void CScratchPad3D::DrawRectYZ( float xPos, const Vector2D &vMin, const Vector2D &vMax, const CSPColor &vColor )
{
	DrawRectGeneric( 0, 1, 2, xPos, vMin, vMax, vColor );
}

void CScratchPad3D::DrawRectXZ( float yPos, const Vector2D &vMin, const Vector2D &vMax, const CSPColor &vColor )
{
	DrawRectGeneric( 1, 0, 2, yPos, vMin, vMax, vColor );
}

void CScratchPad3D::DrawRectXY( float zPos, const Vector2D &vMin, const Vector2D &vMax, const CSPColor &vColor )
{
	DrawRectGeneric( 2, 0, 1, zPos, vMin, vMax, vColor );
}

void CScratchPad3D::SetRenderState( RenderState state, unsigned long val )
{
	CCommand_RenderState *cmd = new CCommand_RenderState;
	m_Commands.AddToTail( cmd );

	cmd->m_State = (unsigned long)state;
	cmd->m_Val = val;
}

void CScratchPad3D::DrawWireframeBox( const Vector &vMin, const Vector &vMax, const Vector &vColor )
{
	// Bottom 4.
	DrawLine( 
		CSPVert(Vector(vMin.x, vMin.y, vMin.z), vColor), 
		CSPVert(Vector(vMax.x, vMin.y, vMin.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMin.x, vMin.y, vMin.z), vColor), 
		CSPVert(Vector(vMin.x, vMax.y, vMin.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMax.x, vMin.y, vMin.z), vColor), 
		CSPVert(Vector(vMax.x, vMax.y, vMin.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMax.x, vMax.y, vMin.z), vColor), 
		CSPVert(Vector(vMin.x, vMax.y, vMin.z), vColor) );

	// Top 4.
	DrawLine( 
		CSPVert(Vector(vMin.x, vMin.y, vMax.z), vColor), 
		CSPVert(Vector(vMax.x, vMin.y, vMax.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMin.x, vMin.y, vMax.z), vColor), 
		CSPVert(Vector(vMin.x, vMax.y, vMax.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMax.x, vMin.y, vMax.z), vColor), 
		CSPVert(Vector(vMax.x, vMax.y, vMax.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMax.x, vMax.y, vMax.z), vColor), 
		CSPVert(Vector(vMin.x, vMax.y, vMax.z), vColor) );

	// Connecting 4.
	DrawLine( 
		CSPVert(Vector(vMin.x, vMin.y, vMin.z), vColor), 
		CSPVert(Vector(vMin.x, vMin.y, vMax.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMin.x, vMax.y, vMin.z), vColor), 
		CSPVert(Vector(vMin.x, vMax.y, vMax.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMax.x, vMax.y, vMin.z), vColor), 
		CSPVert(Vector(vMax.x, vMax.y, vMax.z), vColor) );

	DrawLine( 
		CSPVert(Vector(vMax.x, vMin.y, vMin.z), vColor), 
		CSPVert(Vector(vMax.x, vMin.y, vMax.z), vColor) );
}


void CScratchPad3D::DrawText( const char *pStr, const CTextParams &params )
{
	CCommand_Text *cmd = new CCommand_Text;
	m_Commands.AddToTail( cmd );

	cmd->m_String.CopyArray( pStr, V_strlen( pStr ) + 1 );
	cmd->m_TextParams = params;

	AutoFlush();
}


void CScratchPad3D::Clear()
{
	FileHandle_t fp;
	
	while( ( fp = m_pFileSystem->Open(m_pFilename, "wb") ) == NULL )
	{
#ifdef _WIN32
		Sleep( 5 );
#elif POSIX
		usleep( 5 );
#endif
	}

	m_pFileSystem->Close( fp );

	DeleteCommands();
}


void CScratchPad3D::Flush()
{
	FileHandle_t fp;

	while( ( fp = m_pFileSystem->Open(m_pFilename, "ab+") ) == NULL )
	{
#ifdef _WIN32
		Sleep( 5 );
#elif POSIX
		usleep( 5 );
#endif
	}
	
	// Append the new commands to the file.
	for( int i=0; i < m_Commands.Size(); i++ )
	{
		m_pFileSystem->Write( &m_Commands[i]->m_iCommand, sizeof(m_Commands[i]->m_iCommand), fp );
		m_Commands[i]->Write( m_pFileSystem, fp );
	}
	
	m_pFileSystem->Close( fp );

	DeleteCommands();
}

void CScratchPad3D::DrawImageBW(
		unsigned char const *pData, 
		int width, 
		int height, 
		int pitchInBytes, 
		bool bOutlinePixels, 
		bool bOutlineImage,
		Vector *vCorners )
{
	SPRGBA *pRGBA = new SPRGBA[width*height];
	for( int y=0; y < height; y++ )
	{
		SPRGBA *pDest = &pRGBA[ y * width ];
		unsigned char const *pSrc = &pData[ y * pitchInBytes ];
		for( int x=0; x < width; x++ )
		{
			pDest->r = pDest->g = pDest->b = *pSrc;
			++pSrc;
			++pDest;
		}
	}

	DrawImageRGBA( pRGBA, width, height, width*sizeof(SPRGBA), bOutlinePixels, bOutlineImage, vCorners );
	delete [] pRGBA;
}


void CScratchPad3D::DrawPolygonsForPixels(
		SPRGBA *pData, 
		int width, 
		int height, 
		int pitchInBytes, 
		Vector *vCorners )
{
	// Scan top-down.
	Vector vCurLeft = vCorners[1];
	Vector vCurRight = vCorners[2];
	
	Vector vLeftInc = (vCorners[0] - vCorners[1]) / height;
	Vector vRightInc = (vCorners[3] - vCorners[2]) / height;
	
	Vector vNextLeft = vCurLeft + vLeftInc;
	Vector vNextRight = vCurRight + vRightInc;

	Vector vPolyBox[4];
	Vector &vTopLeft = vPolyBox[0];
	Vector &vTopRight = vPolyBox[1];
	Vector &vBottomRight = vPolyBox[2];
	Vector &vBottomLeft = vPolyBox[3];

	for( int y=0; y < height; y++ )
	{
		vTopLeft = vCurLeft;
		vBottomLeft = vNextLeft;
		
		Vector vTopXInc = (vCurRight - vCurLeft) / width;
		Vector vBottomXInc = (vNextRight - vNextLeft) / width;

		vTopRight = vTopLeft + vTopXInc;
		vBottomRight = vBottomLeft + vBottomXInc;

		SPRGBA *pSrc = &pData[ y * (pitchInBytes/sizeof(SPRGBA)) ];
		for( int x=0; x < width; x++ )
		{
			if ( pData )
				DrawPolygon( CSPVertList( vPolyBox, 4, Vector(pSrc->r/255.1f, pSrc->g/255.1f, pSrc->b/255.1f) ) );
			else
				DrawPolygon( CSPVertList( vPolyBox, 4, Vector(1,1,1) ) );
			
			++pSrc;
			vTopLeft += vTopXInc;
			vTopRight += vTopXInc;
			vBottomLeft += vBottomXInc;
			vBottomRight += vBottomXInc;
		}

		vCurLeft += vLeftInc;
		vNextLeft += vLeftInc;
		vCurRight += vRightInc;
		vNextRight += vRightInc;
	}
}


void CScratchPad3D::DrawImageRGBA(
		SPRGBA *pData, 
		int width, 
		int height, 
		int pitchInBytes, 
		bool bOutlinePixels,
		bool bOutlineImage,
		Vector *vCorners )
{
	Assert( pitchInBytes % sizeof(SPRGBA) == 0 );

	Vector vDefaultCorners[4];
	if ( !vCorners )
	{
		vCorners = vDefaultCorners;
		vDefaultCorners[0].Init( -100, -100 );
		vDefaultCorners[1].Init( -100, 100 );
		vDefaultCorners[2].Init( 100,  100 );
		vDefaultCorners[3].Init( 100, -100 );
	}

	// Don't auto-flush while drawing all these primitives.
	bool bOldAutoFlush = m_bAutoFlush;
	m_bAutoFlush = false;

	// Draw solids.
	SetRenderState( IScratchPad3D::RS_FillMode, IScratchPad3D::FillMode_Solid );
	DrawPolygonsForPixels( pData, width, height, pitchInBytes, vCorners );

	// Draw wireframe.
	if ( bOutlinePixels )
	{
		SetRenderState( IScratchPad3D::RS_FillMode, IScratchPad3D::FillMode_Wireframe );
		DrawPolygonsForPixels( NULL, width, height, pitchInBytes, vCorners );
	}

	// Draw an outline around the whole image.
	if ( bOutlineImage )
	{
		SetRenderState( IScratchPad3D::RS_FillMode, IScratchPad3D::FillMode_Wireframe );
		DrawPolygon( CSPVertList( vCorners, 4 ) );
	}

	// Restore the old auto-flush state.
	m_bAutoFlush = bOldAutoFlush;
	AutoFlush();
}


// ------------------------------------------------------------------------ //
// Global functions.
// ------------------------------------------------------------------------ //
IFileSystem* ScratchPad3D_SetupFileSystem()
{
	// Get a filesystem interface.
	CSysModule *pModule = Sys_LoadModule( "filesystem_stdio" );
	if( !pModule )
		return NULL;

	CreateInterfaceFn fn = Sys_GetFactory( pModule );
	IFileSystem *pFileSystem;
	if( !fn || (pFileSystem = (IFileSystem *)fn( FILESYSTEM_INTERFACE_VERSION, NULL )) == NULL )
	{
		Sys_UnloadModule( pModule );
		return NULL;
	}

	return pFileSystem;
}

IScratchPad3D* ScratchPad3D_Create( char const *pFilename )
{
	IFileSystem *pFileSystem = ScratchPad3D_SetupFileSystem();
	if( !pFileSystem )
		return NULL;

	CScratchPad3D *pRet = new CScratchPad3D( pFilename, pFileSystem, true );
	return pRet;
}
#endif // POSIX

