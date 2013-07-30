#ifndef _NF_CONNTRACK_TCP_H
#define _NF_CONNTRACK_TCP_H
/* TCP tracking. */

#include <linux/types.h>

/* This is exposed to userspace (ctnetlink) */
enum tcp_conntrack {
	TCP_CONNTRACK_NONE,
	TCP_CONNTRACK_SYN_SENT,
	TCP_CONNTRACK_SYN_RECV,
	TCP_CONNTRACK_ESTABLISHED,
	TCP_CONNTRACK_FIN_WAIT,
	TCP_CONNTRACK_CLOSE_WAIT,
	TCP_CONNTRACK_LAST_ACK,
	TCP_CONNTRACK_TIME_WAIT,
	TCP_CONNTRACK_CLOSE,
	TCP_CONNTRACK_LISTEN,	/* obsolete */
#define TCP_CONNTRACK_SYN_SENT2	TCP_CONNTRACK_LISTEN
	TCP_CONNTRACK_MAX,
	TCP_CONNTRACK_IGNORE
};

/* Window scaling is advertised by the sender */
#define IP_CT_TCP_FLAG_WINDOW_SCALE		0x01

/* SACK is permitted by the sender */
#define IP_CT_TCP_FLAG_SACK_PERM		0x02

/* This sender sent FIN first */
#define IP_CT_TCP_FLAG_CLOSE_INIT		0x04

/* Be liberal in window checking */
#define IP_CT_TCP_FLAG_BE_LIBERAL		0x08

/* Has unacknowledged data */
#define IP_CT_TCP_FLAG_DATA_UNACKNOWLEDGED	0x10

/* The field td_maxack has been set */
#define IP_CT_TCP_FLAG_MAXACK_SET		0x20

struct nf_ct_tcp_flags {
	__u8 flags;
	__u8 mask;
};


#endif /* _NF_CONNTRACK_TCP_H */
