//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_ROPE_H
#define C_ROPE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "rope_physics.h"
#include "materialsystem/imaterial.h"
#include "rope_shared.h"
#include "bitvec.h"


class KeyValues;
class C_BaseAnimating;
struct RopeSegData_t;

#define MAX_ROPE_SUBDIVS		8
#define MAX_ROPE_SEGMENTS		(ROPE_MAX_SEGMENTS+(ROPE_MAX_SEGMENTS-1)*MAX_ROPE_SUBDIVS)

//=============================================================================
class C_RopeKeyframe : public C_BaseEntity
{
public:

	DECLARE_CLASS( C_RopeKeyframe, C_BaseEntity );
	DECLARE_CLIENTCLASS();


private:

	class CPhysicsDelegate : public CSimplePhysics::IHelper
	{
	public:
		virtual void	GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel );
		virtual void	ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes );
	
		C_RopeKeyframe	*m_pKeyframe;
	};

	friend class CPhysicsDelegate;


public:

					C_RopeKeyframe();
					~C_RopeKeyframe();

	// This can be used for client-only ropes.
	static C_RopeKeyframe* Create(
		C_BaseEntity *pStartEnt,
		C_BaseEntity *pEndEnt,
		int iStartAttachment=0,
		int iEndAttachment=0,
		float ropeWidth = 2,
		const char *pMaterialName = "cable/cable",		// Note: whoever creates the rope must
														// use PrecacheModel for whatever material
														// it specifies here.
		int numSegments = 5,
		int ropeFlags = ROPE_SIMULATE
		);

	// Create a client-only rope and initialize it with the parameters from the KeyValues.
	static C_RopeKeyframe* CreateFromKeyValues( C_BaseAnimating *pEnt, KeyValues *pValues );

	// Find ropes (with both endpoints connected) that intersect this AABB. This is just an approximation.
	static int GetRopesIntersectingAABB( C_RopeKeyframe **pRopes, int nMaxRopes, const Vector &vAbsMin, const Vector &vAbsMax );

	// Set the slack.
	void SetSlack( int slack );

	void SetRopeFlags( int flags );
	int GetRopeFlags() const;

	void SetupHangDistance( float flHangDist );

	// Change which entities the rope is connected to.
	void SetStartEntity( C_BaseEntity *pEnt );
	void SetEndEntity( C_BaseEntity *pEnt );

	C_BaseEntity* GetStartEntity() const;
	C_BaseEntity* GetEndEntity() const;

	// Hook the physics. Pass in your own implementation of CSimplePhysics::IHelper. The
	// default implementation is returned so you can call through to it if you want.
	CSimplePhysics::IHelper*	HookPhysics( CSimplePhysics::IHelper *pHook );

	// Attach to things (you can also just lock the endpoints down yourself if you hook the physics).

	// Client-only right now. This could be moved to the server if there was a good reason.
	void			SetColorMod( const Vector &vColorMod );

	// Use this when rope length and slack change to recompute the spring length.
	void			RecomputeSprings();

	void ShakeRope( const Vector &vCenter, float flRadius, float flMagnitude );

	// Get the attachment position of one of the endpoints.
	bool			GetEndPointPos( int iPt, Vector &vPos );

	// Get the rope material data.
	IMaterial		*GetSolidMaterial( void );
	IMaterial		*GetBackMaterial( void );

	struct BuildRopeQueuedData_t
	{
		Vector	*m_pPredictedPositions;
		Vector	*m_pLightValues;
		int		m_iNodeCount;
		Vector	m_vColorMod;
		float	m_RopeLength;
		float	m_Slack;
	};

	void			BuildRope( RopeSegData_t *pRopeSegment, const Vector &vCurrentViewForward, const Vector &vCurrentViewOrigin, BuildRopeQueuedData_t *pQueuedData, bool bQueued );

// C_BaseEntity overrides.
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink();
	virtual int		DrawModel( int flags );
	virtual bool	ShouldDraw();
	virtual const Vector& WorldSpaceCenter() const;

	// Specify ROPE_ATTACHMENT_START_POINT or ROPE_ATTACHMENT_END_POINT for the attachment.
	virtual	bool	GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool	GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool	GetAttachment( int number, Vector &origin );
	virtual bool	GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel );

