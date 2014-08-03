//========= Copyright Valve Corporation, All rights reserved. ============//
//
// glmgrbasics.h
//	types, common headers, forward declarations, utilities
//
//===============================================================================

#ifndef GLMBASICS_H
#define	GLMBASICS_H

#pragma once

#ifdef OSX
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>
	#include <OpenGL/CGLTypes.h>
	#include <OpenGL/CGLRenderers.h>
	#include <OpenGL/CGLCurrent.h>
	#include <OpenGL/CGLProfiler.h>
	#include <ApplicationServices/ApplicationServices.h>
#elif defined(DX_TO_GL_ABSTRACTION)
	#include <GL/gl.h>
	#include <GL/glext.h>
#else
	#error
#endif

#include "tier0/platform.h"

#include "bitmap/imageformat.h"
#include "bitvec.h"
#include "tier1/checksum_md5.h"
#include "tier1/utlvector.h"
#include "tier1/convar.h"

#include <sys/stat.h>

#include "dxabstract_types.h"

struct GLMRect;
typedef void *PseudoGLContextPtr;

// types

	// 3-d integer box (used for texture lock/unlock etc)
struct	GLMRegion
{
	int	xmin,xmax;
	int	ymin,ymax;
	int zmin,zmax;
};

struct GLMRect	// follows GL convention - if coming from the D3D rect you will need to fiddle the Y's
{
    int xmin;	// left
    int ymin;	// bottom
    int xmax;	// right
    int ymax;	// top
};

// macros

//#define	GLMassert(x)	assert(x)

// forward decls
class	GLMgr;				// singleton
class	GLMContext;			// GL context
class	CGLMContextTester;	// testing class
class	CGLMTex;
class	CGLMFBO;
class	CGLMProgram;
class	CGLMBuffer;


// utilities

typedef enum
{
	// D3D codes
	eD3D_DEVTYPE,
	eD3D_FORMAT,
	eD3D_RTYPE,
	eD3D_USAGE,
	eD3D_RSTATE,	// render state
	eD3D_SIO,		// D3D shader bytecode
	eD3D_VTXDECLUSAGE,
	
	// CGL codes
	eCGL_RENDID,
	
	// OpenGL error codes
	eGL_ERROR,
	
	// OpenGL enums
	eGL_ENUM,
	eGL_RENDERER

}	GLMThing_t;

// these will look at the string to guess its flavor: <, >, ---, -M-, -S- 
#ifdef TOGL_DLL_EXPORT
	DLL_EXPORT const char* GLMDecode( GLMThing_t type, unsigned long value );		// decode a numeric const
#else
	DLL_IMPORT const char* GLMDecode( GLMThing_t type, unsigned long value );		// decode a numeric const
#endif
		
const char* GLMDecodeMask( GLMThing_t type, unsigned long value );	// decode a bitmask

FORCEINLINE void GLMStop( void ) { DXABSTRACT_BREAK_ON_ERROR(); }

void GLMEnableTrace( bool on );

//===============================================================================
// output functions

// expose these in release now
// Mimic PIX events so we can decorate debug spew
DLL_EXPORT void	GLMBeginPIXEvent( const char *str );
DLL_EXPORT void	GLMEndPIXEvent( void );

class CScopedGLMPIXEvent
{
	CScopedGLMPIXEvent( const CScopedGLMPIXEvent & );
	CScopedGLMPIXEvent& operator= ( const CScopedGLMPIXEvent & );
public:
	inline CScopedGLMPIXEvent( const char *pName ) { GLMBeginPIXEvent( pName ); }
	inline ~CScopedGLMPIXEvent() { GLMEndPIXEvent( ); }
};

#if GLMDEBUG


//===============================================================================
// classes

// helper class making function tracking easier to wire up

class GLMFuncLogger
{
	public:
	
		// simple function log
		GLMFuncLogger( const char *funcName )
		{
			m_funcName = funcName;
			m_earlyOut = false;
			
			GLMPrintf( ">%s", m_funcName );
		};
		
		// more advanced version lets you pass args (i.e. called parameters or anything else of interest)
		// no macro for this one, since no easy way to pass through the args as well as the funcname
		GLMFuncLogger( const char *funcName, char *fmt, ... )
		{
			m_funcName = funcName;
			m_earlyOut = false;

			// this acts like GLMPrintf here
			// all the indent policy is down in GLMPrintfVA
			// which means we need to inject a ">" at the front of the format string to make this work... sigh.
			
			char modifiedFmt[2000];
			modifiedFmt[0] = '>';
			strcpy( modifiedFmt+1, fmt );
			
			va_list	vargs;
			va_start(vargs, fmt);
			GLMPrintfVA( modifiedFmt, vargs );
			va_end( vargs );
		}
		
