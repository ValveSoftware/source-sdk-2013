#ifndef _IP_SET_H
#define _IP_SET_H

/* Copyright (C) 2000-2002 Joakim Axelsson <gozem@linux.nu>
 *                         Patrick Schaaf <bof@bof.de>
 *                         Martin Josefsson <gandalf@wlug.westbo.se>
 * Copyright (C) 2003-2011 Jozsef Kadlecsik <kadlec@blackhole.kfki.hu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* The protocol version */
#define IPSET_PROTOCOL		6

/* The max length of strings including NUL: set and type identifiers */
#define IPSET_MAXNAMELEN	32

/* Message types and commands */
enum ipset_cmd {
	IPSET_CMD_NONE,
	IPSET_CMD_PROTOCOL,	/* 1: Return protocol version */
	IPSET_CMD_CREATE,	/* 2: Create a new (empty) set */
	IPSET_CMD_DESTROY,	/* 3: Destroy a (empty) set */
	IPSET_CMD_FLUSH,	/* 4: Remove all elements from a set */
	IPSET_CMD_RENAME,	/* 5: Rename a set */
	IPSET_CMD_SWAP,		/* 6: Swap two sets */
	IPSET_CMD_LIST,		/* 7: List sets */
	IPSET_CMD_SAVE,		/* 8: Save sets */
	IPSET_CMD_ADD,		/* 9: Add an element to a set */
	IPSET_CMD_DEL,		/* 10: Delete an element from a set */
	IPSET_CMD_TEST,		/* 11: Test an element in a set */
	IPSET_CMD_HEADER,	/* 12: Get set header data only */
	IPSET_CMD_TYPE,		/* 13: Get set type */
	IPSET_MSG_MAX,		/* Netlink message commands */

	/* Commands in userspace: */
	IPSET_CMD_RESTORE = IPSET_MSG_MAX, /* 14: Enter restore mode */
	IPSET_CMD_HELP,		/* 15: Get help */
	IPSET_CMD_VERSION,	/* 16: Get program version */
	IPSET_CMD_QUIT,		/* 17: Quit from interactive mode */

	IPSET_CMD_MAX,

	IPSET_CMD_COMMIT = IPSET_CMD_MAX, /* 18: Commit buffered commands */
};

/* Attributes at command level */
enum {
	IPSET_ATTR_UNSPEC,
	IPSET_ATTR_PROTOCOL,	/* 1: Protocol version */
	IPSET_ATTR_SETNAME,	/* 2: Name of the set */
	IPSET_ATTR_TYPENAME,	/* 3: Typename */
	IPSET_ATTR_SETNAME2 = IPSET_ATTR_TYPENAME, /* Setname at rename/swap */
	IPSET_ATTR_REVISION,	/* 4: Settype revision */
	IPSET_ATTR_FAMILY,	/* 5: Settype family */
	IPSET_ATTR_FLAGS,	/* 6: Flags at command level */
	IPSET_ATTR_DATA,	/* 7: Nested attributes */
	IPSET_ATTR_ADT,		/* 8: Multiple data containers */
	IPSET_ATTR_LINENO,	/* 9: Restore lineno */
	IPSET_ATTR_PROTOCOL_MIN, /* 10: Minimal supported version number */
	IPSET_ATTR_REVISION_MIN	= IPSET_ATTR_PROTOCOL_MIN, /* type rev min */
	__IPSET_ATTR_CMD_MAX,
};
#define IPSET_ATTR_CMD_MAX	(__IPSET_ATTR_CMD_MAX - 1)

