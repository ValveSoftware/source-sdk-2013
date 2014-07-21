//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef ANIMATION_H
#define ANIMATION_H

#define ACTIVITY_NOT_AVAILABLE		-1

struct animevent_t;
struct studiohdr_t;
class CStudioHdr;
struct mstudioseqdesc_t;

int ExtractBbox( CStudioHdr *pstudiohdr, int sequence, Vector& mins, Vector& maxs );

void IndexModelSequences( CStudioHdr *pstudiohdr );
void ResetActivityIndexes( CStudioHdr *pstudiohdr );
void VerifySequenceIndex( CStudioHdr *pstudiohdr );
int SelectWeightedSequence( CStudioHdr *pstudiohdr, int activity, int curSequence = -1 );
int SelectHeaviestSequence( CStudioHdr *pstudiohdr, int activity );
void SetEventIndexForSequence( mstudioseqdesc_t &seqdesc );
void BuildAllAnimationEventIndexes( CStudioHdr *pstudiohdr );
void ResetEventIndexes( CStudioHdr *pstudiohdr );

void GetEyePosition( CStudioHdr *pstudiohdr, Vector &vecEyePosition );

int LookupActivity( CStudioHdr *pstudiohdr, const char *label );
int LookupSequence( CStudioHdr *pstudiohdr, const char *label );

#define NOMOTION 99999
void GetSequenceLinearMotion( CStudioHdr *pstudiohdr, int iSequence, const float poseParameter[], Vector *pVec );

const char *GetSequenceName( CStudioHdr *pstudiohdr, int sequence );
const char *GetSequenceActivityName( CStudioHdr *pstudiohdr, int iSequence );

int GetSequenceFlags( CStudioHdr *pstudiohdr, int sequence );
int GetAnimationEvent( CStudioHdr *pstudiohdr, int sequence, animevent_t *pNPCEvent, float flStart, float flEnd, int index );
bool HasAnimationEventOfType( CStudioHdr *pstudiohdr, int sequence, int type );

int FindTransitionSequence( CStudioHdr *pstudiohdr, int iCurrentSequence, int iGoalSequence, int *piDir );
bool GotoSequence( CStudioHdr *pstudiohdr, int iCurrentSequence, float flCurrentCycle, float flCurrentRate, int iGoalSequence, int &nNextSequence, float &flNextCycle, int &iNextDir );

void SetBodygroup( CStudioHdr *pstudiohdr, int& body, int iGroup, int iValue );
int GetBodygroup( CStudioHdr *pstudiohdr, int body, int iGroup );

const char *GetBodygroupName( CStudioHdr *pstudiohdr, int iGroup );
int FindBodygroupByName( CStudioHdr *pstudiohdr, const char *name );
int GetBodygroupCount( CStudioHdr *pstudiohdr, int iGroup );
int GetNumBodyGroups( CStudioHdr *pstudiohdr );

int GetSequenceActivity( CStudioHdr *pstudiohdr, int sequence, int *pweight = NULL );

void GetAttachmentLocalSpace( CStudioHdr *pstudiohdr, int attachIndex, matrix3x4_t &pLocalToWorld );

float SetBlending( CStudioHdr *pstudiohdr, int sequence, int *pblendings, int iBlender, float flValue );

int FindHitboxSetByName( CStudioHdr *pstudiohdr, const char *name );
const char *GetHitboxSetName( CStudioHdr *pstudiohdr, int setnumber );
int GetHitboxSetCount( CStudioHdr *pstudiohdr );

#endif	//ANIMATION_H
