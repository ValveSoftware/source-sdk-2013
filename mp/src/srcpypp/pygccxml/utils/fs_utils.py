# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines few classes, that simplifies a file system entries iteration"""

import os
from types import *

##If you want include files that doesn't have extension then use filter like '*.'

def _make_list( argument ):
    if type(argument) in StringTypes:
        if argument:
            return [argument]
        else:
            return []
    elif type(argument) is ListType:
        return argument
    else:
        raise TypeError( 'Argument "%s" must be or list of strings or string.' % argument )

class base_files_iterator:
    def __init__(self, file_exts, is_include_exts = True):
        self.__file_exts = _make_list( file_exts )
        self.__is_include_exts = is_include_exts

    def _is_to_skip(self, file_path):
        if not self.__file_exts:
            return 0
        file_ext = os.path.splitext( file_path )[1]
        if not file_ext:
            file_ext = '.' + file_ext
        file_ext = '*' + file_ext
        if file_ext.lower() in self.__file_exts:
            return not self.__is_include_exts
        else:
            return self.__is_include_exts

    def _subdirectories_and_files(self, directory_path):
        files, directories = [], []
        directory_contents = os.listdir(directory_path)
        for object_name in directory_contents:
            object_path = os.path.join(directory_path, object_name)
            if os.path.isfile( object_path ) and not self._is_to_skip( object_path ):
                files.append( object_path )
            elif os.path.isdir( object_path ):
                directories.append( object_path )
            else:
                pass
        return directories, files

    def __iter__(self):
        raise NotImplementedError

    def __next__(self):
        raise NotImplementedError

    def restart(self):
        raise NotImplementedError

class files_walker(base_files_iterator):
    def __init__(self, directories, file_ext_filter = '', is_include_filter = True, is_recursive = True):
        base_files_iterator.__init__(self, file_ext_filter, is_include_filter)
        self.__directories = _make_list( directories )
        self.__is_recursive = is_recursive
        self.__file_generator = None

    def __walk(self):
        directories = self.__directories[:]
        while directories:
            sub_directories, files = self._subdirectories_and_files( directories.pop(0) )
            if self.__is_recursive:
                for directory in sub_directories:
                    directories.append( directory )
            for file_os in files:
                yield file_os

    def __iter__(self):
        self.__file_generator = self.__walk()
        return self

    def __next__(self):
        return next(self.__file_generator)

    def restart(self):
        self.__file_generator = None


class directories_walker:
    def __init__(self, directories, is_recursive = 1):
        self.__directories = []
        for root in _make_list( directories ):
            self.__directories.extend( self.__sub_directories( root ) )
        self.__is_recursive = is_recursive
        self.__directory_generator = None

    def __sub_directories(self, directory_path):
        sub_directories = []
        directory_contains = os.listdir(directory_path)
        for object_in_directory in directory_contains:
            full_path = os.path.join(directory_path, object_in_directory)
            if os.path.isdir( full_path ):
                sub_directories.append( full_path )
        return sub_directories

    def __walk(self):
        directories = self.__directories[:]
        for curr_directory in directories:
            yield curr_directory
            if self.__is_recursive:
                for f in directories_walker( [curr_directory], True ):
                    yield f

    def __iter__(self):
        self.__directory_generator = self.__walk()
        return self

    def __next__(self):
        return next(self.__directory_generator)

    def restart(self):
        self.__directory_generator = None


if '__main__' == __name__:
    pass
    #lFileCount = 0
    #for file_os in files_iterator( r'C:\Program Files\Microsoft Visual Studio\VC98\Include\stlport', ['*.h', '*.'], True, False):
        #print file_os
        #lFileCount += 1
    #print lFileCount

    #~ for directory in directories_iterator( '/home/roman/language-binding', False ):
        #~ print directory
    #~ for directory in directories_iterator( '/home/roman/language-binding', True ):
        #~ print directory
