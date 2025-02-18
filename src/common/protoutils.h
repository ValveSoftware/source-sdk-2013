//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef VALVE_PROTO_UTILS_H
#define VALVE_PROTO_UTILS_H
#ifdef _WIN32
#pragma once
#endif

#include <initializer_list>

namespace google { namespace protobuf { class Message; class Descriptor; }; };

namespace ValveProtoUtils {
	// Allows you to assert a message messages this field list for code that should be checked upon message changes
	bool MessageHasExactFields( const google::protobuf::Descriptor &desc, std::initializer_list<int> fields );
	bool MessageHasExactFields( const google::protobuf::Message &msg, std::initializer_list<int> fields );
};

#endif // VALVE_PROTO_UTILS_H
