/******************************************************************************

 Copyright (c) 1999 Advanced Micro Devices, Inc.

 LIMITATION OF LIABILITY:  THE MATERIALS ARE PROVIDED *AS IS* WITHOUT ANY
 EXPRESS OR IMPLIED WARRANTY OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY,
 NONINFRINGEMENT OF THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY
 PARTICULAR PURPOSE.  IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY
 DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
 BUSINESS INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
 INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY
 OF SUCH DAMAGES.  BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION OR LIMITATION
 OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE LIMITATION MAY
 NOT APPLY TO YOU.

 AMD does not assume any responsibility for any errors which may appear in the
 Materials nor any responsibility to support or update the Materials.  AMD retains
 the right to make changes to its test specifications at any time, without notice.

 NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
 further information, software, technical information, know-how, or show-how
 available to you.

 So that all may benefit from your experience, please report  any  problems
 or  suggestions about this software to 3dsdk.support@amd.com

 AMD Developer Technologies, M/S 585
 Advanced Micro Devices, Inc.
 5900 E. Ben White Blvd.
 Austin, TX 78741
 3dsdk.support@amd.com

*******************************************************************************

 AMD3DX.H

 MACRO FORMAT
 ============
 This file contains inline assembly macros that
 generate AMD-3D instructions in binary format.
 Therefore, C or C++ programmer can use AMD-3D instructions
 without any penalty in their C or C++ source code.

 The macro's name and format conventions are as follow:


 1. First argument of macro is a destination and
    second argument is a source operand.
      ex) _asm PFCMPEQ (mm3, mm4)
                         |    |
                        dst  src

 2. The destination operand can be m0 to m7 only.
    The source operand can be any one of the register
    m0 to m7 or _eax, _ecx, _edx, _ebx, _esi, or _edi
    that contains effective address.
      ex) _asm PFRCP    (MM7, MM6)
      ex) _asm PFRCPIT2 (mm0, mm4)
      ex) _asm PFMUL    (mm3, _edi)

  3. The prefetch(w) takes one src operand _eax, ecx, _edx,
     _ebx, _esi, or _edi that contains effective address.
      ex) _asm PREFETCH (_edi)

 For WATCOM C/C++ users, when using #pragma aux instead if 
 _asm, all macro names should be prefixed by a p_ or P_. 
 Macros should not be enclosed in quotes.
              ex) p_pfrcp (MM7,MM6)

 NOTE: Not all instruction macros, nor all possible
       combinations of operands have been explicitely
       tested. If any errors are found, please report
       them.

 EXAMPLE
 =======
 Following program doesn't do anything but it shows you
 how to use inline assembly AMD-3D instructions in C.
 Note that this will only work in flat memory model which
 segment registers cs, ds, ss and es point to the same
 linear address space total less than 4GB.

 Used Microsoft VC++ 5.0

 #include <stdio.h>
 #include "amd3d.h"

 void main ()
 {
      float x = (float)1.25;
      float y = (float)1.25;
      float z, zz;

     _asm {
              movd mm1, x
              movd mm2, y
              pfmul (mm1, mm2)
              movd z, mm1
              femms
      }

      printf ("value of z = %f\n", z);

      //
      // Demonstration of using the memory instead of
      // multimedia register
      //
      _asm {
              movd mm3, x
              lea esi, y   // load effective address of y
              pfmul (mm3, _esi)
              movd zz, mm3
              femms
      }

      printf ("value of zz = %f\n", zz);
  }

 #pragma aux EXAMPLE with WATCOM C/C++ v11.x
 ===========================================

    extern void Add(float *__Dest, float *__A, float *__B);
    #pragma aux Add =               \
            p_femms                 \
            "movd mm6,[esi]"        \
            p_pfadd(mm6,_edi)       \
            "movd [ebx],mm6"        \
            p_femms                 \
            parm [ebx] [esi] [edi];

*******************************************************************************/

#ifndef _K3DMACROSINCLUDED_
#define _K3DMACROSINCLUDED_

#if defined (__WATCOMC__)

// The WATCOM C/C++ version of the 3DNow! macros.
//
// The older, compbined register style for WATCOM C/C++ macros is not 
// supported.

/* Operand defines for instructions two operands */
#define _k3d_mm0_mm0 0xc0
#define _k3d_mm0_mm1 0xc1
#define _k3d_mm0_mm2 0xc2
#define _k3d_mm0_mm3 0xc3
#define _k3d_mm0_mm4 0xc4
#define _k3d_mm0_mm5 0xc5
#define _k3d_mm0_mm6 0xc6
#define _k3d_mm0_mm7 0xc7
#define _k3d_mm0_eax 0x00
#define _k3d_mm0_ecx 0x01
#define _k3d_mm0_edx 0x02
#define _k3d_mm0_ebx 0x03
#define _k3d_mm0_esi 0x06
#define _k3d_mm0_edi 0x07
#define _k3d_mm1_mm0 0xc8
#define _k3d_mm1_mm1 0xc9
#define _k3d_mm1_mm2 0xca
#define _k3d_mm1_mm3 0xcb
#define _k3d_mm1_mm4 0xcc
#define _k3d_mm1_mm5 0xcd
#define _k3d_mm1_mm6 0xce
#define _k3d_mm1_mm7 0xcf
#define _k3d_mm1_eax 0x08
#define _k3d_mm1_ecx 0x09
#define _k3d_mm1_edx 0x0a
#define _k3d_mm1_ebx 0x0b
#define _k3d_mm1_esi 0x0e
#define _k3d_mm1_edi 0x0f
#define _k3d_mm2_mm0 0xd0
#define _k3d_mm2_mm1 0xd1
#define _k3d_mm2_mm2 0xd2
#define _k3d_mm2_mm3 0xd3
#define _k3d_mm2_mm4 0xd4
#define _k3d_mm2_mm5 0xd5
#define _k3d_mm2_mm6 0xd6
#define _k3d_mm2_mm7 0xd7
#define _k3d_mm2_eax 0x10
#define _k3d_mm2_ecx 0x11
#define _k3d_mm2_edx 0x12
#define _k3d_mm2_ebx 0x13
#define _k3d_mm2_esi 0x16
#define _k3d_mm2_edi 0x17
#define _k3d_mm3_mm0 0xd8
#define _k3d_mm3_mm1 0xd9
#define _k3d_mm3_mm2 0xda
#define _k3d_mm3_mm3 0xdb
#define _k3d_mm3_mm4 0xdc
#define _k3d_mm3_mm5 0xdd
#define _k3d_mm3_mm6 0xde
#define _k3d_mm3_mm7 0xdf
#define _k3d_mm3_eax 0x18
#define _k3d_mm3_ecx 0x19
#define _k3d_mm3_edx 0x1a
#define _k3d_mm3_ebx 0x1b
#define _k3d_mm3_esi 0x1e
#define _k3d_mm3_edi 0x1f
#define _k3d_mm4_mm0 0xe0
#define _k3d_mm4_mm1 0xe1
#define _k3d_mm4_mm2 0xe2
#define _k3d_mm4_mm3 0xe3
#define _k3d_mm4_mm4 0xe4
#define _k3d_mm4_mm5 0xe5
#define _k3d_mm4_mm6 0xe6
#define _k3d_mm4_mm7 0xe7
#define _k3d_mm4_eax 0x20
#define _k3d_mm4_ecx 0x21
#define _k3d_mm4_edx 0x22
#define _k3d_mm4_ebx 0x23
#define _k3d_mm4_esi 0x26
#define _k3d_mm4_edi 0x27
#define _k3d_mm5_mm0 0xe8
#define _k3d_mm5_mm1 0xe9
#define _k3d_mm5_mm2 0xea
#define _k3d_mm5_mm3 0xeb
#define _k3d_mm5_mm4 0xec
#define _k3d_mm5_mm5 0xed
#define _k3d_mm5_mm6 0xee
#define _k3d_mm5_mm7 0xef
#define _k3d_mm5_eax 0x28
#define _k3d_mm5_ecx 0x29
#define _k3d_mm5_edx 0x2a
#define _k3d_mm5_ebx 0x2b
#define _k3d_mm5_esi 0x2e
#define _k3d_mm5_edi 0x2f
#define _k3d_mm6_mm0 0xf0
#define _k3d_mm6_mm1 0xf1
#define _k3d_mm6_mm2 0xf2
#define _k3d_mm6_mm3 0xf3
#define _k3d_mm6_mm4 0xf4
#define _k3d_mm6_mm5 0xf5
#define _k3d_mm6_mm6 0xf6
#define _k3d_mm6_mm7 0xf7
#define _k3d_mm6_eax 0x30
#define _k3d_mm6_ecx 0x31
#define _k3d_mm6_edx 0x32
#define _k3d_mm6_ebx 0x33
#define _k3d_mm6_esi 0x36
#define _k3d_mm6_edi 0x37
#define _k3d_mm7_mm0 0xf8
#define _k3d_mm7_mm1 0xf9
#define _k3d_mm7_mm2 0xfa
#define _k3d_mm7_mm3 0xfb
#define _k3d_mm7_mm4 0xfc
#define _k3d_mm7_mm5 0xfd
#define _k3d_mm7_mm6 0xfe
#define _k3d_mm7_mm7 0xff
#define _k3d_mm7_eax 0x38
#define _k3d_mm7_ecx 0x39
#define _k3d_mm7_edx 0x3a
#define _k3d_mm7_ebx 0x3b
#define _k3d_mm7_esi 0x3e
#define _k3d_mm7_edi 0x3f

