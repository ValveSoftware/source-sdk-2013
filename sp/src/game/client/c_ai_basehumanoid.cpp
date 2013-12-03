//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if 0

class C_AI_BaseHumanoid : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_AI_BaseHumanoid, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

	C_AI_BaseHumanoid();

	// model specific
	virtual bool	Interpolate( float currentTime );
	virtual	void	StandardBlendingRules( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	float				m_recanimtime[3];
	AnimationLayer_t	m_Layer[4][3];
};



C_AI_BaseHumanoid::C_AI_BaseHumanoid()
{
	memset(m_recanimtime, 0, sizeof(m_recanimtime));
	memset(m_Layer, 0, sizeof(m_Layer));
}


BEGIN_RECV_TABLE_NOBASE(AnimationLayer_t, DT_Animationlayer)
	RecvPropInt(RECVINFO_NAME(nSequence,sequence)),
	RecvPropFloat(RECVINFO_NAME(flCycle,cycle)),
	RecvPropFloat(RECVINFO_NAME(flPlaybackrate,playbackrate)),
	RecvPropFloat(RECVINFO_NAME(flWeight,weight))
END_RECV_TABLE()



IMPLEMENT_CLIENTCLASS_DT(C_AI_BaseHumanoid, DT_BaseHumanoid, CAI_BaseHumanoid)
	/*
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[0][2],m_Layer0),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[1][2],m_Layer1),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[2][2],m_Layer2),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	RecvPropDataTable(RECVINFO_DTNAME(m_Layer[3][2],m_Layer3),0, &REFERENCE_RECV_TABLE(DT_Animationlayer)),
	*/
	RecvPropInt(RECVINFO_NAME(m_Layer[0][2].nSequence,sequence0)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[0][2].flCycle,cycle0)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[0][2].flPlaybackrate,playbackrate0)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[0][2].flWeight,weight0)),
	RecvPropInt(RECVINFO_NAME(m_Layer[1][2].nSequence,sequence1)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[1][2].flCycle,cycle1)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[1][2].flPlaybackrate,playbackrate1)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[1][2].flWeight,weight1)),
	RecvPropInt(RECVINFO_NAME(m_Layer[2][2].nSequence,sequence2)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[2][2].flCycle,cycle2)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[2][2].flPlaybackrate,playbackrate2)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[2][2].flWeight,weight2)),
	RecvPropInt(RECVINFO_NAME(m_Layer[3][2].nSequence,sequence3)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[3][2].flCycle,cycle3)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[3][2].flPlaybackrate,playbackrate3)),
	RecvPropFloat(RECVINFO_NAME(m_Layer[3][2].flWeight,weight3))
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_AI_BaseHumanoid::StandardBlendingRules( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	VPROF( "C_AI_BaseHumanoid::StandardBlendingRules" );

	BaseClass::StandardBlendingRules( pStudioHdr, pos, q, currentTime, boneMask );

	if ( !hdr )
	{
		return;
	}

#if 0
	float poseparam[MAXSTUDIOPOSEPARAM];

	if ( GetSequence() >= hdr->numseq ) 
	{
		SetSequence( 0 );
	}

	// interpolate pose parameters
	for (int i = 0; i < hdr->numposeparameters; i++)
	{
		poseparam[ i ] = m_flPoseParameter[i];
	}

	// build root animation
	float fCycle = GetCycle();
	CalcPose( hdr, NULL, pos, q, GetSequence(), fCycle, poseparam );

	// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), 0, 0, "%30s %6.2f : %6.2f", hdr->pSeqdesc( GetSequence() )->pszLabel( ), fCycle, 1.0 );

	MaintainSequenceTransitions( hdr, fCycle, poseparam, pos, q, boneMask );

#if 1
	for (i = 0; i < 4; i++)
	{
		if (m_Layer[i][2].nSequence != m_Layer[i][1].nSequence)
		{
			if (m_Layer[i][2].flWeight > 0.5) m_Layer[i][1].flWeight = 1.0; else m_Layer[i][1].flWeight = 0;
		}
	}
#endif

#if 1
	for (i = 0; i < 4; i++)
	{
		Vector		pos2[MAXSTUDIOBONES];
		Quaternion	q2[MAXSTUDIOBONES];
		float fWeight = m_Layer[i][1].flWeight * (1 - dadt) + m_Layer[i][2].flWeight * dadt;

		/*
		debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), -i - 1, 0, 
			"%2d %6.2f %6.2f : %2d %6.2f %6.2f : %2d %6.2f %6.2f", 
				m_Layer[i][0].nSequence, m_Layer[i][0].flCycle, m_Layer[i][0].flWeight,
				m_Layer[i][1].nSequence, m_Layer[i][1].flCycle, m_Layer[i][1].flWeight,
				m_Layer[i][2].nSequence, m_Layer[i][2].flCycle, m_Layer[i][2].flWeight );
		*/

		if (fWeight > 0)
		{
			mstudioseqdesc_t *pseqdesc = hdr->pSeqdesc( m_Layer[i][2].nSequence );

			float fCycle =  m_Layer[i][2].flCycle;

			// UNDONE: Do IK here.
			CalcPose( hdr, NULL, pos2, q2, m_Layer[i][2].nSequence, fCycle, poseparam );

			if (fWeight > 1)
				fWeight = 1;
			SlerpBones( hdr, q, pos, pseqdesc, q2, pos2, fWeight );

			engine->Con_NPrintf( 10 + i, "%30s %6.2f : %6.2f", pseqdesc->pszLabel(), fCycle, fWeight );
		}
		else
		{
			engine->Con_NPrintf( 10 + i, "%30s %6.2f : %6.2f", "            ", 0, 0 );
		}

	}
#endif

	CIKContext auto_ik;
	auto_ik.Init( hdr, GetRenderAngles(), GetRenderOrigin(), gpGlobals->curtime );
	CalcAutoplaySequences( hdr, &auto_ik, pos, q, poseparam, boneMask, currentTime );

	float controllers[MAXSTUDIOBONECTRLS];
	GetBoneControllers(controllers);
	CalcBoneAdj( hdr, pos, q, controllers );
#endif
}


#endif
