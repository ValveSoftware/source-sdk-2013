//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Custom implementation of VScript in Source 2013, created from scratch
//			using the Alien Swarm SDK as a reference for Valve's library.
// 
// Author(s): ReDucTor (header written by Blixibon)
//
// $NoKeywords: $
//=============================================================================//

#include "vscript/ivscript.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlmap.h"
#include "tier1/utlstring.h"

#include "squirrel.h"
#include "sqstdaux.h"
//#include "sqstdblob.h"
//#include "sqstdsystem.h"
#include "sqstdtime.h"
//#include "sqstdio.h"
#include "sqstdmath.h"
#include "sqstdstring.h"

// HACK: Include internal parts of squirrel for serialization
#include "squirrel/squirrel/sqobject.h"
#include "squirrel/squirrel/sqstate.h"
#include "squirrel/squirrel/sqtable.h"
#include "squirrel/squirrel/sqclass.h"
#include "squirrel/squirrel/sqfuncproto.h"
#include "squirrel/squirrel/sqvm.h"
#include "squirrel/squirrel/sqclosure.h"

#include "color.h"
#include "tier1/utlbuffer.h"
#include "tier1/mapbase_con_groups.h"

#include "vscript_squirrel.nut"

#include <cstdarg>

struct WriteStateMap
{
	CUtlMap<void*, int> cache;
	WriteStateMap() : cache(DefLessFunc(void*))
	{}

	bool CheckCache(CUtlBuffer* pBuffer, void* ptr)
	{
		auto idx = cache.Find(ptr);
		if (idx != cache.InvalidIndex())
		{
			pBuffer->PutInt(cache[idx]);
			return true;
		}
		else
		{
			int newIdx = cache.Count();
			cache.Insert(ptr, newIdx);
			pBuffer->PutInt(newIdx);
			return false;
		}
	}
};

struct ReadStateMap
{
	CUtlMap<int, HSQOBJECT> cache;
	HSQUIRRELVM vm_;
	ReadStateMap(HSQUIRRELVM vm) : cache(DefLessFunc(int)), vm_(vm)
	{}

	~ReadStateMap()
	{
		FOR_EACH_MAP_FAST(cache, i)
		{
			HSQOBJECT& obj = cache[i];
			sq_release(vm_, &obj);
		}
	}

	bool CheckCache(CUtlBuffer* pBuffer, HSQOBJECT** obj)
	{
		int marker = pBuffer->GetInt();

		auto idx = cache.Find(marker);
		if (idx != cache.InvalidIndex())
		{
			*obj = &cache[idx];
			return true;
		}
		else
		{
			HSQOBJECT temp;
			sq_resetobject(&temp);
			auto idx = cache.Insert(marker, temp);
			*obj = &cache[idx];
			return false;
		}
	}
};

class SquirrelVM : public IScriptVM
{
public:
	virtual bool Init() override;
	virtual void Shutdown() override;

	virtual bool ConnectDebugger() override;
	virtual void DisconnectDebugger() override;

	virtual ScriptLanguage_t GetLanguage() override;
	virtual const char* GetLanguageName() override;

	virtual void AddSearchPath(const char* pszSearchPath) override;

	//--------------------------------------------------------

	virtual bool Frame(float simTime) override;

	//--------------------------------------------------------
	// Simple script usage
	//--------------------------------------------------------
	virtual ScriptStatus_t Run(const char* pszScript, bool bWait = true) override;

	//--------------------------------------------------------
	// Compilation
	//--------------------------------------------------------
	virtual HSCRIPT CompileScript(const char* pszScript, const char* pszId = NULL) override;
	virtual void ReleaseScript(HSCRIPT) override;

	//--------------------------------------------------------
	// Execution of compiled
	//--------------------------------------------------------
	virtual ScriptStatus_t Run(HSCRIPT hScript, HSCRIPT hScope = NULL, bool bWait = true) override;
	virtual ScriptStatus_t Run(HSCRIPT hScript, bool bWait) override;

	//--------------------------------------------------------
	// Scope
	//--------------------------------------------------------
	virtual HSCRIPT CreateScope(const char* pszScope, HSCRIPT hParent = NULL) override;
	virtual void ReleaseScope(HSCRIPT hScript) override;

	//--------------------------------------------------------
	// Script functions
	//--------------------------------------------------------
	virtual HSCRIPT LookupFunction(const char* pszFunction, HSCRIPT hScope = NULL) override;
	virtual void ReleaseFunction(HSCRIPT hScript) override;

	//--------------------------------------------------------
	// Script functions (raw, use Call())
	//--------------------------------------------------------
	virtual ScriptStatus_t ExecuteFunction(HSCRIPT hFunction, ScriptVariant_t* pArgs, int nArgs, ScriptVariant_t* pReturn, HSCRIPT hScope, bool bWait) override;

	//--------------------------------------------------------
	// External functions
	//--------------------------------------------------------
	virtual void RegisterFunction(ScriptFunctionBinding_t* pScriptFunction) override;

	//--------------------------------------------------------
	// External classes
	//--------------------------------------------------------
	virtual bool RegisterClass(ScriptClassDesc_t* pClassDesc) override;

	//--------------------------------------------------------
	// External constants
	//--------------------------------------------------------
	virtual void RegisterConstant(ScriptConstantBinding_t *pScriptConstant) override;

	//--------------------------------------------------------
	// External enums
	//--------------------------------------------------------
	virtual void RegisterEnum(ScriptEnumDesc_t *pEnumDesc) override;

	//--------------------------------------------------------
	// External instances. Note class will be auto-registered.
	//--------------------------------------------------------

	virtual HSCRIPT RegisterInstance(ScriptClassDesc_t* pDesc, void* pInstance, bool bAllowDestruct = false) override;
	virtual void SetInstanceUniqeId(HSCRIPT hInstance, const char* pszId) override;
	virtual void RemoveInstance(HSCRIPT hInstance) override;

	virtual void* GetInstanceValue(HSCRIPT hInstance, ScriptClassDesc_t* pExpectedType = NULL) override;

	//----------------------------------------------------------------------------

	virtual bool GenerateUniqueKey(const char* pszRoot, char* pBuf, int nBufSize) override;

	//----------------------------------------------------------------------------

	virtual bool ValueExists(HSCRIPT hScope, const char* pszKey) override;

	virtual bool SetValue(HSCRIPT hScope, const char* pszKey, const char* pszValue) override;
	virtual bool SetValue(HSCRIPT hScope, const char* pszKey, const ScriptVariant_t& value) override;

	virtual void CreateTable(ScriptVariant_t& Table) override;
	virtual int	GetNumTableEntries(HSCRIPT hScope) override;
	virtual int GetKeyValue(HSCRIPT hScope, int nIterator, ScriptVariant_t* pKey, ScriptVariant_t* pValue) override;

	virtual bool GetValue(HSCRIPT hScope, const char* pszKey, ScriptVariant_t* pValue) override;
	virtual void ReleaseValue(ScriptVariant_t& value) override;

	virtual bool ClearValue(HSCRIPT hScope, const char* pszKey) override;

	// virtual void CreateArray(ScriptVariant_t &arr, int size = 0) override;
	virtual bool ArrayAppend(HSCRIPT hArray, const ScriptVariant_t &val) override;

	//----------------------------------------------------------------------------

	virtual void WriteState(CUtlBuffer* pBuffer) override;
	virtual void ReadState(CUtlBuffer* pBuffer) override;
	virtual void RemoveOrphanInstances() override;

	virtual void DumpState() override;

	virtual void SetOutputCallback(ScriptOutputFunc_t pFunc) override;
	virtual void SetErrorCallback(ScriptErrorFunc_t pFunc) override;

	//----------------------------------------------------------------------------

	virtual bool RaiseException(const char* pszExceptionText) override;


	void WriteObject(CUtlBuffer* pBuffer, WriteStateMap& writeState, SQInteger idx);
	void ReadObject(CUtlBuffer* pBuffer, ReadStateMap& readState);
	HSQUIRRELVM vm_ = nullptr;
	HSQOBJECT lastError_;
	HSQOBJECT vectorClass_;
	HSQOBJECT regexpClass_;
};

SQUserPointer TYPETAG_VECTOR = "VectorTypeTag";

namespace SQVector
{
	SQInteger Construct(HSQUIRRELVM vm)
	{
		// TODO: There must be a nicer way to store the data with the actual instance, there are
		// default slots but they are really just pointers anyway and you need to hold a member
		// pointer which is yet another pointer dereference
		int numParams = sq_gettop(vm);

		float x = 0;
		float y = 0;
		float z = 0;

		if ((numParams >= 2 && SQ_FAILED(sq_getfloat(vm, 2, &x))) ||
			(numParams >= 3 && SQ_FAILED(sq_getfloat(vm, 3, &y))) ||
			(numParams >= 4 && SQ_FAILED(sq_getfloat(vm, 4, &z))))
		{
			return sq_throwerror(vm, "Expected Vector(float x, float y, float z)");
		}

		SQUserPointer p;
		sq_getinstanceup(vm, 1, &p, 0);
		new (p) Vector(x, y, z);

		return 0;
	}

	SQInteger Get(HSQUIRRELVM vm)
	{
		const char* key = nullptr;
		sq_getstring(vm, 2, &key);

		if (key == nullptr)
		{
			return sq_throwerror(vm, "Expected Vector._get(string)");
		}

		if (key[0] < 'x' || key['0'] > 'z' || key[1] != '\0')
		{
			return sqstd_throwerrorf(vm, "the index '%.50s' does not exist", key);
		}

		Vector* v = nullptr;
		if (SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Unable to get Vector");
		}

		int idx = key[0] - 'x';
		sq_pushfloat(vm, (*v)[idx]);
		return 1;
	}

	SQInteger Set(HSQUIRRELVM vm)
	{
		const char* key = nullptr;
		sq_getstring(vm, 2, &key);

		if (key == nullptr)
		{
			return sq_throwerror(vm, "Expected Vector._set(string)");
		}

		if (key[0] < 'x' || key['0'] > 'z' || key[1] != '\0')
		{
			return sqstd_throwerrorf(vm, "the index '%.50s' does not exist", key);
		}

		Vector* v = nullptr;
		if (SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Unable to get Vector");
		}

		float val = 0;
		if (SQ_FAILED(sq_getfloat(vm, 3, &val)))
		{
			return sqstd_throwerrorf(vm, "Vector.%s expects float", key);
		}

		int idx = key[0] - 'x';
		(*v)[idx] = val;
		return 0;
	}