#define _k3d_name_xlat_m0 _mm0
#define _k3d_name_xlat_m1 _mm1
#define _k3d_name_xlat_m2 _mm2
#define _k3d_name_xlat_m3 _mm3
#define _k3d_name_xlat_m4 _mm4
#define _k3d_name_xlat_m5 _mm5
#define _k3d_name_xlat_m6 _mm6
#define _k3d_name_xlat_m7 _mm7
#define _k3d_name_xlat_M0 _mm0
#define _k3d_name_xlat_M1 _mm1
#define _k3d_name_xlat_M2 _mm2
#define _k3d_name_xlat_M3 _mm3
#define _k3d_name_xlat_M4 _mm4
#define _k3d_name_xlat_M5 _mm5
#define _k3d_name_xlat_M6 _mm6
#define _k3d_name_xlat_M7 _mm7
#define _k3d_name_xlat_mm0 _mm0
#define _k3d_name_xlat_mm1 _mm1
#define _k3d_name_xlat_mm2 _mm2
#define _k3d_name_xlat_mm3 _mm3
#define _k3d_name_xlat_mm4 _mm4
#define _k3d_name_xlat_mm5 _mm5
#define _k3d_name_xlat_mm6 _mm6
#define _k3d_name_xlat_mm7 _mm7
#define _k3d_name_xlat_MM0 _mm0
#define _k3d_name_xlat_MM1 _mm1
#define _k3d_name_xlat_MM2 _mm2
#define _k3d_name_xlat_MM3 _mm3
#define _k3d_name_xlat_MM4 _mm4
#define _k3d_name_xlat_MM5 _mm5
#define _k3d_name_xlat_MM6 _mm6
#define _k3d_name_xlat_MM7 _mm7
#define _k3d_name_xlat_eax _eax
#define _k3d_name_xlat_ebx _ebx
#define _k3d_name_xlat_ecx _ecx
#define _k3d_name_xlat_edx _edx
#define _k3d_name_xlat_esi _esi
#define _k3d_name_xlat_edi _edi
#define _k3d_name_xlat_ebp _ebp
#define _k3d_name_xlat_EAX _eax
#define _k3d_name_xlat_EBX _ebx
#define _k3d_name_xlat_ECX _ecx
#define _k3d_name_xlat_EDX _edx
#define _k3d_name_xlat_ESI _esi
#define _k3d_name_xlat_EDI _edi
#define _k3d_name_xlat_EBP _ebp
#define _k3d_name_xlat__eax _eax
#define _k3d_name_xlat__ebx _ebx
#define _k3d_name_xlat__ecx _ecx
#define _k3d_name_xlat__edx _edx
#define _k3d_name_xlat__esi _esi
#define _k3d_name_xlat__edi _edi
#define _k3d_name_xlat__ebp _ebp
#define _k3d_name_xlat__EAX _eax
#define _k3d_name_xlat__EBX _ebx
#define _k3d_name_xlat__ECX _ecx
#define _k3d_name_xlat__EDX _edx
#define _k3d_name_xlat__ESI _esi
#define _k3d_name_xlat__EDI _edi
#define _k3d_name_xlat__EBP _ebp

#define _k3d_xglue3(a,b,c) a##b##c
#define _k3d_glue3(a,b,c) _k3d_xglue3(a,b,c)
#define _k3d_MODRM(dst, src) _k3d_glue3(_k3d,_k3d_name_xlat_##dst,_k3d_name_xlat_##src)

/* Operand defines for prefetch and prefetchw */

#define _k3d_pref_eax 0x00
#define _k3d_pref_ecx 0x01
#define _k3d_pref_edx 0x02
#define _k3d_pref_ebx 0x03
#define _k3d_pref_esi 0x06
#define _k3d_pref_edi 0x07
#define _k3d_pref_EAX 0x00
#define _k3d_pref_ECX 0x01
#define _k3d_pref_EDX 0x02
#define _k3d_pref_EBX 0x03
#define _k3d_pref_ESI 0x06
#define _k3d_pref_EDI 0x07
#define _k3d_prefw_eax 0x08
#define _k3d_prefw_ecx 0x09
#define _k3d_prefw_edx 0x0A
#define _k3d_prefw_ebx 0x0B
#define _k3d_prefw_esi 0x0E
#define _k3d_prefw_edi 0x0F
#define _k3d_prefw_EAX 0x08
#define _k3d_prefw_ECX 0x09
#define _k3d_prefw_EDX 0x0A
#define _k3d_prefw_EBX 0x0B
#define _k3d_prefw_ESI 0x0E
#define _k3d_prefw_EDI 0x0F

/* Defines for 3DNow! instructions */
#define PF2ID(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x1d
#define PFACC(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xae
#define PFADD(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x9e
#define PFCMPEQ(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb0
#define PFCMPGE(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x90
#define PFCMPGT(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa0
#define PFMAX(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa4
#define PFMIN(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x94
#define PFMUL(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb4
#define PFRCP(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x96
#define PFRCPIT1(dst, src)      db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa6
#define PFRCPIT2(dst, src)      db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb6
#define PFRSQRT(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x97
#define PFRSQIT1(dst, src)      db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa7
#define PFSUB(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x9a
#define PFSUBR(dst, src)        db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xaa
#define PI2FD(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x0d
#define FEMMS                   db 0x0f, 0x0e
#define PAVGUSB(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xbf
#define PMULHRW(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb7
#define PREFETCH(src)           db 0x0f, 0x0d, _k3d_pref_##src
#define PREFETCHW(src)          db 0x0f, 0x0d, _k3d_prefw_##src
#define CPUID                   db 0x0f, 0xa2

