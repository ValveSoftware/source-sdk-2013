//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		A link that can be turned on and off.  Unlike normal links
//				dyanimc links must be entities so they and receive messages.
//				They update the state of the actual links.  Allows us to save
//				a lot of memory by not making all links into entities
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_DYNAMICLINK_H
#define AI_DYNAMICLINK_H
#pragma once

enum DynamicLinkState_t
{
	LINK_OFF = 0,
	LINK_ON = 1,
};

class CAI_Link;

//=============================================================================
//	>> CAI_DynanicLink
//=============================================================================
class CAI_DynamicLink : public CServerOnlyEntity
{
	DECLARE_CLASS( CAI_DynamicLink, CServerOnlyEntity );
public:
	static void					InitDynamicLinks(void);
	static void					ResetDynamicLinks(void);
	static void					PurgeDynamicLinks(void);
	static void 				GenerateControllerLinks();

	static bool					gm_bInitialized;

	static CAI_DynamicLink*		GetDynamicLink(int nSrcID, int nDstID);

	static CAI_DynamicLink*		m_pAllDynamicLinks;		// A linked list of all dynamic link
	CAI_DynamicLink*			m_pNextDynamicLink;		// The next dynamic link in the list of dynamic links

	int							m_nSrcEditID;			// the node that 'owns' this link
	int							m_nDestEditID;			// the node on the other end of the link. 

	int							m_nSrcID;				// the node that 'owns' this link
	int							m_nDestID;				// the node on the other end of the link. 
	DynamicLinkState_t			m_nLinkState;			// 
	string_t					m_strAllowUse;			// Only this entity name or classname may use the link
	bool						m_bInvertAllow;			// Instead of only allowing the m_strAllowUse entity, exclude only it
	
	bool						m_bFixedUpIds;
	bool						m_bNotSaved;
	int							m_nLinkType;

	void						SetLinkState( void );
	bool						IsLinkValid( void );

	CAI_Link *					FindLink();

	int							ObjectCaps();

	// ----------------
	//	Inputs
	// ----------------
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	DECLARE_DATADESC();

	CAI_DynamicLink();
	~CAI_DynamicLink();
};

//=============================================================================
//	>> CAI_DynanicLinkVolume
//=============================================================================
class CAI_DynamicLinkController : public CServerOnlyEntity
{
	DECLARE_CLASS( CAI_DynamicLinkController, CServerOnlyEntity );
public:
	void GenerateLinksFromVolume();

	// ----------------
	//	Inputs
	// ----------------
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputSetAllowed( inputdata_t &inputdata );
	void InputSetInvert( inputdata_t &inputdata );
	
	CUtlVector< CHandle<CAI_DynamicLink> > m_ControlledLinks;
	DynamicLinkState_t			m_nLinkState;
	string_t					m_strAllowUse;		// Only this entity name or classname may use the link
	bool						m_bInvertAllow;		// Instead of only allowing the m_strAllowUse entity, exclude only it
	bool						m_bUseAirLinkRadius;

	DECLARE_DATADESC();
};

//=============================================================================
//=============================================================================
class CAI_RadialLinkController : public CBaseEntity
{
	DECLARE_CLASS( CAI_RadialLinkController, CBaseEntity );

public:
	void	Spawn();
	void	Activate();
	void	PollMotionThink();
	void	ModifyNodeLinks( bool bMakeStale );

public:
	float	m_flRadius;
	Vector	m_vecAtRestOrigin;
	bool	m_bAtRest;

	DECLARE_DATADESC();
};

#endif // AI_DYNAMICLINK_H