	SQInteger Add(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;
		Vector* v2 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)) ||
			SQ_FAILED(sq_getinstanceup(vm, 2, (SQUserPointer*)&v2, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector, Vector)");
		}

		sq_getclass(vm, 1);
		sq_createinstance(vm, -1);
		SQUserPointer p;
		sq_getinstanceup(vm, -1, &p, 0);
		new(p) Vector((*v1) + (*v2));
		sq_remove(vm, -2);

		return 1;
	}

	SQInteger Sub(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;
		Vector* v2 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)) ||
			SQ_FAILED(sq_getinstanceup(vm, 2, (SQUserPointer*)&v2, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector, Vector)");
		}

		sq_getclass(vm, 1);
		sq_createinstance(vm, -1);
		SQUserPointer p;
		sq_getinstanceup(vm, -1, &p, 0);
		new(p) Vector((*v1) - (*v2));
		sq_remove(vm, -2);

		return 1;
	}

	SQInteger Multiply(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector, float) or (Vector, Vector)");
		}

		SQObjectType paramType = sq_gettype(vm, 2);

		float s = 0.0;
		Vector* v2 = nullptr;

		if ((paramType & SQOBJECT_NUMERIC) &&
			SQ_SUCCEEDED(sq_getfloat(vm, 2, &s)))
		{
			sq_getclass(vm, 1);
			sq_createinstance(vm, -1);
			SQUserPointer p;
			sq_getinstanceup(vm, -1, &p, 0);
			new(p) Vector((*v1) * s);
			sq_remove(vm, -2);

			return 1;
		}
		else if (paramType == OT_INSTANCE &&
			SQ_SUCCEEDED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v2, TYPETAG_VECTOR)))
		{
			sq_getclass(vm, 1);
			sq_createinstance(vm, -1);
			SQUserPointer p;
			sq_getinstanceup(vm, -1, &p, 0);
			new(p) Vector((*v1) * (*v2));
			sq_remove(vm, -2);

			return 1;
		}
		else
		{
			return sq_throwerror(vm, "Expected (Vector, float) or (Vector, Vector)");
		}
	}

	SQInteger Divide(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector, float) or (Vector, Vector)");
		}

		SQObjectType paramType = sq_gettype(vm, 2);

		float s = 0.0;
		Vector* v2 = nullptr;

		if ((paramType & SQOBJECT_NUMERIC) &&
			SQ_SUCCEEDED(sq_getfloat(vm, 2, &s)))
		{
			sq_getclass(vm, 1);
			sq_createinstance(vm, -1);
			SQUserPointer p;
			sq_getinstanceup(vm, -1, &p, 0);
			new(p) Vector((*v1) / s);
			sq_remove(vm, -2);

			return 1;
		}
		else if (paramType == OT_INSTANCE &&
			SQ_SUCCEEDED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v2, TYPETAG_VECTOR)))
		{
			sq_getclass(vm, 1);
			sq_createinstance(vm, -1);
			SQUserPointer p;
			sq_getinstanceup(vm, -1, &p, 0);
			new(p) Vector((*v1) / (*v2));
			sq_remove(vm, -2);

			return 1;
		}
		else
		{
			return sq_throwerror(vm, "Expected (Vector, float) or (Vector, Vector)");
		}
	}

	SQInteger Length(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		sq_pushfloat(vm, v1->Length());
		return 1;
	}

	SQInteger LengthSqr(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		sq_pushfloat(vm, v1->LengthSqr());
		return 1;
	}

	SQInteger Length2D(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		sq_pushfloat(vm, v1->Length2D());
		return 1;
	}

	SQInteger Length2DSqr(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		sq_pushfloat(vm, v1->Length2DSqr());
		return 1;
	}

	SQInteger Normalized(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		sq_getclass(vm, 1);
		sq_createinstance(vm, -1);
		SQUserPointer p;
		sq_getinstanceup(vm, -1, &p, 0);
		new(p) Vector((*v1).Normalized());
		sq_remove(vm, -2);

		return 1;
	}

	SQInteger Norm(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		float len = v1->NormalizeInPlace();
		sq_pushfloat(vm, len);

		return 1;
	}

	SQInteger Scale(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector, float)");
		}

		float s = 0.0;

		if (SQ_SUCCEEDED(sq_getfloat(vm, 2, &s)))
		{
			sq_getclass(vm, 1);
			sq_createinstance(vm, -1);
			SQUserPointer p;
			sq_getinstanceup(vm, -1, &p, 0);
			new(p) Vector((*v1) * s);
			sq_remove(vm, -2);

			return 1;
		}
		else
		{
			return sq_throwerror(vm, "Expected (Vector, float)");
		}
	}

	SQInteger Dot(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;
		Vector* v2 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)) ||
			SQ_FAILED(sq_getinstanceup(vm, 2, (SQUserPointer*)&v2, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector, Vector)");
		}

		sq_pushfloat(vm, v1->Dot(*v2));
		return 1;
	}

	SQInteger ToKVString(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		sqstd_pushstringf(vm, "%f %f %f", v1->x, v1->y, v1->z);
		return 1;
	}

	SQInteger Cross(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;
		Vector* v2 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)) ||
			SQ_FAILED(sq_getinstanceup(vm, 2, (SQUserPointer*)&v2, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector, Vector)");
		}

		sq_getclass(vm, 1);
		sq_createinstance(vm, -1);
		SQUserPointer p;
		sq_getinstanceup(vm, -1, &p, 0);
		new(p) Vector((*v1).Cross(*v2));
		sq_remove(vm, -2);

		return 1;
	}

	SQInteger ToString(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 1 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		sqstd_pushstringf(vm, "(vector: (%f, %f, %f))", v1->x, v1->y, v1->z);
		return 1;
	}

	SQInteger TypeOf(HSQUIRRELVM vm)
	{
		sq_pushstring(vm, "Vector", -1);
		return 1;
	}

	SQInteger Nexti(HSQUIRRELVM vm)
	{
		Vector* v1 = nullptr;

		if (sq_gettop(vm) != 2 ||
			SQ_FAILED(sq_getinstanceup(vm, 1, (SQUserPointer*)&v1, TYPETAG_VECTOR)))
		{
			return sq_throwerror(vm, "Expected (Vector)");
		}

		HSQOBJECT obj;
		sq_resetobject(&obj);
		sq_getstackobj(vm, 2, &obj);

		const char* curkey = nullptr;

		if (sq_isnull(obj))
		{
			curkey = "w";
		}
		else if (sq_isstring(obj))
		{
			curkey = sq_objtostring(&obj);
		}
		else
		{
			return sq_throwerror(vm, "Invalid iterator");
		}

		Assert(curkey && curkey[0] >= 'w' && curkey[0] <= 'z');

		if (curkey[0] == 'z')
		{
			// Reached the end
			sq_pushnull(vm);
			return 1;
		}

		char newkey = curkey[0] + 1;
		sq_pushstring(vm, &newkey, 1);

		return 1;
	}

	static const SQRegFunction funcs[] = {
		{_SC("constructor"), Construct,0,nullptr},
		{_SC("_get"), Get, 2, _SC(".s")},
		{_SC("_set"), Set, 3, _SC(".sn")},
		{_SC("_add"), Add, 2, _SC("..")},
		{_SC("_sub"), Sub, 2, _SC("..")},
		{_SC("_mul"), Multiply, 2, _SC("..")},
		{_SC("_div"), Divide, 2, _SC("..")},
		{_SC("Length"), Length, 1, _SC(".")},
		{_SC("LengthSqr"), LengthSqr, 1, _SC(".")},
		{_SC("Length2D"), Length2D, 1, _SC(".")},
		{_SC("Length2DSqr"), Length2DSqr, 1, _SC(".")},
		{_SC("Normalized"), Normalized, 1, _SC(".")},
		{_SC("Norm"), Norm, 1, _SC(".")},
		{_SC("Scale"), Scale, 2, _SC("..")},
		{_SC("Dot"), Dot, 2, _SC("..")},
		{_SC("Cross"), Cross, 2, _SC("..")},
		{_SC("ToKVString"), ToKVString, 1, _SC(".")},
		{_SC("_tostring"), ToString, 1, _SC(".")},
		{_SC("_typeof"), TypeOf, 1, _SC(".")},
		{_SC("_nexti"), Nexti, 2, _SC("..")},

		{nullptr,(SQFUNCTION)0,0,nullptr}
	};

	HSQOBJECT register_class(HSQUIRRELVM v)
	{
		sq_pushstring(v, _SC("Vector"), -1);
		sq_newclass(v, SQFalse);
		sq_settypetag(v, -1, TYPETAG_VECTOR);
		sq_setclassudsize(v, -1, sizeof(Vector));
		SQInteger i = 0;
		while (funcs[i].name != 0) {
			const SQRegFunction& f = funcs[i];
			sq_pushstring(v, f.name, -1);
			sq_newclosure(v, f.f, 0);
			sq_setparamscheck(v, f.nparamscheck, f.typemask);
			sq_setnativeclosurename(v, -1, f.name);
			sq_newslot(v, -3, SQFalse);
			i++;
		}
		HSQOBJECT klass;
		sq_resetobject(&klass);
		sq_getstackobj(v, -1, &klass);
		sq_addref(v, &klass);
		sq_newslot(v, -3, SQFalse);
		return klass;
	}
} // SQVector

struct ClassInstanceData
{
	ClassInstanceData(void* instance, ScriptClassDesc_t* desc, const char* instanceId = nullptr, bool allowDestruct = false) :
		instance(instance),
		desc(desc),
		instanceId(instanceId),
		allowDestruct(allowDestruct)
	{}

	void* instance;
	ScriptClassDesc_t* desc;
	CUtlString instanceId;

	// Indicates this game-created instance is a weak reference and can be destructed (Blixibon)
	bool allowDestruct;
};

bool CreateParamCheck(const ScriptFunctionBinding_t& func, char* output)
{
	*output++ = '.';
	for (int i = 0; i < func.m_desc.m_Parameters.Count(); ++i)
	{
		switch (func.m_desc.m_Parameters[i])
		{
		case FIELD_FLOAT:
			*output++ = 'n'; // NOTE: Can be int or float
			break;
		case FIELD_CSTRING:
			*output++ = 's';
			break;
		case FIELD_VECTOR:
			*output++ = 'x'; // Generic instance, we validate on arrival
			break;
		case FIELD_INTEGER:
			*output++ = 'i'; // could use 'n' also which is int or float
			break;
		case FIELD_BOOLEAN:
			*output++ = 'b';
			break;
		case FIELD_CHARACTER:
			*output++ = 's';
			break;
		case FIELD_HSCRIPT:
			*output++ = '.';
			break;
		default:
			Assert(!"Unsupported type");
			return false;
		};
	}
	*output++ = 0;
	return true;
}

void PushVariant(HSQUIRRELVM vm, const ScriptVariant_t& value)
{
	switch (value.m_type)
	{
	case FIELD_VOID:
		sq_pushnull(vm);
		break;
	case FIELD_FLOAT:
		sq_pushfloat(vm, value);
		break;
	case FIELD_CSTRING:
		if ( value.m_pszString )
			sq_pushstring(vm, value, -1);
		else
			sq_pushnull(vm);
		break;
	case FIELD_VECTOR:
	{
		SquirrelVM* pSquirrelVM = (SquirrelVM*)sq_getforeignptr(vm);
		assert(pSquirrelVM);
		sq_pushobject(vm, pSquirrelVM->vectorClass_);
		sq_createinstance(vm, -1);
		SQUserPointer p;
		sq_getinstanceup(vm, -1, &p, 0);
		new(p) Vector(value);
		sq_remove(vm, -2);
		break;
	}
	case FIELD_INTEGER:
		sq_pushinteger(vm, value.m_int);
		break;
	case FIELD_BOOLEAN:
		sq_pushbool(vm, value.m_bool);
		break;
	case FIELD_CHARACTER:
	{
		char buf[2] = { value.m_char, 0 };
		sq_pushstring(vm, buf, 1);
		break;
	}
	case FIELD_HSCRIPT:
		if (value.m_hScript)
		{
			sq_pushobject(vm, *((HSQOBJECT*)value.m_hScript));
		}
		else
		{
			sq_pushnull(vm);
		}
		break;
	}
}

void GetVariantScriptString(const ScriptVariant_t& value, char *szValue, int iSize)
{
	switch (value.m_type)
	{
		case FIELD_VOID:
			V_strncpy( szValue, "null", iSize );
			break;
		case FIELD_FLOAT:
			V_snprintf( szValue, iSize, "%f", value.m_float );
			break;
		case FIELD_CSTRING:
			V_snprintf( szValue, iSize, "\"%s\"", value.m_pszString );
			break;
		case FIELD_VECTOR:
			V_snprintf( szValue, iSize, "Vector( %f, %f, %f )", value.m_pVector->x, value.m_pVector->y, value.m_pVector->z );
			break;
		case FIELD_INTEGER:
			V_snprintf( szValue, iSize, "%i", value.m_int );
			break;
		case FIELD_BOOLEAN:
			V_snprintf( szValue, iSize, "%d", value.m_bool );
			break;
		case FIELD_CHARACTER:
			//char buf[2] = { value.m_char, 0 };
			V_snprintf( szValue, iSize, "\"%c\"", value.m_char );
			break;
	}
}

bool getVariant(HSQUIRRELVM vm, SQInteger idx, ScriptVariant_t& variant)
{
	switch (sq_gettype(vm, idx))
	{
	case OT_NULL:
	{
		// TODO: Should this be (HSCRIPT)nullptr
		variant.m_type = FIELD_VOID;
		return true;
	}
	case OT_INTEGER:
	{
		SQInteger val;
		if (SQ_FAILED(sq_getinteger(vm, idx, &val)))
		{
			return false;
		}
		variant = (int)val;
		return true;
	}
	case OT_FLOAT:
	{
		SQFloat val;
		if (SQ_FAILED(sq_getfloat(vm, idx, &val)))
		{
			return false;
		}
		variant = (float)val;
		return true;
	}
	case OT_BOOL:
	{
		SQBool val;
		if (SQ_FAILED(sq_getbool(vm, idx, &val)))
		{
			return false;
		}
		variant = val ? true : false;
		return true;
	}
	case OT_STRING:
	{
		const char* val;
		SQInteger size = 0;
		if (SQ_FAILED(sq_getstringandsize(vm, idx, &val, &size)))
		{
			return false;
		}
		char* buffer = new char[size + 1];
		V_memcpy(buffer, val, size);
		buffer[size] = 0;
		variant = buffer;
		variant.m_flags |= SV_FREE;
		return true;
	}
	case OT_INSTANCE:
	{
		Vector* v = nullptr;
		SQUserPointer tag;
		if (SQ_SUCCEEDED(sq_gettypetag(vm, idx, &tag)) &&
			tag == TYPETAG_VECTOR &&
			SQ_SUCCEEDED(sq_getinstanceup(vm, idx, (SQUserPointer*)&v, TYPETAG_VECTOR)))
		{
			variant = new Vector(*v);
			variant.m_flags |= SV_FREE;
			return true;
		}
		// fall-through for non-vector
	}
	default:
	{
		HSQOBJECT* obj = new HSQOBJECT;
		sq_resetobject(obj);
		sq_getstackobj(vm, idx, obj);
		sq_addref(vm, obj);
		variant = (HSCRIPT)obj;
	}
	};


	return true;
}