/* Defines for new, K7 opcodes */
#define PFNACC(dst,src)         db 0x0f, 0x0f, _k3d_MODRM(dst,src), 0x8a
#define FPPNACC(dst,src)        db 0x0f, 0x0f, _k3d_MODRM(dst,src), 0x8e
#define PSWAPD(dst,src)         db 0x0f, 0x0f, _k3d_MODRM(dst,src), 0xbb
#define PMINUB(dst,src)         db 0x0f, 0xda, _k3d_MODRM(dst,src)
#define PMAXUB(dst,src)         db 0x0f, 0xde, _k3d_MODRM(dst,src)
#define PMINSW(dst,src)         db 0x0f, 0xea, _k3d_MODRM(dst,src)
#define PMAXSW(dst,src)         db 0x0f, 0xee, _k3d_MODRM(dst,src)
#define PMULHUW(dst,src)        db 0x0f, 0xe4, _k3d_MODRM(dst,src)
#define PAVGB(dst,src)          db 0x0f, 0xe0, _k3d_MODRM(dst,src)
#define PAVGW(dst,src)          db 0x0f, 0xe3, _k3d_MODRM(dst,src)
#define PSADBW(dst,src)         db 0x0f, 0xf6, _k3d_MODRM(dst,src)
#define PMOVMSKB(dst,src)       db 0x0f, 0xd7, _k3d_MODRM(dst,src)
#define PMASKMOVQ(dst,src)      db 0x0f, 0xf7, _k3d_MODRM(dst,src)
#define PINSRW(dst,src,msk)     db 0x0f, 0xc4, _k3d_MODRM(dst,src), msk
#define PEXTRW(dst,src,msk)     db 0x0f, 0xc5, _k3d_MODRM(dst,src), msk
#define PSHUFW(dst,src,msk)     db 0x0f, 0x70, _k3d_MODRM(dst,src), msk
#define MOVNTQ(dst,src)         db 0x0f, 0xe7, _k3d_MODRM(src,dst)
#define SFENCE                  db 0x0f, 0xae, 0xf8

/* Memory/offset versions of the opcodes */
#define PF2IDM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x1d
#define PFACCM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xae
#define PFADDM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x9e
#define PFCMPEQM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb0
#define PFCMPGEM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x90
#define PFCMPGTM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa0
#define PFMAXM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa4
#define PFMINM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x94
#define PFMULM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb4
#define PFRCPM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x96
#define PFRCPIT1M(dst,src,off)  db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa6
#define PFRCPIT2M(dst,src,off)  db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb6
#define PFRSQRTM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x97
#define PFRSQIT1M(dst,src,off)  db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa7
#define PFSUBM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x9a
#define PFSUBRM(dst,src,off)    db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xaa
#define PI2FDM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x0d
#define PAVGUSBM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xbf
#define PMULHRWM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb7


/* Memory/offset versions of the new, K7 opcodes */
#define PFNACCM(dst,src,off)        db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x8a
#define FPPNACCM(dst,src,off)       db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x8e
#define PSWAPDM(dst,src,off)        db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xbb
#define PMINUBM(dst,src,off)        db 0x0f, 0xda, _k3d_MODRM(dst,src) | 0x40, off
#define PMAXUBM(dst,src,off)        db 0x0f, 0xde, _k3d_MODRM(dst,src) | 0x40, off
#define PMINSWM(dst,src,off)        db 0x0f, 0xea, _k3d_MODRM(dst,src) | 0x40, off
#define PMAXSWM(dst,src,off)        db 0x0f, 0xee, _k3d_MODRM(dst,src) | 0x40, off
#define PMULHUWM(dst,src,off)       db 0x0f, 0xe4, _k3d_MODRM(dst,src) | 0x40, off
#define PAVGBM(dst,src,off)         db 0x0f, 0xe0, _k3d_MODRM(dst,src) | 0x40, off
#define PAVGWM(dst,src,off)         db 0x0f, 0xe3, _k3d_MODRM(dst,src) | 0x40, off
#define PSADBWM(dst,src,off)        db 0x0f, 0xf6, _k3d_MODRM(dst,src) | 0x40, off
#define PMOVMSKBM(dst,src,off)      db 0x0f, 0xd7, _k3d_MODRM(dst,src) | 0x40, off
#define PMASKMOVQM(dst,src,off)     db 0x0f, 0xf7, _k3d_MODRM(dst,src) | 0x40, off
#define MOVNTQM(dst,src,off)        db 0x0f, 0xe7, _k3d_MODRM(src,dst) | 0x40, off
#define PINSRWM(dst,src,off,msk)    db 0x0f, 0xc4, _k3d_MODRM(dst,src) | 0x40, off, msk
#define PSHUFWM(dst,src,off,msk)    db 0x0f, 0x70, _k3d_MODRM(dst,src) | 0x40, off, msk


/* Defines for 3DNow! instructions for use in pragmas */
#define p_pf2id(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0x1d
#define p_pfacc(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0xae
#define p_pfadd(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0x9e
#define p_pfcmpeq(dst,src)      0x0f 0x0f _k3d_MODRM(dst,src) 0xb0
#define p_pfcmpge(dst,src)      0x0f 0x0f _k3d_MODRM(dst,src) 0x90
#define p_pfcmpgt(dst,src)      0x0f 0x0f _k3d_MODRM(dst,src) 0xa0
#define p_pfmax(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0xa4
#define p_pfmin(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0x94
#define p_pfmul(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0xb4
#define p_pfrcp(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0x96
#define p_pfrcpit1(dst,src)     0x0f 0x0f _k3d_MODRM(dst,src) 0xa6
#define p_pfrcpit2(dst,src)     0x0f 0x0f _k3d_MODRM(dst,src) 0xb6
#define p_pfrsqrt(dst,src)      0x0f 0x0f _k3d_MODRM(dst,src) 0x97
#define p_pfrsqit1(dst,src)     0x0f 0x0f _k3d_MODRM(dst,src) 0xa7
#define p_pfsub(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0x9a
#define p_pfsubr(dst,src)       0x0f 0x0f _k3d_MODRM(dst,src) 0xaa
#define p_pi2fd(dst,src)        0x0f 0x0f _k3d_MODRM(dst,src) 0x0d
#define p_femms                 0x0f 0x0e
#define p_pavgusb(dst,src)      0x0f 0x0f _k3d_MODRM(dst,src) 0xbf
#define p_pmulhrw(dst,src)      0x0f 0x0f _k3d_MODRM(dst,src) 0xb7
#define p_prefetch(src)         0x0f 0x0d _k3d_pref_##src
#define p_prefetchw(src)        0x0f 0x0d _k3d_prefw_##src
#define P_PFNACC(dst,src)       0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x8a
#define P_FPPNACC(dst,src)      0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x8e
#define P_PSWAPD(dst,src)       0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xbb
#define P_PMINUB(dst,src)       0x0f 0xda (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMAXUB(dst,src)       0x0f 0xde (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMINSW(dst,src)       0x0f 0xea (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMAXSW(dst,src)       0x0f 0xee (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMULHUW(dst,src)      0x0f 0xe4 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PAVGB(dst,src)        0x0f 0xe0 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PAVGW(dst,src)        0x0f 0xe3 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PSADBW(dst,src)       0x0f 0xf6 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMOVMSKB(dst,src)     0x0f 0xd7 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMASKMOVQ(dst,src)    0x0f 0xf7 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PINSRW(dst,src,msk)   0x0f 0xc4 (_k3d_MODRM(dst,src) | 0x40) off msk
#define P_PEXTRW(dst,src,msk)   0x0f 0xc5 (_k3d_MODRM(dst,src) | 0x40) off msk
#define P_PSHUFW(dst,src,msk)   0x0f 0x70 (_k3d_MODRM(dst,src) | 0x40) off msk
#define P_MOVNTQ(dst,src)       0x0f 0xe7 (_k3d_MODRM(src,dst) | 0x40) off

