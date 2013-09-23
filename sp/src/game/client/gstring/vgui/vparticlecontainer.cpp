
#include "cbase.h"
#include "Gstring/vgui/vParticleContainer.h"

using namespace vgui;

CVGUIParticleContainer::CVGUIParticleContainer( vgui::Panel *parent, const char *szName )
	: BaseClass( parent, szName )
{
	SetPaintBackgroundEnabled( false );

	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );
}

CVGUIParticleContainer::~CVGUIParticleContainer()
{
	FlushParticles();

	m_hOperators.PurgeAndDeleteElements();
}

void CVGUIParticleContainer::FlushParticles()
{
	for ( int i = 0; i < m_hParticles.Count(); i++ )
		m_hParticles[ i ]->SafeDelete();

	m_hParticles.Purge();
}

void CVGUIParticleContainer::AddParticle( vParticle *p )
{
	m_hParticles.AddToTail( p );
}

int CVGUIParticleContainer::GetNumParticles()
{
	return m_hParticles.Count();
}

void CVGUIParticleContainer::AddGlobalOperator( vParticleOperatorBase *op )
{
	m_hOperators.AddToTail( op );
}

void CVGUIParticleContainer::RenderParticles()
{
	DoSimulation();

	for ( int i = 0; i < m_hParticles.Count(); i++ )
	{
		m_hParticles[ i ]->Render();
	}
}

void CVGUIParticleContainer::OnThink()
{
}

void CVGUIParticleContainer::Paint()
{
	BaseClass::Paint();

	RenderParticles();
}

void CVGUIParticleContainer::DoSimulation()
{
	for ( int i = 0; i < m_hParticles.Count(); i++ )
	{
		vParticle *p = m_hParticles[ i ];

		if ( p->ShouldDie() )
		{
			m_hParticles.Remove( i );
			i--;
			p->SafeDelete();
			continue;
		}

		p->Revert();

		for ( int o = 0; o < m_hOperators.Count(); o++ )
			m_hOperators[ o ]->Simulate( p );

		p->Simulate();
	}
}