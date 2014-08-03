//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef IA32DETECT_H
#define IA32DETECT_H

#ifdef PLATFORM_WINDOWS_PC
#include <intrin.h>
#endif

/*
    This section from http://iss.cs.cornell.edu/ia32.htm


 */
typedef	unsigned			bit;

enum CPUVendor
{
    INTEL,
    AMD,
    UNKNOWN_VENDOR
};
class ia32detect
{
public:

	enum type_t
	{
		type_OEM,
		type_OverDrive,
		type_Dual,
		type_reserved
	};

	enum brand_t
	{
		brand_na,
		brand_Celeron,
		brand_PentiumIII,
		brand_PentiumIIIXeon,
		brand_reserved1,
		brand_reserved2,
		brand_PentiumIIIMobile,
		brand_reserved3,
		brand_Pentium4,
		brand_invalid
	};

#	pragma pack(push, 1)

	struct version_t
	{
		bit	Stepping	: 4;
		bit	Model		: 4;
		bit	Family		: 4;
		bit	Type		: 2;
		bit	Reserved1	: 2;
		bit	XModel		: 4;
		bit	XFamily		: 8;
		bit	Reserved2	: 4;
	};

	struct misc_t
	{
		byte	Brand;
		byte	CLFLUSH;
		byte	Reserved;
		byte	APICId;
	};

	struct feature_t
	{
		bit	FPU			: 1; // Floating Point Unit On-Chip
		bit	VME			: 1; // Virtual 8086 Mode Enhancements
		bit	DE			: 1; // Debugging Extensions
		bit	PSE			: 1; // Page Size Extensions
		bit	TSC			: 1; // Time Stamp Counter
		bit	MSR			: 1; // Model Specific Registers
		bit	PAE			: 1; // Physical Address Extension
		bit	MCE			: 1; // Machine Check Exception
		bit	CX8			: 1; // CMPXCHG8 Instruction
		bit	APIC		: 1; // APIC On-Chip
		bit	Reserved1	: 1; 
		bit	SEP			: 1; // SYSENTER and SYSEXIT instructions
		bit	MTRR		: 1; // Memory Type Range Registers
		bit	PGE			: 1; // PTE Global Bit
		bit	MCA			: 1; // Machine Check Architecture
		bit	CMOV		: 1; // Conditional Move Instructions
		bit	PAT			: 1; // Page Attribute Table
		bit	PSE36		: 1; // 32-bit Page Size Extension
		bit	PSN			: 1; // Processor Serial Number
		bit	CLFSH		: 1; // CLFLUSH Instruction
		bit	Reserved2	: 1;
		bit	DS			: 1; // Debug Store
		bit	ACPI		: 1; // Thermal Monitor and Software Controlled Clock Facilities
		bit	MMX			: 1; // Intel MMX Technology
		bit	FXSR		: 1; // FXSAVE and FXRSTOR Instructions
		bit	SSE			: 1; // Intel SSE Technology
		bit	SSE2		: 1; // Intel SSE2 Technology
		bit	SS			: 1; // Self Snoop
		bit	HTT	        : 1; // Hyper Threading
		bit	TM			: 1; // Thermal Monitor
		bit	Reserved3	: 1;
		bit	PBE	        : 1; // Pending Brk. EN.
	};

#	pragma pack(pop)

	tstring vendor_name;
    CPUVendor vendor;
	tstring brand;
	version_t version;
	misc_t misc;
	feature_t feature;
	byte *cache;

	ia32detect ()
	{

        cache = 0;
		uint32 m = init0();

		uint32 *d = new uint32[m * 4];

		for (uint32 i = 1; i <= m; i++)
		{
#ifdef COMPILER_MSVC64
			__cpuid((int *) (d + (i-1) * 4), i);

#else
			uint32 *t = d + (i - 1) * 4;

			__asm
			{
				mov	eax, i;
				mov esi, t;

				cpuid;

				mov dword ptr [esi + 0x0], eax;
				mov dword ptr [esi + 0x4], ebx;
				mov dword ptr [esi + 0x8], ecx;
				mov dword ptr [esi + 0xC], edx;
			}
#endif
		}

		if (m >= 1)
			init1(d);

		if (m >= 2)
			init2(d[4] & 0xFF);

		delete [] d;

		init0x80000000();


        //-----------------------------------------------------------------------
        // Get the vendor of the processor
        //-----------------------------------------------------------------------
        if (_tcscmp(vendor_name.c_str(), _T("GenuineIntel")) == 0)
        {
            vendor = INTEL;

        }
        else if (_tcscmp(vendor_name.c_str(), _T("AuthenticAMD")) == 0)
        {
            vendor = AMD;

        }
        else 
        {
            vendor = UNKNOWN_VENDOR;
        }
	}

