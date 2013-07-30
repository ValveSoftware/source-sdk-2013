# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
This package contains few classes, which write :class:`code_creators.module_t` to files.
"""

import types
from .writer import writer_t
from .single_file import single_file_t
from .multiple_files import multiple_files_t
from .balanced_files import balanced_files_t
from .class_multiple_files import class_multiple_files_t
from .md5sum_repository import repository_t
from .md5sum_repository import cached_repository_t

def has_pypp_extenstion( fname ):
    """returns True if file has `Py++` specific extension, otherwise False"""
    for ext in ( multiple_files_t.HEADER_EXT, multiple_files_t.SOURCE_EXT ):
        if fname.endswith( ext ):
            return True
    return False

def write_file( data, file_path, encoding='ascii' ):
    """writes data to file"""
    if isinstance( data, str ):
        writer_t.write_file( data, file_path, encoding=encoding )
    else:
        sf = single_file_t( data, file_path, encoding=encoding )
        sf.write()

def write_multiple_files( extmodule, dir_path, files_sum_repository=None, encoding='ascii' ):
    """writes extmodule to multiple files"""
    mfs = multiple_files_t( extmodule, dir_path, files_sum_repository=files_sum_repository, encoding=encoding )
    mfs.write()
    return mfs.written_files

def write_balanced_files( extmodule, dir_path, number_of_buckets, files_sum_repository=None, encoding='ascii' ):
    """writes extmodule to fixed number of multiple .cpp files"""
    mfs = balanced_files_t( extmodule, dir_path, number_of_buckets, files_sum_repository=files_sum_repository, encoding=encoding )
    mfs.write()
    return mfs.written_files

def write_class_multiple_files( extmodule, dir_path, huge_classes, files_sum_repository, encoding='ascii' ):
    """writes extmodule to multiple files and splits huge classes to few source files"""
    mfs = class_multiple_files_t( extmodule, dir_path, huge_classes, files_sum_repository=files_sum_repository, encoding=encoding )
    mfs.write()
    return mfs.written_files
