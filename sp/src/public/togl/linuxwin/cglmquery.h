//========= Copyright Valve Corporation, All rights reserved. ============//
//
// cglmquery.h
//	GLMgr queries
//
//===============================================================================

#ifndef CGLMQUERY_H
#define	CGLMQUERY_H

#pragma once

#ifdef OSX
#include "glmgr/glmgrbasics.h"
#endif

//===============================================================================

// forward declarations

class	GLMContext;
class	CGLMQuery;

//===============================================================================

enum EGLMQueryType
{
	EOcclusion,
	EFence,
	EGLMQueryCount
};

struct GLMQueryParams
{
	EGLMQueryType	m_type;
};

class CGLMQuery
{
	// leave everything public til it's running
public:
	friend class GLMContext;			// only GLMContext can make CGLMTex objects
	friend struct IDirect3DDevice9;
	friend struct IDirect3DQuery9;
		
	GLMContext		*m_ctx;				// link back to parent context
	GLMQueryParams	m_params;			// params created with
	
	GLuint			m_name;				// name of the query object per se - could be fence, could be query object ... NOT USED WITH GL_ARB_sync!
#ifdef HAVE_GL_ARB_SYNC
	GLsync			m_syncobj;			// GL_ARB_sync object. NOT USED WITH GL_NV_fence or GL_APPLE_fence!
#else
	GLuint			m_syncobj;
#endif
	
	bool			m_started;
	bool			m_stopped;
	bool			m_done;
	
	bool			m_nullQuery;		// was gl_nullqueries true at Start time - if so, continue to act like a null query through Stop/IsDone/Complete time
										// restated - only Start should examine the convar.
	static uint		s_nTotalOcclusionQueryCreatesOrDeletes;

			CGLMQuery( GLMContext *ctx, GLMQueryParams *params );
			~CGLMQuery( );
			
	// for an occlusion query:
	//	Start = BeginQuery		query-start goes into stream
	//	Stop = EndQuery			query-end goes into stream - a fence is also set so we can probe for completion
	//	IsDone = TestFence		use the added fence to ask if query-end has passed (i.e. will Complete block?)
	//	Complete = GetQueryObjectuivARB(uint id, enum pname, uint *params) - extract the sample count

	// for a fence query:
	//	Start = SetFence		fence goes into command stream
	//	Stop = NOP				fences are self finishing - no need to call Stop on a fence
	//	IsDone = TestFence		ask if fence passed
	//	Complete = FinishFence

	void	Start		( void );
	void	Stop		( void );
	bool	IsDone		( void );
	void	Complete	( uint *result );

	// accessors for the started/stopped state
	bool	IsStarted	( void );
	bool	IsStopped	( void );
};


#endif