	const tstring version_text () const
	{
		tchar b[128];

		_stprintf(b, _T("%d.%d.%d %s XVersion(%d.%d)"), 
			version.Family, version.Model, version.Stepping, type_text(), version.XFamily, version.XModel);

		return tstring(b);
	}

protected:

	const tchar * type_text () const
	{
		static const tchar *text[] =
		{
			_T("Intel OEM Processor"),
			_T("Intel OverDrive(R) Processor"),
			_T("Intel Dual Processor"),
			_T("reserved")
		};

		return text[version.Type];
	}

	const tstring brand_text () const
	{
		static const tchar *text[] =
		{
			_T("n/a"),
			_T("Celeron"),
			_T("Pentium III"),
			_T("Pentium III Xeon"),
			_T("reserved (4)"),
			_T("reserved (5)"),
			_T("Pentium III Mobile"),
			_T("reserved (7)"),
			_T("Pentium 4")
		};

		if (misc.Brand < brand_invalid)
			return tstring(text[misc.Brand]);
		else
		{
			tchar b[32];

			_stprintf(b, _T("Brand %d (Update)"), misc.Brand);

			return tstring(b);
		}
	}

private:

	uint32 init0 ()
	{
		uint32 m;

		int data[4 + 1];
		tchar * s1;

		s1 = (tchar *) &data[1];
		__cpuid(data, 0);
		data[4] = 0;
		// Returns something like this:
		//  data[0] = 0x0000000b
		//  data[1] = 0x756e6547 	Genu
		//  data[2] = 0x6c65746e 	ntel
		//  data[3] = 0x49656e69 	ineI
		//  data[4] = 0x00000000

		m = data[0];
		int t = data[2];
		data[2] = data[3];
		data[3] = t;
		vendor_name = s1;
		return m;
	}

	void init1 (uint32 *d)
	{
		version = *(version_t *)&d[0];
		misc = *(misc_t *)&d[1];
		feature = *(feature_t *)&d[3];
	}

	void process2 (uint32 d, bool c[])
	{
		if ((d & 0x80000000) == 0)
			for (int i = 0; i < 32; i += 8)
				c[(d >> i) & 0xFF] = true;
	}

	void init2 (byte count)
	{
		uint32 d[4];
		bool c[256];

		for (int ci1 = 0; ci1 < 256; ci1++)
			c[ci1] = false;

		for (int i = 0; i < count; i++)
		{
#ifdef COMPILER_MSVC64
			__cpuid((int *) d, 2);
#else
			__asm
			{
				mov	eax, 2;
				lea esi, d;
				cpuid;
				mov [esi + 0x0], eax;
				mov [esi + 0x4], ebx;
				mov [esi + 0x8], ecx;
				mov [esi + 0xC], edx;
			}
#endif

			if (i == 0)
				d[0] &= 0xFFFFFF00;

			process2(d[0], c);
			process2(d[1], c);
			process2(d[2], c);
			process2(d[3], c);
		}

		int m = 0;

		for (int ci2 = 0; ci2 < 256; ci2++)
			if (c[ci2])
				m++;

		cache = new byte[m];

		m = 0;

		for (int ci3 = 1; ci3 < 256; ci3++)
			if (c[ci3])
				cache[m++] = ci3;

		cache[m] = 0;
	}

	void init0x80000000 ()
	{
		uint32 m;

#ifdef COMPILER_MSVC64
		int data[4];
		__cpuid(data, 0x80000000);
		m = data[0];
#else
		__asm
		{
			mov	eax, 0x80000000;
			cpuid;
			mov m, eax
		}
#endif

		if ((m & 0x80000000) != 0)
		{
			uint32 *d = new uint32[(m - 0x80000000) * 4];

			for (uint32 i = 0x80000001; i <= m; i++)
			{
				uint32 *t = d + (i - 0x80000001) * 4;

#ifdef COMPILER_MSVC64
				__cpuid((int *) (d + (i - 0x80000001) * 4), i);
#else
				__asm
				{
					mov	eax, i;
					mov	esi, t;
					cpuid;
					mov dword ptr [esi + 0x0], eax;
					mov dword ptr [esi + 0x4], ebx;
					mov dword ptr [esi + 0x8], ecx;
					mov dword ptr [esi + 0xC], edx;
				}
#endif
			}

			if (m >= 0x80000002)
				brand = (tchar *)(d + 4);

			// note the assignment to brand above does a copy, we need to delete
			delete[] d;
		}
	}
};

#endif // IA32DETECT_H
