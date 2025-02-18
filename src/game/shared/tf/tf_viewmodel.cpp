//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//===========================================================================//
#include "cbase.h"
#include "tf_viewmodel.h"
#include "tf_shareddefs.h"
#include "tf_weapon_minigun.h"
#include "tf_weapon_invis.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"

// for spy material proxy
#include "tf_proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "prediction.h"

#endif

#include "bone_setup.h"	//temp

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_viewmodel, CTFViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( TFViewModel, DT_TFViewModel )

BEGIN_NETWORK_TABLE( CTFViewModel, DT_TFViewModel )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
CTFViewModel::CTFViewModel() 
	: m_LagAnglesHistory("CPredictedViewModel::m_LagAnglesHistory")
	, m_bBodygroupsDirty( true )
{
	m_vLagAngles.Init();
	m_LagAnglesHistory.Setup( &m_vLagAngles, 0 );
	m_vLoweredWeaponOffset.Init();
}
#else
CTFViewModel::CTFViewModel()
{
}
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFViewModel::~CTFViewModel()
{
}

#ifdef CLIENT_DLL
void DrawEconEntityAttachedModels( CBaseAnimating *pEnt, CEconEntity *pAttachedModelSource, const ClientModelRenderInfo_t *pInfo, int iMatchDisplayFlags );

