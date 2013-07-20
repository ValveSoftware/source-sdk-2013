from srcpy.module_generators import SharedModuleGenerator
from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies
from pyplusplus import messages
from pygccxml.declarations import matchers, pointer_t, reference_t, declarated_t, const_t, float_t, cpptypes

class calldef_excludetypes(matchers.custom_matcher_t):
    def __init__(self, excludetypes=None):
        super(calldef_excludetypes, self).__init__(self.testdecl)
        
        self.excludetypes = excludetypes
        
    def __compare_types( self, type_or_str, type ):
        assert type_or_str
        if type is None:
            return False
        if isinstance( type_or_str, cpptypes.type_t ):
            if type_or_str != type:
                return False
        else:
            if type_or_str != type.decl_string:
                return False
        return True
        
    def testdecl( self, decl ):
        if self.excludetypes:
            if isinstance(self.excludetypes, (list, tuple)):
                excludetypes = self.excludetypes
            else:
                excludetypes = [self.excludetypes]
                
            for t in excludetypes:
                if self.__compare_types( t, decl.return_type ):
                    return True
        
                for arg in decl.arguments:
                    if self.__compare_types( t, arg.type ):
                        return True
        
        return False

class VMath(SharedModuleGenerator):
    module_name = '_vmath'
    
    files = [
        'fmtstr.h',
        'mathlib/mathlib.h',
        'mathlib/vector.h',
        'mathlib/vector2d.h',
        'mathlib/vmatrix.h',
    ]

    def Parse(self, mb):
        mb.decls().exclude()
        
        # Get item
        getitem_wrapper =    'static ::vec_t GetItem( %(cls_name)s const & inst, int i ) {\r\n' + \
                                '   if( i < 0 || i > %(nitems)s ) {\r\n' + \
                                '       PyErr_SetString(PyExc_IndexError, "Index out of range" );\r\n' + \
                                '       throw boost::python::error_already_set();\r\n' + \
                                '   }\r\n' + \
                                '   return inst[i];\r\n' + \
                                '}\r\n'
                                
        getitem_reg = '%(cls_name)s_exposer.def( "__getitem__", &::%(cls_name)s_wrapper::GetItem );\r\n'
        
        # Set item
        setitem_wrapper =    'static void SetItem( %(cls_name)s & inst, int i, ::vec_t v ) {\r\n' + \
                                '   if( i < 0 || i > %(nitems)s ) {\r\n' + \
                                '       PyErr_SetString(PyExc_IndexError, "Index out of range" );\r\n' + \
                                '       throw boost::python::error_already_set();\r\n' + \
                                '   }\r\n' + \
                                '   inst[i] = v;\r\n' + \
                                '}\r\n'
                                
        setitem_reg = '%(cls_name)s_exposer.def( "__setitem__", &::%(cls_name)s_wrapper::SetItem );\r\n'
        
        
        # String
        str_3d_wrapper = 'static boost::python::object Str( %(cls_name)s const & inst ) {\r\n' + \
                                '   char buf[256];\r\n' + \
                                '   Q_snprintf(buf, 256, "(%%f, %%f, %%f)", inst.x, inst.y, inst.z);\r\n' + \
                                '   return boost::python::object(buf);\r\n' + \
                                '}\r\n'
        str_2d_wrapper = 'static boost::python::object Str( Vector2D const & inst ) {\r\n' + \
                                '   char buf[256];\r\n' + \
                                '   Q_snprintf(buf, 256, "(%f, %f)", inst.x, inst.y);\r\n' + \
                                '   return boost::python::object(buf);\r\n' + \
                                '}\r\n'
                                
        str_vmatrix_wrapper = 'static boost::python::object Str( VMatrix const & inst ) {\r\n' + \
                                '   return boost::python::object(VMatToString(inst));\r\n' + \
                                '}\r\n'
                                
        str_reg =   '%(cls_name)s_exposer.def( "__str__", &::%(cls_name)s_wrapper::Str );\r\n'
        
        # Classes
        cls = mb.class_('Vector')
        cls.include()
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it

        cls.add_wrapper_code( getitem_wrapper % {'cls_name' : 'Vector', 'nitems' : '2' } )
        cls.add_registration_code( getitem_reg % {'cls_name':'Vector'}, False)
        cls.add_wrapper_code( setitem_wrapper % {'cls_name' : 'Vector', 'nitems' : '2' } )
        cls.add_registration_code( setitem_reg % {'cls_name':'Vector'}, False)
        
        cls = mb.class_('Vector2D')
        cls.include()
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it
        
        cls.add_wrapper_code( getitem_wrapper % {'cls_name' : 'Vector2D', 'nitems' : '1' } )
        cls.add_registration_code( getitem_reg % {'cls_name':'Vector2D'}, False)
        cls.add_wrapper_code( setitem_wrapper % {'cls_name' : 'Vector2D', 'nitems' : '1' } )
        cls.add_registration_code( setitem_reg % {'cls_name':'Vector2D'}, False)
        
        cls = mb.class_('QAngle')
        cls.include()
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it

        cls.add_wrapper_code( getitem_wrapper % {'cls_name' : 'QAngle', 'nitems' : '2' } )
        cls.add_registration_code( getitem_reg % {'cls_name':'QAngle'}, False)
        cls.add_wrapper_code( setitem_wrapper % {'cls_name' : 'QAngle', 'nitems' : '2' } )
        cls.add_registration_code( setitem_reg % {'cls_name':'QAngle'}, False)
        
        cls = mb.class_('Quaternion')
        cls.include()
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it
        
        # Call policies
        mb.mem_funs('AsVector2D').call_policies = call_policies.return_internal_reference()

        # Transform functions that take pointers as arguments
        mb.free_functions('SolveInverseQuadraticMonotonic').add_transformation( FT.output('a'), FT.output('b'), FT.output('c') )
        mb.free_functions('ComputeTrianglePlane').add_transformation( FT.output('intercept') )
        mb.free_functions('CalcSqrDistAndClosestPointOnAABB').add_transformation( FT.output('distSqrOut') )
        mb.free_functions('SolveInverseReciprocalQuadratic').add_transformation( FT.output('a'), FT.output('b'), FT.output('c') )
        mb.free_functions('SolveQuadratic').add_transformation( FT.output('root1'), FT.output('root2') )
        mb.free_functions('SolveInverseQuadratic').add_transformation( FT.output('a'), FT.output('b'), FT.output('c') )
        mb.free_functions('QuaternionAxisAngle').add_transformation( FT.output('axis'), FT.output('angle') )
        mb.free_functions('RotationDeltaAxisAngle').add_transformation( FT.output('deltaAxis'), FT.output('deltaAngle') )
        
        # Compressed color
        mb.class_('ColorRGBExp32').include()
        
        # cplane_t
        mb.class_('cplane_t').include()
        mb.class_('cplane_t').var('pad').exclude()
        
        # matrix3x4_t
        mb.class_('matrix3x4_t').include()
        mb.class_('matrix3x4_t').mem_opers().exclude()    
        
        # -----
        # Add custom item access functions to the Vector class
        mb.global_ns.mem_opers('[]').exclude()
        
        mb.class_('Vector').add_registration_code( 'def( bp::init< const Vector & >(( bp::arg("vOther") )) )')
        mb.class_('QAngle').add_registration_code( 'def( bp::init< const QAngle & >(( bp::arg("vOther") )) )')
        
        # Vars
        mb.vars('vec3_origin').include()
        mb.vars('vec3_angle').include()
        mb.vars('vec3_invalid').include()
        mb.vars('nanmask').include()
        
        # Mathlib.h functions
        mb.free_function('RandomAngularImpulse').include()
        
        mb.free_functions('VectorMaximum').include()
        mb.free_functions('VectorMAInline').include()
        mb.free_functions('VectorMA').include()
 
        mb.free_functions('RoundInt').include()
        mb.free_functions('Q_log2').include()
        mb.free_functions('SinCos').include()
        mb.free_functions('TableCos').include()
        mb.free_functions('TableSin').include()
        #mb.free_functions('IsPowerOfTwo').include()
        mb.free_functions('SmallestPowerOfTwoGreaterOrEqual').include()
        mb.free_functions('LargestPowerOfTwoLessThanOrEqual').include()
        mb.free_functions('FloorDivMod').include()
        mb.free_functions('GreatestCommonDivisor').include()
        mb.free_functions('IsDenormal').include()
        
        mb.free_functions('MatrixVectors').include()
        mb.free_functions('VectorRotate').include()
        mb.free_functions('TransformAnglesToLocalSpace').include()
        
        mb.free_functions('MatrixInitialize').include()
        mb.free_functions('MatrixCopy').include()
        mb.free_functions('MatrixInvert').include()
        mb.free_functions('MatricesAreEqual').include()
        mb.free_functions('MatrixGetColumn').include()
        mb.free_functions('MatrixSetColumn').include()
        mb.free_functions('ConcatRotations').include()
        mb.free_functions('ConcatTransforms').include()
        mb.free_functions('MatrixMultiply').include()

        mb.free_function('QuaternionSlerp').include()
        mb.free_function('QuaternionSlerpNoAlign').include()
        mb.free_function('QuaternionBlend').include()
        mb.free_function('QuaternionBlendNoAlign').include()
        mb.free_function('QuaternionIdentityBlend').include()
        mb.free_function('QuaternionAngleDiff').include()
        mb.free_function('QuaternionScale').include()
        mb.free_function('QuaternionDotProduct').include()
        mb.free_function('QuaternionConjugate').include()
        mb.free_function('QuaternionInvert').include()
        mb.free_function('QuaternionNormalize').include()
        mb.free_function('QuaternionAdd').include()
        mb.free_function('QuaternionMult').include()
        mb.free_functions('QuaternionMatrix').include()
        mb.free_functions('QuaternionAngles').include()
        mb.free_functions('AngleQuaternion').include()
        mb.free_function('QuaternionAxisAngle').include()
        mb.free_function('AxisAngleQuaternion').include()
        mb.free_function('BasisToQuaternion').include()
        mb.free_function('MatrixQuaternion').include()
        
        mb.free_functions('MatrixRowDotProduct').include()
        mb.free_functions('MatrixColumnDotProduct').include()
        
        mb.free_functions('anglemod').include()
        mb.free_functions('RemapVal').include()
        mb.free_functions('RemapValClamped').include()
        mb.free_functions('Lerp').include()
        mb.free_functions('Sqr').include()
        mb.free_functions('FLerp').include()
        mb.free_functions('Sign').include()
        mb.free_functions('ClampArrayBounds').include()
        
        mb.free_functions('AngleVectors').include()
        mb.free_functions('AngleVectors').include()
        mb.free_functions('AngleVectorsTranspose').include()
        mb.free_functions('AngleMatrix').include()
        mb.free_functions('AngleMatrix').include()
        mb.free_functions('AngleIMatrix').include()
        mb.free_functions('VectorAngles').include()
        mb.free_functions('VectorMatrix').include()
        mb.free_functions('VectorVectors').include()
        mb.free_functions('SetIdentityMatrix').include()
        mb.free_functions('SetScaleMatrix').include()
        mb.free_functions('MatrixBuildRotationAboutAxis').include()
        
        mb.free_functions('MatrixTranspose').include()
        mb.free_functions('MatrixInverseTranspose').include()
        mb.free_functions('PositionMatrix').include()
        mb.free_functions('MatrixPosition').include()
        mb.free_functions('VectorRotate').include()
        mb.free_functions('VectorIRotate').include()
        mb.free_functions('MatrixAngles').include()
        mb.free_functions('VectorCompare').include()
        mb.free_functions('VectorTransform').include()
        mb.free_functions('VectorITransform').include()
        
        mb.free_functions('BoxOnPlaneSide').include()
        mb.free_functions('VectorFill').include()
        mb.free_functions('VectorNegate').include()
        mb.free_functions('VectorAvg').include()
        mb.free_functions('BoxOnPlaneSide2').include()
        mb.free_functions('ClearBounds').include()
        mb.free_functions('AddPointToBounds').include()
        
        mb.free_functions('BuildGammaTable').include()
        mb.free_functions('TexLightToLinear').include()
        mb.free_functions('LinearToTexture').include()
        mb.free_functions('LinearToScreenGamma').include()
        mb.free_functions('TextureToLinear').include()
        
        mb.free_functions('SolveQuadratic').include()
        mb.free_functions('SolveInverseQuadratic').include()
        mb.free_functions('SolveInverseQuadraticMonotonic').include()
        mb.free_functions('SolveInverseReciprocalQuadratic').include()
        
        mb.free_functions('VectorYawRotate').include()
        
        mb.free_functions('Bias').include()
        mb.free_functions('Gain').include()
        mb.free_functions('SmoothCurve').include()
        mb.free_functions('SmoothCurve_Tweak').include()
        mb.free_functions('ExponentialDecay').include()
        mb.free_functions('ExponentialDecay').include()
        mb.free_functions('ExponentialDecayIntegral').include()
        mb.free_functions('SimpleSpline').include()
        mb.free_functions('SimpleSplineRemapVal').include()
        mb.free_functions('SimpleSplineRemapValClamped').include()
        mb.free_functions('RoundFloatToInt').include()
        mb.free_functions('RoundFloatToByte').include()
        mb.free_functions('RoundFloatToUnsignedLong').include()
        mb.free_functions('IsIntegralValue').include()
        mb.free_functions('Float2Int').include()
        mb.free_functions('Floor2Int').include()
        mb.free_functions('FastFToC').include()
        mb.free_functions('ClampToMsec').include()
        mb.free_functions('Ceil2Int').include()
        
        mb.free_functions('GetBarycentricCoords2D').include()
        mb.free_functions('QuickBoxSphereTest').include()
        mb.free_functions('QuickBoxIntersectTest').include()
        
        mb.free_functions('GammaToLinearFullRange').include()
        mb.free_functions('LinearToGammaFullRange').include()
        mb.free_functions('GammaToLinear').include()
        mb.free_functions('LinearToGamma').include()
        mb.free_functions('SrgbGammaToLinear').include()
        mb.free_functions('SrgbLinearToGamma').include()
        mb.free_functions('X360GammaToLinear').include()
        mb.free_functions('X360LinearToGamma').include()
        mb.free_functions('SrgbGammaTo360Gamma').include()
        mb.free_functions('LinearToVertexLight').include()
        mb.free_functions('LinearToLightmap').include()
        mb.free_functions('ColorClamp').include()
        mb.free_functions('ColorClampTruncate').include()
        
        mb.free_functions('Catmull_Rom_Spline').include()
        mb.free_functions('Catmull_Rom_Spline_Tangent').include()
        mb.free_functions('Catmull_Rom_Spline_Integral').include()
        mb.free_functions('Catmull_Rom_Spline_Integral').include()
        mb.free_functions('Catmull_Rom_Spline_Normalize').include()
        mb.free_functions('Catmull_Rom_Spline_Integral_Normalize').include()
        mb.free_functions('Catmull_Rom_Spline_NormalizeX').include()
        mb.free_functions('Catmull_Rom_Spline_NormalizeX').include()
        mb.free_functions('Hermite_Spline').include()
        #mb.free_functions('Hermite_SplineBasis').include()
        mb.free_functions('Kochanek_Bartels_Spline').include()
        mb.free_functions('Kochanek_Bartels_Spline_NormalizeX').include()
        mb.free_functions('Cubic_Spline').include()
        mb.free_functions('Cubic_Spline_NormalizeX').include()
        mb.free_functions('BSpline').include()
        mb.free_functions('BSpline_NormalizeX').include()
        mb.free_functions('Parabolic_Spline').include()
        mb.free_functions('Parabolic_Spline_NormalizeX').include()
        mb.free_functions('QuinticInterpolatingPolynomial').include()
        #mb.free_functions('GetInterpolationData').include()
        mb.free_functions('RangeCompressor').include()
        
        mb.free_functions('CalcSqrDistanceToAABB').include()
        mb.free_functions('CalcClosestPointOnAABB').include()
        mb.free_functions('CalcSqrDistAndClosestPointOnAABB').include()
        mb.free_functions('CalcDistanceToAABB').include()
        
        mb.free_functions('CalcClosestPointOnLine').include()
        mb.free_functions('CalcDistanceToLine').include()
        mb.free_functions('CalcDistanceSqrToLine').include()
        mb.free_functions('CalcClosestPointOnLineSegment').include()
        mb.free_functions('CalcDistanceToLineSegment').include()
        mb.free_functions('CalcDistanceSqrToLineSegment').include()
        mb.free_functions('CalcLineToLineIntersectionSegment').include()
        mb.free_functions('CalcClosestPointOnLine2D').include()
        mb.free_functions('CalcDistanceToLine2D').include()
        mb.free_functions('CalcDistanceSqrToLine2D').include()
        mb.free_functions('CalcClosestPointOnLineSegment2D').include()
        mb.free_functions('CalcDistanceToLineSegment2D').include()
        mb.free_functions('CalcDistanceSqrToLineSegment2D').include()
        
        mb.free_functions('Approach').include()
        mb.free_functions('ApproachAngle').include()
        mb.free_functions('AngleDiff').include()
        mb.free_functions('AngleDistance').include()
        mb.free_functions('AngleNormalize').include()
        mb.free_functions('AngleNormalizePositive').include()
        mb.free_functions('AnglesAreEqual').include()
        mb.free_functions('RotationDeltaAxisAngle').include()
        mb.free_functions('RotationDelta').include()
        mb.free_functions('ComputeTrianglePlane').include()
        mb.free_functions('PolyFromPlane').include()
        mb.free_functions('ClipPolyToPlane').include()
        mb.free_functions('ClipPolyToPlane_Precise').include()
        mb.free_functions('CalcTriangleTangentSpace').include()
        
        mb.free_functions('TransformAABB').include()
        mb.free_functions('ITransformAABB').include()
        mb.free_functions('RotateAABB').include()
        mb.free_functions('IRotateAABB').include()
        mb.free_functions('MatrixTransformPlane').include()
        mb.free_functions('MatrixITransformPlane').include()
        mb.free_functions('CeilPow2').include()
        mb.free_functions('FloorPow2').include()
        mb.free_functions('RGBtoHSV').include()
        mb.free_functions('HSVtoRGB').include()
        
        # Vector.h functions
        mb.free_functions('VectorClear').include()
        mb.free_functions('VectorCopy').include()
        mb.free_functions('VectorAdd').include()
        mb.free_functions('VectorSubtract').include()
        mb.free_functions('VectorMultiply').include()
        mb.free_functions('VectorDivide').include()
        mb.free_functions('VectorScale').include()
        mb.free_functions('VectorMA').include()
        mb.free_functions('VectorsAreEqual').include()
        mb.free_functions('ComputeClosestPoint').include()
        mb.free_functions('VectorAbs').include()
        mb.free_functions('VectorLength').include()
        mb.free_functions('DotProduct').include()
        mb.free_functions('CrossProduct').include()
        mb.free_functions('VectorMin').include()
        mb.free_functions('VectorMax').include()
        mb.free_functions('VectorLerp').include()
        mb.free_functions('RandomVector').include()
        
        mb.free_functions('QAnglesAreEqual').include()
        #mb.free_functions('QAngleToAngularImpulse').include()
        #mb.free_functions('AngularImpulseToQAngle').include()
        
        mb.free_functions('VectorNormalize').include()
        mb.free_functions('VectorNormalizeFast').include()
        
        # Vector2d.h functions
        mb.free_functions('Vector2DClear').include()
        mb.free_functions('Vector2DCopy').include()
        mb.free_functions('Vector2DAdd').include()
        mb.free_functions('Vector2DSubtract').include()
        mb.free_functions('Vector2DMultiply').include()
        mb.free_functions('Vector2DDivide').include()
        mb.free_functions('Vector2DMA').include()
        mb.free_functions('Vector2DMin').include()
        mb.free_functions('Vector2DMax').include()
        mb.free_functions('Vector2DLength').include()
        mb.free_functions('DotProduct2D').include()
        mb.free_functions('Vector2DLerp').include()
        mb.free_functions('Vector2DNormalize').include()
        mb.free_functions('ComputeClosestPoint2D').include()
        
        # QAngle functions
        mb.free_function('RandomAngle').include()
        
        # VMatrix
        cls = mb.class_('VMatrix')
        cls.include()
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it
        cls.mem_opers('[]').exclude()
        cls.mem_funs('Base').exclude()
        cls.mem_funs('As3x4').exclude()
        cls.mem_funs('GetTranslation').exclude()
        cls.vars('m').exclude()
        
        cls.add_wrapper_code( str_vmatrix_wrapper % {'cls_name':'VMatrix'} )
        cls.add_registration_code( str_reg % {'cls_name':'VMatrix'}, False)
        
        mb.free_functions('MatrixSetIdentity').include()
        mb.free_functions('MatrixTranspose').include()
        mb.free_functions('MatrixCopy').include()
        mb.free_functions('MatrixMultiply').include()
        
        mb.free_functions('MatrixGetColumn').include()
        mb.free_functions('MatrixSetColumn').include()
        mb.free_functions('MatrixGetRow').include()
        mb.free_functions('MatrixSetRow').include()
        
        mb.free_functions('MatrixTranslate').include()
        mb.free_functions('MatrixBuildRotationAboutAxis').include()
        mb.free_functions('MatrixBuildRotateZ').include()
        mb.free_functions('MatrixRotate').include()
        
        mb.free_functions('MatrixFromAngles').include()
        mb.free_functions('MatrixToAngles').include()

        # Exclude
        #mb.vars('pfVectorNormalizeFast').exclude()
        #mb.vars('pfVectorNormalize').exclude()
        #mb.vars('pfInvRSquared').exclude()
        mb.vars('m_flMatVal').exclude()
        mb.vars('quat_identity').exclude() # <- Does not even exist except for a declaration?
        
        # Exclude some functions
        mb.mem_funs('Base').exclude()           # Base gives a pointer to the address of the data. Not very python like.
        mb.free_functions( 'AllocTempVector' ).exclude()
        mb.class_('Vector2D').mem_funs('Cross').exclude()   # Declaration only?
        mb.free_function('ConcatRotations').exclude() # Declaration only?  
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        # Remove any function with "float *" values
        # A lot of functions have two versions (or more), of which one takes "float *" arguments
        excludetypes = [
            pointer_t(float_t()),
            pointer_t(const_t(float_t())),
        ]
        mb.calldefs( calldef_excludetypes( excludetypes ) ).exclude()
        
        # Silent warnings of generating class wrappers
        mb.classes().disable_warnings( messages.W1027 )
