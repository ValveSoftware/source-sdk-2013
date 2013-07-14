//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//===========================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_usermessage.h"
#include "srcpy_entities.h"

#ifndef CLIENT_DLL
	#include "enginecallback.h"
#else
	#include "usermessages.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace bp = boost::python;

// In python we don't want to specify the types, so we need to send the type data
// with the message. Additionally we need to ensure begin and end function is called
// This is why the message is done in one list, so we can hide everything in one function
enum PyType{
	PYTYPE_INT = 0,
	PYTYPE_FLOAT,
	PYTYPE_STRING,
	PYTYPE_BOOL,
	PYTYPE_NONE,
	PYTYPE_VECTOR,
	PYTYPE_QANGLE,
	PYTYPE_HANDLE,
	PYTYPE_LIST,
	PYTYPE_DICT,
	PYTYPE_TUPLE,
	PYTYPE_SET,
};

#ifndef CLIENT_DLL
// TODO: Could use some optimization probably
void PyFillWriteElement( pywrite &w, bp::object data )
{
	bp::object datatype = fntype(data);

	if( datatype == types.attr("IntType") )
	{
		w.type = PYTYPE_INT;
		w.writeint = boost::python::extract<int>(data);
	}
	else if( datatype == types.attr("FloatType") )
	{
		w.type = PYTYPE_FLOAT;
		w.writefloat = boost::python::extract<float>(data);
	}
	else if( datatype == types.attr("StringType") )
	{
		w.type = PYTYPE_STRING;
		w.writestr = boost::python::extract<const char *>(data);
	}
	else if( datatype == types.attr("BooleanType") )
	{
		w.type = PYTYPE_BOOL;
		w.writebool = boost::python::extract<bool>(data);
	}
	else if( datatype == types.attr("NoneType") )
	{
		w.type = PYTYPE_NONE;

	}
	else if( datatype == types.attr("ListType") )
	{
		w.type = PYTYPE_LIST;

		int length = boost::python::len(data);
		for(int i=0; i<length; i++)
		{
			pywrite write;
			PyFillWriteElement( write, data[i] );
			w.writelist.AddToTail(write);
		}
	}
	else if( datatype == builtins.attr("set") )
	{
		w.type = PYTYPE_SET;

		bp::list items( data );

		int length = boost::python::len(data);
		for(int i=0; i<length; i++)
		{
			pywrite write;
			PyFillWriteElement( write, data[i] );
			w.writelist.AddToTail(write);
		}
	}
	else if( datatype == types.attr("TupleType") )
	{
		w.type = PYTYPE_TUPLE;

		int length = boost::python::len(data);
		for(int i=0; i<length; i++)
		{
			pywrite write;
			PyFillWriteElement( write, data[i] );
			w.writelist.AddToTail(write);
		}
	}
	else if( datatype == types.attr("DictType") )
	{
		w.type = PYTYPE_DICT;

		bp::object objectKey, objectValue;
		const bp::object objectKeys = bp::dict(data).iterkeys();
		const bp::object objectValues = bp::dict(data).itervalues();
		unsigned long ulCount = bp::len(data); 
		for( unsigned long u = 0; u < ulCount; u++ )
		{
			objectKey = objectKeys.attr( "next" )();
			objectValue = objectValues.attr( "next" )();

			pywrite write;
			PyFillWriteElement( write, objectKey );
			w.writelist.AddToTail(write);

			pywrite write2;
			PyFillWriteElement( write2, objectValue );
			w.writelist.AddToTail(write2);
		}
	}
	else if( !Q_strncmp( Py_TYPE(data.ptr())->tp_name, "Vector", 6 ) )
	{
		w.type = PYTYPE_VECTOR;
		w.writevector = boost::python::extract<Vector>(data);
	}
	else if( !Q_strncmp( Py_TYPE(data.ptr())->tp_name, "QAngle", 6 ) )
	{
		w.type = PYTYPE_QANGLE;
		QAngle angle = boost::python::extract<QAngle>(data);
		w.writevector = Vector(angle.x, angle.y, angle.z);
	}
	else
	{
		try {
			CBaseHandle handle = boost::python::extract<CBaseHandle>(data);
			w.type = PYTYPE_HANDLE;
			w.writehandle = handle;
			return;
		} catch( bp::error_already_set & ) {
			PyErr_Clear();
		}
		try {
			CBaseEntity *pEnt = boost::python::extract<CBaseEntity *>(data);
			w.type = PYTYPE_HANDLE;
			w.writehandle = pEnt;
			return;
		} catch( bp::error_already_set & ) {
			PyErr_Clear();
		}
		PyErr_SetString(PyExc_ValueError, "Unsupported type in message list" );
		throw boost::python::error_already_set(); 
	}
}