private:
	
	void			FinishInit( const char *pMaterialName );

	void			RunRopeSimulation( float flSeconds );
	Vector			ConstrainNode( const Vector &vNormal, const Vector &vNodePosition, const Vector &vMidpiont, float fNormalLength );
	void			ConstrainNodesBetweenEndpoints( void );

	bool			AnyPointsMoved();

	bool			DidEndPointMove( int iPt );
	bool			DetectRestingState( bool &bApplyWind );

	void			UpdateBBox();
	bool			InitRopePhysics();
	
	bool			GetEndPointAttachment( int iPt, Vector &vPos, QAngle &angle );
	
	Vector			*GetRopeSubdivVectors( int *nSubdivs );
	void			CalcLightValues();

	void			ReceiveMessage( int classID, bf_read &msg );
	bool			CalculateEndPointAttachment( C_BaseEntity *pEnt, int iAttachment, Vector &vPos, QAngle *pAngles );


private:
	// Track which links touched something last frame. Used to prevent wind from gusting on them.
	CBitVec<ROPE_MAX_SEGMENTS>		m_LinksTouchingSomething;
	int								m_nLinksTouchingSomething;
	bool							m_bApplyWind;
	int								m_fPrevLockedPoints;	// Which points are locked down.
	int								m_iForcePointMoveCounter;

	// Used to control resting state.
	bool			m_bPrevEndPointPos[2];
	Vector			m_vPrevEndPointPos[2];

	float			m_flCurScroll;		// for scrolling texture.
	float			m_flScrollSpeed;

	int				m_RopeFlags;			// Combo of ROPE_ flags.
	int				m_iRopeMaterialModelIndex;	// Index of sprite model with the rope's material.
		
	CRopePhysics<ROPE_MAX_SEGMENTS>	m_RopePhysics;
	Vector			m_LightValues[ROPE_MAX_SEGMENTS]; // light info when the rope is created.

	int				m_nSegments;		// Number of segments.
	
	EHANDLE			m_hStartPoint;		// StartPoint/EndPoint are entities
	EHANDLE			m_hEndPoint;
	short			m_iStartAttachment;	// StartAttachment/EndAttachment are attachment points.
	short			m_iEndAttachment;

	unsigned char	m_Subdiv;			// Number of subdivions in between segments.

	int				m_RopeLength;		// Length of the rope, used for tension.
	int				m_Slack;			// Extra length the rope is given.
	float			m_TextureScale;		// pixels per inch
	
	int				m_fLockedPoints;	// Which points are locked down.

	float				m_Width;

	CPhysicsDelegate	m_PhysicsDelegate;

	IMaterial		*m_pMaterial;
	IMaterial		*m_pBackMaterial;			// Optional translucent background material for the rope to help reduce aliasing.

	int				m_TextureHeight;	// Texture height, for texture scale calculations.

	// Instantaneous force
	Vector			m_flImpulse;
	Vector			m_flPreviousImpulse;

	// Simulated wind gusts.
	float			m_flCurrentGustTimer;
	float			m_flCurrentGustLifetime;	// How long will the current gust last?

	float			m_flTimeToNextGust;			// When will the next wind gust be?
	Vector			m_vWindDir;					// What direction does the current gust go in?

	Vector			m_vColorMod;				// Color modulation on all verts?

	Vector			m_vCachedEndPointAttachmentPos[2];
	QAngle			m_vCachedEndPointAttachmentAngle[2];

	// In network table, can't bit-compress
	bool			m_bConstrainBetweenEndpoints;	// Simulated segment points won't stretch beyond the endpoints

	bool			m_bEndPointAttachmentPositionsDirty : 1;
	bool			m_bEndPointAttachmentAnglesDirty : 1;
	bool			m_bNewDataThisFrame : 1;			// Set to true in OnDataChanged so that we simulate that frame
	bool			m_bPhysicsInitted : 1;				// It waits until all required entities are 
	// present to start simulating and rendering.

	friend class CRopeManager;
};


// Profiling info.
void Rope_ResetCounters();
//void Rope_ShowRSpeeds();

//=============================================================================
//
// Rope Manager
//
abstract_class IRopeManager
{
public:
	virtual						~IRopeManager() {}
	virtual void				ResetRenderCache( void ) = 0;
	virtual void				AddToRenderCache( C_RopeKeyframe *pRope ) = 0;
	virtual void				DrawRenderCache( bool bShadowDepth ) = 0;
	virtual void				OnRenderStart( void ) = 0;
	virtual void				SetHolidayLightMode( bool bHoliday ) = 0;
	virtual bool				IsHolidayLightMode( void ) = 0;
	virtual int					GetHolidayLightStyle( void ) = 0;
};

IRopeManager *RopeManager();

#endif // C_ROPE_H
