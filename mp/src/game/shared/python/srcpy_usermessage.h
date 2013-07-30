//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Python usermessages
//
//=============================================================================//

#ifndef SRC_PYTHON_USERMESSAGE_H
#define SRC_PYTHON_USERMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#ifndef CLIENT_DLL
	typedef struct pywrite {
		pywrite() {}
		pywrite(const pywrite &w) 
		{
			memcpy(this, &w, sizeof(pywrite)-sizeof(CUtlVector< struct pywrite >));
			writelist = w.writelist;
		}

		char			type;
		union
		{
			int				writeint;
			float			writefloat;
			const char *	writestr;
			bool			writebool;
		};

		Vector			writevector;
		//QAngle			writeangle;
		EHANDLE			writehandle;
		CUtlVector< struct pywrite > writelist;
	} pywrite;

	void PyFillWriteElement( pywrite &w, boost::python::object data );
	void PyWriteElement( pywrite &w );
	void PyPrintElement( pywrite &w );
	void PySendUserMessage( IRecipientFilter& filter, const char *messagename, boost::python::list msg );
#else
	boost::python::object PyReadElement( bf_read &msg );
	void HookPyMessage();
#endif // CLIENT_DLL

#endif // SRC_PYTHON_USERMESSAGE_H