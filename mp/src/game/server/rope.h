//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ROPE_H
#define ROPE_H
#ifdef _WIN32
#pragma once
#endif


#include "baseentity.h"

#include "positionwatcher.h"

class CRopeKeyframe : public CBaseEntity, public IPositionWatcher
{
	DECLARE_CLASS( CRopeKeyframe, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

					CRopeKeyframe();
	virtual			~CRopeKeyframe();

	// Create a rope and attach it to two entities.
	// Attachment points on the entities are optional.
	static CRopeKeyframe* Create(
		CBaseEntity *pStartEnt,
		CBaseEntity *pEndEnt,
		int iStartAttachment=0,
		int iEndAttachment=0,
		int ropeWidth = 2,
		const char *pMaterialName = "cable/cable.vmt",		// Note: whoever creates the rope must
															// use PrecacheModel for whatever material
															// it specifies here.
		int numSegments = 5
		);

	static CRopeKeyframe* CreateWithSecondPointDetached(
		CBaseEntity *pStartEnt,
		int iStartAttachment = 0,	// must be 0 if you don't want to use a specific model attachment.
		int ropeLength = 20,
		int ropeWidth = 2,
		const char *pMaterialName = "cable/cable.vmt",		// Note: whoever creates the rope
															// use PrecacheModel for whatever material
															// it specifies here.
		int numSegments = 5,
		bool bInitialHang = false
		);

	bool		SetupHangDistance( float flHangDist );
	void		ActivateStartDirectionConstraints( bool bEnable );
	void		ActivateEndDirectionConstraints( bool bEnable );

	
	// Shakes all ropes near vCenter. The higher flMagnitude is, the larger the shake will be.
	static void ShakeRopes( const Vector &vCenter, float flRadius, float flMagnitude );


// CBaseEntity overrides.
public:
	
	// don't cross transitions
	virtual int		ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void	Activate();
	virtual void	Precache();
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual bool	KeyValue( const char *szKeyName, const char *szValue );

	void			PropagateForce(CBaseEntity *pActivator, CBaseEntity *pCaller, CBaseEntity *pFirstLink, float x, float y, float z);

	// Once-off length recalculation
	void			RecalculateLength( void );

	// Kill myself when I next come to rest
	void			DieAtNextRest( void );

	virtual int		UpdateTransmitState(void);
	virtual void	SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	virtual void	SetParent( CBaseEntity *pParentEntity, int iAttachment );

// Input functions.
public:

	void InputSetScrollSpeed( inputdata_t &inputdata );
	void InputSetForce( inputdata_t &inputdata );
	void InputBreak( inputdata_t &inputdata );

public:

	bool			Break( void );
	void			DetachPoint( int iPoint );
	
	void			EndpointsChanged();

	// By default, ropes don't collide with the world. Call this to enable it.
	void			EnableCollision();

	// Toggle wind.
	void			EnableWind( bool bEnable );

	// Unless this is called during initialization, the caller should have done
	// PrecacheModel on whatever material they specify in here.
	void			SetMaterial( const char *pName );

	CBaseEntity*	GetEndPoint() { return m_hEndPoint.Get(); }
	int				GetEndAttachment() { return m_iStartAttachment; };

	void			SetStartPoint( CBaseEntity *pStartPoint, int attachment = 0 );
	void			SetEndPoint( CBaseEntity *pEndPoint, int attachment = 0 );

	// See ROPE_PLAYER_WPN_ATTACH for info.
	void			EnablePlayerWeaponAttach( bool bAttach );


	// IPositionWatcher
	virtual void NotifyPositionChanged( CBaseEntity *pEntity );

private:

	void			SetAttachmentPoint( CBaseHandle &hOutEnt, short &iOutAttachment, CBaseEntity *pEnt, int iAttachment );

	// This is normally called by Activate but if you create the rope at runtime,
	// you must call it after you have setup its variables.
	void			Init();

	// These work just like the client-side versions.
	bool			GetEndPointPos2( CBaseEntity *pEnt, int iAttachment, Vector &v );
	bool			GetEndPointPos( int iPt, Vector &v );

	void			UpdateBBox( bool bForceRelink );


public:

	CNetworkVar( int, m_RopeFlags );		// Combination of ROPE_ defines in rope_shared.h
	
	string_t	m_iNextLinkName;
	CNetworkVar( int, m_Slack );
	CNetworkVar( float, m_Width );
	CNetworkVar( float, m_TextureScale );
	CNetworkVar( int, m_nSegments );		// Number of segments.
	CNetworkVar( bool, m_bConstrainBetweenEndpoints );

	string_t m_strRopeMaterialModel;
	CNetworkVar( int, m_iRopeMaterialModelIndex );	// Index of sprite model with the rope's material.
	
	// Number of subdivisions in between segments.
	CNetworkVar( int, m_Subdiv );
	
	//EHANDLE		m_hNextLink;
	
	CNetworkVar( int, m_RopeLength );	// Rope length at startup, used to calculate tension.

	CNetworkVar( int, m_fLockedPoints );

	bool		m_bCreatedFromMapFile; // set to false when creating at runtime

	CNetworkVar( float, m_flScrollSpeed );

private:
	// Used to detect changes.
	bool		m_bStartPointValid;
	bool		m_bEndPointValid;
	
	CNetworkHandle( CBaseEntity, m_hStartPoint );		// StartPoint/EndPoint are entities
	CNetworkHandle( CBaseEntity, m_hEndPoint );
	CNetworkVar( short, m_iStartAttachment );	// StartAttachment/EndAttachment are attachment points.
	CNetworkVar( short, m_iEndAttachment );
};


#endif // ROPE_H
