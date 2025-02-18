//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_SHARED_H
#define VSCRIPT_SHARED_H

#include "vscript/ivscript.h"

#if defined( _WIN32 )
#pragma once
#endif

DECLARE_LOGGING_CHANNEL( LOG_VScript );

extern IScriptVM * g_pScriptVM;

HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing = false );
bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing = false );
inline bool VScriptRunScript( const char *pszScriptName, bool bWarnMissing = false ) { return VScriptRunScript( pszScriptName, NULL, bWarnMissing ); }

#define DECLARE_ENT_SCRIPTDESC()													ALLOW_SCRIPT_ACCESS(); virtual ScriptClassDesc_t *GetScriptDesc()

#define BEGIN_ENT_SCRIPTDESC( className, baseClass, description )					_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC( className, baseClass, description )
#define BEGIN_ENT_SCRIPTDESC_ROOT( className, description )							_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC_ROOT( className, description )
#define BEGIN_ENT_SCRIPTDESC_NAMED( className, baseClass, scriptName, description )	_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC_NAMED( className, baseClass, scriptName, description )
#define BEGIN_ENT_SCRIPTDESC_ROOT_NAMED( className, scriptName, description )		_IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className ); BEGIN_SCRIPTDESC_ROOT_NAMED( className, scriptName, description )

#define _IMPLEMENT_ENT_SCRIPTDESC_ACCESSOR( className )					template <> ScriptClassDesc_t * GetScriptDesc<className>( className * ); ScriptClassDesc_t *className::GetScriptDesc()  { return ::GetScriptDesc( this ); }		

// Only allow scripts to create entities during map initialization
bool IsEntityCreationAllowedInScripts( void );

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

/*! An auto-referenced wrapper for HSCRIPTs to automatically handle refcount bookkeeping and make 
	storing script references in C++ objects safe.

	In some cases, persistently storing an HSCRIPT in a C++ can be tricky because of explicit addref/decref
	semantics -- if you store an HSCRIPT without performing a ReferenceScope() on it, then it may become
	invalid after some script operation. If you ReferenceScope() without remembering to match a Release(),
	then you'll leak memory at best and possibly cause a crash at worst. 
	
	The CScriptAutoRef class stores an HSCRIPT and automatically handles the refcounting for you in a similar
	way to std::shared_ptr. You can use it wherever you would ordinarily store an HSCRIPT. It can be passed by
	value (but const ref is more efficient). If you were planning to store an HSCRIPT from one to the next, you 
	probably want a CScriptAutoRef instead.

	Tested under Squirrel ; other languages have not yet been tested. 
 */
class CScriptAutoRef
{
public:
	/// Construct from a handle, adding a reference to it. 
	inline CScriptAutoRef( void ) : m_h(NULL) {}
	inline CScriptAutoRef( const HSCRIPT handle );
	inline CScriptAutoRef( const CScriptAutoRef &other );
	inline CScriptAutoRef & operator=( const CScriptAutoRef &other );
	inline CScriptAutoRef & operator=( const HSCRIPT &other );
	inline CScriptAutoRef( const ScriptVariant_t &other );
	inline ~CScriptAutoRef();
	inline HSCRIPT Set( HSCRIPT handle );

	/// Release the reference and set it to NULL. Safe to call when internal reference is NULL already.
	inline void Term();

	/// Set the internal handle *without* adding to its refcount -- eg if you've got an hscript on the stack
	/// and want to transfer its ownership to this managed class.
	inline CScriptAutoRef &SetNoAddref( HSCRIPT handle );

	/// Access this HSCRIPT. Does not increment the refcount of the handle it gives you.
	inline HSCRIPT Get() const { return m_h; }
	inline operator HSCRIPT() const { return Get(); }
	inline operator bool() const { return m_h != NULL; }
	inline operator ScriptVariant_t() const { return ScriptVariant_t(Get()); }

	/// Must be defined to point at whatever VM this autoref belongs to. At the moment this is 
	/// always one single global VM, so is defined in the same header that defines the extern g_pScriptVM
	static inline IScriptVM *VM();

private:
	HSCRIPT m_h;
};

IScriptVM *CScriptAutoRef::VM() 
{ 
	return g_pScriptVM; 
}

CScriptAutoRef &CScriptAutoRef::SetNoAddref( HSCRIPT handle )
{ 
	Term(); 
	m_h = handle; 
	return *this; 
}

HSCRIPT CScriptAutoRef::Set( HSCRIPT handle )
{
	Assert( VM() );
	// clear out existing ref if any.
	Term();
	if ( handle != NULL )
		m_h = VM()->ReferenceScope( handle );
	return m_h;
}

void CScriptAutoRef::Term( )
{
	Assert( VM() );
	if ( m_h )
	{
		VM()->ReleaseScript( m_h );
		m_h = NULL;
	}
}

inline CScriptAutoRef::CScriptAutoRef( const ScriptVariant_t &other )
: m_h(NULL)
{
	Assert( other.GetType() == FIELD_HSCRIPT );
	Set( other );
}

CScriptAutoRef::~CScriptAutoRef()
{
	Term();
}

inline CScriptAutoRef::CScriptAutoRef( const HSCRIPT handle )
: m_h(NULL) // important -- otherwise Term() will access uninitialized memory
{
	Set( handle );
}

inline CScriptAutoRef::CScriptAutoRef( const CScriptAutoRef &other )
: m_h(NULL) // important -- otherwise Term() will access uninitialized memory
{
	Set( other.Get() );
}

inline CScriptAutoRef & CScriptAutoRef::operator=( const CScriptAutoRef &other )
{
	Set( other.Get() );
	return *this;
}

inline CScriptAutoRef & CScriptAutoRef::operator=( const HSCRIPT &other )
{
	Set( other );
	return *this;
}

#endif // VSCRIPT_SHARED_H
