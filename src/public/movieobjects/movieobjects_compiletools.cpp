//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: See notes below
//
//=============================================================================

#include "movieobjects/movieobjects.h"

// YOU MUST INCLUDE THIS FILE INTO ANY PROJECT WHICH USES THE movieobjects.lib FILE
// This hack causes the class factories for the element types to be imported into the compiled code...

// Basic type (in datamodel)
USING_ELEMENT_FACTORY( DmElement );

// Movie types
USING_ELEMENT_FACTORY( DmeTransform );
USING_ELEMENT_FACTORY( DmeTransformList );
USING_ELEMENT_FACTORY( DmeVertexData );
USING_ELEMENT_FACTORY( DmeMesh );
USING_ELEMENT_FACTORY( DmeDag );
USING_ELEMENT_FACTORY( DmeFaceSet );
USING_ELEMENT_FACTORY( DmeModel );
USING_ELEMENT_FACTORY( DmeJoint );
USING_ELEMENT_FACTORY( DmeAttachment );
USING_ELEMENT_FACTORY( DmeMakefile );
USING_ELEMENT_FACTORY( DmeMDLMakefile );
USING_ELEMENT_FACTORY( DmeDCCMakefile );
USING_ELEMENT_FACTORY( DmeMayaMakefile );
USING_ELEMENT_FACTORY( DmeXSIMakefile );
USING_ELEMENT_FACTORY( DmeSourceDCCFile );
USING_ELEMENT_FACTORY( DmeSourceMayaFile );
USING_ELEMENT_FACTORY( DmeSourceXSIFile );
USING_ELEMENT_FACTORY( DmeAnimationList );
USING_ELEMENT_FACTORY( DmeClip );
USING_ELEMENT_FACTORY( DmeChannelsClip );
USING_ELEMENT_FACTORY( DmeChannel );
USING_ELEMENT_FACTORY( DmeTimeFrame );
USING_ELEMENT_FACTORY( DmeTrackGroup );
USING_ELEMENT_FACTORY( DmeTrack );

USING_ELEMENT_FACTORY( DmeCombinationDominationRule );
USING_ELEMENT_FACTORY( DmeCombinationInputControl );
USING_ELEMENT_FACTORY( DmeCombinationOperator );

USING_ELEMENT_FACTORY( DmeIntLog );
USING_ELEMENT_FACTORY( DmeFloatLog );
USING_ELEMENT_FACTORY( DmeBoolLog );
USING_ELEMENT_FACTORY( DmeColorLog );
USING_ELEMENT_FACTORY( DmeVector2Log );
USING_ELEMENT_FACTORY( DmeVector3Log );
USING_ELEMENT_FACTORY( DmeVector4Log );
USING_ELEMENT_FACTORY( DmeQAngleLog );
USING_ELEMENT_FACTORY( DmeQuaternionLog );
USING_ELEMENT_FACTORY( DmeVMatrixLog );

USING_ELEMENT_FACTORY( DmeIntLogLayer );
USING_ELEMENT_FACTORY( DmeFloatLogLayer );
USING_ELEMENT_FACTORY( DmeBoolLogLayer );
USING_ELEMENT_FACTORY( DmeColorLogLayer );
USING_ELEMENT_FACTORY( DmeVector2LogLayer );
USING_ELEMENT_FACTORY( DmeVector3LogLayer );
USING_ELEMENT_FACTORY( DmeVector4LogLayer );
USING_ELEMENT_FACTORY( DmeQAngleLogLayer );
USING_ELEMENT_FACTORY( DmeQuaternionLogLayer );
USING_ELEMENT_FACTORY( DmeVMatrixLogLayer );

USING_ELEMENT_FACTORY( DmeIntCurveInfo );
USING_ELEMENT_FACTORY( DmeFloatCurveInfo );
USING_ELEMENT_FACTORY( DmeBoolCurveInfo );
USING_ELEMENT_FACTORY( DmeColorCurveInfo );
USING_ELEMENT_FACTORY( DmeVector2CurveInfo );
USING_ELEMENT_FACTORY( DmeVector3CurveInfo );
USING_ELEMENT_FACTORY( DmeVector4CurveInfo );
USING_ELEMENT_FACTORY( DmeQAngleCurveInfo );
USING_ELEMENT_FACTORY( DmeQuaternionCurveInfo );
USING_ELEMENT_FACTORY( DmeVMatrixCurveInfo );

USING_ELEMENT_FACTORY( DmeComponent );
USING_ELEMENT_FACTORY( DmeSingleIndexedComponent );
USING_ELEMENT_FACTORY( DmeDrawSettings );
USING_ELEMENT_FACTORY( DmeEyePosition );
USING_ELEMENT_FACTORY( DmeEyeball );
