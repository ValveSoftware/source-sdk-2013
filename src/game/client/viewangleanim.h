//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "utlvector.h"

#define VIEWANIM_RELATIVE (1<<0)	// angles in keyframe are relative, add anim to current angles
#define VIEWANIM_IGNORE_X (1<<1)	// ignore the x component of this animation
#define VIEWANIM_IGNORE_Y (1<<2)	// ditto for y
#define VIEWANIM_IGNORE_Z (1<<3)	// ditto for z

#define QAngleToVector(a,v) { v[0] = a[0]; v[1] = a[1]; v[2] = a[2]; }

class CViewAngleKeyFrame
{	
public:
	CViewAngleKeyFrame( QAngle vecAngles, float flTime, int iFlags )
	{
		m_vecAngles = vecAngles;
		m_flTime = flTime;
		m_iFlags = iFlags;
	}

	// the target angles for this keyframe in the view angle animation
	QAngle m_vecAngles;

	// time position of this keyframe
	float m_flTime;	

	int m_iFlags;
};


typedef void (*ViewAnimCompleteCallback)( void );

class CViewAngleAnimation : public C_BaseEntity
{
public:
	CViewAngleAnimation();
	~CViewAngleAnimation();

	virtual void Spawn();

	void DeleteKeyFrames();


	void LoadViewAnimFile( const char *pKeyFrameFileName );
	void SaveAsAnimFile( const char *pKeyFrameFileName );

	void AddKeyFrame( CViewAngleKeyFrame *pKeyFrame );
	bool IsFinished( void );
	void RunAnimation( QAngle angles );
	void ClientThink();

	void SetAnimCompleteCallback( ViewAnimCompleteCallback pFunc )
	{
		m_pAnimCompleteCallback = pFunc;
	}

private:
	void SetAngles( QAngle vecCalculatedAngles );

	float m_flAnimStartTime;	// time this animation started
	bool m_bFinished;

	CUtlVector<CViewAngleKeyFrame *> m_KeyFrames;

	QAngle m_vecBaseAngles;

	int m_iFlags;

	ViewAnimCompleteCallback m_pAnimCompleteCallback;
};