//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A panel that display particle systems
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PARTICLEPANEL_H
#define TF_PARTICLEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tier2/camerautils.h"

class CTFParticlePanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFParticlePanel, vgui::EditablePanel );
public:
	// constructor, destructor
	CTFParticlePanel( vgui::Panel *pParent, const char *pName );
	virtual ~CTFParticlePanel();

	virtual void OnTick();
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnSizeChanged( int wide, int tall ) OVERRIDE;

	void FireParticleEffect( const char *pszName, int xPos, int yPos, float flScale, bool bLoop, float flEndTime = FLT_MAX );
private:

	// paint it!
	virtual void Paint() OVERRIDE;
	void UpdateParticlesFromKV();

	// Class to contain all of the per-instance particle system data
	struct ParticleEffect_t
	{
		ParticleEffect_t();
		~ParticleEffect_t();

		// Shutdown, startup particle collection
		void StartupParticleCollection();
		void ShutdownParticleCollection();

		// Accessor for control point values
		const Vector& GetControlPointValue( int nControlPoint ) const { return m_pControlPointValue[ nControlPoint ]; };
		void SetControlPointValue( int nControlPoint, const Vector &value ) { m_pControlPointValue[ nControlPoint ] = value; }
		// Set the particle system to draw
		void SetParticleSystem( const char* pszParticleSystemName );

		bool Update( float flTime );
		void Paint( CMatRenderContextPtr& pRenderContext, int iXOffset, int iYOffset, int nWide, int nTall );
		bool BNeedsToPaint() const { return m_pParticleSystem && m_bStarted; }

		Vector m_pControlPointValue[MAX_PARTICLE_CONTROL_POINTS];
		CParticleCollection *m_pParticleSystem;
		CUtlString m_ParticleSystemName;
		float m_flLastTime;
		float m_flScale;
		float m_flEndTime;
		int m_nXPos;
		int m_nYPos;
		QAngle m_Angles;
		bool m_bStartActivated;	// Start the effect immediately?
		bool m_bLoop;	// Loop the effect?
		bool m_bForceStopped;
		bool m_bAutoDelete;
		bool m_bStarted;

		Panel* m_pParent;
	};

	CUtlVector< ParticleEffect_t* > m_vecParticleEffects;
	KeyValues* m_pKVParticles = NULL;

	// A texture to use for a lightmap
	CTextureReference m_pLightmapTexture;

	// The default env_cubemap
	CTextureReference m_DefaultEnvCubemap;

	Camera_t m_Camera;
};

#endif // TF_PARTICLEPANEL_H
