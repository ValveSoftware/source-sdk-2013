#ifndef PARTICLE_CONTAINER_H
#define PARTICLE_CONTAINER_H

#include "cbase.h"
#include "vgui_controls/controls.h"
#include "vgui_controls/panel.h"
#include "Gstring/vgui/vParticle.h"

class CVGUIParticleContainer : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CVGUIParticleContainer, vgui::Panel );
public:

	CVGUIParticleContainer( vgui::Panel *parent, const char *szName = NULL );
	~CVGUIParticleContainer();

	void FlushParticles();

	void AddParticle( vParticle *p );
	int GetNumParticles();

	void AddGlobalOperator( vParticleOperatorBase *op );

protected:

	virtual void OnThink();

	virtual void Paint();

private:

	void DoSimulation();
	void RenderParticles();

	bool m_bAutoRender;

	CUtlVector< vParticleOperatorBase* > m_hOperators;

	CUtlVector< vParticle* > m_hParticles;
};

#endif