#define P_PF2IDM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x1d
#define P_PFACCM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xae
#define P_PFADDM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x9e
#define P_PFCMPEQM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb0
#define P_PFCMPGEM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x90
#define P_PFCMPGTM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa0
#define P_PFMAXM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa4
#define P_PFMINM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x94
#define P_PFMULM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb4
#define P_PFRCPM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x96
#define P_PFRCPIT1M(dst,src,off) 0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa6
#define P_PFRCPIT2M(dst,src,off) 0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb6
#define P_PFRSQRTM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x97
#define P_PFRSQIT1M(dst,src,off) 0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa7
#define P_PFSUBM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x9a
#define P_PFSUBRM(dst,src,off)   0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xaa
#define P_PI2FDM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x0d
#define P_PAVGUSBM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xbf
#define P_PMULHRWM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb7
#define P_PFNACCM(dst,src,off)   0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x8a
#define P_FPPNACCM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x8e
#define P_PSWAPDM(dst,src,off)   0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xbb
#define P_PMINUBM(dst,src,off)   0x0f 0xda (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMAXUBM(dst,src,off)   0x0f 0xde (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMINSWM(dst,src,off)   0x0f 0xea (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMAXSWM(dst,src,off)   0x0f 0xee (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMULHUWM(dst,src,off)  0x0f 0xe4 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PAVGBM(dst,src,off)    0x0f 0xe0 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PAVGWM(dst,src,off)    0x0f 0xe3 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PSADBWM(dst,src,off)   0x0f 0xf6 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PMOVMSKBM(dst,src,off) 0x0f 0xd7 (_k3d_MODRM(dst,src) | 0x40) off
#define P_MOVNTQM(dst,src,off)   0x0f 0xe7 (_k3d_MODRM(src,dst) | 0x40) off
#define P_PMASKMOVQM(dst,src,off)   0x0f 0xf7 (_k3d_MODRM(dst,src) | 0x40) off
#define P_PINSRWM(dst,src,off,msk)  0x0f 0xc4 (_k3d_MODRM(dst,src) | 0x40) off msk
#define P_PSHUFWM(dst,src,off,msk)  0x0f 0x70 (_k3d_MODRM(dst,src) | 0x40) off msk


#define P_PF2ID(dst,src)            p_pf2id(dst,src)
#define P_PFACC(dst,src)            p_pfacc(dst,src)
#define P_PFADD(dst,src)            p_pfadd(dst,src)
#define P_PFCMPEQ(dst,src)          p_pfcmpeq(dst,src)
#define P_PFCMPGE(dst,src)          p_pfcmpge(dst,src)
#define P_PFCMPGT(dst,src)          p_pfcmpgt(dst,src)
#define P_PFMAX(dst,src)            p_pfmax(dst,src)
#define P_PFMIN(dst,src)            p_pfmin(dst,src)
#define P_PFMUL(dst,src)            p_pfmul(dst,src)
#define P_PFRCP(dst,src)            p_pfrcp(dst,src)
#define P_PFRCPIT1(dst,src)         p_pfrcpit1(dst,src)
#define P_PFRCPIT2(dst,src)         p_pfrcpit2(dst,src)
#define P_PFRSQRT(dst,src)          p_pfrsqrt(dst,src)
#define P_PFRSQIT1(dst,src)         p_pfrsqit1(dst,src)
#define P_PFSUB(dst,src)            p_pfsub(dst,src)
#define P_PFSUBR(dst,src)           p_pfsubr(dst,src)
#define P_PI2FD(dst,src)            p_pi2fd(dst,src)
#define P_FEMMS                     p_femms
#define P_PAVGUSB(dst,src)          p_pavgusb(dst,src)
#define P_PMULHRW(dst,src)          p_pmulhrw(dst,src)
#define P_PREFETCH(src)             p_prefetch(src)
#define P_PREFETCHW(src)            p_prefetchw(src)
#define p_CPUID                     0x0f 0xa2
#define p_pf2idm(dst,src,off)       P_PF2IDM(dst,src,off)
#define p_pfaccm(dst,src,off)       P_PFACCM(dst,src,off)
#define p_pfaddm(dst,src,off)       P_PFADDM(dst,src,off)
#define p_pfcmpeqm(dst,src,off)     P_PFCMPEQM(dst,src,off)
#define p_pfcmpgem(dst,src,off)     P_PFCMPGEM(dst,src,off)
#define p_pfcmpgtm(dst,src,off)     P_PFCMPGTM(dst,src,off)
#define p_pfmaxm(dst,src,off)       P_PFMAXM(dst,src,off)
#define p_pfminm(dst,src,off)       P_PFMINM(dst,src,off)
#define p_pfmulm(dst,src,off)       P_PFMULM(dst,src,off)
#define p_pfrcpm(dst,src,off)       P_PFRCPM(dst,src,off)
#define p_pfrcpit1m(dst,src,off)    P_PFRCPIT1M(dst,src,off)
#define p_pfrcpit2m(dst,src,off)    P_PFRCPIT2M(dst,src,off)
#define p_pfrsqrtm(dst,src,off)     P_PFRSQRTM(dst,src,off)
#define p_pfrsqit1m(dst,src,off)    P_PFRSQIT1M(dst,src,off)
#define p_pfsubm(dst,src,off)       P_PFSUBM(dst,src,off)
#define p_pfsubrm(dst,src,off)      P_PFSUBRM(dst,src,off)
#define p_pi2fdm(dst,src,off)       P_PI2FDM(dst,src,off)
#define p_pavgusbm(dst,src,off)     P_PAVGUSBM(dst,src,off)
#define p_pmulhrwm(dst,src,off)     P_PMULHRWM(dst,src,off)

#define P_PFNACC(dst,src)           p_pfnacc(dst,src)
#define P_FPPNACC(dst,src)          p_pfpnacc(dst,src)
#define P_PSWAPD(dst,src)           p_pswapd(dst,src)
#define P_PMINUB(dst,src)           p_pminub(dst,src)
#define P_PMAXUB(dst,src)           p_pmaxub(dst,src)
#define P_PMINSW(dst,src)           p_pminsw(dst,src)
#define P_PMAXSW(dst,src)           p_pmaxsw(dst,src)
#define P_PMULHUW(dst,src)          p_pmulhuw(dst,src)
#define P_PAVGB(dst,src)            p_pavgb(dst,src)
#define P_PAVGW(dst,src)            p_avgw(dst,src)
#define P_PSADBW(dst,src)           p_psadbw(dst,src)
#define P_PMOVMSKB(dst,src)         p_pmovmskb(dst,src)
#define P_PMASKMOVQ(dst,src)        p_pmaskmovq(dst,src)
#define P_PINSRW(dst,src,msk)       p_pinsrw(dst,src)
#define P_PEXTRW(dst,src,msk)       p_pextrw(dst,src)
#define P_PSHUFW(dst,src,msk)       p_pshufw(dst,src)
#define P_MOVNTQ(dst,src)           p_movntq(dst,src)

#define P_PFNACCM(dst,src,off)          p_pfnaccm(dst,src,off)
#define P_FPPNACCM(dst,src,off)         p_pfpnaccm(dst,src,off)
#define P_PSWAPDM(dst,src,off)          p_pswapdm(dst,src,off)
#define P_PMINUBM(dst,src,off)          p_pminubm(dst,src,off)
#define P_PMAXUBM(dst,src,off)          p_pmaxubm(dst,src,off)
#define P_PMINSWM(dst,src,off)          p_pminswm(dst,src,off)
#define P_PMAXSWM(dst,src,off)          p_pmaxswm(dst,src,off)
#define P_PMULHUWM(dst,src,off)         p_pmulhuwm(dst,src,off)
#define P_PAVGBM(dst,src,off)           p_pavgbm(dst,src,off)
#define P_PAVGWM(dst,src,off)           p_avgwm(dst,src,off)
#define P_PSADBWM(dst,src,off)          p_psadbwm(dst,src,off)
#define P_PMOVMSKBM(dst,src,off)        p_pmovmskbm(dst,src,off)
#define P_PMASKMOVQM(dst,src,off)       p_pmaskmovqm(dst,src,off)
#define P_PINSRWM(dst,src,off,msk)      p_pinsrwm(dst,src,off,msk)
#define P_PSHUFWM(dst,src,off,msk)      p_pshufwm(dst,src,off,msk)
#define P_MOVNTQM(dst,src,off)          p_movntqm(dst,src,off)

