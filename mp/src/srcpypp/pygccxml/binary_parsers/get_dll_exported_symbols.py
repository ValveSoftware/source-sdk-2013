#The content of this file was contributed by leppton
# (http://mail.python.org/pipermail/patches/2006-November/020942.html) to ctypes
# project, under MIT License.

# This example shows how to use ctypes module to read all
# function names from dll export directory

import os
if os.name != "nt":
    raise Exception("Wrong OS")

import ctypes as ctypes
import ctypes.wintypes as wintypes

def convert_cdef_to_pydef(line):
    """\
convert_cdef_to_pydef(line_from_c_header_file) -> python_tuple_string
'DWORD  var_name[LENGTH];' -> '("var_name", DWORD*LENGTH)'

doesn't work for all valid c/c++ declarations"""
    l = line[:line.find(';')].split()
    if len(l) != 2:
        return None
    type_ = l[0]
    name = l[1]
    i = name.find('[')
    if i != -1:
        name, brac = name[:i], name[i:][1:-1]
        return '("%s", %s*%s)'%(name,type_,brac)
    else:
        return '("%s", %s)'%(name,type_)

def convert_cdef_to_structure(cdef, name, data_dict=ctypes.__dict__):
    """\
convert_cdef_to_structure(struct_definition_from_c_header_file)
  -> python class derived from ctypes.Structure

limited support for c/c++ syntax"""
    py_str = '[\n'
    for line in cdef.split('\n'):
        field = convert_cdef_to_pydef(line)
        if field != None:
            py_str += ' '*4 + field + ',\n'
    py_str += ']\n'

    pyarr = eval(py_str, data_dict)
    class ret_val(ctypes.Structure):
        _fields_ = pyarr
    ret_val.__name__ = name
    ret_val.__module__ = None
    return ret_val

#struct definitions we need to read dll file export table
winnt = (
    ('IMAGE_DOS_HEADER', """\
    WORD   e_magic;
    WORD   e_cblp;
    WORD   e_cp;
    WORD   e_crlc;
    WORD   e_cparhdr;
    WORD   e_minalloc;
    WORD   e_maxalloc;
    WORD   e_ss;
    WORD   e_sp;
    WORD   e_csum;
    WORD   e_ip;
    WORD   e_cs;
    WORD   e_lfarlc;
    WORD   e_ovno;
    WORD   e_res[4];
    WORD   e_oemid;
    WORD   e_oeminfo;
    WORD   e_res2[10];
    LONG   e_lfanew;
"""),

    ('IMAGE_FILE_HEADER', """\
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
"""),

    ('IMAGE_DATA_DIRECTORY', """\
    DWORD   VirtualAddress;
    DWORD   Size;
"""),

    ('IMAGE_OPTIONAL_HEADER32', """\
    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;
    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
""",
     {'IMAGE_NUMBEROF_DIRECTORY_ENTRIES':16}),

    ('IMAGE_NT_HEADERS', """\
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
"""),

    ('IMAGE_EXPORT_DIRECTORY', """\
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Name;
    DWORD   Base;
    DWORD   NumberOfFunctions;
    DWORD   NumberOfNames;
    DWORD   AddressOfFunctions;
    DWORD   AddressOfNames;
    DWORD   AddressOfNameOrdinals;
"""),
    )

#Construct python ctypes.Structures from above definitions
data_dict = dict(wintypes.__dict__)
for definition in winnt:
    name = definition[0]
    def_str = definition[1]
    if len(definition) == 3:
        data_dict.update(definition[2])
    type_ = convert_cdef_to_structure(def_str, name, data_dict)
    data_dict[name] = type_
    globals()[name] = type_

    ptype = ctypes.POINTER(type_)
    pname = 'P'+name
    data_dict[pname] = ptype
    globals()[pname] = ptype

del data_dict
del winnt

class DllException(Exception):
    pass

