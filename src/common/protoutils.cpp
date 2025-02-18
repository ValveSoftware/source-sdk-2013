//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "protoutils.h"

#include "google/protobuf/message.h"

//-----------------------------------------------------------------------------
bool ValveProtoUtils::MessageHasExactFields( const google::protobuf::Message &msg,
                                             std::initializer_list<int> fields )
{
	auto &desc = *msg.GetDescriptor();
	return ValveProtoUtils::MessageHasExactFields( desc, std::move( fields ) );
}

//-----------------------------------------------------------------------------
bool ValveProtoUtils::MessageHasExactFields( const google::protobuf::Descriptor &msgDesc,
                                             std::initializer_list<int> fields )
{
	int nFields = msgDesc.field_count();
	if ( nFields != (int)fields.size() )
		{ return false; }

	for ( int field : fields )
	{
		if ( msgDesc.FindFieldByNumber( field ) == nullptr )
			{ return false; }
	}

	return true;
}
