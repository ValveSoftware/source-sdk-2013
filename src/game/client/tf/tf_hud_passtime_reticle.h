//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_PASSTIME_RETICLE_H
#define TF_HUD_PASSTIME_RETICLE_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "Color.h"
#include "fx_quad.h"

//-----------------------------------------------------------------------------
CFXQuad *CreateReticleSprite( const char *pModelName, float scale, float spinSpeed );
class C_FuncPasstimeGoal;

//-----------------------------------------------------------------------------
class C_PasstimeReticle
{
public:
	virtual ~C_PasstimeReticle();
	void OnClientThink();

protected:
	C_PasstimeReticle() {}
	virtual bool Update() = 0;

	void AddSprite( CFXQuad *pEnt );
	void SetAllOrigins( const Vector& pos );
	void SetAllNormals( const Vector& normal );
	void SetAllAlphas( byte a );
	void SetAllScales( float s );
	void SetOrigin( int i, const Vector& pos );
	void SetNormal( int i, const Vector& normal );
	void SetAlpha( int i, byte a );
	void SetRgba( int i, byte r, byte g, byte b, byte a );
	void SetScale( int i, float s );

	CUtlVector<CFXQuad*> m_pSprites;

private:
	// noncopyable
	C_PasstimeReticle( const C_PasstimeReticle& ) = delete;
	C_PasstimeReticle( C_PasstimeReticle&& ) = delete;
	C_PasstimeReticle& operator=( const C_PasstimeReticle& ) = delete;
	C_PasstimeReticle& operator=( C_PasstimeReticle&& ) = delete;
};

//-----------------------------------------------------------------------------
class C_PasstimeBallReticle : public C_PasstimeReticle
{
public:
	C_PasstimeBallReticle();
private:
	virtual bool Update() OVERRIDE;
};

//-----------------------------------------------------------------------------
class C_PasstimeGoalReticle : public C_PasstimeReticle
{
public:
	C_PasstimeGoalReticle( C_FuncPasstimeGoal *pGoal );
private:
	virtual bool Update() OVERRIDE;
	CHandle<C_FuncPasstimeGoal> m_hGoal;
};

//-----------------------------------------------------------------------------
class C_PasstimePassReticle : public C_PasstimeReticle
{
public:
	C_PasstimePassReticle();
private:
	virtual bool Update() OVERRIDE;
	void FindPassHintTarget( C_TFPlayer *pLocalPlayer );
	float m_flTargetScore;
	CHandle<C_BaseEntity> m_hTarget;
};

//-----------------------------------------------------------------------------
class C_PasstimeBounceReticle : public C_PasstimeReticle
{
public:
	C_PasstimeBounceReticle();
	void Show( const Vector& pos, const Vector& normal );
	void Hide();
private:
	virtual bool Update() OVERRIDE;
};

//-----------------------------------------------------------------------------
class C_PasstimePlayerReticle : public C_PasstimeReticle
{
public:
	C_PasstimePlayerReticle( C_TFPlayer *pPlayer );
private:
	virtual bool Update() OVERRIDE;
	CHandle<C_TFPlayer> m_hPlayer;
};

//-----------------------------------------------------------------------------
class C_PasstimeAskForBallReticle : public C_PasstimeReticle
{
public:
	C_PasstimeAskForBallReticle( C_TFPlayer *pPlayer );
private:
	virtual bool Update() OVERRIDE;
	CHandle<C_TFPlayer> m_hPlayer;
};

#endif // TF_HUD_PASSTIME_RETICLE_H  
