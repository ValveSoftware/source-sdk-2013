# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""defines interface for repository of generated files hash"""

import os
try:
    from hashlib import md5
except:
    from md5 import new as md5


def get_md5_text_value( text ):
    m = md5()
    data = text
    if isinstance(text, str):
        data = text.encode()
    m.update( data )
    return m.hexdigest() 

def get_md5_file_value( fpath ):
    if not os.path.exists( fpath ):
        return None #file does not exist
    f = open( fpath, 'rb' )
    fcontent = f.read()
    f.close()
    return get_md5_text_value( fcontent )

class repository_t( object ):
    def __init__( self ):
        object.__init__( self )
        
    def get_file_value( self, fpath ):
        return NotImplementedError( self.__class__.__name__ )

    def get_text_value( self, fpath ):
        return NotImplementedError( self.__class__.__name__ )

    def update_value( self, fpath, hash_value ):
        return NotImplementedError( self.__class__.__name__ )
        
    def save_values( self ):
        return NotImplementedError( self.__class__.__name__ )

class dummy_repository_t( repository_t ):
    def __init__( self ):
        repository_t.__init__( self )
        
    def get_file_value( self, fpath ):
        return get_md5_file_value( fpath )
        
    def get_text_value( self, text ):
        return get_md5_text_value( text )
        
    def update_value( self, fpath, hash_value ):
        pass
        
    def save_values( self ):
        pass

class cached_repository_t( repository_t ):
    separator = ' '
    hexdigest_len = 32
    hexdigest_separator_len = 33
    
    def __init__( self, file_name ):
        repository_t.__init__( self )
        self.__repository = {}
        self.__repository_file = file_name
        if os.path.exists( self.__repository_file ):
            f = open( self.__repository_file, 'r' )
            for line in f:
                if len(line) < self.hexdigest_separator_len:
                    continue
                hexdigest = line[:self.hexdigest_len]
                fname = line[self.hexdigest_separator_len:].rstrip()
                self.__repository[ fname ] = hexdigest
            f.close()

    def get_file_value( self, fpath ):
        try:
            return self.__repository[ fpath ]
        except KeyError:
            return None
    
    def get_text_value( self, text ):
        return get_md5_text_value( text )

    def update_value( self, fpath, hash_value ):
        self.__repository[ fpath ] = hash_value
        
    def save_values( self ):
        lines = []
        for fpath, hexdigest in self.__repository.items():
            lines.append( '%s%s%s%s' % ( hexdigest, self.separator, fpath, os.linesep ) )        
        f = open( self.__repository_file, 'w+' )
        f.writelines( lines )
        f.close()
