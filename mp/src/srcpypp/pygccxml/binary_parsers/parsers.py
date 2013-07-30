# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)


"""
defines few simple classes( parsers ), which deals with .dll, .map, .so files.

Those classes extract decorated\mangled names from the files. Later the extracted
symbols are used for:

   * building "dynamic library" public interface
   * extracting function calling convention

"""

import os
import re
import sys
import ctypes
from . import undname
import warnings
#import exceptions
import subprocess
from .. import utils
from .. import declarations

# class LicenseWarning( exceptions.UserWarning ):
#     def __init__( self, *args, **keywd ):
#         exceptions.UserWarning.__init__( self, *args, **keywd )


dll_file_parser_warning = \
"""

----------------------------->> LICENSE  WARNING <<-----------------------------
"dll_file_parser_t" functionality uses code licensed under MIT license.
pygccxml project uses Boost Software License, Version 1.0.
For more information about this functionality take a look on
get_dll_exported_symbols.py file.
================================================================================


"""

class libparser_t( object ):
    """base class for .dll, .map, .so parser classes"""
    def __init__( self, global_ns, binary_file ):
        """global_ns - reference to global namespace
        binary_file - s
        """
        self.__global_ns = global_ns
        self.__binary_file = binary_file
        self.__loaded_symbols = None

    @property
    def global_ns( self ):
        """reference to global namespace"""
        return self.__global_ns

    @property
    def binary_file( self ):
        """binary file path"""
        return self.__binary_file

    @property
    def loaded_symbols( self ):
        """list of symbols, which were loaded from the binary file.
        The actual type of return value defined by the derived class"""
        return self.__loaded_symbols

    def load_symbols( self ):
        """loads public( shared ) symbols from the binary file.

        This method should be overridden in the derived classes.
        """
        raise NotImplementedError()

    def merge( self, symbol):
        """extracts and merges information from the symbol to the declarations tree.

        This method should be overridden in the derived classes.
        """
        raise NotImplementedError()

    def parse( self ):
        """the main method of the class

        loads information from the binary file and merges it into the declarations
        tree.

        The return value of the function is dictionary, where the key is
        decorated/mangled declaration name and the value is a declaration.
        """
        self.__loaded_symbols = self.load_symbols()
        result = {}
        for smbl in self.__loaded_symbols:
            try:
                decorated, decl = self.merge( smbl )
                if decl:
                    decl.decorated_name = decorated
                    result[ decorated ] = decl
            except declarations.matcher.declaration_not_found_t:
                pass
        return result

CCTS = declarations.CALLING_CONVENTION_TYPES


CCTS = declarations.CALLING_CONVENTION_TYPES

class formated_mapping_parser_t( libparser_t ):
    """convenience class, which formats existing declarations"""
    def __init__( self, global_ns, binary_file, hint ):
        libparser_t.__init__( self, global_ns, binary_file )
        self.__formated_decls = {}
        self.undname_creator = undname.undname_creator_t()

        formatter = lambda decl: self.undname_creator.format_decl( decl, hint )

        for f in self.global_ns.calldefs( allow_empty=True, recursive=True ):
            self.__formated_decls[ formatter( f ) ] = f

        for v in self.global_ns.variables( allow_empty=True, recursive=True ):
            self.__formated_decls[ formatter( v ) ] = v

    @property
    def formated_decls( self ):
        return self.__formated_decls

class map_file_parser_t( formated_mapping_parser_t ):
    """parser for MSVC .map file"""
    c_entry = re.compile( r' +\d+    (?P<internall>.+?)(?:\s+exported name\:\s(?P<name>.*)$)')
    cpp_entry = re.compile( r' +\d+    (?P<decorated>.+?) \((?P<undecorated>.+)\)$' )

    def __init__( self, global_ns, map_file_path ):
        formated_mapping_parser_t.__init__( self, global_ns, map_file_path, 'msvc' )

    def load_symbols( self ):
        """returns dictionary { decorated symbol : original declaration name }"""
        f = open( self.binary_file )
        lines = []
        was_exports = False
        for line in f:
            if was_exports:
                lines.append( line )
            elif 'Exports' == line.strip():
                was_exports = True
            else:
                pass

        index = 0
        result = []
        while index < len( lines ):
            line = lines[index].rstrip()
            found = self.cpp_entry.match( line.decode('ascii') )
            if found:
                result.append( ( found.group( 'decorated' ), found.group( 'undecorated' ) ) )
            elif index + 1 < len( lines ):
                two_lines = line + lines[index+1].rstrip()
                found = self.c_entry.match( two_lines )
                if found:
                    result.append( ( found.group( 'name' ), found.group( 'name' ) ) )
                    index += 1
            else:
                pass
            index += 1
        return result

    def merge( self, smbl ):
        decorated, undecorated = smbl
        if decorated == undecorated:
            #we deal with C function ( or may be we deal with variable?, I have to check the latest
            f = self.global_ns.free_fun( decorated )
            #TODO create usecase, where C function uses different calling convention
            f.calling_convention = CCTS.CDECL
            return decorated, f
        else:
            undecorated_normalized = self.undname_creator.normalize_undecorated( undecorated )
            if undecorated_normalized not in self.formated_decls:
                return None, None
            decl = self.formated_decls[ undecorated_normalized ]
            if isinstance( decl, declarations.calldef_t ):
                decl.calling_convention = CCTS.extract( undecorated, CCTS.CDECL )
            return decorated, decl