		~GLMFuncLogger( )
		{
			if (m_earlyOut)
			{
				GLMPrintf( "<%s (early out)", m_funcName );
			}
			else
			{
				GLMPrintf( "<%s", m_funcName );
			}
		};
	
		void EarlyOut( void )
		{
			m_earlyOut = true;
		};
		
		const char *m_funcName;				// set at construction time
		bool m_earlyOut;
};

// handy macro to go with the function tracking class
#define	GLM_FUNC			GLMFuncLogger _logger_ ( __FUNCTION__ )
#else
#define	GLM_FUNC            tmZone( TELEMETRY_LEVEL1, TMZF_NONE, "%s", __FUNCTION__ )
#endif


// class to keep an in-memory mirror of a file which may be getting edited during run
class CGLMFileMirror
{
public:
	CGLMFileMirror( char *fullpath );		// just associates mirror with file. if file exists it will be read.
											//if non existent it will be created with size zero
	~CGLMFileMirror( );
	
	bool		HasData( void );								// see if data avail
	void		GetData( char **dataPtr, uint *dataSizePtr );	// read it out
	void		SetData( char *data, uint dataSize );			// put data in (and write it to disk)
	bool		PollForChanges( void );							// check disk copy.  If different, read it back in and return true.
	
	void		UpdateStatInfo( void );		// make sure stat info is current for our file
	void		ReadFile( void );
	void		WriteFile( void );

	void		OpenInEditor( bool foreground=false );			// pass TRUE if you would like the editor to pop to foreground
	
	/// how about a "wait for change" method..

	char		*m_path;	// fullpath to file
	bool		m_exists;
	struct stat	m_stat;		// stat results for the file (last time checked)
	
	char		*m_data;	// content of file
	uint		m_size;		// length of content

};

// class based on the file mirror, that makes it easy to edit them outside the app.

// it receives an initial block of text from the engine, and hashes it. ("orig")
// it munges it by duplicating all the text after the "!!" line, and appending it in commented form. ("munged")
// a mirror file is activated, using a filename based on the hash from the orig text.
// if there is already content on disk matching that filename, use that content *unless* the 'blitz' parameter is set.
//		(i.e. engine is instructing this subsystem to wipe out any old/modified variants of the text)


class CGLMEditableTextItem
{
public:
					CGLMEditableTextItem( char *text, uint size, bool forceOverwrite, char *prefix, char *suffix = NULL );		// create a text blob from text source, optional filename suffix
					~CGLMEditableTextItem( );

	bool			HasData( void );
	bool			PollForChanges( void );									// return true if stale i.e. you need to get a new edition
	void			GetCurrentText( char **textOut, uint *sizeOut );		// query for read access to the active blob (could be the original, could be external edited copy)
	void			OpenInEditor( bool foreground=false );									// call user attention to this text

	// internal methods
	void			GenHashOfOrigText( void );
	void			GenBaseNameAndFullPath( char *prefix, char *suffix );
	void			GenMungedText( bool fromMirror );

	// members
	// orig
	uint			m_origSize;
	char			*m_origText;						// what was submitted
	unsigned char	m_origDigest[MD5_DIGEST_LENGTH];	// digest of what was submitted
	
	// munged
	uint			m_mungedSize;
	char			*m_mungedText;						// re-processed edition, initial content submission to the file mirror

	// mirror
	char			*m_mirrorBaseName;					// generated from the hash of the orig text, plus the label / prefix
	char			*m_mirrorFullPath;					// base name 
	CGLMFileMirror	*m_mirror;							// file mirror itself.  holds "official" copy for GetCurrentText to return.
};


// debug font
extern unsigned char g_glmDebugFontMap[16384];

// class for cracking multi-part text blobs
// sections are demarcated by beginning-of-line markers submitted in a table by the caller

struct GLMTextSection
{
	int		m_markerIndex;		// based on table of markers passed in to constructor
	uint	m_textOffset;		// where is the text - offset
	int		m_textLength;		// how big is the section
};

class CGLMTextSectioner
{
public:
					CGLMTextSectioner( char *text, int textSize, const char **markers );		// constructor finds all the sections
					~CGLMTextSectioner( );
					
	int				Count( void );			// how many sections found
	void			GetSection( int index, uint *offsetOut, uint *lengthOut, int *markerIndexOut );
		// find section, size, what marker
		// note that more than one section can be marked similarly.
		// so policy isn't made here, you walk the sections and decide what to do if there are dupes.
	
	//members
	
	//section table
	CUtlVector< GLMTextSection >	m_sectionTable;
};

void GLMGPUTimestampManagerInit();
void GLMGPUTimestampManagerDeinit();
void GLMGPUTimestampManagerTick();

#endif // GLMBASICS_H
