//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_GIB_H
#define C_GIB_H
#ifdef _WIN32
#pragma once
#endif

#define	DEFAULT_GIB_LIFETIME	4.0f

// Base client gibs

class C_Gib : public C_BaseAnimating
{
	typedef C_BaseAnimating BaseClass;
public:

	~C_Gib( void );

	static C_Gib	*CreateClientsideGib( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp, float flLifetime = DEFAULT_GIB_LIFETIME );
	
	bool	InitializeGib( const char *pszModelName, Vector vecOrigin, Vector vecForceDir, AngularImpulse vecAngularImp, float flLifetime = DEFAULT_GIB_LIFETIME );
	void	ClientThink( void );
	void	StartTouch( C_BaseEntity *pOther );

	virtual	void HitSurface( C_BaseEntity *pOther );

protected:

	float	m_flTouchDelta;		// Amount of time that must pass before another touch function can be called
};

#ifdef HL2_CLIENT_DLL
class CAntlionGibManager : public CAutoGameSystemPerFrame
{
public:
	CAntlionGibManager( char const *name ) : CAutoGameSystemPerFrame( name )
	{
	}

	// Methods of IGameSystem
	virtual void Update( float frametime );
	virtual void LevelInitPreEntity( void );

	void	AddGib( C_BaseEntity *pEntity ); 
	void	RemoveGib( C_BaseEntity *pEntity );

private:
	typedef CHandle<C_BaseEntity> CGibHandle;
	CUtlLinkedList< CGibHandle > m_LRU; 
	
};


extern CAntlionGibManager s_AntlionGibManager;

#endif


#endif // C_GIB_H