void PyWriteElement( pywrite &w )
{
	WRITE_BYTE(w.type);
	switch(w.type)
	{
	case PYTYPE_INT:
		WRITE_LONG(w.writeint);
		break;
	case PYTYPE_FLOAT:
		WRITE_FLOAT(w.writefloat);
		break;
	case PYTYPE_STRING:
		WRITE_STRING(w.writestr);
		break;
	case PYTYPE_BOOL:
		WRITE_BYTE(w.writebool);
		break;
	case PYTYPE_VECTOR:
	case PYTYPE_QANGLE:
		WRITE_FLOAT(w.writevector[0]);
		WRITE_FLOAT(w.writevector[1]);
		WRITE_FLOAT(w.writevector[2]);
		break;
	case PYTYPE_LIST:
	case PYTYPE_DICT:
	case PYTYPE_TUPLE:
	case PYTYPE_SET:
		WRITE_SHORT(w.writelist.Count());
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyWriteElement( w.writelist[i] );
		break;
	case PYTYPE_NONE:
		break;
	case PYTYPE_HANDLE:
		WRITE_EHANDLE(w.writehandle);
		break;
	}
}

void PyPrintElement( pywrite &w )
{
	switch(w.type)
	{
	case PYTYPE_INT:
		DevMsg("Int: %ld\n", w.writeint);
		break;
	case PYTYPE_FLOAT:
		DevMsg("Float: %f\n", w.writefloat);
		break;
	case PYTYPE_STRING:
		DevMsg("String: %s\n", w.writestr);
		break;
	case PYTYPE_BOOL:
		DevMsg("Bool: %d\n", w.writebool);
		break;
	case PYTYPE_VECTOR:
		DevMsg("Vector: %f %f %f\n", w.writevector[0],
			w.writevector[1],
			w.writevector[2] );
		break;
	case PYTYPE_QANGLE:
		DevMsg("QAngle: %f %f %f\n", w.writevector[0],
			w.writevector[1],
			w.writevector[2] );
		break;
	case PYTYPE_NONE:
		DevMsg("None\n");
		break;
	case PYTYPE_LIST:
	case PYTYPE_DICT:
	case PYTYPE_TUPLE:
	case PYTYPE_SET:
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyPrintElement( w.writelist[i] );
		break;
	case PYTYPE_HANDLE:
		DevMsg("Handle: %d\n", w.writehandle.Get());
		break;
	}
}

ConVar g_debug_pyusermessage("g_debug_pyusermessage", "0", FCVAR_CHEAT|FCVAR_GAMEDLL);

