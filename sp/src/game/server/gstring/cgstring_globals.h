#ifndef CGSTRING_GLOBALS_H
#define CGSTRING_GLOBALS_H

#include "cbase.h"

class CGstringGlobals : public CBaseEntity
{
	DECLARE_CLASS( CGstringGlobals, CBaseEntity );
	DECLARE_DATADESC();

public:
	CGstringGlobals();
	~CGstringGlobals();

	virtual void Spawn();

	virtual int ObjectCaps( void ){
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};

	void InputNightvisionEnable( inputdata_t &inputdata );
	void InputNightvisionDisable( inputdata_t &inputdata );
	void InputNightvisionToggle( inputdata_t &inputdata );

	void SetNightvisionEnabled( bool bEnabled );
	bool IsNightvisionEnabled() const;

private:

	bool m_bNightvisionEnabled;
};

extern CGstringGlobals *g_pGstringGlobals;

#endif