class dll_file_parser_t( formated_mapping_parser_t ):
    """parser for Windows .dll file"""
    def __init__( self, global_ns, map_file_path ):
        global dll_file_parser_warning
        warnings.warn( dll_file_parser_warning, LicenseWarning )
        formated_mapping_parser_t.__init__( self, global_ns, map_file_path, 'msvc' )

    def load_symbols( self ):
        from . import get_dll_exported_symbols
        return get_dll_exported_symbols.read_export_table( self.binary_file )

    def merge( self, smbl ):
        blob = smbl
        blob_undecorated = self.undname_creator.undecorate_blob( blob, undname.UNDECORATE_NAME_OPTIONS.UNDNAME_COMPLETE )
        blob_undecorated_normalized = self.undname_creator.undecorate_blob( blob )
        if blob == blob_undecorated == blob_undecorated_normalized:
            #we deal with C function ( or may be we deal with variable?, I have to check the latest
            f = self.global_ns.free_fun( blob )
            #TODO create usecase, where C function uses different calling convention
            f.calling_convention = CCTS.CDECL
            return blob, f
        else:
            if blob_undecorated_normalized not in self.formated_decls:
                return None, None
            decl = self.formated_decls[ blob_undecorated_normalized ]
            if isinstance( decl, declarations.calldef_t ):
                decl.calling_convention = CCTS.extract( blob_undecorated, CCTS.CDECL )
            return blob, decl


class so_file_parser_t( formated_mapping_parser_t ):
    """parser for Linux .so file"""
    nm_executable = 'nm'
    #numeric-sort used for mapping between mangled and unmangled name
    cmd_mangled = '%(nm)s --extern-only --dynamic --defined-only --numeric-sort %(lib)s'.split()
    cmd_demangled = '%(nm)s --extern-only --dynamic --defined-only --demangle --numeric-sort %(lib)s'.split()

    entry = re.compile( r'^(?P<address>(?:\w|\d)+)\s\w\s(?P<symbol>.+)$' )

    def __init__( self, global_ns, binary_file ):
        formated_mapping_parser_t.__init__( self, global_ns, binary_file, 'nm' )

    def __execute_nm( self, cmd ):
        process = subprocess.Popen( args=cmd
                                    , stdin=subprocess.PIPE
                                    , stdout=subprocess.PIPE
                                    , stderr=subprocess.STDOUT )
        process.stdin.close()

        output = []
        while process.poll() is None:
            output.append( process.stdout.readline() )
        #the process already finished, read th rest of the output
        output.extend( process.stdout.readlines() )
        if process.returncode:
            msg = ["Unable to extract public\\exported symbols from '%s' file." % self.binary_file ]
            msg.append( 'The command line, which was used to extract symbols, is "%s"' % cmd )
            raise RuntimeError( os.linesep.join(msg) )
        return output

    def __extract_symbols( self, cmd ):
        output = self.__execute_nm( cmd )
        result = {}
        for line in output:
            found = self.entry.match( line )
            if found:
                result[ found.group( 'address' ) ] = found.group( 'symbol' )
        return result

    def load_symbols( self ):
        tmpl_args = dict( nm=self.nm_executable, lib=self.binary_file )
        mangled_smbls = self.__extract_symbols( [part % tmpl_args for part in self.cmd_mangled] )
        demangled_smbls = self.__extract_symbols(  [part % tmpl_args for part in self.cmd_demangled] )

        result = []
        for address, blob in mangled_smbls.items():
            if address in demangled_smbls:
                result.append( ( blob, demangled_smbls[address] ) )
        return result

    def merge( self, smbl ):
        decorated, undecorated = smbl
        if decorated == undecorated:
            #we deal with C function ( or may be we deal with variable?, I have to check the latest
            try:
                f = self.global_ns.free_fun( decorated )
                #TODO create usecase, where C function uses different calling convention
                f.calling_convention = CCTS.CDECL
                return decorated, f
            except self.global_ns.declaration_not_found_t:
                v = self.global_ns.vars( decorated, allow_empty=True, recursive=False )
                if v:
                    return decorated, v[0]
                else:
                    return None, None
        else:
            undecorated_normalized = self.undname_creator.normalize_undecorated( undecorated )
            if undecorated_normalized not in self.formated_decls:
                return None, None
            decl = self.formated_decls[ undecorated_normalized ]
            if isinstance( decl, declarations.calldef_t ):
                decl.calling_convention = CCTS.extract( undecorated, CCTS.CDECL )
            return decorated, decl

def merge_information( global_ns, fname, runs_under_unittest=False ):
    """high level function - select the appropriate binary file parser and integrates
    the information from the file to the declarations tree. """
    ext = os.path.splitext( fname )[1]
    parser = None
    if '.dll' == ext:
        parser = dll_file_parser_t( global_ns, fname )
    elif '.map' == ext:
        parser = map_file_parser_t( global_ns, fname )
    elif '.so' == ext or '.so.' in os.path.basename(fname):
        parser = so_file_parser_t( global_ns, fname )
    else:
        raise RuntimeError( "Don't know how to read exported symbols from file '%s'"
                            % fname )
    symbols = parser.parse()
    if runs_under_unittest:
        return symbols, parser
    else:
        return symbols