#elif defined (_MSC_VER) && !defined (__MWERKS__)
// The Microsoft Visual C++ version of the 3DNow! macros.

// Stop the "no EMMS" warning, since it doesn't detect FEMMS properly
#pragma warning(disable:4799)

// Defines for operands.
#define _K3D_MM0 0xc0
#define _K3D_MM1 0xc1
#define _K3D_MM2 0xc2
#define _K3D_MM3 0xc3
#define _K3D_MM4 0xc4
#define _K3D_MM5 0xc5
#define _K3D_MM6 0xc6
#define _K3D_MM7 0xc7
#define _K3D_mm0 0xc0
#define _K3D_mm1 0xc1
#define _K3D_mm2 0xc2
#define _K3D_mm3 0xc3
#define _K3D_mm4 0xc4
#define _K3D_mm5 0xc5
#define _K3D_mm6 0xc6
#define _K3D_mm7 0xc7
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07

// These defines are for compatibility with the previous version of the header file.
#define _K3D_M0   0xc0
#define _K3D_M1   0xc1
#define _K3D_M2   0xc2
#define _K3D_M3   0xc3
#define _K3D_M4   0xc4
#define _K3D_M5   0xc5
#define _K3D_M6   0xc6
#define _K3D_M7   0xc7
#define _K3D_m0   0xc0
#define _K3D_m1   0xc1
#define _K3D_m2   0xc2
#define _K3D_m3   0xc3
#define _K3D_m4   0xc4
#define _K3D_m5   0xc5
#define _K3D_m6   0xc6
#define _K3D_m7   0xc7
#define _K3D__EAX 0x00
#define _K3D__ECX 0x01
#define _K3D__EDX 0x02
#define _K3D__EBX 0x03
#define _K3D__ESI 0x06
#define _K3D__EDI 0x07
#define _K3D__eax 0x00
#define _K3D__ecx 0x01
#define _K3D__edx 0x02
#define _K3D__ebx 0x03
#define _K3D__esi 0x06
#define _K3D__edi 0x07

// General 3DNow! instruction format that is supported by 
// these macros. Note that only the most basic form of memory 
// operands are supported by these macros. 

#define InjK3DOps(dst,src,inst)                         \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit 0x0f                                      \
   _asm _emit ((_K3D_##dst & 0x3f) << 3) | _K3D_##src   \
   _asm _emit _3DNowOpcode##inst                        \
}

#define InjK3DMOps(dst,src,off,inst)                    \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit 0x0f                                      \
   _asm _emit (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40) \
   _asm _emit off                                       \
   _asm _emit _3DNowOpcode##inst                        \
}

#define InjMMXOps(dst,src,inst)                         \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit _3DNowOpcode##inst                        \
   _asm _emit ((_K3D_##dst & 0x3f) << 3) | _K3D_##src   \
}

#define InjMMXMOps(dst,src,off,inst)                    \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit _3DNowOpcode##inst                        \
   _asm _emit (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40) \
   _asm _emit off                                       \
}

#define _3DNowOpcodePF2ID    0x1d
#define _3DNowOpcodePFACC    0xae
#define _3DNowOpcodePFADD    0x9e
#define _3DNowOpcodePFCMPEQ  0xb0
#define _3DNowOpcodePFCMPGE  0x90
#define _3DNowOpcodePFCMPGT  0xa0
#define _3DNowOpcodePFMAX    0xa4
#define _3DNowOpcodePFMIN    0x94
#define _3DNowOpcodePFMUL    0xb4
#define _3DNowOpcodePFRCP    0x96
#define _3DNowOpcodePFRCPIT1 0xa6
#define _3DNowOpcodePFRCPIT2 0xb6
#define _3DNowOpcodePFRSQRT  0x97
#define _3DNowOpcodePFRSQIT1 0xa7
#define _3DNowOpcodePFSUB    0x9a
#define _3DNowOpcodePFSUBR   0xaa
#define _3DNowOpcodePI2FD    0x0d
#define _3DNowOpcodePAVGUSB  0xbf
#define _3DNowOpcodePMULHRW  0xb7
#define _3DNowOpcodePFNACC   0x8a
#define _3DNowOpcodeFPPNACC  0x8e
#define _3DNowOpcodePSWAPD   0xbb
#define _3DNowOpcodePMINUB   0xda
#define _3DNowOpcodePMAXUB   0xde
#define _3DNowOpcodePMINSW   0xea
#define _3DNowOpcodePMAXSW   0xee
#define _3DNowOpcodePMULHUW  0xe4
#define _3DNowOpcodePAVGB    0xe0
#define _3DNowOpcodePAVGW    0xe3
#define _3DNowOpcodePSADBW   0xf6
#define _3DNowOpcodePMOVMSKB 0xd7
#define _3DNowOpcodePMASKMOVQ   0xf7
#define _3DNowOpcodePINSRW   0xc4
#define _3DNowOpcodePEXTRW   0xc5
#define _3DNowOpcodePSHUFW   0x70
#define _3DNowOpcodeMOVNTQ   0xe7
#define _3DNowOpcodePREFETCHT 0x18


#define PF2ID(dst,src)      InjK3DOps(dst, src, PF2ID)
#define PFACC(dst,src)      InjK3DOps(dst, src, PFACC)
#define PFADD(dst,src)      InjK3DOps(dst, src, PFADD)
#define PFCMPEQ(dst,src)    InjK3DOps(dst, src, PFCMPEQ)
#define PFCMPGE(dst,src)    InjK3DOps(dst, src, PFCMPGE)
#define PFCMPGT(dst,src)    InjK3DOps(dst, src, PFCMPGT)
#define PFMAX(dst,src)      InjK3DOps(dst, src, PFMAX)
#define PFMIN(dst,src)      InjK3DOps(dst, src, PFMIN)
#define PFMUL(dst,src)      InjK3DOps(dst, src, PFMUL)
#define PFRCP(dst,src)      InjK3DOps(dst, src, PFRCP)
#define PFRCPIT1(dst,src)   InjK3DOps(dst, src, PFRCPIT1)
#define PFRCPIT2(dst,src)   InjK3DOps(dst, src, PFRCPIT2)
#define PFRSQRT(dst,src)    InjK3DOps(dst, src, PFRSQRT)
#define PFRSQIT1(dst,src)   InjK3DOps(dst, src, PFRSQIT1)
#define PFSUB(dst,src)      InjK3DOps(dst, src, PFSUB)
#define PFSUBR(dst,src)     InjK3DOps(dst, src, PFSUBR)
#define PI2FD(dst,src)      InjK3DOps(dst, src, PI2FD)
#define PAVGUSB(dst,src)    InjK3DOps(dst, src, PAVGUSB)
#define PMULHRW(dst,src)    InjK3DOps(dst, src, PMULHRW)

#define FEMMS                                   \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0e                              \
}

#define PREFETCH(src)                           \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit (_K3D_##src & 0x07)               \
}

/* Prefetch with a short offset, < 127 or > -127
   Carefull!  Doesn't check for your offset being
   in range. */

#define PREFETCHM(src,off)					    \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d								\
   _asm _emit (0x40 | (_K3D_##src & 0x07))		\
   _asm _emit off								\
}

/* Prefetch with a long offset */

#define PREFETCHMLONG(src,off)					\
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d								\
   _asm _emit (0x80 | (_K3D_##src & 0x07))		\
   _asm _emit (off & 0x000000ff)				\
   _asm _emit (off & 0x0000ff00) >>	8			\
   _asm _emit (off & 0x00ff0000) >>	16			\
   _asm _emit (off & 0xff000000) >>	24			\
}

#define PREFETCHW(src)                          \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit (0x08 | (_K3D_##src & 0x07))      \
}

#define PREFETCHWM(src,off)                     \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit 0x48 | (_K3D_##src & 0x07)        \
   _asm	_emit off								\
}

