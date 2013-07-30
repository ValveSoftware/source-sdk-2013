# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""this module defines few function that will guess what creator(s) should be
created for exposing a declaration
"""

from pygccxml import declarations
from pyplusplus import code_creators


ACCESS_TYPES = declarations.ACCESS_TYPES
VIRTUALITY_TYPES = declarations.VIRTUALITY_TYPES


def find_out_mem_fun_creator_classes( decl ):
    """return tuple of ( registration, declaration ) code creator classes"""
    maker_cls = None
    fwrapper_cls = None
    access_level = decl.parent.find_out_member_access_type( decl )
    if len( decl.transformations ) not in ( 0, 1 ):
        raise RuntimeError( "Right now `Py++` does not support multiple transformation applied on a single function." )
    if access_level == ACCESS_TYPES.PUBLIC:
        if decl.virtuality == VIRTUALITY_TYPES.NOT_VIRTUAL:
            if decl.transformations:
                maker_cls = code_creators.mem_fun_transformed_t
                fwrapper_cls = code_creators.mem_fun_transformed_wrapper_t
            else:
                maker_cls = code_creators.mem_fun_t
        elif decl.virtuality == VIRTUALITY_TYPES.PURE_VIRTUAL:
            if decl.transformations:
                maker_cls = code_creators.mem_fun_v_transformed_t
                fwrapper_cls = code_creators.mem_fun_v_transformed_wrapper_t
            else:
                fwrapper_cls = code_creators.mem_fun_pv_wrapper_t
                maker_cls = code_creators.mem_fun_pv_t
        else:
            if decl.transformations:
                fwrapper_cls = code_creators.mem_fun_v_transformed_wrapper_t
                maker_cls = code_creators.mem_fun_v_transformed_t
            else:
                if decl.overridable:
                    fwrapper_cls = code_creators.mem_fun_v_wrapper_t
                maker_cls = code_creators.mem_fun_v_t
    elif access_level == ACCESS_TYPES.PROTECTED:
        if decl.virtuality == VIRTUALITY_TYPES.NOT_VIRTUAL:
            if decl.has_static:
                fwrapper_cls = code_creators.mem_fun_protected_s_wrapper_t
                maker_cls = code_creators.mem_fun_protected_s_t
            else:
                fwrapper_cls = code_creators.mem_fun_protected_wrapper_t
                maker_cls = code_creators.mem_fun_protected_t
        elif decl.virtuality == VIRTUALITY_TYPES.VIRTUAL:
            if decl.overridable:
                fwrapper_cls = code_creators.mem_fun_protected_v_wrapper_t
                maker_cls = code_creators.mem_fun_protected_v_t
        else:
            fwrapper_cls = code_creators.mem_fun_protected_pv_wrapper_t
            maker_cls = code_creators.mem_fun_protected_pv_t
    else: #private
        if decl.virtuality == VIRTUALITY_TYPES.NOT_VIRTUAL:
            pass#in general we should not come here
        elif decl.virtuality == VIRTUALITY_TYPES.PURE_VIRTUAL:
            fwrapper_cls = code_creators.mem_fun_private_pv_wrapper_t
        else:
            if decl.overridable:
                fwrapper_cls = code_creators.mem_fun_v_wrapper_t
                maker_cls = code_creators.mem_fun_v_t
    return ( maker_cls, fwrapper_cls )