SQInteger function_stub(HSQUIRRELVM vm)
{
	SQInteger top = sq_gettop(vm);

	SQUserPointer userptr = nullptr;
	sq_getuserpointer(vm, top, &userptr);

	Assert(userptr);

	ScriptFunctionBinding_t* pFunc = (ScriptFunctionBinding_t*)userptr;

	auto nargs = pFunc->m_desc.m_Parameters.Count();

	if (nargs > top)
	{
		// TODO: Handle optional parameters?
		return sq_throwerror(vm, "Invalid number of parameters");
	}

	CUtlVector<ScriptVariant_t> params;
	params.SetCount(nargs);

	for (int i = 0; i < nargs; ++i)
	{
		switch (pFunc->m_desc.m_Parameters[i])
		{
		case FIELD_FLOAT:
		{
			float val = 0.0;
			if (SQ_FAILED(sq_getfloat(vm, i + 2, &val)))
				return sq_throwerror(vm, "Expected float");
			params[i] = val;
			break;
		}
		case FIELD_CSTRING:
		{
			const char* val;
			if (SQ_FAILED(sq_getstring(vm, i + 2, &val)))
				return sq_throwerror(vm, "Expected string");
			params[i] = val;
			break;
		}
		case FIELD_VECTOR:
		{
			Vector* val;
			if (SQ_FAILED(sq_getinstanceup(vm, i + 2, (SQUserPointer*)&val, TYPETAG_VECTOR)))
				return sq_throwerror(vm, "Expected Vector");
			params[i] = *val;
			break;
		}
		case FIELD_INTEGER:
		{
			SQInteger val = 0;
			if (SQ_FAILED(sq_getinteger(vm, i + 2, &val)))
				return sq_throwerror(vm, "Expected integer");
			params[i] = (int)val;
			break;
		}
		case FIELD_BOOLEAN:
		{
			SQBool val = 0;
			if (SQ_FAILED(sq_getbool(vm, i + 2, &val)))
				return sq_throwerror(vm, "Expected bool");
			params[i] = val ? true : false;
			break;
		}
		case FIELD_CHARACTER:
		{
			const char* val;
			if (SQ_FAILED(sq_getstring(vm, i + 2, &val)))
				return sq_throwerror(vm, "Expected string");
			params[i] = val[i];
			break;
		}
		case FIELD_HSCRIPT:
		{
			HSQOBJECT val;
			if (SQ_FAILED(sq_getstackobj(vm, i + 2, &val)))
				return sq_throwerror(vm, "Expected handle");

			if (sq_isnull(val))
			{
				params[i] = (HSCRIPT)nullptr;
			}
			else
			{
				HSQOBJECT* pObject = new HSQOBJECT;
				*pObject = val;
				sq_addref(vm, pObject);
				params[i] = (HSCRIPT)pObject;
			}
			break;
		}
		default:
			Assert(!"Unsupported type");
			return false;
		}
	}

	void* instance = nullptr;

	if (pFunc->m_flags & SF_MEMBER_FUNC)
	{
		SQUserPointer self;
		sq_getinstanceup(vm, 1, &self, nullptr);

		if (!self)
		{
			return sq_throwerror(vm, "Accessed null instance");
		}

		instance = ((ClassInstanceData*)self)->instance;
	}

	ScriptVariant_t retval;

	SquirrelVM* pSquirrelVM = (SquirrelVM*)sq_getforeignptr(vm);
	assert(pSquirrelVM);

	sq_resetobject(&pSquirrelVM->lastError_);

	(*pFunc->m_pfnBinding)(pFunc->m_pFunction, instance, params.Base(), nargs,
		pFunc->m_desc.m_ReturnType == FIELD_VOID ? nullptr : &retval);

	if (!sq_isnull(pSquirrelVM->lastError_))
	{
		sq_pushobject(vm, pSquirrelVM->lastError_);
		sq_resetobject(&pSquirrelVM->lastError_);
		return sq_throwobject(vm);
	}

	PushVariant(vm, retval);

	return pFunc->m_desc.m_ReturnType != FIELD_VOID;
}


SQInteger destructor_stub(SQUserPointer p, SQInteger size)
{
	auto classInstanceData = (ClassInstanceData*)p;

	if (classInstanceData->desc->m_pfnDestruct)
		classInstanceData->desc->m_pfnDestruct(classInstanceData->instance);

	classInstanceData->~ClassInstanceData();
	return 0;
}

SQInteger destructor_stub_instance(SQUserPointer p, SQInteger size)
{
	auto classInstanceData = (ClassInstanceData*)p;
	// We don't call destructor here because this is owned by the game
	classInstanceData->~ClassInstanceData();
	return 0;
}

SQInteger constructor_stub(HSQUIRRELVM vm)
{
	ScriptClassDesc_t* pClassDesc = nullptr;
	sq_gettypetag(vm, 1, (SQUserPointer*)&pClassDesc);

	if (!pClassDesc->m_pfnConstruct)
	{
		return sqstd_throwerrorf(vm, "Unable to construct instances of %s", pClassDesc->m_pszScriptName);
	}

	SquirrelVM* pSquirrelVM = (SquirrelVM*)sq_getforeignptr(vm);
	assert(pSquirrelVM);

	sq_resetobject(&pSquirrelVM->lastError_);

	void* instance = pClassDesc->m_pfnConstruct();

	if (!sq_isnull(pSquirrelVM->lastError_))
	{
		sq_pushobject(vm, pSquirrelVM->lastError_);
		sq_resetobject(&pSquirrelVM->lastError_);
		return sq_throwobject(vm);
	}

	{
		SQUserPointer p;
		sq_getinstanceup(vm, 1, &p, 0);
		new(p) ClassInstanceData(instance, pClassDesc, nullptr, true);
	}

	sq_setreleasehook(vm, 1, &destructor_stub);

	return 0;
}

SQInteger tostring_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);

	char buffer[128] = "";

	if (classInstanceData &&
		classInstanceData->instance &&
		classInstanceData->desc->pHelper &&
		classInstanceData->desc->pHelper->ToString(classInstanceData->instance, buffer, sizeof(buffer)))
	{
		sq_pushstring(vm, buffer, -1);
	}
	else if (classInstanceData)
	{
		sqstd_pushstringf(vm, "(%s : 0x%p)", classInstanceData->desc->m_pszScriptName, classInstanceData->instance);
	}
	else
	{
		HSQOBJECT obj;
		sq_resetobject(&obj);
		sq_getstackobj(vm, 1, &obj);
		// Semi-based on SQVM::ToString default case
		sqstd_pushstringf(vm, "(%s: 0x%p)", IdType2Name(obj._type), (void*)_rawval(obj));
	}

	return 1;
}

SQInteger get_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);

	const char* key = nullptr;
	sq_getstring(vm, 2, &key);

	if (key == nullptr)
	{
		return sq_throwerror(vm, "Expected _get(string)");
	}

	ScriptVariant_t var;
	if (classInstanceData &&
		classInstanceData->instance &&
		classInstanceData->desc->pHelper &&
		classInstanceData->desc->pHelper->Get(classInstanceData->instance, key, var))
	{
		PushVariant(vm, var);
	}
	else
	{
		return sqstd_throwerrorf(vm, "the index '%.50s' does not exist", key);
	}

	return 1;
}

SQInteger set_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);

	const char* key = nullptr;
	sq_getstring(vm, 2, &key);

	if (key == nullptr)
	{
		return sq_throwerror(vm, "Expected _set(string)");
	}

	ScriptVariant_t var;
	getVariant( vm, -1, var );

	if (classInstanceData &&
		classInstanceData->instance &&
		classInstanceData->desc->pHelper &&
		classInstanceData->desc->pHelper->Set(classInstanceData->instance, key, var))
	{
		sq_pop(vm, 1);
	}
	else
	{
		sq_pop(vm, 1);
		return sqstd_throwerrorf(vm, "the index '%.50s' does not exist", key);
	}

	return 0;
}

SQInteger add_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);

	ScriptVariant_t var;
	getVariant( vm, 1, var );

	if (classInstanceData &&
		classInstanceData->instance &&
		classInstanceData->desc->pHelper)
	{
		ScriptVariant_t *result = classInstanceData->desc->pHelper->Add( classInstanceData->instance, var );
		if (result != nullptr)
		{
			PushVariant( vm, *result );
			sq_pop(vm, 1);
			return 1;
		}
	}

	sq_pop(vm, 1);
	return sqstd_throwerrorf(vm, "invalid arith op +");
}

SQInteger sub_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);

	ScriptVariant_t var;
	getVariant( vm, 1, var );

	if (classInstanceData &&
		classInstanceData->instance &&
		classInstanceData->desc->pHelper)
	{
		ScriptVariant_t *result = classInstanceData->desc->pHelper->Subtract( classInstanceData->instance, var );
		if (result != nullptr)
		{
			PushVariant( vm, *result );
			sq_pop(vm, 1);
			return 1;
		}
	}

	sq_pop(vm, 1);
	return sqstd_throwerrorf(vm, "invalid arith op -");
}

SQInteger mul_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);

	ScriptVariant_t var;
	getVariant( vm, 1, var );

	if (classInstanceData &&
		classInstanceData->instance &&
		classInstanceData->desc->pHelper )
	{
		ScriptVariant_t *result = classInstanceData->desc->pHelper->Add( classInstanceData->instance, var );
		if (result != nullptr)
		{
			PushVariant( vm, *result );
			sq_pop(vm, 1);
			return 1;
		}
	}

	sq_pop(vm, 1);
	return sqstd_throwerrorf(vm, "invalid arith op *");
}

SQInteger div_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);

	ScriptVariant_t var;
	getVariant( vm, 1, var );

	if (classInstanceData &&
		classInstanceData->instance &&
		classInstanceData->desc->pHelper )
	{
		ScriptVariant_t *result = classInstanceData->desc->pHelper->Add( classInstanceData->instance, var );
		if (result != nullptr)
		{
			PushVariant( vm, *result );
			sq_pop(vm, 1);
			return 1;
		}
	}

	sq_pop(vm, 1);
	return sqstd_throwerrorf(vm, "invalid arith op /");
}

SQInteger IsValid_stub(HSQUIRRELVM vm)
{
	ClassInstanceData* classInstanceData = nullptr;
	sq_getinstanceup(vm, 1, (SQUserPointer*)&classInstanceData, 0);
	sq_pushbool(vm, classInstanceData != nullptr);
	return 1;
}

struct SquirrelSafeCheck
{
	SquirrelSafeCheck(HSQUIRRELVM vm, int outputCount = 0) :
		vm_{ vm },
		top_{ sq_gettop(vm) },
		outputCount_{ outputCount }
	{}

	~SquirrelSafeCheck()
	{
		if (top_ != (sq_gettop(vm_) - outputCount_))
		{
			Assert(!"Squirrel VM stack is not consistent");
			Error("Squirrel VM stack is not consistent\n");
		}

		// TODO: Handle error state checks
	}

	HSQUIRRELVM vm_;
	SQInteger top_;
	SQInteger outputCount_;
};


void printfunc(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar* format, ...)
{
	va_list args;
	char buffer[2048];
	va_start(args, format);
	V_vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	CGMsg(0, CON_GROUP_VSCRIPT_PRINT, "%s", buffer);
}

void errorfunc(HSQUIRRELVM SQ_UNUSED_ARG(v), const SQChar* format, ...)
{
	va_list args;
	char buffer[2048];
	va_start(args, format);
	V_vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	Warning("%s", buffer);
}