//-----------------------------------------------------------------------------
// Purpose: Sends a user message
//-----------------------------------------------------------------------------
void PySendUserMessage( IRecipientFilter& filter, const char *messagename, boost::python::list msg )
{
	// Skip parsing if none
	if( msg.ptr() == Py_None )
	{
		// Shortcut
		UserMessageBegin( filter, "PyMessage");
		WRITE_STRING(messagename);
		WRITE_BYTE(0);	// len(msg) == 0
		MessageEnd();
		return;
	}

	// Parse list
	int length = 0;
	CUtlVector<pywrite> writelist;
	try {
		length = boost::python::len(msg);
		for(int i=0; i<length; i++)
		{
			pywrite write;
			PyFillWriteElement( write, boost::python::object(msg[i]) );
			writelist.AddToTail(write);
		}
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	UserMessageBegin( filter, "PyMessage");
	WRITE_STRING(messagename);
	WRITE_BYTE(length);
	for(int i=0; i<writelist.Count(); i++)
	{
		PyWriteElement(writelist.Element(i));
	}
	MessageEnd();

	if( g_debug_pyusermessage.GetBool() )
	{
		DevMsg("== PySendUserMessage: Sending message to %s, consisting of %d elements\n", messagename, writelist.Count());
		for(int i=0; i<writelist.Count(); i++)
		{	
			PyPrintElement(writelist.Element(i));
		}
		DevMsg("== End PySendUserMessage\n"); 
	}
}
#else

boost::python::object PyReadElement( bf_read &msg )
{
	int type = msg.ReadByte();
	char buf[255];	// User message is max 255 bytes, so if a string it is likely smaller
	float x, y, z;
	long iEncodedEHandle;
	int iSerialNum, iEntryIndex;
	EHANDLE handle;
	int length;
	boost::python::list embeddedlist;
	boost::python::dict embeddeddict;

	switch(type)
	{
	case PYTYPE_INT:
		return boost::python::object( msg.ReadLong() );
	case PYTYPE_FLOAT:
		return boost::python::object( msg.ReadFloat() );
	case PYTYPE_STRING:
		msg.ReadString(buf, 255);
		return boost::python::object( buf );
	case PYTYPE_BOOL:
		return boost::python::object( msg.ReadByte() );
	case PYTYPE_NONE:
		return boost::python::object();
	case PYTYPE_VECTOR:
		x = msg.ReadFloat();
		y = msg.ReadFloat();
		z = msg.ReadFloat();
		return boost::python::object(Vector(x, y, z));
	case PYTYPE_QANGLE:
		x = msg.ReadFloat();
		y = msg.ReadFloat();
		z = msg.ReadFloat();
		return boost::python::object(QAngle(x, y, z));
	case PYTYPE_HANDLE:
		// Decode handle
		iEncodedEHandle = msg.ReadLong();
		iSerialNum = (iEncodedEHandle >> MAX_EDICT_BITS);
		iEntryIndex = iEncodedEHandle & ~(iSerialNum << MAX_EDICT_BITS);

		// If the entity already exists on te client, return the handle from the entity
		handle = EHANDLE( iEntryIndex, iSerialNum );
		if( handle )
			return handle->GetPyHandle();

		// If it does not exist, create a new handle
		try {																
			return _entities.attr("PyHandle")( iEntryIndex, iSerialNum );							
		} catch(boost::python::error_already_set &) {					
			Warning("Failed to create a PyHandle\n");				
			PyErr_Print();																		
			PyErr_Clear();												
		}	
		return bp::object();
	case PYTYPE_LIST:
		length = msg.ReadShort();
		for( int i = 0; i < length; i++ )
			embeddedlist.append( PyReadElement(msg) );
		return embeddedlist;
	case PYTYPE_TUPLE:
		length = msg.ReadShort();
		for( int i = 0; i < length; i++ )
			embeddedlist.append( PyReadElement(msg) );
		return bp::tuple(embeddedlist);
	case PYTYPE_SET:
		length = msg.ReadShort();
		for( int i = 0; i < length; i++ )
			embeddedlist.append( PyReadElement(msg) );
		return builtins.attr("set")(embeddedlist);
	case PYTYPE_DICT:
		length = msg.ReadShort();
		for( int i = 0; i < length; i++ )
			embeddeddict[PyReadElement(msg)] = PyReadElement(msg);
		return embeddeddict;
	default:
		Warning("PyReadElement: Unknown type %d\n", type);
		break;
	}
	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: Messaging
//-----------------------------------------------------------------------------
void __MsgFunc_PyMessage( bf_read &msg )
{
	//short lookup = msg.ReadShort();
	//const char *mod_name = SrcPySystem()->GetModuleNameFromIndex(lookup);
	char messagename[128];
	msg.ReadString(messagename, 128);
	
	// Read message and add to a list
	// Then call the method function
	int i, length;
	boost::python::list recvlist;
	i = 0;
	length = msg.ReadByte();
	for( i=0; i<length; i++)
	{
		try {
			recvlist.append( PyReadElement(msg) );
		} catch(boost::python::error_already_set &) {
			PyErr_Print();
			PyErr_Clear();
			return;
		}
	}

	SrcPySystem()->Run<const char *, boost::python::object>( SrcPySystem()->Get("_DispatchMessage", "core.usermessages", true ), messagename, recvlist );
}

// register message handler once
void HookPyMessage() 
{
	usermessages->HookMessage( "PyMessage", __MsgFunc_PyMessage );
}

#endif // CLIENT_DLL