// TODO:  Turning this off by setting interp 0.0 instead of 0.1 for now since we have a timing bug to resolve
ConVar cl_wpn_sway_interp( "cl_wpn_sway_interp", "0.0", FCVAR_CLIENTDLL | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar cl_wpn_sway_scale( "cl_wpn_sway_scale", "5.0", FCVAR_CLIENTDLL | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
#endif

//-----------------------------------------------------------------------------
// Purpose:  Adds head bob for off hand models
//-----------------------------------------------------------------------------
void CTFViewModel::AddViewModelBob( CBasePlayer *owner, Vector& eyePosition, QAngle& eyeAngles )
{
#ifdef CLIENT_DLL
	// if we are an off hand view model (index 1) and we have a model, add head bob.
	// (Head bob for main hand model added by the weapon itself.)
	if ( ViewModelIndex() == 1 && GetModel() != null )
	{
		CalcViewModelBobHelper( owner, &m_BobState );
		AddViewModelBobHelper( eyePosition, eyeAngles, &m_BobState );
	}
#endif
}

void CTFViewModel::CalcViewModelLag( Vector& origin, QAngle& angles, QAngle& original_angles )
{
#ifdef CLIENT_DLL
	if ( prediction->InPrediction() )
	{
		return;
	}

	if ( cl_wpn_sway_interp.GetFloat() <= 0.0f )
	{
		return;
	}

	// Calculate our drift
	Vector	forward, right, up;
	AngleVectors( angles, &forward, &right, &up );

	// Add an entry to the history.
	m_vLagAngles = angles;
	m_LagAnglesHistory.NoteChanged( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat(), false );

	// Interpolate back 100ms.
	m_LagAnglesHistory.Interpolate( gpGlobals->curtime, cl_wpn_sway_interp.GetFloat() );

	// Now take the 100ms angle difference and figure out how far the forward vector moved in local space.
	Vector vLaggedForward;
	QAngle angleDiff = m_vLagAngles - angles;
	AngleVectors( -angleDiff, &vLaggedForward, 0, 0 );
	Vector vForwardDiff = Vector(1,0,0) - vLaggedForward;

	// Now offset the origin using that.
	vForwardDiff *= cl_wpn_sway_scale.GetFloat();
	origin += forward*vForwardDiff.x + right*-vForwardDiff.y + up*vForwardDiff.z;
#endif
}

#ifdef CLIENT_DLL
ConVar cl_gunlowerangle( "cl_gunlowerangle", "90", FCVAR_CLIENTDLL | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar cl_gunlowerspeed( "cl_gunlowerspeed", "2", FCVAR_CLIENTDLL | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar tf_use_min_viewmodels( "tf_use_min_viewmodels", "0", FCVAR_ARCHIVE, "Use minimized viewmodels." );

ConVar tf_viewmodels_offset_override( "tf_viewmodels_offset_override", "", FCVAR_CHEAT, "If set, this will override the position of all viewmodels. Usage 'x y z'" );
#endif

void CTFViewModel::CalcViewModelView( CBasePlayer *owner, const Vector& eyePosition, const QAngle& eyeAngles )
{
#if defined( CLIENT_DLL )

	Vector vecNewOrigin = eyePosition;
	QAngle vecNewAngles = eyeAngles;

	// Check for lowering the weapon
	C_TFPlayer *pPlayer = ToTFPlayer( owner );

	Assert( pPlayer );

	bool bLowered = pPlayer->IsWeaponLowered();

	QAngle vecLoweredAngles(0,0,0);

	m_vLoweredWeaponOffset.x = Approach( bLowered ? cl_gunlowerangle.GetFloat() : 0, m_vLoweredWeaponOffset.x, cl_gunlowerspeed.GetFloat() );
	vecLoweredAngles.x += m_vLoweredWeaponOffset.x;

	vecNewAngles += vecLoweredAngles;

	CTFWeaponBase *pWeapon = assert_cast< CTFWeaponBase* >( GetWeapon() );
	if ( pWeapon )
	{
		bool bInspecting = pWeapon && pWeapon->GetInspectStage() != CTFWeaponBase::INSPECT_INVALID;

		static float s_inspectInterp = 0.f;
		if ( bInspecting )
		{
			if ( pWeapon->GetInspectStage() == CTFWeaponBase::INSPECT_END )
			{
				// use the last second of the anim
				const float flOutroDuration = 0.3f;
				s_inspectInterp = Clamp( ( pWeapon->GetInspectAnimEndTime() - gpGlobals->curtime ) - flOutroDuration, 0.f, 1.f );
			}
			else
			{
				s_inspectInterp = Clamp( s_inspectInterp + gpGlobals->frametime, 0.f, 1.f );
			}
		}
		else
		{
			s_inspectInterp = Clamp( s_inspectInterp - gpGlobals->frametime, 0.f, 1.f );
		}

		// inspect custom offset
		if ( bInspecting )
		{
			CAttribute_String attrInspectOffsetVMOverride;
			CALL_ATTRIB_HOOK_STRING_ON_OTHER( pWeapon, attrInspectOffsetVMOverride, inspect_viewmodel_offset );
			const char *pszValue = attrInspectOffsetVMOverride.value().c_str();
			if ( pszValue && *pszValue )
			{
				Vector vmOffset;
				UTIL_StringToVector( vmOffset.Base(), pszValue );

				Vector forward, right, up;
				AngleVectors( eyeAngles, &forward, &right, &up );

				Vector vOffset = vmOffset.x * forward + vmOffset.y * right + vmOffset.z * up;
				vOffset *= Gain( s_inspectInterp, 0.5f );
				vecNewOrigin += vOffset;
			}
		}

		// we want to always enable this internally
		bool bMinMode = tf_use_min_viewmodels.GetBool();

		// are we overriding vm offset?
		const char *pszVMOffsetOverride = tf_viewmodels_offset_override.GetString();
		bool bForceOverride = ( pszVMOffsetOverride && *pszVMOffsetOverride );
		bMinMode |= bForceOverride;

		// min mode custom offset
		if ( bMinMode )
		{
			Vector forward, right, up;
			AngleVectors( eyeAngles, &forward, &right, &up );

			Vector viewmodelOffset;
			if ( bForceOverride )
			{
				UTIL_StringToVector( viewmodelOffset.Base(), pszVMOffsetOverride );
			}
			else
			{
				viewmodelOffset = pWeapon->GetViewmodelOffset();
			}
			Vector vOffset = viewmodelOffset.x * forward + viewmodelOffset.y * right + viewmodelOffset.z * up;
			vOffset *= Gain( 1.f - s_inspectInterp, 0.5f );
			vecNewOrigin += vOffset;
		}
	}

	BaseClass::CalcViewModelView( owner, vecNewOrigin, vecNewAngles );

#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Don't render the weapon if its supposed to be lowered and we have 
// finished the lowering animation
//-----------------------------------------------------------------------------
int CTFViewModel::DrawModel( int flags )
{
	// Check for lowering the weapon
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	Assert( pPlayer );

	if ( m_bBodygroupsDirty )
	{
		m_nBody = 0;
		pPlayer->RecalcBodygroupsIfDirty();
		m_bBodygroupsDirty = false;
	}

	bool bLowered = pPlayer->IsWeaponLowered();

	if ( bLowered && fabs( m_vLoweredWeaponOffset.x - cl_gunlowerangle.GetFloat() ) < 0.1 )
	{
		// fully lowered, stop drawing
		return 1;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && 
		pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverTarget()->IsPlayer() )
	{
		pPlayer = ToTFPlayer( pLocalPlayer->GetObserverTarget() );
	}

	if ( pPlayer != GetOwner() && pPlayer->GetViewModel() != GetMoveParent() )
	{
		return 0;
	}

	if ( pPlayer->IsAlive() == false )
	{
		 return 0;
	}

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFViewModel::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Correct the ambient lighting position to match our owner entity
	if ( GetOwner() && pInfo )
	{
		pInfo->pLightingOrigin = &( GetOwner()->WorldSpaceCenter() );
	}

	return BaseClass::OnInternalDrawModel( pInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFViewModel::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( !BaseClass::OnPostInternalDrawModel( pInfo ) )
		return false;

	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )GetOwningWeapon();

	if ( pWeapon && !pWeapon->WantsToOverrideViewmodelAttachments() )
	{
		// only need to draw the attached models if the weapon doesn't want to override the viewmodel attachments
		// (used for Natascha's attachments, the Backburner, and the Kritzkrieg)
		DrawEconEntityAttachedModels( this, pWeapon, pInfo, kAttachedModelDisplayFlag_ViewModel );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewModel::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )GetOwningWeapon();

	if ( !pWeapon ) 
		return;

	if ( pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN )
	{
		CTFMinigun *pMinigun = ( CTFMinigun * )pWeapon;

		int iBarrelBone = Studio_BoneIndexByName( hdr, "v_minigun_barrel" );

//		Assert( iBarrelBone != -1 );

		if ( iBarrelBone != -1 )
		{
			if ( hdr->boneFlags( iBarrelBone ) & boneMask )
			{
				RadianEuler a;
				QuaternionAngles( q[iBarrelBone], a );

				a.x = pMinigun->GetBarrelRotation();

				AngleQuaternion( a, q[iBarrelBone] );
			}
		}
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFViewModel::ProcessMuzzleFlashEvent()
{
	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )GetOwningWeapon();

	if ( !pWeapon || C_BasePlayer::ShouldDrawLocalPlayer() ) 
		return;

	pWeapon->ProcessMuzzleFlashEvent();
}


//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
int CTFViewModel::GetSkin()
{
	int nSkin = BaseClass::GetSkin();

	CTFWeaponBase *pWeapon = ( CTFWeaponBase * )GetOwningWeapon();

	if ( !pWeapon ) 
		return nSkin;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if ( pPlayer )
	{
		// See if the item wants to override the skin
		int iItemSkin = -1;
		CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
		if ( pItem->IsValid() )
		{
			iItemSkin = pItem->GetSkin( pPlayer->GetTeamNumber(), true );
		}

		if ( iItemSkin != -1 )
		{
			nSkin = iItemSkin;
		}
		else if ( pWeapon->GetTFWpnData().m_bHasTeamSkins_Viewmodel )
		{
			switch( pPlayer->GetTeamNumber() )
			{
			case TF_TEAM_RED:
				nSkin = 0;
				break;
			case TF_TEAM_BLUE:
				nSkin = 1;
				break;
			}
		}	
	}

	return nSkin;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* CTFViewModel::ModifyEventParticles( const char* token )
{
	CTFWeaponBase *pWeapon = (CTFWeaponBase*) GetOwningWeapon();
	if ( pWeapon )
	{
		return pWeapon->ModifyEventParticles( token );
	}
	return BaseClass::ModifyEventParticles( token );
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CViewModelInvisProxy : public CBaseInvisMaterialProxy
{
public:
	virtual void OnBind( C_BaseEntity *pC_BaseEntity );
};

#define TF_VM_MIN_INVIS		0.22
#define TF_VM_MAX_INVIS		0.5

//-----------------------------------------------------------------------------
// Purpose: 
// Input :
//-----------------------------------------------------------------------------
void CViewModelInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	bool bIsViewModel = false;

	CTFPlayer *pPlayer = NULL;
	C_BaseEntity *pMoveParent = pEnt->GetMoveParent();

	//Check if we have a move parent and if its a player
	if ( pMoveParent )
	{
		if ( pMoveParent->IsPlayer() )
		{
			pPlayer = ToTFPlayer( pMoveParent );
		}
	}
	
	//If its not a player then check for viewmodel.
	if ( pPlayer == NULL )
	{
		CBaseEntity *pEntParent = pMoveParent;

		if ( pEntParent == NULL )
		{
			pEntParent = pEnt;
		}

		CTFViewModel *pVM = dynamic_cast<CTFViewModel *>( pEntParent );

		if ( pVM )
		{
			pPlayer = ToTFPlayer( pVM->GetOwner() );
			bIsViewModel = true;
		}
	}

	// do we have a player from viewmodel?
	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}
	
	float flPercentInvisible = pPlayer->GetPercentInvisible();
	float flWeaponInvis = flPercentInvisible;

	if ( bIsViewModel == true )
	{
		// remap from 0.22 to 0.5
		// but drop to 0.0 if we're not invis at all
		flWeaponInvis = ( flPercentInvisible < 0.01 ) ?
			0.0 :
			RemapVal( flPercentInvisible, 0.0, 1.0, TF_VM_MIN_INVIS, TF_VM_MAX_INVIS );

		// Exaggerated blink effect on bump.
		if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
		{
			flWeaponInvis = 0.3f;
		}

		// Also exaggerate the effect if we're using motion cloak and our well has run dry.
		CTFWeaponInvis *pWpn = (CTFWeaponInvis *) pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS );
		if ( pWpn && pWpn->HasMotionCloak() && (pPlayer->m_Shared.GetSpyCloakMeter() <= 0.f ) )
		{
			flWeaponInvis = 0.3f;
		}
	}

	m_pPercentInvisible->SetFloatValue( flWeaponInvis );
}

EXPOSE_INTERFACE( CViewModelInvisProxy, IMaterialProxy, "vm_invis" IMATERIAL_PROXY_INTERFACE_VERSION );


//-----------------------------------------------------------------------------
// Purpose: Generic invis proxy that can handle invis for both weapons & viewmodels.
//			Makes the vm_invis & weapon_invis proxies obsolete, do not use them.
//-----------------------------------------------------------------------------
class CInvisProxy : public CBaseInvisMaterialProxy
{
public:
	virtual void OnBind( C_BaseEntity *pC_BaseEntity ) OVERRIDE;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInvisProxy::OnBind( C_BaseEntity *pC_BaseEntity )
{
	if( !m_pPercentInvisible )
		return;

	C_BaseEntity *pEnt = pC_BaseEntity;

	CTFPlayer *pPlayer = NULL;

	// Check if we have a move parent and if it's a player
	C_BaseEntity *pMoveParent = pEnt->GetMoveParent();
	if ( pMoveParent && pMoveParent->IsPlayer() )
	{
		pPlayer = ToTFPlayer( pMoveParent );
	}

	// If it's not a player then check for viewmodel.
	if ( !pPlayer )
	{
		CBaseEntity *pEntParent = pMoveParent ? pMoveParent : pEnt;

		CTFViewModel *pVM = dynamic_cast<CTFViewModel *>( pEntParent );
		if ( pVM )
		{
			pPlayer = ToTFPlayer( pVM->GetOwner() );
		}
	}
	
	if ( !pPlayer )
	{
		if ( pEnt->IsPlayer() )
		{
			pPlayer = dynamic_cast<C_TFPlayer*>( pEnt );
		}
		else
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner*>( pEnt );
			if ( pOwnerInterface )
			{
				pPlayer = ToTFPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}
	}
	
	if ( !pPlayer )
	{
		C_TFRagdoll *pRagdoll = dynamic_cast<C_TFRagdoll*>( pEnt );
		if ( !pRagdoll || !pRagdoll->IsCloaked() )
		{
			m_pPercentInvisible->SetFloatValue( 0.0f );
		}
		return;
	}

	// If we're the local player, use the old "vm_invis" code. Otherwise, use the "weapon_invis".
	if ( pPlayer->IsLocalPlayer() )
	{
		float flPercentInvisible = pPlayer->GetPercentInvisible();
		float flWeaponInvis = flPercentInvisible;

		// remap from 0.22 to 0.5
		// but drop to 0.0 if we're not invis at all
		flWeaponInvis = ( flPercentInvisible < 0.01 ) ?
			0.0 :
		RemapVal( flPercentInvisible, 0.0, 1.0, TF_VM_MIN_INVIS, TF_VM_MAX_INVIS );

		// Exaggerated blink effect on bump.
		if ( pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
		{
			flWeaponInvis = 0.3f;
		}

		// Also exaggerate the effect if we're using motion cloak and our well has run dry.
		CTFWeaponInvis *pWpn = (CTFWeaponInvis *) pPlayer->Weapon_OwnsThisID( TF_WEAPON_INVIS );
		if ( pWpn && pWpn->HasMotionCloak() && (pPlayer->m_Shared.GetSpyCloakMeter() <= 0.f ) )
		{
			flWeaponInvis = 0.3f;
		}

		m_pPercentInvisible->SetFloatValue( flWeaponInvis );
	}
	else
	{
		m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
	}
}

//	Generic invis proxy that can handle invis for both weapons & viewmodels.
//	Makes the vm_invis & weapon_invis proxies obsolete, do not use them.
EXPOSE_INTERFACE( CInvisProxy, IMaterialProxy, "invis" IMATERIAL_PROXY_INTERFACE_VERSION );


#endif // CLIENT_DLL