const char * ScriptDataTypeToName(ScriptDataType_t datatype)
{
	switch (datatype)
	{
	case FIELD_VOID:		return "void";
	case FIELD_FLOAT:		return "float";
	case FIELD_CSTRING:		return "string";
	case FIELD_VECTOR:		return "Vector";
	case FIELD_INTEGER:		return "int";
	case FIELD_BOOLEAN:		return "bool";
	case FIELD_CHARACTER:	return "char";
	case FIELD_HSCRIPT:		return "handle";
	case FIELD_VARIANT:		return "variant";
	default:				return "<unknown>";
	}
}

void RegisterDocumentation(HSQUIRRELVM vm, const ScriptFuncDescriptor_t& pFuncDesc, ScriptClassDesc_t* pClassDesc = nullptr)
{
	SquirrelSafeCheck safeCheck(vm);

	if (pFuncDesc.m_pszDescription && pFuncDesc.m_pszDescription[0] == SCRIPT_HIDE[0])
		return;

	char name[256] = "";

	if (pClassDesc)
	{
		V_strcat_safe(name, pClassDesc->m_pszScriptName);
		V_strcat_safe(name, "::");
	}

	V_strcat_safe(name, pFuncDesc.m_pszScriptName);


	char signature[256] = "";
	V_snprintf(signature, sizeof(signature), "%s %s(", ScriptDataTypeToName(pFuncDesc.m_ReturnType), name);

	for (int i = 0; i < pFuncDesc.m_Parameters.Count(); ++i)
	{
		if (i != 0)
			V_strcat_safe(signature, ", ");

		V_strcat_safe(signature, ScriptDataTypeToName(pFuncDesc.m_Parameters[i]));
	}

	V_strcat_safe(signature, ")");

	// RegisterHelp(name, signature, description)
	sq_pushroottable(vm);
	sq_pushstring(vm, "RegisterHelp", -1);
	sq_get(vm, -2);
	sq_remove(vm, -2);
	sq_pushroottable(vm);
	sq_pushstring(vm, name, -1);
	sq_pushstring(vm, signature, -1);
	sq_pushstring(vm, pFuncDesc.m_pszDescription ? pFuncDesc.m_pszDescription : "", -1);
	sq_call(vm, 4, SQFalse, SQFalse);
	sq_pop(vm, 1);
}

void RegisterClassDocumentation(HSQUIRRELVM vm, const ScriptClassDesc_t* pClassDesc)
{
	SquirrelSafeCheck safeCheck(vm);

	const char *name = pClassDesc->m_pszScriptName;
	const char *base = "";
	if (pClassDesc->m_pBaseDesc)
	{
		base = pClassDesc->m_pBaseDesc->m_pszScriptName;
	}

	const char *description = pClassDesc->m_pszDescription;
	if (description)
	{
		if (description[0] == SCRIPT_HIDE[0])
			return;
		if (description[0] == SCRIPT_SINGLETON[0])
			description++;
	}
	else
	{
		description = "";
	}

	// RegisterClassHelp(name, base, description)
	sq_pushroottable(vm);
	sq_pushstring(vm, "RegisterClassHelp", -1);
	sq_get(vm, -2);
	sq_remove(vm, -2);
	sq_pushroottable(vm);
	sq_pushstring(vm, name, -1);
	sq_pushstring(vm, base, -1);
	sq_pushstring(vm, description, -1);
	sq_call(vm, 4, SQFalse, SQFalse);
	sq_pop(vm, 1);
}

void RegisterEnumDocumentation(HSQUIRRELVM vm, const ScriptEnumDesc_t* pClassDesc)
{
	SquirrelSafeCheck safeCheck(vm);

	if (pClassDesc->m_pszDescription && pClassDesc->m_pszDescription[0] == SCRIPT_HIDE[0])
		return;

	const char *name = pClassDesc->m_pszScriptName;

	// RegisterEnumHelp(name, description)
	sq_pushroottable(vm);
	sq_pushstring(vm, "RegisterEnumHelp", -1);
	sq_get(vm, -2);
	sq_remove(vm, -2);
	sq_pushroottable(vm);
	sq_pushstring(vm, name, -1);
	sq_pushinteger(vm, pClassDesc->m_ConstantBindings.Count());
	sq_pushstring(vm, pClassDesc->m_pszDescription ? pClassDesc->m_pszDescription : "", -1);
	sq_call(vm, 4, SQFalse, SQFalse);
	sq_pop(vm, 1);
}

void RegisterConstantDocumentation( HSQUIRRELVM vm, const ScriptConstantBinding_t* pConstDesc, const char *pszAsString, ScriptEnumDesc_t* pEnumDesc = nullptr )
{
	SquirrelSafeCheck safeCheck(vm);

	if (pConstDesc->m_pszDescription && pConstDesc->m_pszDescription[0] == SCRIPT_HIDE[0])
		return;

	char name[256] = "";

	if (pEnumDesc)
	{
		V_strcat_safe(name, pEnumDesc->m_pszScriptName);
		V_strcat_safe(name, ".");
	}

	V_strcat_safe(name, pConstDesc->m_pszScriptName);

	char signature[256] = "";
	V_snprintf(signature, sizeof(signature), "%s (%s)", pszAsString, ScriptDataTypeToName(pConstDesc->m_data.m_type));

	// RegisterConstHelp(name, signature, description)
	sq_pushroottable(vm);
	sq_pushstring(vm, "RegisterConstHelp", -1);
	sq_get(vm, -2);
	sq_remove(vm, -2);
	sq_pushroottable(vm);
	sq_pushstring(vm, name, -1);
	sq_pushstring(vm, signature, -1);
	sq_pushstring(vm, pConstDesc->m_pszDescription ? pConstDesc->m_pszDescription : "", -1);
	sq_call(vm, 4, SQFalse, SQFalse);
	sq_pop(vm, 1);
}

void RegisterHookDocumentation(HSQUIRRELVM vm, const ScriptHook_t* pHook, const ScriptFuncDescriptor_t& pFuncDesc, ScriptClassDesc_t* pClassDesc = nullptr)
{
	SquirrelSafeCheck safeCheck(vm);

	if (pFuncDesc.m_pszDescription && pFuncDesc.m_pszDescription[0] == SCRIPT_HIDE[0])
		return;

	char name[256] = "";

	if (pClassDesc)
	{
		V_strcat_safe(name, pClassDesc->m_pszScriptName);
		V_strcat_safe(name, " -> ");
	}

	V_strcat_safe(name, pFuncDesc.m_pszScriptName);


	char signature[256] = "";
	V_snprintf(signature, sizeof(signature), "%s %s(", ScriptDataTypeToName(pFuncDesc.m_ReturnType), name);

	for (int i = 0; i < pFuncDesc.m_Parameters.Count(); ++i)
	{
		if (i != 0)
			V_strcat_safe(signature, ", ");

		V_strcat_safe(signature, ScriptDataTypeToName(pFuncDesc.m_Parameters[i]));
		V_strcat_safe(signature, " [");
		V_strcat_safe(signature, pHook->m_pszParameterNames[i]);
		V_strcat_safe(signature, "]");
	}

	V_strcat_safe(signature, ")");

	// RegisterHookHelp(name, signature, description)
	sq_pushroottable(vm);
	sq_pushstring(vm, "RegisterHookHelp", -1);
	sq_get(vm, -2);
	sq_remove(vm, -2);
	sq_pushroottable(vm);
	sq_pushstring(vm, name, -1);
	sq_pushstring(vm, signature, -1);
	sq_pushstring(vm, pFuncDesc.m_pszDescription ? pFuncDesc.m_pszDescription : "", -1);
	sq_call(vm, 4, SQFalse, SQFalse);
	sq_pop(vm, 1);
}

void RegisterMemberDocumentation(HSQUIRRELVM vm, const ScriptMemberDesc_t& pDesc, ScriptClassDesc_t* pClassDesc = nullptr)
{
	SquirrelSafeCheck safeCheck(vm);

	if (pDesc.m_pszDescription && pDesc.m_pszDescription[0] == SCRIPT_HIDE[0])
		return;

	char name[256] = "";

	if (pClassDesc)
	{
		V_strcat_safe(name, pClassDesc->m_pszScriptName);
		V_strcat_safe(name, ".");
	}

	if (pDesc.m_pszScriptName)
		V_strcat_safe(name, pDesc.m_pszScriptName);


	char signature[256] = "";
	V_snprintf(signature, sizeof(signature), "%s %s", ScriptDataTypeToName(pDesc.m_ReturnType), name);

	// RegisterHookHelp(name, signature, description)
	sq_pushroottable(vm);
	sq_pushstring(vm, "RegisterMemberHelp", -1);
	sq_get(vm, -2);
	sq_remove(vm, -2);
	sq_pushroottable(vm);
	sq_pushstring(vm, name, -1);
	sq_pushstring(vm, signature, -1);
	sq_pushstring(vm, pDesc.m_pszDescription ? pDesc.m_pszDescription : "", -1);
	sq_call(vm, 4, SQFalse, SQFalse);
	sq_pop(vm, 1);
}


bool SquirrelVM::Init()
{
	vm_ = sq_open(1024); //creates a VM with initial stack size 1024

	if (vm_ == nullptr)
		return false;

	sq_setforeignptr(vm_, this);
	sq_resetobject(&lastError_);

	sq_setprintfunc(vm_, printfunc, errorfunc);


	{
		sq_pushroottable(vm_);

		sqstd_register_mathlib(vm_);
		sqstd_register_stringlib(vm_);
		vectorClass_ = SQVector::register_class(vm_);

		// We exclude these libraries as they create a security risk on the client even
		// though I'm sure if someone tried hard enough they could achieve all sorts of
		// things with the other APIs, this just makes it a little bit harder for a map
		// created by someone in the community causing a bunch of security vulnerablilties.
		//
		// Pretty sure DoIncludeScript() is already a vulnerability vector here, however
		// that also depends on compile errors not showing up and relies on IFilesystem with
		// a path prefix.
		//
		//sqstd_register_bloblib(vm_);
		//sqstd_register_iolib(vm_);
		//sqstd_register_systemlib(vm_);

		// There is no vulnerability in getting time.
		sqstd_register_timelib(vm_);


		sqstd_seterrorhandlers(vm_);

		{
			// Unfortunately we can not get the pattern from a regexp instance
			// so we need to wrap it with our own to get it.
			if (Run(R"script(
				class regexp extends regexp  
				{
					constructor(pattern) 
					{
						base.constructor(pattern);
						this.pattern_ = pattern;
					}
					pattern_ = "";
				}
			)script") == SCRIPT_ERROR)
			{
				this->Shutdown();
				return false;
			}

			sq_resetobject(&regexpClass_);
			sq_pushstring(vm_, "regexp", -1);
			sq_rawget(vm_, -2);
			sq_getstackobj(vm_, -1, &regexpClass_);
			sq_addref(vm_, &regexpClass_);
			sq_pop(vm_, 1);
		}

		sq_pop(vm_, 1);
	}

	if (Run(g_Script_vscript_squirrel) != SCRIPT_DONE)
	{
		this->Shutdown();
		return false;
	}

	return true;
}

void SquirrelVM::Shutdown()
{
	if (vm_)
	{
		sq_release(vm_, &vectorClass_);
		sq_release(vm_, &regexpClass_);

		sq_close(vm_);
		vm_ = nullptr;
	}
}

bool SquirrelVM::ConnectDebugger()
{
	// TODO: Debugger support
	return false;
}

void SquirrelVM::DisconnectDebugger()
{
	// TODO: Debugger support
}

ScriptLanguage_t SquirrelVM::GetLanguage()
{
	return SL_SQUIRREL;
}

const char* SquirrelVM::GetLanguageName()
{
	return "squirrel";
}

void SquirrelVM::AddSearchPath(const char* pszSearchPath)
{
	// TODO: Search path support
}

bool SquirrelVM::Frame(float simTime)
{
	// TODO: Frame support
	return false;
}

ScriptStatus_t SquirrelVM::Run(const char* pszScript, bool bWait)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (SQ_FAILED(sq_compilebuffer(vm_, pszScript, strlen(pszScript), "<run>", SQTrue)))
	{
		return SCRIPT_ERROR;
	}

	sq_pushroottable(vm_);
	if (SQ_FAILED(sq_call(vm_, 1, SQFalse, SQTrue)))
	{
		sq_pop(vm_, 1);
		return SCRIPT_ERROR;
	}

	sq_pop(vm_, 1);
	return SCRIPT_DONE;
}

