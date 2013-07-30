# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""implementation details"""

import types

class parser_t( object ):
    """implementation details"""
    def __init__( self
                  , pattern_char_begin
                  , pattern_char_end
                  , pattern_char_separator ):
        self.__begin = pattern_char_begin
        self.__end = pattern_char_end
        self.__separator = pattern_char_separator
        #right now parser does not take into account next qualifiers, but it will
        self.__text_qualifier = '"'
        self.__char_qualifier = "'"
        self.__escape = '\\'

    def has_pattern( self, decl_string ):
        """implementation details"""
        last_part = decl_string.split( '::' )[-1]
        return -1 != decl_string.find( self.__begin ) and -1 != last_part.find( self.__end )

    def name( self, decl_string ):
        """implementation details"""
        assert isinstance( decl_string, str )
        if not self.has_pattern( decl_string ):
            return decl_string
        args_begin = decl_string.find( self.__begin )
        return decl_string[0: args_begin].strip()

    def __find_args_separator( self, decl_string, start_pos ):
        """implementation details"""
        bracket_depth = 0
        for index, ch in enumerate( decl_string[start_pos:] ):
            if ch not in ( self.__begin, self.__end, self.__separator ):
                continue #I am interested only in < and >
            elif self.__separator == ch:
                if not bracket_depth:
                    return index + start_pos
            elif self.__begin == ch:
                bracket_depth += 1
            elif not bracket_depth:
                return index + start_pos
            else:
                bracket_depth -= 1
        return -1

    def args( self, decl_string ):
        """implementation details"""
        args_begin = decl_string.find( self.__begin )
        args_end = decl_string.rfind( self.__end )
        if -1 in ( args_begin, args_end ) or args_begin == args_end:
            raise RuntimeError( "%s doesn't valid template instantiation string" % decl_string )

        args_only = decl_string[args_begin + 1: args_end ]
        args = []
        previous_found, found = 0, 0
        while True:
            found = self.__find_args_separator( args_only, previous_found)
            if -1 == found:
                args.append( args_only[ previous_found : ] )
                break
            #elif decl_string[ found ] == self.__end:
            #    print args
            #    raise RuntimeError( "unmatched '%s' token has been found." % self.__end )
            else:
                args.append( args_only[ previous_found : found ] )
            previous_found = found + 1 #skip found sep
        return [ arg.strip() for arg in args ]

    NOT_FOUND = ( -1, -1 )
    """implementation details"""

    def find_args(self, text, start=None ):
        """implementation details"""
        if start==None:
            start = 0
        first_occurance = text.find( self.__begin, start )
        if first_occurance == -1:
            return self.NOT_FOUND
        previous_found, found = first_occurance + 1, 0
        while True:
            found = self.__find_args_separator( text, previous_found)
            if -1 == found:
                return self.NOT_FOUND
            elif text[ found ] == self.__end:
                return ( first_occurance, found )
            else:
                previous_found = found + 1 #skip found sep

    def split( self, decl_string ):
        """implementation details"""
        assert self.has_pattern( decl_string )
        return self.name( decl_string ), self.args( decl_string )

    def split_recursive( self, decl_string ):
        """implementation details"""
        assert self.has_pattern( decl_string )
        answer = []
        to_go = [ decl_string ]
        while to_go:
            name, args = self.split( to_go.pop() )
            answer.append( ( name, args ) )
            for arg in args:
                if self.has_pattern( arg ):
                    to_go.append( arg )
        return answer

    def join( self, name, args, arg_separator=None ):
        """implementation details"""
        if None is arg_separator:
            arg_separator = ', '
        args = [_f for _f in args if _f]
        args_str = ''
        if not args:
            args_str = ' '
        elif 1 == len( args ):
            args_str = ' ' + args[0] + ' '
        else:
            args_str = ' ' + arg_separator.join( args ) + ' '

        return ''.join( [ name, self.__begin, args_str, self.__end ] )

    def normalize( self, decl_string, arg_separator=None ):
        """implementation details"""
        if not self.has_pattern( decl_string ):
            return decl_string
        name, args = self.split( decl_string )
        for i, arg in enumerate( args ):
            args[i] = self.normalize( arg )
        return self.join( name, args, arg_separator )



