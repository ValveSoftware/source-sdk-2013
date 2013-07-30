# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

from pygccxml import declarations
from pyplusplus import decl_wrappers

class COLOR:
    WHITE = 0
    GRAY = 1
    BLACK = 2

class class_organizer_t(object):
    def __init__( self, decls, include_vars=False):
        object.__init__( self )

        self.__include_vars = include_vars
        self.__classes = [x for x in decls if isinstance( x, declarations.class_t )]
        self.__classes.sort( key = lambda cls: cls.decl_string )
        self.__dependencies_graph = self._build_graph()
        self.__time = 0
        self.__colors = dict( list(zip( list(self.__dependencies_graph.keys())
                              , [ COLOR.WHITE ] * len( self.__dependencies_graph ) )) )
        self.__class_discovered = dict( list(zip( list(self.__dependencies_graph.keys())
                                        , [ 0 ] * len( self.__dependencies_graph ) )) )
        self.__class_treated = dict( list(zip( list(self.__dependencies_graph.keys())
                                     , [ 0 ] * len( self.__dependencies_graph ) )) )

        self.__desired_order = []

        self._topological_sort()

    def _build_graph(self):
        full_name = declarations.full_name
        graph = {} #
        for class_ in self.__classes:
            assert isinstance( class_, declarations.class_t )
            fname = full_name( class_ )
            graph[ fname ] = self.__find_out_class_dependencies( class_ )
        return graph

    def __find_out_class_dependencies( self, class_ ):
        full_name = declarations.full_name
        #class depends on it's base classes
        i_depend_on_them = set( [ full_name( base.related_class ) for base in class_.bases ] )
        #class depends on all classes that used in function as argument
        # types and those arguments have default value
        calldefs = [decl for decl in declarations.make_flatten( class_ ) if isinstance( decl, declarations.calldef_t )]
        for calldef in calldefs:
            for arg in calldef.arguments:
                if declarations.is_enum( arg.type ):
                    top_class_inst = self.__get_top_class_inst( declarations.enum_declaration( arg.type ) )
                    if top_class_inst:
                        i_depend_on_them.add( full_name( top_class_inst ) )
                    continue
                if not arg.default_value:
                    continue
                if declarations.is_pointer( arg.type ) and arg.default_value == 0:
                    continue
                base_type = declarations.base_type( arg.type )
                if not isinstance( base_type, declarations.declarated_t ):
                    continue
                top_class_inst = self.__get_top_class_inst( base_type.declaration )
                if top_class_inst:
                    i_depend_on_them.add( full_name( top_class_inst ) )

        if self.__include_vars:
            vars = [decl for decl in declarations.make_flatten( class_ ) if isinstance( decl, declarations.variable_t )]
            for var in vars:
                if declarations.is_pointer( var.type ):
                    continue
                base_type = declarations.base_type( var.type )
                if not isinstance( base_type, declarations.declarated_t ):
                    continue
                top_class_inst = self.__get_top_class_inst( base_type.declaration )
                if top_class_inst:
                    i_depend_on_them.add( full_name( top_class_inst ) )

        for internal_cls in class_.classes(allow_empty=True):
            internal_cls_dependencies = self.__find_out_class_dependencies( internal_cls )
            i_depend_on_them.update( internal_cls_dependencies )

        i_depend_on_them = list( i_depend_on_them )
        i_depend_on_them.sort()
        return i_depend_on_them

    def __get_top_class_inst( self, decl ):
        curr = decl
        while isinstance( curr.parent, declarations.class_t ):
            curr = curr.parent
        if isinstance( curr, declarations.class_t ):
            return curr

    def _topological_sort(self):
        self._dfs()

    def _dfs( self ):
        for class_ in sorted( self.__dependencies_graph.keys() ):
            if self.__colors[class_] == COLOR.WHITE:
                self._dfs_visit(class_)

    def _dfs_visit(self, base):
        self.__colors[base] = COLOR.GRAY
        self.__time += 1
        self.__class_discovered[base] = self.__time
        for derived in self.__dependencies_graph[base]:
            if derived in self.__colors and self.__colors[derived] == COLOR.WHITE:
                self._dfs_visit( derived )
            else:
                pass
                #there is usecase where base class defined within some class
                #but his derives defined out of the class. right now `Py++`
                #doesn't supports this situation.

        self.__colors[base] = COLOR.BLACK
        self.__time += 1
        self.__class_treated = self.__time
        self.__desired_order.append(base)

    def desired_order(self):
        full_name = declarations.full_name
        fname2inst = {}
        for class_inst in self.__classes:
            fname2inst[ full_name( class_inst ) ] = class_inst
        answer = []
        for fname in self.__desired_order:
            answer.append( fname2inst[fname] )
        return answer