HSCRIPT SquirrelVM::CompileScript(const char* pszScript, const char* pszId)
{
	SquirrelSafeCheck safeCheck(vm_);

	Assert(vm_);
	if (pszId == nullptr) pszId = "<unnamed>";
	if (SQ_FAILED(sq_compilebuffer(vm_, pszScript, strlen(pszScript), pszId, SQTrue)))
	{
		return nullptr;
	}
	HSQOBJECT* obj = new HSQOBJECT;
	sq_resetobject(obj);
	sq_getstackobj(vm_, -1, obj);
	sq_addref(vm_, obj);
	sq_pop(vm_, 1);

	return (HSCRIPT)obj;
}

void SquirrelVM::ReleaseScript(HSCRIPT hScript)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (!hScript) return;
	HSQOBJECT* obj = (HSQOBJECT*)hScript;
	sq_release(vm_, obj);
	delete obj;
}

ScriptStatus_t SquirrelVM::Run(HSCRIPT hScript, HSCRIPT hScope, bool bWait)
{
	SquirrelSafeCheck safeCheck(vm_);
	HSQOBJECT* obj = (HSQOBJECT*)hScript;
	sq_pushobject(vm_, *obj);

	if (hScope)
	{
		Assert(hScope != INVALID_HSCRIPT);
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	auto result = sq_call(vm_, 1, false, true);
	sq_pop(vm_, 1);
	if (SQ_FAILED(result))
	{
		return SCRIPT_ERROR;
	}
	return SCRIPT_DONE;
}

ScriptStatus_t SquirrelVM::Run(HSCRIPT hScript, bool bWait)
{
	SquirrelSafeCheck safeCheck(vm_);
	HSQOBJECT* obj = (HSQOBJECT*)hScript;
	sq_pushobject(vm_, *obj);
	sq_pushroottable(vm_);
	auto result = sq_call(vm_, 1, false, true);
	sq_pop(vm_, 1);
	if (SQ_FAILED(result))
	{
		return SCRIPT_ERROR;
	}
	return SCRIPT_DONE;
}

HSCRIPT SquirrelVM::CreateScope(const char* pszScope, HSCRIPT hParent)
{
	SquirrelSafeCheck safeCheck(vm_);

	sq_newtable(vm_);

	if (hParent)
	{
		HSQOBJECT* parent = (HSQOBJECT*)hParent;
		Assert(hParent != INVALID_HSCRIPT);
		sq_pushobject(vm_, *parent);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	sq_pushstring(vm_, pszScope, -1);
	sq_push(vm_, -3);
	sq_rawset(vm_, -3);

	sq_pushstring(vm_, "__vname", -1);
	sq_pushstring(vm_, pszScope, -1);
	sq_rawset(vm_, -4);

	if (SQ_FAILED(sq_setdelegate(vm_, -2)))
	{
		sq_pop(vm_, 2);
		return nullptr;
	}

	HSQOBJECT* obj = new HSQOBJECT;
	sq_resetobject(obj);
	sq_getstackobj(vm_, -1, obj);
	sq_addref(vm_, obj);
	sq_pop(vm_, 1);

	return (HSCRIPT)obj;
}

void SquirrelVM::ReleaseScope(HSCRIPT hScript)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (!hScript) return;
	HSQOBJECT* obj = (HSQOBJECT*)hScript;
	sq_pushobject(vm_, *obj);

	sq_getdelegate(vm_, -1);

	sq_pushstring(vm_, "__vname", -1);
	sq_rawdeleteslot(vm_, -2, SQFalse);

	sq_pop(vm_, 2);

	sq_release(vm_, obj);
	delete obj;
}

HSCRIPT SquirrelVM::LookupFunction(const char* pszFunction, HSCRIPT hScope)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (hScope)
	{
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		Assert(hScope != INVALID_HSCRIPT);
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}
	sq_pushstring(vm_, _SC(pszFunction), -1);

	HSQOBJECT obj;
	sq_resetobject(&obj);

	if (sq_get(vm_, -2) == SQ_OK)
	{
		sq_getstackobj(vm_, -1, &obj);
		sq_addref(vm_, &obj);
		sq_pop(vm_, 1);
	}
	sq_pop(vm_, 1);

	if (!sq_isclosure(obj))
	{
		sq_release(vm_, &obj);
		return nullptr;
	}

	HSQOBJECT* pObj = new HSQOBJECT;
	*pObj = obj;
	return (HSCRIPT)pObj;
}

void SquirrelVM::ReleaseFunction(HSCRIPT hScript)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (!hScript) return;
	HSQOBJECT* obj = (HSQOBJECT*)hScript;
	sq_release(vm_, obj);
	delete obj;
}

ScriptStatus_t SquirrelVM::ExecuteFunction(HSCRIPT hFunction, ScriptVariant_t* pArgs, int nArgs, ScriptVariant_t* pReturn, HSCRIPT hScope, bool bWait)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (!hFunction)
		return SCRIPT_ERROR;

	if (hFunction == INVALID_HSCRIPT)
		return SCRIPT_ERROR;

	HSQOBJECT* pFunc = (HSQOBJECT*)hFunction;
	sq_pushobject(vm_, *pFunc);

	if (hScope)
	{
		Assert(hScope != INVALID_HSCRIPT);
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	for (int i = 0; i < nArgs; ++i)
	{
		PushVariant(vm_, pArgs[i]);
	}

	bool hasReturn = pReturn != nullptr;

	if (SQ_FAILED(sq_call(vm_, nArgs + 1, hasReturn, SQTrue)))
	{
		sq_pop(vm_, 1);
		return SCRIPT_ERROR;
	}

	if (hasReturn)
	{
		if (!getVariant(vm_, -1, *pReturn))
		{
			sq_pop(vm_, 1);
			return SCRIPT_ERROR;
		}

		sq_pop(vm_, 1);
	}

	sq_pop(vm_, 1);
	return SCRIPT_DONE;
}

void SquirrelVM::RegisterFunction(ScriptFunctionBinding_t* pScriptFunction)
{
	SquirrelSafeCheck safeCheck(vm_);
	Assert(pScriptFunction);

	if (!pScriptFunction)
		return;

	char typemask[64];
	if (!CreateParamCheck(*pScriptFunction, typemask))
	{
		return;
	}

	sq_pushroottable(vm_);

	sq_pushstring(vm_, pScriptFunction->m_desc.m_pszScriptName, -1);

	sq_pushuserpointer(vm_, pScriptFunction);
	sq_newclosure(vm_, function_stub, 1);

	sq_setnativeclosurename(vm_, -1, pScriptFunction->m_desc.m_pszScriptName);

	sq_setparamscheck(vm_, pScriptFunction->m_desc.m_Parameters.Count() + 1, typemask);
	bool isStatic = false;
	sq_newslot(vm_, -3, isStatic);

	sq_pop(vm_, 1);

	RegisterDocumentation(vm_, pScriptFunction->m_desc);
}

bool SquirrelVM::RegisterClass(ScriptClassDesc_t* pClassDesc)
{
	SquirrelSafeCheck safeCheck(vm_);

	sq_pushroottable(vm_);
	sq_pushstring(vm_, pClassDesc->m_pszScriptName, -1);

	// Check if class name is already taken
	if (sq_get(vm_, -2) == SQ_OK)
	{
		HSQOBJECT obj;
		sq_resetobject(&obj);
		sq_getstackobj(vm_, -1, &obj);
		if (!sq_isnull(obj))
		{
			sq_pop(vm_, 2);
			return false;
		}

		sq_pop(vm_, 1);
	}

	// Register base in case it doesn't exist
	if (pClassDesc->m_pBaseDesc)
	{
		RegisterClass(pClassDesc->m_pBaseDesc);

		// Check if the base class can be
		sq_pushstring(vm_, pClassDesc->m_pBaseDesc->m_pszScriptName, -1);
		if (sq_get(vm_, -2) != SQ_OK)
		{
			sq_pop(vm_, 1);
			return false;
		}
	}

	if (SQ_FAILED(sq_newclass(vm_, pClassDesc->m_pBaseDesc ? SQTrue : SQFalse)))
	{
		sq_pop(vm_, 2 + (pClassDesc->m_pBaseDesc ? 1 : 0));
		return false;
	}

	sq_settypetag(vm_, -1, pClassDesc);

	sq_setclassudsize(vm_, -1, sizeof(ClassInstanceData));

	sq_pushstring(vm_, "constructor", -1);
	sq_newclosure(vm_, constructor_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "_tostring", -1);
	sq_newclosure(vm_, tostring_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "_get", -1);
	sq_newclosure(vm_, get_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "_set", -1);
	sq_newclosure(vm_, set_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "_add", -1);
	sq_newclosure(vm_, add_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "_sub", -1);
	sq_newclosure(vm_, sub_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "_mul", -1);
	sq_newclosure(vm_, mul_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "_div", -1);
	sq_newclosure(vm_, div_stub, 0);
	sq_newslot(vm_, -3, SQFalse);

	sq_pushstring(vm_, "IsValid", -1);
	sq_newclosure(vm_, IsValid_stub, 0);
	sq_newslot(vm_, -3, SQFalse);


	for (int i = 0; i < pClassDesc->m_FunctionBindings.Count(); ++i)
	{
		auto& scriptFunction = pClassDesc->m_FunctionBindings[i];

		char typemask[64];
		if (!CreateParamCheck(scriptFunction, typemask))
		{
			Warning("Unable to create param check for %s.%s\n",
				pClassDesc->m_pszClassname, scriptFunction.m_desc.m_pszFunction);
			break;
		}

		sq_pushstring(vm_, scriptFunction.m_desc.m_pszScriptName, -1);

		sq_pushuserpointer(vm_, &scriptFunction);
		sq_newclosure(vm_, function_stub, 1);

		sq_setnativeclosurename(vm_, -1, scriptFunction.m_desc.m_pszScriptName);

		sq_setparamscheck(vm_, scriptFunction.m_desc.m_Parameters.Count() + 1, typemask);
		bool isStatic = false;
		sq_newslot(vm_, -3, isStatic);

		RegisterDocumentation(vm_, scriptFunction.m_desc, pClassDesc);
	}

	for (int i = 0; i < pClassDesc->m_Hooks.Count(); ++i)
	{
		auto& scriptHook = pClassDesc->m_Hooks[i];

		RegisterHookDocumentation(vm_, scriptHook, scriptHook->m_desc, pClassDesc);
	}

	for (int i = 0; i < pClassDesc->m_Members.Count(); ++i)
	{
		auto& member = pClassDesc->m_Members[i];

		RegisterMemberDocumentation(vm_, member, pClassDesc);
	}

	sq_pushstring(vm_, pClassDesc->m_pszScriptName, -1);
	sq_push(vm_, -2);

	if (SQ_FAILED(sq_newslot(vm_, -4, SQFalse)))
	{
		sq_pop(vm_, 4);
		return false;
	}

	sq_pop(vm_, 2);

	RegisterClassDocumentation(vm_, pClassDesc);

	return true;
}

void SquirrelVM::RegisterConstant(ScriptConstantBinding_t* pScriptConstant)
{
	SquirrelSafeCheck safeCheck(vm_);
	Assert(pScriptConstant);

	if (!pScriptConstant)
		return;

	sq_pushconsttable(vm_);
	sq_pushstring(vm_, pScriptConstant->m_pszScriptName, -1);

	PushVariant(vm_, pScriptConstant->m_data);

	sq_newslot(vm_, -3, SQFalse);

	sq_pop(vm_, 1);

	char szValue[64];
	GetVariantScriptString( pScriptConstant->m_data, szValue, sizeof(szValue) );
	RegisterConstantDocumentation(vm_, pScriptConstant, szValue);
}

void SquirrelVM::RegisterEnum(ScriptEnumDesc_t* pEnumDesc)
{
	SquirrelSafeCheck safeCheck(vm_);
	Assert(pEnumDesc);

	if (!pEnumDesc)
		return;

	sq_pushconsttable(vm_);
	sq_pushstring(vm_, pEnumDesc->m_pszScriptName, -1);
	
	// Check if class name is already taken
	if (sq_get(vm_, -2) == SQ_OK)
	{
		HSQOBJECT obj;
		sq_resetobject(&obj);
		sq_getstackobj(vm_, -1, &obj);
		if (!sq_isnull(obj))
		{
			sq_pop(vm_, 2);
			return;
		}
	}

	sq_pop(vm_, 1);

	// HACKHACK: I have no idea how to declare enums with the current API.
	// For now, we'll just cram everything into a script buffer and compile it. (Blixibon)
	char szScript[2048];
	V_snprintf( szScript, sizeof(szScript), "enum %s {\n", pEnumDesc->m_pszScriptName );

	for (int i = 0; i < pEnumDesc->m_ConstantBindings.Count(); ++i)
	{
		auto& scriptConstant = pEnumDesc->m_ConstantBindings[i];

		char szValue[64];
		GetVariantScriptString( scriptConstant.m_data, szValue, sizeof(szValue) );

		V_snprintf( szScript, sizeof(szScript), "%s%s = %s\n", szScript, scriptConstant.m_pszScriptName, szValue );

		RegisterConstantDocumentation(vm_, &scriptConstant, szValue, pEnumDesc);
	}

	V_strcat_safe( szScript, "}" );

	Run( szScript );

	RegisterEnumDocumentation(vm_, pEnumDesc);
}

