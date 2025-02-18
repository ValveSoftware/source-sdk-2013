//========= Copyright Valve Corporation, All rights reserved. ============//
//
// cglmprogram.h
//	GLMgr programs (ARBVP/ARBfp)
//
//===============================================================================

#ifndef CGLMPROGRAM_H
#define	CGLMPROGRAM_H

#include <sys/stat.h>

#pragma once

// good ARB program references
// http://petewarden.com/notes/archives/2005/05/fragment_progra_2.html
// http://petewarden.com/notes/archives/2005/06/fragment_progra_3.html

// ext links

// http://www.opengl.org/registry/specs/ARB/vertex_program.txt
// http://www.opengl.org/registry/specs/ARB/fragment_program.txt
// http://www.opengl.org/registry/specs/EXT/gpu_program_parameters.txt


//===============================================================================

// tokens not in the SDK headers

//#ifndef	GL_DEPTH_STENCIL_ATTACHMENT_EXT
//	#define GL_DEPTH_STENCIL_ATTACHMENT_EXT 0x84F9
//#endif

//===============================================================================

// forward declarations

class GLMContext;
class CGLMShaderPair;
class CGLMShaderPairCache;

// CGLMProgram can contain two flavors of the same program, one in assembler, one in GLSL.
// these flavors are pretty different in terms of the API's that are used to activate them - 
// for example, assembler programs can just get bound to the context, whereas GLSL programs
// have to be linked.  To some extent we try to hide that detail inside GLM.

// for now, make CGLMProgram a container, it does not set policy or hold a preference as to which
// flavor you want to use.  GLMContext has to handle that. 

enum EGLMProgramType
{
	kGLMVertexProgram,
	kGLMFragmentProgram,
	
	kGLMNumProgramTypes
};

enum EGLMProgramLang
{
	kGLMARB,
	kGLMGLSL,
	
	kGLMNumProgramLangs
};

struct GLMShaderDesc
{
	union
	{
		GLuint		arb;		// ARB program object name
		GLhandleARB	glsl;		// GLSL shader object handle (void*)
	}	m_object;

	// these can change if shader text is edited
	bool	m_textPresent;	// is this flavor(lang) of text present in the buffer?
	int		m_textOffset;	// where is it
	int		m_textLength;	// how big
	
	bool	m_compiled;		// has this text been through a compile attempt
	bool	m_valid;		// and if so, was the compile successful

	int		m_slowMark;		// has it been flagged during a non native draw batch before. increment every time it's slow.
	
	int		m_highWater;	// vount of vec4's in the major uniform array ("vc" on vs, "pc" on ps)
							// written by dxabstract.... gross!
};

GLenum	GLMProgTypeToARBEnum( EGLMProgramType type );	// map vert/frag to ARB asm bind target
GLenum	GLMProgTypeToGLSLEnum( EGLMProgramType type );	// map vert/frag to ARB asm bind target

class CGLMProgram
{
public:
	friend class CGLMShaderPairCache;
	friend class CGLMShaderPair;
	friend class GLMContext;			// only GLMContext can make CGLMProgram objects
	friend class GLMTester;	
	friend class IDirect3D9;
	friend class IDirect3DDevice9;
		
	//===============================
	
	// constructor is very light, it just makes one empty program object per flavor.
	CGLMProgram( GLMContext *ctx, EGLMProgramType type );
	~CGLMProgram( );	

	void	SetProgramText			( char *text );				// import text to GLM object - invalidate any prev compiled program
	
	bool	CompileActiveSources	( void );					// compile only the flavors that were provided.
	bool	Compile					( EGLMProgramLang lang );	
	bool	CheckValidity			( EGLMProgramLang lang );

	void	LogSlow					( EGLMProgramLang lang );	// detailed spew when called for first time; one liner or perhaps silence after that
	
	void	GetLabelIndexCombo		( char *labelOut, int labelOutMaxChars, int *indexOut, int *comboOut );	
	void	GetComboIndexNameString	( char *stringOut, int stringOutMaxChars );		// mmmmmmmm-nnnnnnnn-filename
	
#if GLMDEBUG
	bool	PollForChanges( void );			// check mirror for changes.
	void	ReloadStringFromEditable( void );	// populate m_string from editable item (react to change)
	bool	SyncWithEditable( void );
#endif

	//===============================
	
	// common stuff

	GLMContext				*m_ctx;					// link back to parent context

	EGLMProgramType			m_type;					// vertex or pixel

	uint					m_serial;				// serial number for hashing
	
	char					*m_text;				// copy of text passed into constructor.  Can change if editable shaders is enabled.
													// note - it can contain multiple flavors, so use CGLMTextSectioner to scan it and locate them
#if GLMDEBUG
	CGLMEditableTextItem	*m_editable;			// editable text item for debugging
#endif	
	
	GLMShaderDesc			m_descs[ kGLMNumProgramLangs ];	

	uint					m_samplerMask;			// (1<<n) mask of sampler active locs, if this is a fragment shader (dxabstract sets this field)
};	

