//========= Copyright Valve Corporation, All rights reserved. ============//
//
// glentrypoints.h
//
//===============================================================================

#ifndef GLENTRYPOINTS_H
#define GLENTRYPOINTS_H

#pragma once

#ifdef DX_TO_GL_ABSTRACTION

#include "tier0/platform.h"
#include "tier0/dynfunction.h"
#include "tier0/vprof_telemetry.h"
#include "interface.h"

#include "togl/rendermechanism.h"

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef CALLBACK
#define CALLBACK
#endif


void *VoidFnPtrLookup_GlMgr(const char *fn, bool &okay, const bool bRequired, void *fallback=NULL);

#if GL_TELEMETRY_ZONES || GL_TRACK_API_TIME
class CGLExecuteHelperBase
{
public:
	inline void StartCall(const char *pName);
	inline void StopCall(const char *pName);
#if GL_TRACK_API_TIME
	TmU64 m_nStartTime;
#endif
};

template < class FunctionType, typename Result >
class CGLExecuteHelper : public CGLExecuteHelperBase
{
public:
	inline CGLExecuteHelper(FunctionType pFn, const char *pName ) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(); StopCall(pName); }
	template<typename A> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a); StopCall(pName); }
	template<typename A, typename B> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b); StopCall(pName); }
	template<typename A, typename B, typename C> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c); StopCall(pName); }
	template<typename A, typename B, typename C, typename D> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c, d); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c, d, e); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c, d, e, f); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c, d, e, f, g); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g, H h) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c, d, e, f, g, h); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g, H h, I i) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c, d, e, f, g, h, i); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) : m_pFn( pFn ) { StartCall(pName); m_Result = (*m_pFn)(a, b, c, d, e, f, g, h, i, j); StopCall(pName); }

	inline operator Result() const { return m_Result; }
	inline operator char*() const { return (char*)m_Result; }
		
	FunctionType m_pFn;
	
	Result m_Result;
};

template < class FunctionType>
class CGLExecuteHelper<FunctionType, void> : public CGLExecuteHelperBase
{
public:
	inline CGLExecuteHelper(FunctionType pFn, const char *pName ) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(); StopCall(pName); }
	template<typename A> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a); StopCall(pName); }
	template<typename A, typename B> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b); StopCall(pName); }
	template<typename A, typename B, typename C> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c); StopCall(pName); }
	template<typename A, typename B, typename C, typename D> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c, d); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c, d, e); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c, d, e, f); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c, d, e, f, g); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g, H h) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c, d, e, f, g, h); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g, H h, I i) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c, d, e, f, g, h, i); StopCall(pName); }
	template<typename A, typename B, typename C, typename D, typename E, typename F, typename G, typename H, typename I, typename J> inline CGLExecuteHelper(FunctionType pFn, const char *pName, A a, B b, C c, D d, E e, F f, G g, H h, I i, J j) : m_pFn( pFn ) { StartCall(pName); (*m_pFn)(a, b, c, d, e, f, g, h, i, j); StopCall(pName); }
		
	FunctionType m_pFn;
};
#endif

template < class FunctionType, typename Result >
class CDynamicFunctionOpenGLBase
{
public:
	// Construct with a NULL function pointer. You must manually call
	//  Lookup() before you can call a dynamic function through this interface.
	CDynamicFunctionOpenGLBase() : m_pFn(NULL) {}

	// Construct and do a lookup right away. You will need to make sure that
	//  the lookup actually succeeded, as the gl library might have failed to load
	//  or (fn) might not exist in it.
	CDynamicFunctionOpenGLBase(const char *fn, FunctionType fallback=NULL) : m_pFn(NULL)
	{
		Lookup(fn, fallback);
	}

	// Construct and do a lookup right away. See comments in Lookup() about what (okay) does.
	CDynamicFunctionOpenGLBase(const char *fn, bool &okay, FunctionType fallback=NULL) : m_pFn(NULL)
	{
		Lookup(fn, okay, fallback);
	}

