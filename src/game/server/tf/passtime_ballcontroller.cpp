//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_passtime_ball.h"
#include "passtime_ballcontroller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
namespace
{
	int SortDescendingPriority( CPasstimeBallController *const *ppA, CPasstimeBallController *const *ppB )
	{
		return static_cast< CPasstimeBallController* >( *ppB )->GetPriority() 
			- static_cast< CPasstimeBallController* >( *ppA )->GetPriority();
	}
}

//-----------------------------------------------------------------------------
// static
int CPasstimeBallController::ApplyTo( CPasstimeBall *pBall )
{
	// Sort controllers by iPriority, then call Apply until one returns true.
	// From then on, skip controllers with a lower iPriority.
	// This sorts the autolist, but that should be OK.
	auto &controllers = GetAutoList();
	controllers.Sort( &SortDescendingPriority );
	int iMinPriority = INT_MIN;
	int iNumControllers = 0;

	for ( auto controller : controllers )
	{
		if ( !controller->IsEnabled() || !controller->IsActive() )
		{
			continue;
		}

		if ( controller->GetPriority() < iMinPriority )
		{
			break;
		}

		if ( controller->Apply( pBall ) )
		{
			iMinPriority = controller->GetPriority();
			++iNumControllers;
		}
	}
	return iNumControllers;
}

//-----------------------------------------------------------------------------
// static
int CPasstimeBallController::DisableOn( const CPasstimeBall *pBall )
{
	auto &controllers = GetAutoList();
	int iCount = 0;
	for ( auto pController : controllers )
	{
		if ( pController->IsEnabled() )
		{
			pController->SetIsEnabled( false );
			++iCount;
		}
	}
	return iCount;
}

//-----------------------------------------------------------------------------
// static
void CPasstimeBallController::BallCollision( CPasstimeBall *pBall,
	int iCollisionIndex, gamevcollisionevent_t *pEvent )
{
	auto &controllers = GetAutoList();
	for ( auto pController : controllers )
	{
		if ( pController->IsEnabled() && pController->IsActive() )
		{
			pController->OnBallCollision( pBall, iCollisionIndex, pEvent );
		}
	}
}

//-----------------------------------------------------------------------------
// static
void CPasstimeBallController::BallPickedUp( CPasstimeBall *pBall,
	CTFPlayer *pCatcher )
{
	auto &controllers = GetAutoList();
	for ( auto pController : controllers )
	{
		if ( pController->IsEnabled() && pController->IsActive() )
		{
			pController->OnBallPickedUp( pBall, pCatcher );
		}
	}
}

//-----------------------------------------------------------------------------
// static
void CPasstimeBallController::BallDamaged( CPasstimeBall *pBall )
{
	auto &controllers = GetAutoList();
	for ( auto pController : controllers )
	{
		if ( pController->IsEnabled() && pController->IsActive() )
		{
			pController->OnBallDamaged( pBall );
		}
	}
}

//-----------------------------------------------------------------------------
// static
void CPasstimeBallController::BallSpawned( CPasstimeBall *pBall )
{
	auto &controllers = GetAutoList();
	for ( auto pController : controllers )
	{
		if ( pController->IsEnabled() && pController->IsActive() )
		{
			pController->OnBallSpawned( pBall );
		}
	}
}

//-----------------------------------------------------------------------------
CPasstimeBallController::CPasstimeBallController( int iPriority )
	: m_bEnabled( false )
	, m_iPriority( iPriority ) {}


//-----------------------------------------------------------------------------
void CPasstimeBallController::SetIsEnabled( bool bEnabled )
{
	if ( m_bEnabled == bEnabled )
	{
		return;
	}

	m_bEnabled = bEnabled;

	if ( bEnabled )
	{
		OnEnabled();
	}
	else
	{
		OnDisabled();
	}
}

//-----------------------------------------------------------------------------
bool CPasstimeBallController::IsEnabled() const
{
	return m_bEnabled;
}

//-----------------------------------------------------------------------------
int CPasstimeBallController::GetPriority() const
{
	return m_iPriority;
}