//===============================================================================

struct GLMShaderPairInfo
{
	int		m_status;		// -1 means req'd index was out of bounds (loop stop..)  0 means not present.  1 means present/active.
	
	char	m_vsName[ 128 ];
	int		m_vsStaticIndex;
	int		m_vsDynamicIndex;
	
	char	m_psName[ 128 ];
	int		m_psStaticIndex;
	int		m_psDynamicIndex;
};


class CGLMShaderPair					// a container for a linked GLSL shader pair, and metadata obtained post-link
{

public:

	friend class CGLMProgram;
	friend class GLMContext;
	friend class CGLMShaderPairCache;
		
	//===============================
	
	// constructor just sets up a GLSL program object and leaves it empty.
	CGLMShaderPair( GLMContext *ctx  );
	~CGLMShaderPair( );	

	bool	SetProgramPair			( CGLMProgram *vp, CGLMProgram *fp );
		// true result means successful link and query

	bool	RefreshProgramPair		( void );
		// re-link and re-query the uniforms
	
	//===============================
	
	// common stuff

	GLMContext				*m_ctx;					// link back to parent context

	CGLMProgram				*m_vertexProg;	
	CGLMProgram				*m_fragmentProg;

	GLhandleARB				m_program;				// linked program object

	// need meta data for attribs / samplers / params
	// actually we only need it for samplers and params.
	// attributes are hardwired.
	
	// vertex stage uniforms
	GLint					m_locVertexParams;		// "vc" per dx9asmtogl2 convention
	GLint					m_locVertexInteger0;	// "i0"
	GLint					m_locVertexBool0;		// "b0"
	GLint					m_locVertexBool1;		// "b1"
	GLint					m_locVertexBool2;		// "b2"
	GLint					m_locVertexBool3;		// "b3"
	
	// fragment stage uniforms
	GLint					m_locFragmentParams;			// "pc" per dx9asmtogl2 convention
	GLint					m_locFragmentFakeSRGBEnable;	// "flSRGBWrite" - set to 1.0 to effect sRGB encoding on output
	float					m_fakeSRGBEnableValue;			// shadow to avoid redundant sets of the m_locFragmentFakeSRGBEnable uniform
		// init it to -1.0 at link or relink, so it will trip on any legit incoming value (0.0 or 1.0)

	GLint					m_locSamplers[ 16 ];			// "sampler0 ... sampler1..."

	// other stuff
	bool					m_valid;				// true on successful link
	bool					m_samplersFixed;		// set on first draw (can't write the uniforms until the program is in use, and we don't want to mess with cur program inside cglmprogram)
	uint					m_revision;				// if this pair is relinked, bump this number.
};	

//===============================================================================

// N-row, M-way associative cache with LRU per row.
// still needs some metric dump ability and some parameter tuning.
// extra credit would be to make an auto-tuner.

struct CGLMPairCacheEntry
{
	long long		m_lastMark;				// a mark of zero means an empty entry
	CGLMProgram		*m_vertexProg;
	CGLMProgram		*m_fragmentProg;
	uint			m_extraKeyBits;
	CGLMShaderPair	*m_pair;
};

class CGLMShaderPairCache				// cache for linked GLSL shader pairs
{

public:

protected:
	friend class CGLMShaderPair;
	friend class CGLMProgram;
	friend class GLMContext;
		
	//===============================
	
	CGLMShaderPairCache( GLMContext *ctx  );
	~CGLMShaderPairCache( );	

	CGLMShaderPair	*SelectShaderPair	( CGLMProgram *vp, CGLMProgram *fp, uint extraKeyBits );
	void			QueryShaderPair		( int index, GLMShaderPairInfo *infoOut );
	
	// shoot down linked pairs that use the program in the arg
	// return true if any had to be skipped due to conflict with currently bound pair
	bool			PurgePairsWithShader( CGLMProgram *prog );
	
	// purge everything (when would GLM know how to do this ?  at context destroy time, but any other times?)
	// return true if any had to be skipped due to conflict with currently bound pair
	bool			Purge				( void );
	
	// stats
	void			DumpStats			( void );
	
	//===============================

	uint			HashRowIndex		( CGLMProgram *vp, CGLMProgram *fp, uint extraKeyBits );
	CGLMPairCacheEntry*	HashRowPtr		( uint hashRowIndex );
	void			HashRowProbe		( CGLMPairCacheEntry *row, CGLMProgram *vp, CGLMProgram *fp, uint extraKeyBits, int *hitwayOut, int *emptywayOut, int *oldestwayOut );
	//===============================

	// common stuff

	GLMContext				*m_ctx;					// link back to parent context

	long long				m_mark;

	uint					m_rowsLg2;
	uint					m_rows;
	
	uint					m_waysLg2;
	uint					m_ways;
	
	uint					m_entryCount;
	
	CGLMPairCacheEntry		*m_entries;				// array[ m_rows ][ m_ways ]

	uint					*m_evictions;			// array[ m_rows ];
	uint					*m_hits;				// array[ m_rows ];
};	


#endif