	// Load library if necessary, look up symbol. Returns true and sets
	//  m_pFn on successful lookup, returns false otherwise. If the
	//  function pointer is already looked up, this return true immediately.
	// Use Reset() first if you want to look up the symbol again.
	//  This function will return false immediately unless (okay) is true.
	//  This allows you to chain lookups like this:
	//     bool okay = true;
	//     x.Lookup(lib, "x", okay);
	//     y.Lookup(lib, "y", okay);
	//     z.Lookup(lib, "z", okay);
	//     if (okay) { printf("All functions were loaded successfully!\n"); }
	// If you supply a fallback, it'll be used if the lookup fails (and if
	//  non-NULL, means this will always return (okay)).
	bool Lookup(const char *fn, bool &okay, FunctionType fallback=NULL)
	{
		if (!okay)
			return false;
		else if (this->m_pFn == NULL)
		{
			this->m_pFn = (FunctionType) VoidFnPtrLookup_GlMgr(fn, okay, false, (void *) fallback);
			this->SetFuncName( fn );
		}
		return okay;
	}

	// Load library if necessary, look up symbol. Returns true and sets
	//  m_pFn on successful lookup, returns false otherwise. If the
	//  function pointer is already looked up, this return true immediately.
	// Use Reset() first if you want to look up the symbol again.
	//  This function will return false immediately unless (okay) is true.
	// If you supply a fallback, it'll be used if the lookup fails (and if
	//  non-NULL, means this will always return true).
	bool Lookup(const char *fn, FunctionType fallback=NULL)
	{
		bool okay = true;
		return Lookup(fn, okay, fallback);
	}

	// Invalidates the current lookup. Makes the function pointer NULL. You
	//  will need to call Lookup() before you can call a dynamic function
	//  through this interface again.
	void Reset() { m_pFn = NULL; }

	// Force this to be a specific function pointer.
	void Force(FunctionType ptr) { m_pFn = ptr; }

	// Retrieve the actual function pointer.
	FunctionType Pointer() const { return m_pFn; }

#if GL_TELEMETRY_ZONES || GL_TRACK_API_TIME
	#if GL_TELEMETRY_ZONES
		#define GL_FUNC_NAME m_szName
	#else
		#define GL_FUNC_NAME ""
	#endif

	inline CGLExecuteHelper<FunctionType, Result> operator() () const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME ); }
	
	template<typename T>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a); }

	template<typename T, typename U>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b); }

	template<typename T, typename U, typename V>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c ) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c); }

	template<typename T, typename U, typename V, typename W>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c, W d) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c, d); }

	template<typename T, typename U, typename V, typename W, typename X>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c, W d, X e) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c, d, e); }

	template<typename T, typename U, typename V, typename W, typename X, typename Y>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c, W d, X e, Y f) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c, d, e, f); }

	template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c, W d, X e, Y f, Z g) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c, d, e, f, g); }

	template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c, W d, X e, Y f, Z g, A h) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c, d, e, f, g, h); }

	template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A, typename B>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c, W d, X e, Y f, Z g, A h, B i) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c, d, e, f, g, h, i); }

	template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A, typename B, typename C>
	inline CGLExecuteHelper<FunctionType, Result> operator() (T a, U b, V c, W d, X e, Y f, Z g, A h, B i, C j) const { return CGLExecuteHelper<FunctionType, Result>(m_pFn, GL_FUNC_NAME, a, b, c, d, e, f, g, h, i, j); }
#else
	operator FunctionType() const { return m_pFn; }
#endif

	// Can be used to verify that we have an actual function looked up and
	//  ready to call: if (!MyDynFunc) { printf("Function not found!\n"); }
	operator bool () const { return m_pFn != NULL; }
	bool operator !() const { return m_pFn == NULL; }
		
protected:
	FunctionType m_pFn;

#if GL_TELEMETRY_ZONES
	char m_szName[32];
	inline void SetFuncName(const char *pFn) { V_strncpy( m_szName, pFn, sizeof( m_szName ) ); }
#else
	inline void SetFuncName(const char *pFn) { (void)pFn; }
#endif
};

// This works a lot like CDynamicFunctionMustInit, but we use SDL_GL_GetProcAddress().
template < const bool bRequired, class FunctionType, typename Result >
class CDynamicFunctionOpenGL : public CDynamicFunctionOpenGLBase< FunctionType, Result >
{
private:  // forbid default constructor.
	CDynamicFunctionOpenGL() {}

public:
	CDynamicFunctionOpenGL(const char *fn, FunctionType fallback=NULL)
	{
        bool okay = true;
		Lookup(fn, okay, fallback);
		this->SetFuncName( fn );
	}