HSCRIPT SquirrelVM::RegisterInstance(ScriptClassDesc_t* pDesc, void* pInstance, bool bAllowDestruct)
{
	SquirrelSafeCheck safeCheck(vm_);

	this->RegisterClass(pDesc);

	sq_pushroottable(vm_);
	sq_pushstring(vm_, pDesc->m_pszScriptName, -1);

	if (sq_get(vm_, -2) != SQ_OK)
	{
		sq_pop(vm_, 1);
		return nullptr;
	}

	if (SQ_FAILED(sq_createinstance(vm_, -1)))
	{
		sq_pop(vm_, 2);
		return nullptr;
	}

	{
		SQUserPointer p;
		sq_getinstanceup(vm_, -1, &p, 0);
		new(p) ClassInstanceData(pInstance, pDesc, nullptr, bAllowDestruct);
	}

	sq_setreleasehook(vm_, -1, bAllowDestruct ? &destructor_stub : &destructor_stub_instance);

	HSQOBJECT* obj = new HSQOBJECT;
	sq_resetobject(obj);
	sq_getstackobj(vm_, -1, obj);
	sq_addref(vm_, obj);
	sq_pop(vm_, 3);

	return (HSCRIPT)obj;
}

void SquirrelVM::SetInstanceUniqeId(HSCRIPT hInstance, const char* pszId)
{
	SquirrelSafeCheck safeCheck(vm_);

	if (!hInstance) return;
	HSQOBJECT* obj = (HSQOBJECT*)hInstance;
	sq_pushobject(vm_, *obj);

	SQUserPointer self;
	sq_getinstanceup(vm_, -1, &self, nullptr);

	auto classInstanceData = (ClassInstanceData*)self;

	classInstanceData->instanceId = pszId;

	sq_poptop(vm_);
}

void SquirrelVM::RemoveInstance(HSCRIPT hInstance)
{
	SquirrelSafeCheck safeCheck(vm_);

	if (!hInstance) return;
	HSQOBJECT* obj = (HSQOBJECT*)hInstance;
	sq_pushobject(vm_, *obj);

	SQUserPointer self;
	sq_getinstanceup(vm_, -1, &self, nullptr);

	((ClassInstanceData*)self)->~ClassInstanceData();

	sq_setinstanceup(vm_, -1, nullptr);
	sq_setreleasehook(vm_, -1, nullptr);
	sq_pop(vm_, 1);

	sq_release(vm_, obj);
	delete obj;
}

void* SquirrelVM::GetInstanceValue(HSCRIPT hInstance, ScriptClassDesc_t* pExpectedType)
{
	SquirrelSafeCheck safeCheck(vm_);

	if (!hInstance) return nullptr;
	HSQOBJECT* obj = (HSQOBJECT*)hInstance;

	if (pExpectedType)
	{
		sq_pushroottable(vm_);
		sq_pushstring(vm_, pExpectedType->m_pszScriptName, -1);

		if (sq_get(vm_, -2) != SQ_OK)
		{
			sq_pop(vm_, 1);
			return nullptr;
		}

		sq_pushobject(vm_, *obj);

		if (sq_instanceof(vm_) != SQTrue)
		{
			sq_pop(vm_, 3);
			return nullptr;
		}

		sq_pop(vm_, 3);
	}

	sq_pushobject(vm_, *obj);
	SQUserPointer self;
	sq_getinstanceup(vm_, -1, &self, nullptr);
	sq_pop(vm_, 1);

	auto classInstanceData = (ClassInstanceData*)self;

	if (!classInstanceData)
	{
		return nullptr;
	}


	return classInstanceData->instance;
}

bool SquirrelVM::GenerateUniqueKey(const char* pszRoot, char* pBuf, int nBufSize)
{
	static int keyIdx = 0;
	// This gets used for script scope, still confused why it needs to be inside IScriptVM
	// is it just to be a compatible name for CreateScope?
	SquirrelSafeCheck safeCheck(vm_);
	V_snprintf(pBuf, nBufSize, "%08X_%s", ++keyIdx, pszRoot);
	return true;
}

bool SquirrelVM::ValueExists(HSCRIPT hScope, const char* pszKey)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (hScope)
	{
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		Assert(hScope != INVALID_HSCRIPT);
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	sq_pushstring(vm_, pszKey, -1);

	if (sq_get(vm_, -2) != SQ_OK)
	{
		sq_pop(vm_, 1);
		return false;
	}

	sq_pop(vm_, 2);
	return true;
}

bool SquirrelVM::SetValue(HSCRIPT hScope, const char* pszKey, const char* pszValue)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (hScope)
	{
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		Assert(hScope != INVALID_HSCRIPT);
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	sq_pushstring(vm_, pszKey, -1);
	sq_pushstring(vm_, pszValue, -1);

	sq_newslot(vm_, -3, SQFalse);

	sq_pop(vm_, 1);
	return true;
}

bool SquirrelVM::SetValue(HSCRIPT hScope, const char* pszKey, const ScriptVariant_t& value)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (hScope)
	{
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		Assert(hScope != INVALID_HSCRIPT);
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	sq_pushstring(vm_, pszKey, -1);
	PushVariant(vm_, value);

	sq_newslot(vm_, -3, SQFalse);

	sq_pop(vm_, 1);
	return true;
}

void SquirrelVM::CreateTable(ScriptVariant_t& Table)
{
	SquirrelSafeCheck safeCheck(vm_);

	sq_newtable(vm_);

	HSQOBJECT* obj = new HSQOBJECT;
	sq_resetobject(obj);
	sq_getstackobj(vm_, -1, obj);
	sq_addref(vm_, obj);
	sq_pop(vm_, 1);

	Table = (HSCRIPT)obj;
}

int	SquirrelVM::GetNumTableEntries(HSCRIPT hScope)
{
	SquirrelSafeCheck safeCheck(vm_);

	if (!hScope)
	{
		// TODO: This is called hScope but seems like just a table so
		// lets not fallback to root table
		return 0;
	}

	HSQOBJECT* scope = (HSQOBJECT*)hScope;
	Assert(hScope != INVALID_HSCRIPT);
	sq_pushobject(vm_, *scope);

	int count = sq_getsize(vm_, -1);

	sq_pop(vm_, 1);

	return count;
}