/* CADT specific attributes */
enum {
	IPSET_ATTR_IP = IPSET_ATTR_UNSPEC + 1,
	IPSET_ATTR_IP_FROM = IPSET_ATTR_IP,
	IPSET_ATTR_IP_TO,	/* 2 */
	IPSET_ATTR_CIDR,	/* 3 */
	IPSET_ATTR_PORT,	/* 4 */
	IPSET_ATTR_PORT_FROM = IPSET_ATTR_PORT,
	IPSET_ATTR_PORT_TO,	/* 5 */
	IPSET_ATTR_TIMEOUT,	/* 6 */
	IPSET_ATTR_PROTO,	/* 7 */
	IPSET_ATTR_CADT_FLAGS,	/* 8 */
	IPSET_ATTR_CADT_LINENO = IPSET_ATTR_LINENO,	/* 9 */
	/* Reserve empty slots */
	IPSET_ATTR_CADT_MAX = 16,
	/* Create-only specific attributes */
	IPSET_ATTR_GC,
	IPSET_ATTR_HASHSIZE,
	IPSET_ATTR_MAXELEM,
	IPSET_ATTR_NETMASK,
	IPSET_ATTR_PROBES,
	IPSET_ATTR_RESIZE,
	IPSET_ATTR_SIZE,
	/* Kernel-only */
	IPSET_ATTR_ELEMENTS,
	IPSET_ATTR_REFERENCES,
	IPSET_ATTR_MEMSIZE,

	__IPSET_ATTR_CREATE_MAX,
};
#define IPSET_ATTR_CREATE_MAX	(__IPSET_ATTR_CREATE_MAX - 1)

/* ADT specific attributes */
enum {
	IPSET_ATTR_ETHER = IPSET_ATTR_CADT_MAX + 1,
	IPSET_ATTR_NAME,
	IPSET_ATTR_NAMEREF,
	IPSET_ATTR_IP2,
	IPSET_ATTR_CIDR2,
	IPSET_ATTR_IP2_TO,
	IPSET_ATTR_IFACE,
	__IPSET_ATTR_ADT_MAX,
};
#define IPSET_ATTR_ADT_MAX	(__IPSET_ATTR_ADT_MAX - 1)

/* IP specific attributes */
enum {
	IPSET_ATTR_IPADDR_IPV4 = IPSET_ATTR_UNSPEC + 1,
	IPSET_ATTR_IPADDR_IPV6,
	__IPSET_ATTR_IPADDR_MAX,
};
#define IPSET_ATTR_IPADDR_MAX	(__IPSET_ATTR_IPADDR_MAX - 1)

/* Error codes */
enum ipset_errno {
	IPSET_ERR_PRIVATE = 4096,
	IPSET_ERR_PROTOCOL,
	IPSET_ERR_FIND_TYPE,
	IPSET_ERR_MAX_SETS,
	IPSET_ERR_BUSY,
	IPSET_ERR_EXIST_SETNAME2,
	IPSET_ERR_TYPE_MISMATCH,
	IPSET_ERR_EXIST,
	IPSET_ERR_INVALID_CIDR,
	IPSET_ERR_INVALID_NETMASK,
	IPSET_ERR_INVALID_FAMILY,
	IPSET_ERR_TIMEOUT,
	IPSET_ERR_REFERENCED,
	IPSET_ERR_IPADDR_IPV4,
	IPSET_ERR_IPADDR_IPV6,

	/* Type specific error codes */
	IPSET_ERR_TYPE_SPECIFIC = 4352,
};

/* Flags at command level */
enum ipset_cmd_flags {
	IPSET_FLAG_BIT_EXIST	= 0,
	IPSET_FLAG_EXIST	= (1 << IPSET_FLAG_BIT_EXIST),
	IPSET_FLAG_BIT_LIST_SETNAME = 1,
	IPSET_FLAG_LIST_SETNAME	= (1 << IPSET_FLAG_BIT_LIST_SETNAME),
	IPSET_FLAG_BIT_LIST_HEADER = 2,
	IPSET_FLAG_LIST_HEADER	= (1 << IPSET_FLAG_BIT_LIST_HEADER),
};

/* Flags at CADT attribute level */
enum ipset_cadt_flags {
	IPSET_FLAG_BIT_BEFORE	= 0,
	IPSET_FLAG_BEFORE	= (1 << IPSET_FLAG_BIT_BEFORE),
	IPSET_FLAG_BIT_PHYSDEV	= 1,
	IPSET_FLAG_PHYSDEV	= (1 << IPSET_FLAG_BIT_PHYSDEV),
};

/* Commands with settype-specific attributes */
enum ipset_adt {
	IPSET_ADD,
	IPSET_DEL,
	IPSET_TEST,
	IPSET_ADT_MAX,
	IPSET_CREATE = IPSET_ADT_MAX,
	IPSET_CADT_MAX,
};


#endif /*_IP_SET_H */
