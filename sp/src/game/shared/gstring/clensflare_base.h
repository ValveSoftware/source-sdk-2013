#ifndef LENSFLARE_H
#define LENSFLARE_H

#include "cbase.h"

class CGlowOverlay;
struct vLensflare_Collection;

class CLensflareBase : public CBaseEntity
{
public:
	DECLARE_CLASS( CLensflareBase, CBaseEntity );
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CLensflareBase();

#ifdef GAME_DLL
	virtual void	Activate( void );

	virtual void SetLensflareEnabled( bool bEnabled );

	virtual int ObjectCaps( void ){
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	};
#else
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void UpdateOnRemove();

	virtual CGlowOverlay *GetGlowSource();

#endif

protected:
#ifdef CLIENT_DLL
	void CreateLensFlare();
	void DestroyLensFlare();
#endif

private:
#ifdef GAME_DLL
	string_t m_strLensFlareScript;

#else

	vLensflare_Collection *m_pLensFlare;

#endif

	CNetworkString( m_netStr_LensFlareScript, MAX_PATH );

	CNetworkVar( bool, m_bLensflareEnabled );

};


#endif