int SquirrelVM::GetKeyValue(HSCRIPT hScope, int nIterator, ScriptVariant_t* pKey, ScriptVariant_t* pValue)
{
	SquirrelSafeCheck safeCheck(vm_);

	if (hScope)
	{
		Assert(hScope != INVALID_HSCRIPT);
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	if (nIterator == -1)
	{
		sq_pushnull(vm_);
	}
	else
	{
		sq_pushinteger(vm_, nIterator);
	}

	SQInteger nextiter = -1;

	if (SQ_SUCCEEDED(sq_next(vm_, -2)))
	{
		if (pKey) getVariant(vm_, -2, *pKey);
		if (pValue) getVariant(vm_, -1, *pValue);

		sq_getinteger(vm_, -3, &nextiter);
		sq_pop(vm_, 2);
	}
	sq_pop(vm_, 2);

	return nextiter;
}

bool SquirrelVM::GetValue(HSCRIPT hScope, const char* pszKey, ScriptVariant_t* pValue)
{
	SquirrelSafeCheck safeCheck(vm_);

	Assert(pValue);
	if (!pValue)
	{
		return false;
	}

	if (hScope)
	{
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		Assert(hScope != INVALID_HSCRIPT);
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	sq_pushstring(vm_, pszKey, -1);

	if (sq_get(vm_, -2) != SQ_OK)
	{
		sq_pop(vm_, 1);
		return false;
	}

	if (!getVariant(vm_, -1, *pValue))
	{
		sq_pop(vm_, 2);
		return false;
	}

	sq_pop(vm_, 2);

	return true;
}

void SquirrelVM::ReleaseValue(ScriptVariant_t& value)
{
	SquirrelSafeCheck safeCheck(vm_);
	if (value.m_type == FIELD_HSCRIPT)
	{
		HSCRIPT hScript = value;
		HSQOBJECT* obj = (HSQOBJECT*)hScript;
		sq_release(vm_, obj);
		delete obj;
	}
	else
	{
		value.Free();
	}

	// Let's prevent this being called again and giving some UB
	value.m_type = FIELD_VOID;
}

bool SquirrelVM::ClearValue(HSCRIPT hScope, const char* pszKey)
{
	SquirrelSafeCheck safeCheck(vm_);

	if (hScope)
	{
		HSQOBJECT* scope = (HSQOBJECT*)hScope;
		Assert(hScope != INVALID_HSCRIPT);
		sq_pushobject(vm_, *scope);
	}
	else
	{
		sq_pushroottable(vm_);
	}

	sq_pushstring(vm_, pszKey, -1);
	if (SQ_FAILED(sq_deleteslot(vm_, -2, SQFalse)))
	{
		sq_pop(vm_, 1);
		return false;
	}

	sq_pop(vm_, 1);
	return true;
}
/*
void SquirrelVM::CreateArray(ScriptVariant_t &arr, int size)
{
	SquirrelSafeCheck safeCheck(vm_);

	HSQOBJECT *obj = new HSQOBJECT;
	sq_resetobject(obj);

	sq_newarray(vm_,size);
	sq_getstackobj(vm_, -1, obj);
	sq_addref(vm_, obj);
	sq_pop(vm_, 1);

	arr = (HSCRIPT)obj;
}
*/
bool SquirrelVM::ArrayAppend(HSCRIPT hArray, const ScriptVariant_t &val)
{
	SquirrelSafeCheck safeCheck(vm_);

	HSQOBJECT *arr = (HSQOBJECT*)hArray;

	sq_pushobject(vm_, *arr);
	PushVariant(vm_, val);
	bool ret = sq_arrayappend(vm_, -2) == SQ_OK;
	sq_pop(vm_, 1);

	return ret;
}

enum ClassType
{
	VectorClassType = 0,
	NativeClassType = 1,
	ScriptClassType = 2
};

SQInteger closure_write(SQUserPointer file, SQUserPointer p, SQInteger size)
{
	((CUtlBuffer*)file)->Put(p, size);
	return size;
}

void SquirrelVM::WriteObject(CUtlBuffer* pBuffer, WriteStateMap& writeState, SQInteger idx)
{
	SquirrelSafeCheck safeCheck(vm_);

	HSQOBJECT obj;
	sq_resetobject(&obj);
	sq_getstackobj(vm_, idx, &obj);

	switch (obj._type)
	{
	case OT_NULL:
	{
		pBuffer->PutInt(OT_NULL);
		break;
	}
	case OT_INTEGER:
	{
		pBuffer->PutInt(OT_INTEGER);
		pBuffer->PutInt64(sq_objtointeger(&obj));
		break;
	}
	case OT_FLOAT:
	{
		pBuffer->PutInt(OT_FLOAT);
		pBuffer->PutFloat(sq_objtofloat(&obj));
		break;
	}
	case OT_BOOL:
	{
		pBuffer->PutInt(OT_BOOL);
		pBuffer->PutChar(sq_objtobool(&obj));
		break;
	}
	case OT_STRING:
	{
		pBuffer->PutInt(OT_STRING);
		const char* val = nullptr;
		SQInteger size = 0;
		sq_getstringandsize(vm_, idx, &val, &size);
		pBuffer->PutInt(size);
		pBuffer->Put(val, size);
		break;
	}
	case OT_TABLE:
	{
		pBuffer->PutInt(OT_TABLE);
		if (writeState.CheckCache(pBuffer, obj._unVal.pTable))
		{
			break;
		}
		sq_getdelegate(vm_, idx);
		WriteObject(pBuffer, writeState, -1);
		sq_poptop(vm_);
		int count = sq_getsize(vm_, idx);
		sq_push(vm_, idx);
		sq_pushnull(vm_);
		pBuffer->PutInt(count);
		while (SQ_SUCCEEDED(sq_next(vm_, -2)))
		{
			WriteObject(pBuffer, writeState, -2);
			WriteObject(pBuffer, writeState, -1);
			sq_pop(vm_, 2);
			--count;
		}
		sq_pop(vm_, 2);
		Assert(count == 0);
		break;
	}
	case OT_ARRAY:
	{
		pBuffer->PutInt(OT_ARRAY);
		if (writeState.CheckCache(pBuffer, obj._unVal.pArray))
		{
			break;
		}
		int count = sq_getsize(vm_, idx);
		pBuffer->PutInt(count);
		sq_push(vm_, idx);
		sq_pushnull(vm_);
		while (SQ_SUCCEEDED(sq_next(vm_, -2)))
		{
			WriteObject(pBuffer, writeState, -1);
			sq_pop(vm_, 2);
			--count;
		}
		sq_pop(vm_, 2);
		Assert(count == 0);
		break;
	}
	case OT_CLOSURE:
	{
		pBuffer->PutInt(OT_CLOSURE);
		if (writeState.CheckCache(pBuffer, obj._unVal.pClosure))
		{
			break;
		}

		SQInteger nparams = 0, nfreevars = 0;
		sq_getclosureinfo(vm_, idx, &nparams, &nfreevars);
		if (nfreevars == 0 && _closure(obj)->_function->_defaultparams == 0)
		{
			pBuffer->PutChar(0);

			sq_push(vm_, idx);
			if (SQ_FAILED(sq_writeclosure(vm_, closure_write, pBuffer)))
			{
				Error("Failed to write closure");
			}
			sq_pop(vm_, 1);
		}
		else
		{
			// Unfortunately we can't use sq_writeclosure because it doesn't work well with
			// outer variables

			pBuffer->PutChar(1);

			if (!_closure(obj)->Save(vm_, pBuffer, closure_write))
			{
				Error("Failed to write closure\n");
			}

			int noutervalues = _closure(obj)->_function->_noutervalues;
			for (int i = 0; i < noutervalues; ++i)
			{
				sq_pushobject(vm_, _closure(obj)->_outervalues[i]);
				WriteObject(pBuffer, writeState, -1);
				sq_poptop(vm_);
			}

			int ndefaultparams = _closure(obj)->_function->_ndefaultparams;
			for (int i = 0; i < ndefaultparams; ++i)
			{
				sq_pushobject(vm_, _closure(obj)->_defaultparams[i]);
				WriteObject(pBuffer, writeState, -1);
				sq_poptop(vm_);
			}
		}

		if (_closure(obj)->_env)
		{
			sq_pushobject(vm_, _closure(obj)->_env->_obj);
		}
		else
		{
			sq_pushnull(vm_);
		}
		WriteObject(pBuffer, writeState, -1);
		sq_poptop(vm_);

		break;
	}
	case OT_NATIVECLOSURE:
	{
		pBuffer->PutInt(OT_NATIVECLOSURE);
		sq_getclosurename(vm_, idx);

		const char* name = nullptr;
		sq_getstring(vm_, -1, &name);
		pBuffer->PutString(name);

		sq_pop(vm_, 1);
		break;
	}
	case OT_CLASS:
	{
		pBuffer->PutInt(OT_CLASS);
		if (writeState.CheckCache(pBuffer, obj._unVal.pClass))
		{
			break;
		}
		SQUserPointer typetag = nullptr;
		sq_gettypetag(vm_, idx, &typetag);
		if (typetag == TYPETAG_VECTOR)
		{
			pBuffer->PutInt(VectorClassType);
		}
		else if (typetag != nullptr)
		{
			// Seems so dangerous to treat typetag as ScriptClassDesc_t*
			// however we don't really have an option without some sort of tagged
			// pointer.
			pBuffer->PutInt(NativeClassType);
			pBuffer->PutString(((ScriptClassDesc_t*)typetag)->m_pszScriptName);
		}
		else
		{
			// HACK: We can't easily identify when the type is a builtin to exclude
			// so we just check against the only class we need to deal with at the moment
			// which is "regexp"
			const char* builtinName = nullptr;
			if (_class(obj) == _class(regexpClass_))
			{
				builtinName = "regexp";
			}

			if (builtinName)
			{
				pBuffer->PutInt(NativeClassType);
				pBuffer->PutString(builtinName);
				break;
			}

			pBuffer->PutInt(ScriptClassType);

			sq_getbase(vm_, idx);
			WriteObject(pBuffer, writeState, -1);
			sq_pop(vm_, 1);

			sq_push(vm_, idx);
			sq_pushnull(vm_);
			sq_getattributes(vm_, -2);
			WriteObject(pBuffer, writeState, -1);
			sq_pop(vm_, 1);

			sq_pushnull(vm_);
			while (SQ_SUCCEEDED(sq_next(vm_, -2)))
			{
				pBuffer->PutChar(1);
				// TODO: Member Attributes
				WriteObject(pBuffer, writeState, -2);
				WriteObject(pBuffer, writeState, -1);
				sq_pop(vm_, 2);
			}
			sq_pop(vm_, 2);

			{
				// HACK: Meta-methods are not included in an iterator of OT_CLASS
				SQObjectPtrVec& metamethods = *(_ss(vm_)->_metamethods);
				for (int i = 0; i < MT_LAST; ++i)
				{
					if (sq_type(_class(obj)->_metamethods[i]) != OT_NULL)
					{
						pBuffer->PutChar(1);
						sq_pushobject(vm_, metamethods[i]);
						sq_pushobject(vm_, _class(obj)->_metamethods[i]);
						WriteObject(pBuffer, writeState, -2);
						WriteObject(pBuffer, writeState, -1);
						sq_pop(vm_, 2);
					}
				}
			}

			pBuffer->PutChar(0);
		}
		break;
	}
	case OT_INSTANCE:
	{
		pBuffer->PutInt(OT_INSTANCE);
		if (writeState.CheckCache(pBuffer, obj._unVal.pInstance))
		{
			break;
		}
		sq_getclass(vm_, idx);
		WriteObject(pBuffer, writeState, -1);
		sq_pop(vm_, 1);

		if (_instance(obj)->_class == _class(regexpClass_))
		{
			sq_push(vm_, idx);
			sq_pushstring(vm_, "pattern_", -1);
			sq_rawget(vm_, -2);
			WriteObject(pBuffer, writeState, -1);
			sq_pop(vm_, 2);
			break;
		}

		{
			// HACK: No way to get the default values part from accessing the class directly
			SQUnsignedInteger nvalues = _instance(obj)->_class->_defaultvalues.size();
			for (SQUnsignedInteger n = 0; n < nvalues; n++) {
				sq_pushobject(vm_, _instance(obj)->_values[n]);
				WriteObject(pBuffer, writeState, -1);
				sq_pop(vm_, 1);
			}
		}

		SQUserPointer typetag;
		sq_gettypetag(vm_, idx, &typetag);

		if (typetag == TYPETAG_VECTOR)
		{
			Vector* v = nullptr;
			sq_getinstanceup(vm_, idx, (SQUserPointer*)&v, TYPETAG_VECTOR);
			Assert(v);
			pBuffer->PutFloat(v->x);
			pBuffer->PutFloat(v->y);
			pBuffer->PutFloat(v->z);
		}
		else if (typetag)
		{
			ClassInstanceData* pClassInstanceData;
			sq_getinstanceup(vm_, idx, (SQUserPointer*)&pClassInstanceData, typetag);

			if (pClassInstanceData)
			{
				if (pClassInstanceData->desc->m_pszDescription[0] == SCRIPT_SINGLETON[0])
				{
					// Do nothing, singleton should be created from just the class
				}
				else if (!pClassInstanceData->instanceId.IsEmpty())
				{
					pBuffer->PutString(pClassInstanceData->instanceId);

					pBuffer->PutChar(pClassInstanceData->allowDestruct ? 1 : 0);
				}
				else
				{
					Warning("SquirrelVM::WriteObject: Unable to find instanceID for object of type %s, unable to serialize\n",
						pClassInstanceData->desc->m_pszClassname);
					pBuffer->PutString("");
				}
			}
			else
			{
				pBuffer->PutString("");
			}
		}

		break;
	}
	case OT_WEAKREF:
	{
		pBuffer->PutInt(OT_WEAKREF);
		sq_getweakrefval(vm_, idx);
		WriteObject(pBuffer, writeState, -1);
		sq_pop(vm_, 1);
		break;
	}
	case OT_FUNCPROTO: //internal usage only
	{
		pBuffer->PutInt(OT_FUNCPROTO);

		if (writeState.CheckCache(pBuffer, obj._unVal.pFunctionProto))
		{
			break;
		}

		_funcproto(obj)->Save(vm_, pBuffer, closure_write);
	}
	case OT_OUTER: //internal usage only
	{
		pBuffer->PutInt(OT_OUTER);

		if (writeState.CheckCache(pBuffer, obj._unVal.pOuter))
		{
			break;
		}

		sq_pushobject(vm_, *_outer(obj)->_valptr);
		WriteObject(pBuffer, writeState, -1);
		sq_poptop(vm_);

		break;
	}
	// case OT_USERDATA:
	// case OT_GENERATOR:
	// case OT_USERPOINTER:
	// case OT_THREAD:
	// 
	default:
		Warning("SquirrelVM::WriteObject: Unexpected type %d", sq_gettype(vm_, idx));
		// Save a null instead
		pBuffer->PutInt(OT_NULL);
	}
}

void SquirrelVM::WriteState(CUtlBuffer* pBuffer)
{
	SquirrelSafeCheck safeCheck(vm_);

	WriteStateMap writeState;

	sq_pushroottable(vm_);

	// Not really a check cache, but adds the root
	HSQOBJECT obj;
	sq_resetobject(&obj);
	sq_getstackobj(vm_, -1, &obj);
	writeState.CheckCache(pBuffer, _table(obj));

	int count = sq_getsize(vm_, 1);
	sq_pushnull(vm_);
	pBuffer->PutInt(count);
	while (SQ_SUCCEEDED(sq_next(vm_, -2)))
	{
		WriteObject(pBuffer, writeState, -2);
		WriteObject(pBuffer, writeState, -1);
		sq_pop(vm_, 2);
		--count;
	}
	sq_pop(vm_, 2);
	Assert(count == 0);
}

SQInteger closure_read(SQUserPointer file, SQUserPointer buf, SQInteger size)
{
	CUtlBuffer* pBuffer = (CUtlBuffer*)file;
	pBuffer->Get(buf, size);
	return pBuffer->IsValid() ? size : -1;
}

void SquirrelVM::ReadObject(CUtlBuffer* pBuffer, ReadStateMap& readState)
{
	SquirrelSafeCheck safeCheck(vm_, 1);

	int thisType = pBuffer->GetInt();

	switch (thisType)
	{
	case OT_NULL:
	{
		sq_pushnull(vm_);
		break;
	}
	case OT_INTEGER:
	{
		sq_pushinteger(vm_, pBuffer->GetInt64());
		break;
	}
	case OT_FLOAT:
	{
		sq_pushfloat(vm_, pBuffer->GetFloat());
		break;
	}
	case OT_BOOL:
	{
		sq_pushbool(vm_, pBuffer->GetChar());
		break;
	}
	case OT_STRING:
	{
		int size = pBuffer->GetInt();
		char* buffer = new char[size + 1];
		pBuffer->Get(buffer, size);
		buffer[size] = 0;
		sq_pushstring(vm_, buffer, size);
		delete[] buffer;
		break;
	}
	case OT_TABLE:
	{
		HSQOBJECT* obj = nullptr;
		if (readState.CheckCache(pBuffer, &obj))
		{
			sq_pushobject(vm_, *obj);
			break;
		}

		ReadObject(pBuffer, readState);

		int count = pBuffer->GetInt();
		sq_newtableex(vm_, count);
		sq_getstackobj(vm_, -1, obj);
		sq_addref(vm_, obj);

		sq_push(vm_, -2);
		sq_setdelegate(vm_, -2);

		sq_remove(vm_, -2);

		for (int i = 0; i < count; ++i)
		{
			ReadObject(pBuffer, readState);
			ReadObject(pBuffer, readState);
			sq_rawset(vm_, -3);
		}

		break;
	}
	case OT_ARRAY:
	{
		HSQOBJECT* obj = nullptr;
		if (readState.CheckCache(pBuffer, &obj))
		{
			sq_pushobject(vm_, *obj);
			break;
		}

		int count = pBuffer->GetInt();
		sq_newarray(vm_, count);
		sq_getstackobj(vm_, -1, obj);
		sq_addref(vm_, obj);

		for (int i = 0; i < count; ++i)
		{
			sq_pushinteger(vm_, i);
			ReadObject(pBuffer, readState);
			sq_rawset(vm_, -3);
		}
		break;
	}
	case OT_CLOSURE:
	{
		HSQOBJECT* obj = nullptr;
		if (readState.CheckCache(pBuffer, &obj))
		{
			sq_pushobject(vm_, *obj);
			break;
		}

		if (pBuffer->GetChar() == 0)
		{
			if (SQ_FAILED(sq_readclosure(vm_, closure_read, pBuffer)))
			{
				Error("Failed to read closure\n");
				sq_pushnull(vm_);
				break;
			}
			sq_getstackobj(vm_, -1, obj);
			sq_addref(vm_, obj);
		}
		else
		{
			SQObjectPtr ret;
			if (!SQClosure::Load(vm_, pBuffer, closure_read, ret))
			{
				Error("Failed to read closure\n");
				sq_pushnull(vm_);
				break;
			}

			int noutervalues = _closure(ret)->_function->_noutervalues;
			for (int i = 0; i < noutervalues; ++i)
			{
				ReadObject(pBuffer, readState);
				HSQOBJECT obj;
				sq_resetobject(&obj);
				sq_getstackobj(vm_, -1, &obj);
				_closure(ret)->_outervalues[i] = obj;
				sq_poptop(vm_);
			}

			int ndefaultparams = _closure(ret)->_function->_ndefaultparams;
			for (int i = 0; i < ndefaultparams; ++i)
			{
				ReadObject(pBuffer, readState);
				HSQOBJECT obj;
				sq_resetobject(&obj);
				sq_getstackobj(vm_, -1, &obj);
				_closure(ret)->_defaultparams[i] = obj;
				sq_poptop(vm_);
			}

			*obj = ret;
			sq_addref(vm_, obj);
			sq_pushobject(vm_, *obj);
		}

		ReadObject(pBuffer, readState);
		HSQOBJECT env;
		sq_resetobject(&env);
		sq_getstackobj(vm_, -1, &env);
		if (!sq_isnull(env))
		{
			if (_closure( *obj ) == nullptr)
				Warning("Closure is null\n");
			else
				_closure(*obj)->_env = _refcounted(env)->GetWeakRef(sq_type(env));
		}
		sq_poptop(vm_);

		break;
	}
	case OT_NATIVECLOSURE:
	{
		char closureName[128] = "";
		pBuffer->GetString(closureName, sizeof(closureName));

		sq_pushroottable(vm_);
		sq_pushstring(vm_, closureName, -1);
		if (SQ_FAILED(sq_get(vm_, -2)))
		{
			Warning("SquirrelVM::ReadObject: Failed to find native closure\n");
			sq_pop(vm_, 1);
			sq_pushnull(vm_);
		}
		sq_remove(vm_, -2);

		break;
	}
	case OT_CLASS:
	{
		HSQOBJECT* obj = nullptr;
		if (readState.CheckCache(pBuffer, &obj))
		{
			sq_pushobject(vm_, *obj);
			break;
		}

		ClassType classType = (ClassType)pBuffer->GetInt();

		if (classType == VectorClassType)
		{
			sq_pushobject(vm_, vectorClass_);
			sq_getstackobj(vm_, -1, obj);
			sq_addref(vm_, obj);
		}
		else if (classType == NativeClassType)
		{
			char className[128] = "";
			pBuffer->GetString(className, sizeof(className));

			sq_pushroottable(vm_);
			sq_pushstring(vm_, className, -1);
			if (SQ_FAILED(sq_get(vm_, -2)))
			{
				Warning("SquirrelVM::ReadObject: Failed to find native class: %s\n", className);
				sq_pushnull(vm_);
			}
			sq_remove(vm_, -2);
			sq_getstackobj(vm_, -1, obj);
			sq_addref(vm_, obj);
		}
		else if (classType == ScriptClassType)
		{
			ReadObject(pBuffer, readState);
			bool hasBase = sq_gettype(vm_, -1) != OT_NULL;
			if (!hasBase)
			{
				sq_poptop(vm_);
			}

			sq_newclass(vm_, hasBase);
			sq_getstackobj(vm_, -1, obj);
			sq_addref(vm_, obj);

			sq_pushnull(vm_);
			ReadObject(pBuffer, readState);
			sq_setattributes(vm_, -3);
			sq_poptop(vm_); // Returns the old attributes

			while (pBuffer->GetChar())
			{
				// TODO: Member Attributes
				ReadObject(pBuffer, readState);
				ReadObject(pBuffer, readState);
				sq_newslot(vm_, -3, false);
			}
		}
		else
		{
			Error("SquirrelVM::ReadObject: Unknown class type\n");
			sq_pushnull(vm_);
		}
		break;
	}
	case OT_INSTANCE:
	{
		HSQOBJECT* obj = nullptr;
		if (readState.CheckCache(pBuffer, &obj))
		{
			sq_pushobject(vm_, *obj);
			break;
		}

		ReadObject(pBuffer, readState);

		HSQOBJECT klass;
		sq_resetobject(&klass);
		sq_getstackobj(vm_, -1, &klass);
		if (_class(klass) == _class(regexpClass_))
		{
			sq_pushnull(vm_);
			ReadObject(pBuffer, readState);
			sq_call(vm_, 2, SQTrue, SQFalse);

			sq_getstackobj(vm_, -1, obj);
			sq_addref(vm_, obj);

			sq_remove(vm_, -2);

			break;
		}

		SQUserPointer typetag;
		sq_gettypetag(vm_, -1, &typetag);

		if (typetag && typetag != TYPETAG_VECTOR &&
			((ScriptClassDesc_t*)typetag)->m_pszDescription[0] == SCRIPT_SINGLETON[0])
		{
			sq_poptop(vm_);

			Assert(sq_isclass(klass));

			// singleton, lets find an equivlent in the root
			bool foundSingleton = false;
			sq_pushroottable(vm_);
			sq_pushnull(vm_);
			HSQOBJECT singleton;
			sq_resetobject(&singleton);
			while (SQ_SUCCEEDED(sq_next(vm_, -2)))
			{
				sq_getstackobj(vm_, -1, &singleton);
				if (sq_isinstance(singleton) && _instance(singleton)->_class == _class(klass))
				{
					foundSingleton = true;
					*obj = singleton;
					sq_addref(vm_, obj);
					sq_pop(vm_, 2);
					break;
				}
				sq_pop(vm_, 2);
			}
			sq_pop(vm_, 2);

			if (!foundSingleton)
			{
				Warning("SquirrelVM::ReadObject: Failed to find singleton for %s\n",
					((ScriptClassDesc_t*)typetag)->m_pszScriptName);
			}

			sq_pushobject(vm_, *obj);
			break;
		}

		sq_createinstance(vm_, -1);
		sq_getstackobj(vm_, -1, obj);
		sq_addref(vm_, obj);

		sq_remove(vm_, -2);

		{
			// HACK: No way to get the default values part from accessing the class directly
			SQUnsignedInteger nvalues = _instance(*obj)->_class->_defaultvalues.size();
			for (SQUnsignedInteger n = 0; n < nvalues; n++) {
				ReadObject(pBuffer, readState);
				HSQOBJECT val;
				sq_resetobject(&val);
				sq_getstackobj(vm_, -1, &val);
				_instance(*obj)->_values[n] = val;
				sq_pop(vm_, 1);
			}
		}

		if (typetag == TYPETAG_VECTOR)
		{
			float x = pBuffer->GetFloat();
			float y = pBuffer->GetFloat();
			float z = pBuffer->GetFloat();
			SQUserPointer p;
			sq_getinstanceup(vm_, -1, &p, 0);
			new(p) Vector(x, y, z);
		}
		else if (typetag)
		{
			ScriptClassDesc_t* pClassDesc = (ScriptClassDesc_t*)typetag;

			char instanceName[128] = "";
			pBuffer->GetString(instanceName, sizeof(instanceName));

			HSQOBJECT* hinstance = new HSQOBJECT;
			sq_resetobject(hinstance);
			sq_getstackobj(vm_, -1, hinstance);
			sq_addref(vm_, hinstance);

			if (*instanceName)
			{
				bool allowDestruct = (pBuffer->GetChar() == 1);

				auto instance = pClassDesc->pHelper->BindOnRead((HSCRIPT)hinstance, nullptr, instanceName);
				if (instance == nullptr)
				{
					sq_release(vm_, hinstance);
					delete hinstance;
					sq_poptop(vm_);
					sq_pushnull(vm_);
					break;
				}

				{
					SQUserPointer p;
					sq_getinstanceup(vm_, -1, &p, 0);
					new(p) ClassInstanceData(instance, pClassDesc, instanceName, allowDestruct);
				}
				sq_setreleasehook(vm_, -1, allowDestruct ? &destructor_stub : &destructor_stub_instance);
			}
			else
			{
				sq_setinstanceup(vm_, -1, nullptr);
			}
		}

		break;
	}
	case OT_WEAKREF:
	{
		ReadObject(pBuffer, readState);
		sq_weakref(vm_, -1);
		break;
	}
	case OT_FUNCPROTO: //internal usage only
	{
		HSQOBJECT* obj = nullptr;
		if (readState.CheckCache(pBuffer, &obj))
		{
			sq_pushobject(vm_, *obj);
			break;
		}

		SQObjectPtr ret;
		if (!SQFunctionProto::Load(vm_, pBuffer, closure_read, ret))
		{
			Error("Failed to deserialize OT_FUNCPROTO\n");
			sq_pushnull(vm_);
			break;
		}

		vm_->Push(ret);
		sq_getstackobj(vm_, -1, obj);
		sq_addref(vm_, obj);
	}
	case OT_OUTER: //internal usage only
	{
		HSQOBJECT* obj = nullptr;
		if (readState.CheckCache(pBuffer, &obj))
		{
			sq_pushobject(vm_, *obj);
			break;
		}

		ReadObject(pBuffer, readState);
		HSQOBJECT inner;
		sq_resetobject(&inner);
		sq_getstackobj(vm_, -1, &inner);
		SQOuter* outer = SQOuter::Create(_ss(vm_), nullptr);
		outer->_value = inner;
		outer->_valptr = &(outer->_value);
		sq_poptop(vm_);
		vm_->Push(outer);
		sq_getstackobj(vm_, -1, obj);
		sq_addref(vm_, obj);

		break;
	}
	// case OT_USERDATA:
	// case OT_GENERATOR:
	// case OT_USERPOINTER:
	// case OT_THREAD:
	// 
	default:
		Error("SquirrelVM::ReadObject: Unexpected type %d", thisType);
	}
}

void SquirrelVM::ReadState(CUtlBuffer* pBuffer)
{
	SquirrelSafeCheck safeCheck(vm_);

	ReadStateMap readState(vm_);

	sq_pushroottable(vm_);

	HSQOBJECT* obj = nullptr;
	readState.CheckCache(pBuffer, &obj);
	sq_getstackobj(vm_, -1, obj);
	sq_addref(vm_, obj);

	int count = pBuffer->GetInt();

	for (int i = 0; i < count; ++i)
	{
		ReadObject(pBuffer, readState);
		ReadObject(pBuffer, readState);

		sq_rawset(vm_, -3);
	}

	sq_pop(vm_, 1);
}

void SquirrelVM::RemoveOrphanInstances()
{
	SquirrelSafeCheck safeCheck(vm_);
	// TODO: Is this the right thing to do here? It's not really removing orphan instances
	sq_collectgarbage(vm_);
}

void SquirrelVM::DumpState()
{
	SquirrelSafeCheck safeCheck(vm_);
	// TODO: Dump state
}

void SquirrelVM::SetOutputCallback(ScriptOutputFunc_t pFunc)
{
	SquirrelSafeCheck safeCheck(vm_);
	// TODO: Support output callbacks
}

void SquirrelVM::SetErrorCallback(ScriptErrorFunc_t pFunc)
{
	SquirrelSafeCheck safeCheck(vm_);
	// TODO: Support error callbacks
}

bool SquirrelVM::RaiseException(const char* pszExceptionText)
{
	SquirrelSafeCheck safeCheck(vm_);
	sq_pushstring(vm_, pszExceptionText, -1);
	sq_resetobject(&lastError_);
	sq_getstackobj(vm_, -1, &lastError_);
	sq_addref(vm_, &lastError_);
	sq_pop(vm_, 1);
	return true;
}


IScriptVM* makeSquirrelVM()
{
	return new SquirrelVM;
}