def cmp_to_key(mycmp):
    """Convert a cmp= function into a key= function"""
    class K(object):
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return mycmp(self.obj, other.obj) < 0
        def __gt__(self, other):
            return mycmp(self.obj, other.obj) > 0
        def __eq__(self, other):
            return mycmp(self.obj, other.obj) == 0
        def __le__(self, other):
            return mycmp(self.obj, other.obj) <= 0
        def __ge__(self, other):
            return mycmp(self.obj, other.obj) >= 0
        def __ne__(self, other):
            return mycmp(self.obj, other.obj) != 0
    return K


class calldef_organizer_t( object ):
    #Take a look on this post:
    #  http://mail.python.org/pipermail/c++-sig/2006-October/011463.html

    #calldef_organizer_t will take into account only required arguments.
    #Next rules are implemented:
    #1. calldef( bool ) will be the last registered function
    #2. T* will come after T ( const T& )
    def __init__( self ):
        object.__init__( self )
        #preserve order in which functions where defined
        self.cmp_calldefs_fallback = lambda d1, d2: cmp( d1.location.line, d2.location.line )

    def build_groups( self, decls ):
        groups = { None: [] }
        decl2order = {}
        for index,d in enumerate( decls ):
            decl2order[d] = index
            if not isinstance( d, declarations.calldef_t ) or 0 == len( d.required_args ):
                groups[ None ].append( d )
            else:
                key = ( d.name, len( d.required_args ) )
                if key not in groups:
                    groups[ key ] = []
                groups[key].append( d )
        #keep backward compatibility
        to_be_deleted = []
        for group, group_decls in groups.items():
            if None is group:
                continue
            if len( group_decls ) == 1:
                groups[ None ].append( group_decls[0] )
                to_be_deleted.append( group )
        for group in to_be_deleted:
            del groups[ group ]
        groups[ None ].sort( key=lambda d: decl2order[d] )
        return groups

    def cmp_args_types( self, t1, t2 ):
        return decl_wrappers.algorithm.registration_order.is_related( t1, t2 )

    def cmp_calldefs( self, f1, f2 ):
        result = self.cmp_args_types( f1.required_args[-1].type, f2.required_args[-1].type )
        if None is result:
            result = self.cmp_calldefs_fallback( f1, f2 )
        return result

    def sort_groups( self, groups ):
        for group in list(groups.keys()):
            if None is group:
                continue
            groups[ group ].sort( key=cmp_to_key(self.cmp_calldefs) )

    def join_groups( self, groups ):
        decls = []
        keys = set(groups.keys())
        if None in keys:
            keys.remove(None)
        for group in sorted(keys):
            decls.extend( groups[group] )
        return decls

    def sort( self, decls ):
        groups = self.build_groups( decls )
        self.sort_groups(groups)
        result = self.join_groups(groups)
        return result

def sort_classes( classes, include_vars=False ):
    organizer = class_organizer_t( classes, include_vars=include_vars )
    return organizer.desired_order()

def sort_calldefs( decls ):
    return calldef_organizer_t().sort( decls )

USE_CALLDEF_ORGANIZER = False
#If you understand what problem calldef_organizer_t solves, than may be you should
#use this.

def sort( decls ):
    classes = [x for x in decls if isinstance( x, declarations.class_t )]
    ordered = sort_classes( classes )

    ids = set( [ id( inst ) for inst in ordered ] )
    for decl in decls:
        if id( decl ) not in ids:
            ids.add( id(decl) )
            ordered.append( decl )
    #type should be exported before it can be used.
    variables = []
    enums = []
    others = []
    classes = []
    constructors = []
    for inst in ordered:
        if isinstance( inst, declarations.variable_t ):
            variables.append( inst )
        elif isinstance( inst, declarations.enumeration_t ):
            enums.append( inst )
        elif isinstance( inst, ( declarations.class_t, declarations.class_declaration_t ) ):
            classes.append( inst )
        elif isinstance( inst, declarations.constructor_t ):
            constructors.append( inst )
        else:
            others.append( inst )
    #this will prevent from py++ to change the order of generated code
    cmp_by_name = lambda d: d.name
    cmp_by_line = lambda d: d.location.line

    enums.sort( key=cmp_by_name )
    variables.sort( key=cmp_by_name )
    if USE_CALLDEF_ORGANIZER:
        others = sort_calldefs(others)
        constructors = sort_calldefs(constructors)
    else:
        others.sort( key=cmp_by_name )
        constructors.sort( key=cmp_by_line )

    new_ordered = []
    new_ordered.extend( enums )
    new_ordered.extend( classes )
    new_ordered.extend( constructors )
    new_ordered.extend( others )
    new_ordered.extend( variables )
    return new_ordered #
