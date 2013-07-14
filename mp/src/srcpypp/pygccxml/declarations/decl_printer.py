# Copyright 2004-2008 Roman Yakovenko, 2006 Allen Bierbaum, Matthias Baas
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""
defines class, :class:`decl_printer_t` that prints declarations tree in a user friendly format
"""

import os
import sys
from . import calldef
from . import algorithm
from . import decl_visitor


class decl_printer_t( decl_visitor.decl_visitor_t ):
    """helper class for printing declarations tree"""

    JUSTIFY = 20
    INDENT_SIZE = 4

    def __init__( self, level=0, print_details=True, recursive=True, writer=None, verbose=True ):
        decl_visitor.decl_visitor_t.__init__(self)
        self.__inst = None
        self.__level = level
        self.__print_details = print_details
        self.__recursive = recursive
        self.__verbose = verbose
        self.__writer = writer
        if not self.__writer:
            self.__writer = lambda x: sys.stdout.write( x )

    def clone(self, increment_level=True):
        level = self.__level
        if increment_level:
            level += 1
        return decl_printer_t( level
                               , self.print_details
                               , recursive=self.recursive
                               , verbose=self.__verbose
                               , writer=self.writer )

    def _get_recursive(self):
        return self.__recursive
    def _set_recursive(self, recursive):
        self.__recursive = recursive
    recursive = property( _get_recursive, _set_recursive)

    def _get_verbose(self):
        return self.__verbose
    def _set_verbose(self, verbose):
        self.__verbose = verbose
    verbose = property( _get_verbose, _set_verbose)

    def _get_level(self):
        return self.__level
    def _set_level(self, lvl):
        self.__level = lvl
    level = property( _get_level, _set_level )

    def _get_print_details(self):
        return self.__print_details
    def _set_print_details(self, details):
        self.__print_details = details
    print_details = property( _get_print_details, _set_print_details)

    def _get_writer(self):
        return self.__writer
    def _set_writer(self, writer):
        self.__writer = writer
    writer = property( _get_writer, _set_writer)

    def _get_inst(self):
        return self.__inst
    def _set_inst(self, inst):
        self.__inst = inst
    instance = property( _get_inst, _set_inst )

    def is_builtin_decl( self, decl ):
        if not decl.name.startswith( '__builtin_' ):
            return False
        if decl.location \
           and decl.location.file_name \
           and decl.location.file_name.endswith('gccxml_builtins.h'):
            return True
        else:
            return False

    def __nice_decl_name( self, inst ):
        name = inst.__class__.__name__
        return name
        #if name.endswith( '_t' ):
        #    name = name[:-len('_t')]
        #return name.replace( '_', ' ' )

    def print_decl_header(self):
        header = self.__nice_decl_name( self.__inst ) + ": '%s'" % self.__inst.name
        self.writer( ' ' * self.level * self.INDENT_SIZE + header.ljust( self.JUSTIFY ) + os.linesep)
        if self.__print_details:
            curr_level = self.level + 1
            if self.__inst.location:
                location = 'location: [%s]:%s'%(self.__inst.location.file_name, self.__inst.location.line)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + location + os.linesep)
            if self.verbose:
                artificial = 'artificial: ' + "'%s'" % str(self.__inst.is_artificial)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + artificial.ljust( self.JUSTIFY ) + os.linesep)
            if self.verbose and self.__inst.attributes:
                attributes = 'attributes: %s'%(self.__inst.attributes)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + attributes + os.linesep)
            if self.verbose and self.__inst.demangled:
                demangled = 'demangled: %s'%(self.__inst.demangled)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + demangled + os.linesep)
            if self.verbose and self.__inst.mangled:
                mangled = 'mangled: %s'%(self.__inst.mangled)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + mangled + os.linesep)
            if self.verbose and self.__inst.decorated_name:
                decorated_name = 'decorated name: %s'%(self.__inst.decorated_name)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + decorated_name + os.linesep)

    def print_calldef_info(self, decl=None):
        if None is decl:
            decl = self.__inst

        retval = None
        if decl.return_type:
            retval = decl.return_type.decl_string
        args = []
        for arg in decl.arguments:
            args.append(arg.type.decl_string + ' ' + arg.name)
        indent = ' ' * (self.level+1) * self.INDENT_SIZE
        self.writer( indent + "is extern: " + str(decl.has_extern)  + os.linesep)
        self.writer( indent + "return type: " + str(retval)  + os.linesep)
        self.writer( indent + "arguments type: " + ', '.join(args) + os.linesep)
        self.writer( indent + "calling convention: __%s__" % decl.calling_convention + os.linesep)
        if isinstance( decl, calldef.member_calldef_t ):
            self.writer( indent + "virtual: " + str(decl.virtuality) + os.linesep)
            self.writer( indent + "is const: " + str(decl.has_const) + os.linesep)
            self.writer( indent + "is static: " + str(decl.has_static) + os.linesep)

    def visit_member_function( self ):
        self.print_decl_header()
        self.print_calldef_info()

    def visit_constructor( self ):
        self.print_decl_header()
        self.print_calldef_info()

        indent = ' ' * (self.level+1) * self.INDENT_SIZE
        self.writer( indent + "explicit: " + str(self.__inst.explicit)  + os.linesep)

        if self.__print_details:
            self.writer( indent
                         + 'copy constructor: ' + str(self.__inst.is_copy_constructor) + os.linesep)

    def visit_destructor( self ):
        self.print_decl_header()

    def visit_member_operator( self ):
        self.print_decl_header()
        self.print_calldef_info()

    def visit_casting_operator( self ):
        self.print_decl_header()
        self.print_calldef_info()

    def visit_free_function( self ):
        self.print_decl_header()
        self.print_calldef_info()

    def visit_free_operator( self ):
        self.print_decl_header()
        self.print_calldef_info()

    def visit_class_declaration(self ):
        self.print_decl_header()

    def visit_class(self ):
        self.print_decl_header()
        curr_level = self.level + 1
        class_type = 'class type: ' + "'%s'" % str(self.__inst.class_type)
        self.writer( ' ' * curr_level * self.INDENT_SIZE + class_type.ljust( self.JUSTIFY ) + os.linesep)
        if self.__print_details:
            byte_size = 'size: %d'%(self.__inst.byte_size)
            self.writer( ' ' * curr_level * self.INDENT_SIZE + byte_size.ljust( self.JUSTIFY ) + os.linesep)
            try:
                byte_align = 'align: %d'%(self.__inst.byte_align)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + byte_align.ljust( self.JUSTIFY ) + os.linesep)
            except NotImplementedError:
                self.writer( ' ' * curr_level * self.INDENT_SIZE + "align: not implemented".ljust( self.JUSTIFY ) + os.linesep)

        if self.__inst.aliases:
            aliases = [typedef.name for typedef in self.__inst.aliases]
            aliases.sort()
            msg = 'aliases: ' + repr(aliases)
            self.writer( ' ' * curr_level * self.INDENT_SIZE + msg.ljust( self.JUSTIFY ) + os.linesep)

        def print_hierarchy(hierarchy_type, classes, curr_level):
            self.writer( ' ' * curr_level * self.INDENT_SIZE + hierarchy_type.ljust( self.JUSTIFY ) + os.linesep)
            curr_level += 1
            for class_ in classes:
                class_str = 'class: ' + "'%s'" % str(class_.related_class.decl_string)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + class_str.ljust( self.JUSTIFY ) + os.linesep)
                access = 'access type: ' + "'%s'" % str(class_.access)
                self.writer( ' ' * (curr_level + 1)* self.INDENT_SIZE + access.ljust( self.JUSTIFY ) + os.linesep)
                if not ( None is class_.is_virtual ):
                    is_virtual = 'virtual inheritance: ' + "'%s'" % str(class_.is_virtual)
                    self.writer( ' ' * (curr_level + 1)* self.INDENT_SIZE + is_virtual.ljust( self.JUSTIFY ) + os.linesep)

        def print_members(members_type, members, curr_level):
            members = members[:]
            members.sort()
            self.writer( ' ' * curr_level * self.INDENT_SIZE + members_type.ljust( self.JUSTIFY ) + os.linesep)
            if self.__recursive:
                curr_level += 1
                for member in members:
                    prn = self.clone()
                    prn.instance = member
                    algorithm.apply_visitor( prn, member )

        if self.__inst.bases:
            print_hierarchy( 'base classes: ', self.__inst.bases, curr_level )

        if self.__inst.derived:
            print_hierarchy( 'derived classes: ', self.__inst.derived, curr_level )

        print_members( 'public: ', self.__inst.public_members, curr_level )
        print_members( 'protected: ', self.__inst.protected_members, curr_level )
        print_members( 'private: ', self.__inst.private_members, curr_level )

    def visit_enumeration(self):
        self.print_decl_header()
        curr_level = self.level + 1
        self.writer( ' ' * curr_level * self.INDENT_SIZE + 'values:'.ljust( self.JUSTIFY )  + os.linesep)
        value_level = ' ' * ( curr_level + 1 )* self.INDENT_SIZE
        self.writer( os.linesep  + os.linesep)
        for name, value in self.__inst.values:
            self.writer( value_level + "%s : %s"% (name, value) + os.linesep)

    def visit_namespace(self ):
        if not self.verbose and not self.__inst.declarations:
            return #don't print info about empty namespaces
        self.print_decl_header()
        if self.__recursive:
            inst_decls = self.__inst.declarations[:]
            inst_decls.sort()
            for decl in inst_decls:
                if self.is_builtin_decl( decl ):
                    continue
                prn = self.clone()
                prn.instance = decl
                algorithm.apply_visitor( prn, decl )

    def visit_typedef(self ):
        self.print_decl_header()
        curr_level = self.level + 1
        self.writer( ' ' * curr_level * self.INDENT_SIZE + 'alias to: ' + self.__inst.type.decl_string + os.linesep)

    def visit_variable(self ):
        self.print_decl_header()
        curr_level = self.level + 1
        self.writer( ' ' * curr_level * self.INDENT_SIZE + 'type: %s' % self.__inst.type.decl_string  + os.linesep)
        self.writer( ' ' * curr_level * self.INDENT_SIZE + 'value: %s' % self.__inst.value + os.linesep)
        if self.__print_details:
            if self.__inst.bits:
                bits = 'bits: %d'%(self.__inst.bits)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + bits.ljust( self.JUSTIFY ) + os.linesep)

            byte_size = 'size: %d'%(self.__inst.type.byte_size)
            self.writer( ' ' * curr_level * self.INDENT_SIZE + byte_size.ljust( self.JUSTIFY ) + os.linesep)
            try:
                byte_align = 'align: %d'%(self.__inst.type.byte_align)
                self.writer( ' ' * curr_level * self.INDENT_SIZE + byte_align.ljust( self.JUSTIFY ) + os.linesep)
            except NotImplementedError:
                self.writer( ' ' * curr_level * self.INDENT_SIZE + "align: not implemented".ljust( self.JUSTIFY ) + os.linesep)
            byte_offset = 'offset: %d'%(self.__inst.byte_offset)
            self.writer( ' ' * curr_level * self.INDENT_SIZE + byte_offset + os.linesep + os.linesep)

def print_declarations( decls
                        , detailed=True
                        , recursive=True
                        , writer=lambda x: sys.stdout.write( x + os.linesep )
                        , verbose=True):
    """
    print declarations tree rooted at each of the included nodes.

    :param decls: either a single :class:declaration_t object or list of :class:declaration_t objects
    """
    prn = decl_printer_t(0, detailed, recursive, writer, verbose=verbose)
    if type(decls) is not list:
        decls = [decls]
    for d in decls:
        prn.level = 0
        prn.instance = d
        algorithm.apply_visitor(prn, d)

def dump_declarations( decls, fpath ):
    """
    dump declarations tree rooted at each of the included nodes to the file

    :param decls: either a single :class:declaration_t object or list of :class:declaration_t objects
    :param fpath: file name
    """
    fobj = open( fpath, 'w+' )
    print_declarations( decls, writer=fobj.write )
    fobj.close()
