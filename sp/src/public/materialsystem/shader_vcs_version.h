//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SHADER_VCS_VERSION_H
#define SHADER_VCS_VERSION_H
#ifdef _WIN32
#pragma once
#endif

// 1 = hl2 shipped
// 2 = compressed with diffs version (lostcoast)
// 3 = compressed with bzip
// 4 = v2 + crc32
// 5 = v3 + crc32
// 6 = v5 + duplicate static combo records
#define SHADER_VCS_VERSION_NUMBER		6

#define MAX_SHADER_UNPACKED_BLOCK_SIZE	(1<<17)
#define MAX_SHADER_PACKED_SIZE			(1+MAX_SHADER_UNPACKED_BLOCK_SIZE)

#pragma pack(1)
struct ShaderHeader_t
{
	int32	m_nVersion;
	int32	m_nTotalCombos;
	int32	m_nDynamicCombos;
	uint32	m_nFlags;
	uint32	m_nCentroidMask;
	uint32	m_nNumStaticCombos;			// includes sentinal key
	uint32	m_nSourceCRC32;				// NOTE: If you move this, update copyshaders.pl, *_prep.pl, updateshaders.pl
};
#pragma pack()

#pragma pack(1)
struct ShaderHeader_t_v4				// still used for assembly shaders
{
	int32	m_nVersion;
	int32	m_nTotalCombos;
	int32	m_nDynamicCombos;
	uint32	m_nFlags;
	uint32	m_nCentroidMask;
	uint32	m_nDiffReferenceSize;
	uint32	m_nSourceCRC32;				// NOTE: If you move this, update copyshaders.pl, *_prep.pl, updateshaders.pl
};
#pragma pack()

// for old format files
struct ShaderDictionaryEntry_t
{
	int m_Offset;
	int m_Size;
};

// record for one static combo
struct StaticComboRecord_t
{
	uint32 m_nStaticComboID;
	uint32 m_nFileOffset;
};


struct StaticComboAliasRecord_t								// for duplicate static combos
{
	uint32 m_nStaticComboID;								// this combo
	uint32 m_nSourceStaticCombo;							// the combo it is the same as
};




#endif // SHADER_VCS_VERSION_H
	
