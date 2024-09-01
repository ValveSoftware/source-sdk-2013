/****************************************************************************
 *                                                                          *
 *      VERSION.H        -- Version information for internal builds         *
 *                                                                          *
 *      This file is only modified by the official builder to update the    *
 *      VERSION, VER_PRODUCTVERSION and VER_PRODUCTVERSION_STR values       *
 *                                                                          *
 *      version.h is created on the fly from verhead.bat and vertail.h,     *
 *      with the current version numbers inserted in between                *
 *                                                                          *
 ****************************************************************************/

#ifndef VER_H
/* ver.h defines constants needed by the VS_VERSION_INFO structure */
//#include <winver.h>
#endif
#include <windows.h>
#define VER_FILETYPE                VFT_APP
#define VER_FILESUBTYPE             VFT2_UNKNOWN
#define VER_FILEDESCRIPTION_STR     "TTS SAMPLE ENGINE 5"
#define VER_INTERNALNAME_STR        "TTSSAMPLEENG5"

/*--------------------------------------------------------------*/
/* the following entry should be phased out in favor of         */
/* VER_PRODUCTVERSION_STR, but is used in the shell today.      */
/*--------------------------------------------------------------*/


/*--------------------------------------------------------------*/
/* the following values should be modified by the official      */
/* builder for each build                                       */
/*--------------------------------------------------------------*/

#define VERSION                     "5.0"
#define VER_FILEVERSION_STR         "5.0"
#define VER_FILEVERSION             5,0
#define VER_PRODUCTVERSION_STR      "5.0"
#define VER_PRODUCTVERSION          5,0

#define OFFICIAL                    1
#define FINAL                       1

/*--------------------------------------------------------------*/
/* the following section defines values used in the version     */
/* data structure for all files, and which do not change.       */
/*--------------------------------------------------------------*/

/* default is nodebug */
#if _DEBUG
#define VER_DEBUG                   VS_FF_DEBUG
#else
#define VER_DEBUG                   0
#endif

/* default is privatebuild */
#ifndef OFFICIAL
#define VER_PRIVATEBUILD            VS_FF_PRIVATEBUILD
#else
#define VER_PRIVATEBUILD            0
#endif

/* default is prerelease */
#ifndef FINAL
#define VER_PRERELEASE              VS_FF_PRERELEASE
#else
#define VER_PRERELEASE              0
#endif

#define VER_FILEFLAGSMASK           VS_FFI_FILEFLAGSMASK
#define VER_FILEOS                  VOS_DOS_WINDOWS32
#define VER_FILEFLAGS               (VER_PRIVATEBUILD|VER_PRERELEASE|VER_DEBUG)

#define VER_COMPANYNAME_STR         "Microsoft Corporation\0"
#define VER_PRODUCTNAME_STR         "Microsoft\256 Windows(TM) Operating System\0"
#define VER_LEGALTRADEMARKS_STR     \
"Microsoft\256 is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation.\0"

//#include "\sapi5\build\common.ver"
