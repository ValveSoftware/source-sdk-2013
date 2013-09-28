#ifndef C_FIRSTPERSONBODY_H
#define C_FIRSTPERSONBODY_H


class C_FirstpersonBody : public C_BaseAnimating
{
	DECLARE_CLASS( C_FirstpersonBody, C_BaseAnimating );
public:
	C_FirstpersonBody();

	virtual int ObjectCaps() {
		return FCAP_DONT_SAVE;
	};

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q,
		const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual CStudioHdr *OnNewModel();

	virtual ShadowType_t ShadowCastType();

	virtual int DrawModel( int flags );

	int m_iPoseParam_MoveYaw;

private:
	int m_iBoneNeck;
	int m_iBoneArmL;
	int m_iBoneArmR;

};


#endif