//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VARIANT_TOOLS_H
#define VARIANT_TOOLS_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

// Quick way to define variants in a datadesc.
extern ISaveRestoreOps *variantFuncs;
#define DEFINE_VARIANT(name) { FIELD_CUSTOM, #name, { offsetof(classNameTypedef, name), 0 }, 1, FTYPEDESC_SAVE, NULL, variantFuncs, NULL }
#define DEFINE_KEYVARIANT(name,mapname)	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef, name), 0 }, 1, FTYPEDESC_SAVE | FTYPEDESC_KEY, mapname, variantFuncs, NULL }

// Most of these are defined in variant_t.cpp.

// Creates a variant_t from the given string.
// It could return as a String or a Float.
variant_t Variant_Parse(const char *szValue);

// Intended to convert FIELD_INPUT I/O parameters to other values, like integers, floats, or even entities.
// This only changes FIELD_STRING variants. Other data like FIELD_EHANDLE or FIELD_INTEGER are not affected.
variant_t Variant_ParseInput(inputdata_t inputdata);

// A simpler version of Variant_ParseInput that does not allow FIELD_EHANDLE.
variant_t Variant_ParseString(variant_t value);

// val1 == val2
bool Variant_Equal(variant_t val1, variant_t val2, bool bLenAllowed = true);

// val1 > val2
bool Variant_Greater(variant_t val1, variant_t val2, bool bLenAllowed = true);

// val1 >= val2
bool Variant_GreaterOrEqual(variant_t val1, variant_t val2, bool bLenAllowed = true);

#endif