#define PREFETCHWMLONG(src,off)                 \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit 0x88 | (_K3D_##src & 0x07)        \
   _asm _emit (off & 0x000000ff)				\
   _asm _emit (off & 0x0000ff00) >>	8			\
   _asm _emit (off & 0x00ff0000) >>	16			\
   _asm _emit (off & 0xff000000) >>	24			\
}

#define CPUID                                   \
{                                               \
    _asm _emit 0x0f                             \
    _asm _emit 0xa2                             \
}


/* Defines for new, K7 opcodes */
#define SFENCE                                  \
{                                               \
    _asm _emit 0x0f                             \
    _asm _emit 0xae                             \
    _asm _emit 0xf8                             \
}

#define PFNACC(dst,src)         InjK3DOps(dst,src,PFNACC)
#define PFPNACC(dst,src)        InjK3DOps(dst,src,PFPNACC)
#define PSWAPD(dst,src)         InjK3DOps(dst,src,PSWAPD)
#define PMINUB(dst,src)         InjMMXOps(dst,src,PMINUB)
#define PMAXUB(dst,src)         InjMMXOps(dst,src,PMAXUB)
#define PMINSW(dst,src)         InjMMXOps(dst,src,PMINSW)
#define PMAXSW(dst,src)         InjMMXOps(dst,src,PMAXSW)
#define PMULHUW(dst,src)        InjMMXOps(dst,src,PMULHUW)
#define PAVGB(dst,src)          InjMMXOps(dst,src,PAVGB)
#define PAVGW(dst,src)          InjMMXOps(dst,src,PAVGW)
#define PSADBW(dst,src)         InjMMXOps(dst,src,PSADBW)
#define PMOVMSKB(dst,src)       InjMMXOps(dst,src,PMOVMSKB)
#define PMASKMOVQ(dst,src)      InjMMXOps(dst,src,PMASKMOVQ)
#define PINSRW(dst,src,msk)     InjMMXOps(dst,src,PINSRW) _asm _emit msk
#define PEXTRW(dst,src,msk)     InjMMXOps(dst,src,PEXTRW) _asm _emit msk
#define PSHUFW(dst,src,msk)     InjMMXOps(dst,src,PSHUFW) _asm _emit msk
#define MOVNTQ(dst,src)         InjMMXOps(src,dst,MOVNTQ)
#define PREFETCHNTA(mem)        InjMMXOps(mm0,mem,PREFETCHT)
#define PREFETCHT0(mem)         InjMMXOps(mm1,mem,PREFETCHT)
#define PREFETCHT1(mem)         InjMMXOps(mm2,mem,PREFETCHT)
#define PREFETCHT2(mem)         InjMMXOps(mm3,mem,PREFETCHT)


/* Memory/offset versions of the opcodes */
#define PAVGUSBM(dst,src,off)   InjK3DMOps(dst,src,off,PAVGUSB)
#define PF2IDM(dst,src,off)     InjK3DMOps(dst,src,off,PF2ID)
#define PFACCM(dst,src,off)     InjK3DMOps(dst,src,off,PFACC)
#define PFADDM(dst,src,off)     InjK3DMOps(dst,src,off,PFADD)
#define PFCMPEQM(dst,src,off)   InjK3DMOps(dst,src,off,PFCMPEQ)
#define PFCMPGEM(dst,src,off)   InjK3DMOps(dst,src,off,PFCMPGE)
#define PFCMPGTM(dst,src,off)   InjK3DMOps(dst,src,off,PFCMPGT)
#define PFMAXM(dst,src,off)     InjK3DMOps(dst,src,off,PFMAX)
#define PFMINM(dst,src,off)     InjK3DMOps(dst,src,off,PFMIN)
#define PFMULM(dst,src,off)     InjK3DMOps(dst,src,off,PFMUL)
#define PFRCPM(dst,src,off)     InjK3DMOps(dst,src,off,PFRCP)
#define PFRCPIT1M(dst,src,off)  InjK3DMOps(dst,src,off,PFRCPIT1)
#define PFRCPIT2M(dst,src,off)  InjK3DMOps(dst,src,off,PFRCPIT2)
#define PFRSQRTM(dst,src,off)   InjK3DMOps(dst,src,off,PFRSQRT)
#define PFRSQIT1M(dst,src,off)  InjK3DMOps(dst,src,off,PFRSQIT1)
#define PFSUBM(dst,src,off)     InjK3DMOps(dst,src,off,PFSUB)
#define PFSUBRM(dst,src,off)    InjK3DMOps(dst,src,off,PFSUBR)
#define PI2FDM(dst,src,off)     InjK3DMOps(dst,src,off,PI2FD)
#define PMULHRWM(dst,src,off)   InjK3DMOps(dst,src,off,PMULHRW)


/* Memory/offset versions of the K7 opcodes */
#define PFNACCM(dst,src,off)     InjK3DMOps(dst,src,off,PFNACC)
#define PFPNACCM(dst,src,off)    InjK3DMOps(dst,src,off,PFPNACC)
#define PSWAPDM(dst,src,off)     InjK3DMOps(dst,src,off,PSWAPD)
#define PMINUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMINUB)
#define PMAXUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXUB)
#define PMINSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMINSW)
#define PMAXSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXSW)
#define PMULHUWM(dst,src,off)    InjMMXMOps(dst,src,off,PMULHUW)
#define PAVGBM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGB)
#define PAVGWM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGW)
#define PSADBWM(dst,src,off)     InjMMXMOps(dst,src,off,PSADBW)
#define PMOVMSKBM(dst,src,off)   InjMMXMOps(dst,src,off,PMOVMSKB)
#define PMASKMOVQM(dst,src,off)  InjMMXMOps(dst,src,off,PMASKMOVQ)
#define PINSRWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PINSRW) _asm _emit msk
#define PSHUFWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PSHUFW) _asm _emit msk
#define MOVNTQM(dst,src,off)     InjMMXMOps(src,dst,off,MOVNTQ)
#define PREFETCHNTAM(mem,off)    InjMMXMOps(mm0,mem,off,PREFETCHT)
#define PREFETCHT0M(mem,off)     InjMMXMOps(mm1,mem,off,PREFETCHT)
#define PREFETCHT1M(mem,off)     InjMMXMOps(mm2,mem,off,PREFETCHT)
#define PREFETCHT2M(mem,off)     InjMMXMOps(mm3,mem,off,PREFETCHT)


#else

/* Assume built-in support for 3DNow! opcodes, replace macros with opcodes */
#define PAVGUSB(dst,src)    pavgusb     dst,src
#define PF2ID(dst,src)      pf2id       dst,src
#define PFACC(dst,src)      pfacc       dst,src
#define PFADD(dst,src)      pfadd       dst,src
#define PFCMPEQ(dst,src)    pfcmpeq     dst,src
#define PFCMPGE(dst,src)    pfcmpge     dst,src
#define PFCMPGT(dst,src)    pfcmpgt     dst,src
#define PFMAX(dst,src)      pfmax       dst,src
#define PFMIN(dst,src)      pfmin       dst,src
#define PFMUL(dst,src)      pfmul       dst,src
#define PFRCP(dst,src)      pfrcp       dst,src
#define PFRCPIT1(dst,src)   pfrcpit1    dst,src
#define PFRCPIT2(dst,src)   pfrcpit2    dst,src
#define PFRSQRT(dst,src)    pfrsqrt     dst,src
#define PFRSQIT1(dst,src)   pfrsqit1    dst,src
#define PFSUB(dst,src)      pfsub       dst,src
#define PFSUBR(dst,src)     pfsubr      dst,src
#define PI2FD(dst,src)      pi2fd       dst,src
#define PMULHRW(dst,src)    pmulhrw     dst,src
#define PREFETCH(src)       prefetch    src
#define PREFETCHW(src)      prefetchw   src