def read_export_table(dll_name, mmap=False, use_kernel=False):
    """\
read_export_table(dll_name [,mmap=False [,use_kernel=False]]])
     -> list of exported names

default is to load dll into memory: dll sections are aligned to
page boundaries, dll entry points is called, etc...

with mmap=True dll file image is mapped to memory, Relative Virtual
Addresses (RVAs) must be mapped to real addresses manually

with use_kernel=True direct kernel32.dll calls are used,
instead of python mmap module

see http://www.windowsitlibrary.com/Content/356/11/1.html
for details on Portable Executable (PE) file format
"""
    if not mmap:
        dll = ctypes.cdll.LoadLibrary(dll_name)
        if dll == None:
            raise DllException("Cant load dll")
        base_addr = dll._handle

    else:
        if not use_kernel:
            fileH = open(dll_name)
            if fileH == None:
                raise DllException("Cant load dll")
            import mmap
            m = mmap.mmap(fileH.fileno(), 0, None, mmap.ACCESS_READ)
            # id(m)+8 sucks, is there better way?
            base_addr = ctypes.cast(id(m)+8, ctypes.POINTER(ctypes.c_int))[0]
        else:
            kernel32 = ctypes.windll.kernel32
            if kernel32 == None:
                raise DllException("cant load kernel")
            fileH = kernel32.CreateFileA(dll_name, 0x00120089, 1,0,3,0,0)
            if fileH == 0:
                raise DllException("Cant open, errcode = %d"%kernel32.GetLastError())
            mapH = kernel32.CreateFileMappingW(fileH,0,0x8000002,0,0,0)
            if mapH == 0:
                raise DllException("Cant mmap, errocode = %d"%kernel32.GetLastError())
            base_addr = ctypes.windll.kernel32.MapViewOfFile(mapH, 0x4, 0, 0, 0)
            if base_addr == 0:
                raise DllException("Cant mmap(2), errocode = %d"%kernel32.GetLastError())

        dbghelp = ctypes.windll.dbghelp
        if dbghelp == None:
            raise DllException("dbghelp.dll not installed")
        pimage_nt_header = dbghelp.ImageNtHeader(base_addr)
        if pimage_nt_header == 0:
            raise DllException("Cant find IMAGE_NT_HEADER")

        #Functions like dbghelp.ImageNtHeader above have no type information
        #let's make one prototype for extra buzz
        #PVOID ImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders, PVOID Base,
        #                   ULONG Rva, PIMAGE_SECTION_HEADER* LastRvaSection)
        # we use integers instead of pointers, coz integers are better
        # for pointer arithmetic
        prototype = ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.c_int,
                 ctypes.c_int, ctypes.c_int, ctypes.c_int)
        paramflags = ((1,"NtHeaders",pimage_nt_header),(1,"Base",base_addr),(1,"Rva"),(1,"LastRvaSection",0))
        ImageRvaToVa = prototype(('ImageRvaToVa', dbghelp), paramflags)

    def cast_rva(rva, type_):
        va = base_addr + rva
        if mmap and va > pimage_nt_header:
             va = ImageRvaToVa(Rva=rva)
             if va == 0:
                 raise DllException("ImageRvaToVa failed")
        return ctypes.cast(va, type_)

    if not mmap:
        dos_header = cast_rva(0, PIMAGE_DOS_HEADER)[0]
        if dos_header.e_magic != 0x5A4D:
            raise DllException("IMAGE_DOS_HEADER.e_magic error")
        nt_header = cast_rva(dos_header.e_lfanew, PIMAGE_NT_HEADERS)[0]
    else:
        nt_header = ctypes.cast(pimage_nt_header, PIMAGE_NT_HEADERS)[0]
    if nt_header.Signature != 0x00004550:
        raise DllException("IMAGE_NT_HEADERS.Signature error")

    opt_header = nt_header.OptionalHeader
    if opt_header.Magic != 0x010b:
        raise DllException("IMAGE_OPTIONAL_HEADERS32.Magic error")

    ret_val = []
    exports_dd = opt_header.DataDirectory[0]
    if opt_header.NumberOfRvaAndSizes > 0 or exports_dd != 0:
        export_dir = cast_rva(exports_dd.VirtualAddress, PIMAGE_EXPORT_DIRECTORY)[0]

        nNames = export_dir.NumberOfNames
        if nNames > 0:
            PNamesType = ctypes.POINTER(ctypes.c_int * nNames)
            names = cast_rva(export_dir.AddressOfNames, PNamesType)[0]
            for rva in names:
                name = cast_rva(rva, ctypes.c_char_p).value
                ret_val.append(name)

    if mmap:
        if use_kernel:
            kernel32.UnmapViewOfFile(base_addr)
            kernel32.CloseHandle(mapH)
            kernel32.CloseHandle(fileH)
        else:
            m.close()
            fileH.close()
    return ret_val


if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print('usage: %s dll_file_name'%sys.argv[0])
        sys.exit()
##    names = read_export_table(sys.argv[1], mmap=False, use_kernel=False)
    names = read_export_table(sys.argv[1], mmap=False, use_kernel=False)
    for name in names:
        print(name)