	CDynamicFunctionOpenGL(const char *fn, bool &okay, FunctionType fallback=NULL)
	{
		Lookup(fn, okay, fallback);
		this->SetFuncName( fn );
	}

	// Please note this is not virtual.
	// !!! FIXME: we might want to fall back and try "EXT" or "ARB" versions in some case.
	bool Lookup(const char *fn, bool &okay, FunctionType fallback=NULL)
	{
		if (this->m_pFn == NULL)
		{
			this->m_pFn = (FunctionType) VoidFnPtrLookup_GlMgr(fn, okay, bRequired, (void *) fallback);
			this->SetFuncName( fn );
		}
		return okay;
	}
};


// This provides all the entry points for a given OpenGL context.
//  ENTRY POINTS ARE ONLY VALID FOR THE CONTEXT THAT WAS CURRENT WHEN
//  YOU LOOKED THEM UP. 99% of the time, this is not a problem, but
//  that 1% is really hard to track down. Always access the GL
//  through this class!
class COpenGLEntryPoints
{
public:
	// The GL context you are looking up entry points for must be current when you construct this object!
	COpenGLEntryPoints();

	uint64 m_nTotalGLCycles, m_nTotalGLCalls;

	int m_nOpenGLVersionMajor;  // if GL_VERSION is 2.1.0, this will be set to 2.
	int m_nOpenGLVersionMinor;  // if GL_VERSION is 2.1.0, this will be set to 1.
	int m_nOpenGLVersionPatch;  // if GL_VERSION is 2.1.0, this will be set to 0.
	bool m_bHave_OpenGL;

	#define GL_EXT(x,glmajor,glminor) bool m_bHave_##x;
	#define GL_FUNC(ext,req,ret,fn,arg,call) CDynamicFunctionOpenGL< req, ret (APIENTRY *) arg, ret > fn;
	#define GL_FUNC_VOID(ext,req,fn,arg,call) CDynamicFunctionOpenGL< req, void (APIENTRY *) arg, void > fn;
	#include "togl/glfuncs.inl"
	#undef GL_FUNC_VOID
	#undef GL_FUNC
	#undef GL_EXT
};

// This will be set to the current OpenGL context's entry points.
extern COpenGLEntryPoints *gGL;
typedef void * (*GL_GetProcAddressCallbackFunc_t)(const char *, bool &, const bool, void *);

#ifdef TOGL_DLL_EXPORT
	DLL_EXPORT COpenGLEntryPoints *ToGLConnectLibraries( CreateInterfaceFn factory );
	DLL_EXPORT void ToGLDisconnectLibraries();
	DLL_EXPORT COpenGLEntryPoints *GetOpenGLEntryPoints(GL_GetProcAddressCallbackFunc_t callback);
#else
	DLL_IMPORT COpenGLEntryPoints *ToGLConnectLibraries( CreateInterfaceFn factory );
	DLL_IMPORT void ToGLDisconnectLibraries();
	DLL_IMPORT COpenGLEntryPoints *GetOpenGLEntryPoints(GL_GetProcAddressCallbackFunc_t callback);
#endif

#if GL_TELEMETRY_ZONES || GL_TRACK_API_TIME
inline void CGLExecuteHelperBase::StartCall(const char *pName) 
{ 
	(void)pName;

#if GL_TELEMETRY_ZONES	
	tmEnter( TELEMETRY_LEVEL3, TMZF_NONE, pName );
#endif

#if GL_TRACK_API_TIME
	m_nStartTime = tmFastTime();
#endif
}

inline void CGLExecuteHelperBase::StopCall(const char *pName) 
{ 
#if GL_TRACK_API_TIME
	uint64 nTotalCycles = tmFastTime() - m_nStartTime;
#endif

#if GL_TELEMETRY_ZONES
	tmLeave( TELEMETRY_LEVEL3 );
#endif

#if GL_TRACK_API_TIME	
	//double flMilliseconds = g_Telemetry.flRDTSCToMilliSeconds * nTotalCycles;
	if (gGL) 
	{ 
		gGL->m_nTotalGLCycles += nTotalCycles;
		gGL->m_nTotalGLCalls++; 
	} 
#endif
}
#endif

#endif // DX_TO_GL_ABSTRACTION

#endif // GLENTRYPOINTS_H