#define PAVGUSBM(dst,src,off)   pavgusb     dst,[src+off]
#define PF2IDM(dst,src,off)     PF2ID       dst,[src+off]
#define PFACCM(dst,src,off)     PFACC       dst,[src+off]
#define PFADDM(dst,src,off)     PFADD       dst,[src+off]
#define PFCMPEQM(dst,src,off)   PFCMPEQ     dst,[src+off]
#define PFCMPGEM(dst,src,off)   PFCMPGE     dst,[src+off]
#define PFCMPGTM(dst,src,off)   PFCMPGT     dst,[src+off]
#define PFMAXM(dst,src,off)     PFMAX       dst,[src+off]
#define PFMINM(dst,src,off)     PFMIN       dst,[src+off]
#define PFMULM(dst,src,off)     PFMUL       dst,[src+off]
#define PFRCPM(dst,src,off)     PFRCP       dst,[src+off]
#define PFRCPIT1M(dst,src,off)  PFRCPIT1    dst,[src+off]
#define PFRCPIT2M(dst,src,off)  PFRCPIT2    dst,[src+off]
#define PFRSQRTM(dst,src,off)   PFRSQRT     dst,[src+off]
#define PFRSQIT1M(dst,src,off)  PFRSQIT1    dst,[src+off]
#define PFSUBM(dst,src,off)     PFSUB       dst,[src+off]
#define PFSUBRM(dst,src,off)    PFSUBR      dst,[src+off]
#define PI2FDM(dst,src,off)     PI2FD       dst,[src+off]
#define PMULHRWM(dst,src,off)   PMULHRW     dst,[src+off]


#if defined (__MWERKS__)
// At the moment, CodeWarrior does not support these opcodes, so hand-assemble them

// Defines for operands.
#define _K3D_MM0 0xc0
#define _K3D_MM1 0xc1
#define _K3D_MM2 0xc2
#define _K3D_MM3 0xc3
#define _K3D_MM4 0xc4
#define _K3D_MM5 0xc5
#define _K3D_MM6 0xc6
#define _K3D_MM7 0xc7
#define _K3D_mm0 0xc0
#define _K3D_mm1 0xc1
#define _K3D_mm2 0xc2
#define _K3D_mm3 0xc3
#define _K3D_mm4 0xc4
#define _K3D_mm5 0xc5
#define _K3D_mm6 0xc6
#define _K3D_mm7 0xc7
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07

#define InjK3DOps(dst,src,inst) \
    db 0x0f, 0x0f, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src), _3DNowOpcode##inst

#define InjK3DMOps(dst,src,off,inst) \
    db 0x0f, 0x0f, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40), off, _3DNowOpcode##inst

#define InjMMXOps(dst,src,inst)                     \
    db 0x0f, _3DNowOpcode##inst, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src)

#define InjMMXMOps(dst,src,off,inst)                \
    db 0x0f, _3DNowOpcode##inst, (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40), off

#define PFNACC(dst,src)         InjK3DOps(dst,src,PFNACC)
#define PFPNACC(dst,src)        InjK3DOps(dst,src,PFPNACC)
#define PSWAPD(dst,src)         InjK3DOps(dst,src,PSWAPD)
#define PMINUB(dst,src)         InjMMXOps(dst,src,PMINUB)
#define PMAXUB(dst,src)         InjMMXOps(dst,src,PMAXUB)
#define PMINSW(dst,src)         InjMMXOps(dst,src,PMINSW)
#define PMAXSW(dst,src)         InjMMXOps(dst,src,PMAXSW)
#define PMULHUW(dst,src)        InjMMXOps(dst,src,PMULHUW)
#define PAVGB(dst,src)          InjMMXOps(dst,src,PAVGB)
#define PAVGW(dst,src)          InjMMXOps(dst,src,PAVGW)
#define PSADBW(dst,src)         InjMMXOps(dst,src,PSADBW)
#define PMOVMSKB(dst,src)       InjMMXOps(dst,src,PMOVMSKB)
#define PMASKMOVQ(dst,src)      InjMMXOps(dst,src,PMASKMOVQ)
#define PINSRW(dst,src,msk)     InjMMXOps(dst,src,PINSRW) db msk
#define PEXTRW(dst,src,msk)     InjMMXOps(dst,src,PEXTRW) db msk
#define PSHUFW(dst,src,msk)     InjMMXOps(dst,src,PSHUFW) db msk
#define MOVNTQ(dst,src)         InjMMXOps(src,dst,MOVNTQ)
#define PREFETCHNTA(mem)        InjMMXOps(mm0,mem,PREFETCHT)
#define PREFETCHT0(mem)         InjMMXOps(mm1,mem,PREFETCHT)
#define PREFETCHT1(mem)         InjMMXOps(mm2,mem,PREFETCHT)
#define PREFETCHT2(mem)         InjMMXOps(mm3,mem,PREFETCHT)


/* Memory/offset versions of the K7 opcodes */
#define PFNACCM(dst,src,off)     InjK3DMOps(dst,src,off,PFNACC)
#define PFPNACCM(dst,src,off)    InjK3DMOps(dst,src,off,PFPNACC)
#define PSWAPDM(dst,src,off)     InjK3DMOps(dst,src,off,PSWAPD)
#define PMINUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMINUB)
#define PMAXUBM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXUB)
#define PMINSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMINSW)
#define PMAXSWM(dst,src,off)     InjMMXMOps(dst,src,off,PMAXSW)
#define PMULHUWM(dst,src,off)    InjMMXMOps(dst,src,off,PMULHUW)
#define PAVGBM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGB)
#define PAVGWM(dst,src,off)      InjMMXMOps(dst,src,off,PAVGW)
#define PSADBWM(dst,src,off)     InjMMXMOps(dst,src,off,PSADBW)
#define PMOVMSKBM(dst,src,off)   InjMMXMOps(dst,src,off,PMOVMSKB)
#define PMASKMOVQM(dst,src,off)  InjMMXMOps(dst,src,off,PMASKMOVQ)
#define PINSRWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PINSRW), msk
#define PEXTRWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PEXTRW), msk
#define PSHUFWM(dst,src,off,msk) InjMMXMOps(dst,src,off,PSHUFW), msk
#define MOVNTQM(dst,src,off)     InjMMXMOps(src,dst,off,MOVNTQ)
#define PREFETCHNTAM(mem,off)    InjMMXMOps(mm0,mem,off,PREFETCHT)
#define PREFETCHT0M(mem,off)     InjMMXMOps(mm1,mem,off,PREFETCHT)
#define PREFETCHT1M(mem,off)     InjMMXMOps(mm2,mem,off,PREFETCHT)
#define PREFETCHT2M(mem,off)     InjMMXMOps(mm3,mem,off,PREFETCHT)


#else

#define PFNACC(dst,src)         PFNACC      dst,src
#define PFPNACC(dst,src)        PFPNACC     dst,src
#define PSWAPD(dst,src)         PSWAPD      dst,src
#define PMINUB(dst,src)         PMINUB      dst,src
#define PMAXUB(dst,src)         PMAXUB      dst,src
#define PMINSW(dst,src)         PMINSW      dst,src
#define PMAXSW(dst,src)         PMAXSW      dst,src
#define PMULHUW(dst,src)        PMULHUW     dst,src
#define PAVGB(dst,src)          PAVGB       dst,src
#define PAVGW(dst,src)          PAVGW       dst,src
#define PSADBW(dst,src)         PSADBW      dst,src
#define PMOVMSKB(dst,src)       PMOVMSKB    dst,src
#define PMASKMOVQ(dst,src)      PMASKMOVQ   dst,src
#define PINSRW(dst,src,msk)     PINSRW      dst,src,msk
#define PEXTRW(dst,src,msk)     PEXTRW      dst,src,msk
#define PSHUFW(dst,src,msk)     PSHUFW      dst,src,msk
#define MOVNTQ(dst,src)         MOVNTQ      dst,src

#define PFNACCM(dst,src,off)    PFNACC      dst,[src+off]
#define PFPNACCM(dst,src,off)   PFPNACC     dst,[src+off]
#define PSWAPDM(dst,src,off)    PSWAPD      dst,[src+off]
#define PMINUBM(dst,src,off)    PMINUB      dst,[src+off]
#define PMAXUBM(dst,src,off)    PMAXUB      dst,[src+off]
#define PMINSWM(dst,src,off)    PMINSW      dst,[src+off]
#define PMAXSWM(dst,src,off)    PMAXSW      dst,[src+off]
#define PMULHUWM(dst,src,off)   PMULHUW     dst,[src+off]
#define PAVGBM(dst,src,off)     PAVGB       dst,[src+off]
#define PAVGWM(dst,src,off)     PAVGW       dst,[src+off]
#define PSADBWM(dst,src,off)    PSADBW      dst,[src+off]
#define PMOVMSKBM(dst,src,off)  PMOVMSKB    dst,[src+off]
#define PMASKMOVQM(dst,src,off) PMASKMOVQ   dst,[src+off]
#define PINSRWM(dst,src,off,msk) PINSRW     dst,[src+off],msk
#define PEXTRWM(dst,src,off,msk) PEXTRW     dst,[src+off],msk
#define PSHUFWM(dst,src,off,msk) PSHUFW     dst,[src+off],msk
#define MOVNTQM(dst,src,off)    MOVNTQ      dst,[src+off]

#endif

#endif

/* Just to deal with lower case. */
#define pf2id(dst,src)          PF2ID(dst,src)
#define pfacc(dst,src)          PFACC(dst,src)
#define pfadd(dst,src)          PFADD(dst,src)
#define pfcmpeq(dst,src)        PFCMPEQ(dst,src)
#define pfcmpge(dst,src)        PFCMPGE(dst,src)
#define pfcmpgt(dst,src)        PFCMPGT(dst,src)
#define pfmax(dst,src)          PFMAX(dst,src)
#define pfmin(dst,src)          PFMIN(dst,src)
#define pfmul(dst,src)          PFMUL(dst,src)
#define pfrcp(dst,src)          PFRCP(dst,src)
#define pfrcpit1(dst,src)       PFRCPIT1(dst,src)
#define pfrcpit2(dst,src)       PFRCPIT2(dst,src)
#define pfrsqrt(dst,src)        PFRSQRT(dst,src)
#define pfrsqit1(dst,src)       PFRSQIT1(dst,src)
#define pfsub(dst,src)          PFSUB(dst,src)
#define pfsubr(dst,src)         PFSUBR(dst,src)
#define pi2fd(dst,src)          PI2FD(dst,src)
#define femms                   FEMMS
#define pavgusb(dst,src)        PAVGUSB(dst,src)
#define pmulhrw(dst,src)        PMULHRW(dst,src)
#define prefetch(src)           PREFETCH(src)
#define prefetchw(src)          PREFETCHW(src)

#define prefetchm(src,off)      PREFETCHM(src,off)
#define prefetchmlong(src,off)	PREFETCHMLONG(src,off)
#define prefetchwm(src,off)     PREFETCHWM(src,off)
#define prefetchwmlong(src,off)	 PREFETCHWMLONG(src,off)

#define pfnacc(dst,src)         PFNACC(dst,src)
#define pfpnacc(dst,src)        PFPNACC(dst,src)
#define pswapd(dst,src)         PSWAPD(dst,src)
#define pminub(dst,src)         PMINUB(dst,src)
#define pmaxub(dst,src)         PMAXUB(dst,src)
#define pminsw(dst,src)         PMINSW(dst,src)
#define pmaxsw(dst,src)         PMAXSW(dst,src)
#define pmulhuw(dst,src)        PMULHUW(dst,src)
#define pavgb(dst,src)          PAVGB(dst,src)
#define pavgw(dst,src)          PAVGW(dst,src)
#define psadbw(dst,src)         PSADBW(dst,src)
#define pmovmskb(dst,src)       PMOVMSKB(dst,src)
#define pmaskmovq(dst,src)      PMASKMOVQ(dst,src)
#define pinsrw(dst,src,msk)     PINSRW(dst,src,msk)
#define pextrw(dst,src,msk)     PEXTRW(dst,src,msk)
#define pshufw(dst,src,msk)     PSHUFW(dst,src,msk)
#define movntq(dst,src)         MOVNTQ(dst,src)
#define prefetchnta(mem)        PREFETCHNTA(mem)
#define prefetcht0(mem)         PREFETCHT0(mem)  
#define prefetcht1(mem)         PREFETCHT1(mem)  
#define prefetcht2(mem)         PREFETCHT2(mem)  


#define pavgusbm(dst,src,off)   PAVGUSBM(dst,src,off)
#define pf2idm(dst,src,off)     PF2IDM(dst,src,off)
#define pfaccm(dst,src,off)     PFACCM(dst,src,off)
#define pfaddm(dst,src,off)     PFADDM(dst,src,off)
#define pfcmpeqm(dst,src,off)   PFCMPEQM(dst,src,off)
#define pfcmpgem(dst,src,off)   PFCMPGEM(dst,src,off)
#define pfcmpgtm(dst,src,off)   PFCMPGTM(dst,src,off)
#define pfmaxm(dst,src,off)     PFMAXM(dst,src,off)
#define pfminm(dst,src,off)     PFMINM(dst,src,off)
#define pfmulm(dst,src,off)     PFMULM(dst,src,off)
#define pfrcpm(dst,src,off)     PFRCPM(dst,src,off)
#define pfrcpit1m(dst,src,off)  PFRCPIT1M(dst,src,off)
#define pfrcpit2m(dst,src,off)  PFRCPIT2M(dst,src,off)
#define pfrsqrtm(dst,src,off)   PFRSQRTM(dst,src,off)
#define pfrsqit1m(dst,src,off)  PFRSQIT1M(dst,src,off)
#define pfsubm(dst,src,off)     PFSUBM(dst,src,off)
#define pfsubrm(dst,src,off)    PFSUBRM(dst,src,off)
#define pi2fdm(dst,src,off)     PI2FDM(dst,src,off)
#define pmulhrwm(dst,src,off)   PMULHRWM(dst,src,off)
#define cpuid                   CPUID
#define sfence                  SFENCE

#define pfnaccm(dst,src,off)    PFNACCM(dst,src,off)
#define pfpnaccm(dst,src,off)   PFPNACCM(dst,src,off)
#define pswapdm(dst,src,off)    PSWAPDM(dst,src,off)
#define pminubm(dst,src,off)    PMINUBM(dst,src,off)
#define pmaxubm(dst,src,off)    PMAXUBM(dst,src,off)
#define pminswm(dst,src,off)    PMINSWM(dst,src,off)
#define pmaxswm(dst,src,off)    PMAXSWM(dst,src,off)
#define pmulhuwm(dst,src,off)   PMULHUWM(dst,src,off)
#define pavgbm(dst,src,off)     PAVGBM(dst,src,off)
#define pavgwm(dst,src,off)     PAVGWM(dst,src,off)
#define psadbwm(dst,src,off)    PSADBWM(dst,src,off)
#define pmovmskbm(dst,src,off)  PMOVMSKBM(dst,src,off)
#define pmaskmovqm(dst,src,off) PMASKMOVQM(dst,src,off)
#define pinsrwm(dst,src,off,msk)    PINSRWM(dst,src,off,msk)
#define pextrwm(dst,src,off,msk)    PEXTRWM(dst,src,off,msk)
#define pshufwm(dst,src,off,msk)    PSHUFWM(dst,src,off,msk)
#define movntqm(dst,src,off)    MOVNTQM(dst,src,off)
#define prefetchntam(mem,off)   PREFETCHNTA(mem,off)
#define prefetcht0m(mem,off)    PREFETCHT0(mem,off)  
#define prefetcht1m(mem,off)    PREFETCHT1(mem,off)  
#define prefetcht2m(mem,off)    PREFETCHT2(mem,off)  

#endif
