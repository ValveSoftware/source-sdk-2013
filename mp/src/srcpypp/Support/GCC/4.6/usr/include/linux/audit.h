/* audit.h -- Auditing support
 *
 * Copyright 2003-2004 Red Hat Inc., Durham, North Carolina.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Written by Rickard E. (Rik) Faith <faith@redhat.com>
 *
 */

#ifndef _LINUX_AUDIT_H_
#define _LINUX_AUDIT_H_

#include <linux/types.h>
#include <linux/elf-em.h>

/* The netlink messages for the audit system is divided into blocks:
 * 1000 - 1099 are for commanding the audit system
 * 1100 - 1199 user space trusted application messages
 * 1200 - 1299 messages internal to the audit daemon
 * 1300 - 1399 audit event messages
 * 1400 - 1499 SE Linux use
 * 1500 - 1599 kernel LSPP events
 * 1600 - 1699 kernel crypto events
 * 1700 - 1799 kernel anomaly records
 * 1800 - 1899 kernel integrity events
 * 1900 - 1999 future kernel use
 * 2000 is for otherwise unclassified kernel audit messages (legacy)
 * 2001 - 2099 unused (kernel)
 * 2100 - 2199 user space anomaly records
 * 2200 - 2299 user space actions taken in response to anomalies
 * 2300 - 2399 user space generated LSPP events
 * 2400 - 2499 user space crypto events
 * 2500 - 2999 future user space (maybe integrity labels and related events)
 *
 * Messages from 1000-1199 are bi-directional. 1200-1299 & 2100 - 2999 are
 * exclusively user space. 1300-2099 is kernel --> user space 
 * communication.
 */
#define AUDIT_GET		1000	/* Get status */
#define AUDIT_SET		1001	/* Set status (enable/disable/auditd) */
#define AUDIT_LIST		1002	/* List syscall rules -- deprecated */
#define AUDIT_ADD		1003	/* Add syscall rule -- deprecated */
#define AUDIT_DEL		1004	/* Delete syscall rule -- deprecated */
#define AUDIT_USER		1005	/* Message from userspace -- deprecated */
#define AUDIT_LOGIN		1006	/* Define the login id and information */
#define AUDIT_WATCH_INS		1007	/* Insert file/dir watch entry */
#define AUDIT_WATCH_REM		1008	/* Remove file/dir watch entry */
#define AUDIT_WATCH_LIST	1009	/* List all file/dir watches */
#define AUDIT_SIGNAL_INFO	1010	/* Get info about sender of signal to auditd */
#define AUDIT_ADD_RULE		1011	/* Add syscall filtering rule */
#define AUDIT_DEL_RULE		1012	/* Delete syscall filtering rule */
#define AUDIT_LIST_RULES	1013	/* List syscall filtering rules */
#define AUDIT_TRIM		1014	/* Trim junk from watched tree */
#define AUDIT_MAKE_EQUIV	1015	/* Append to watched tree */
#define AUDIT_TTY_GET		1016	/* Get TTY auditing status */
#define AUDIT_TTY_SET		1017	/* Set TTY auditing status */

#define AUDIT_FIRST_USER_MSG	1100	/* Userspace messages mostly uninteresting to kernel */
#define AUDIT_USER_AVC		1107	/* We filter this differently */
#define AUDIT_USER_TTY		1124	/* Non-ICANON TTY input meaning */
#define AUDIT_LAST_USER_MSG	1199
#define AUDIT_FIRST_USER_MSG2	2100	/* More user space messages */
#define AUDIT_LAST_USER_MSG2	2999
 
#define AUDIT_DAEMON_START      1200    /* Daemon startup record */
#define AUDIT_DAEMON_END        1201    /* Daemon normal stop record */
#define AUDIT_DAEMON_ABORT      1202    /* Daemon error stop record */
#define AUDIT_DAEMON_CONFIG     1203    /* Daemon config change */

#define AUDIT_SYSCALL		1300	/* Syscall event */
/* #define AUDIT_FS_WATCH	1301	 * Deprecated */
#define AUDIT_PATH		1302	/* Filename path information */
#define AUDIT_IPC		1303	/* IPC record */
#define AUDIT_SOCKETCALL	1304	/* sys_socketcall arguments */
#define AUDIT_CONFIG_CHANGE	1305	/* Audit system configuration change */
#define AUDIT_SOCKADDR		1306	/* sockaddr copied as syscall arg */
#define AUDIT_CWD		1307	/* Current working directory */
#define AUDIT_EXECVE		1309	/* execve arguments */
#define AUDIT_IPC_SET_PERM	1311	/* IPC new permissions record type */
#define AUDIT_MQ_OPEN		1312	/* POSIX MQ open record type */
#define AUDIT_MQ_SENDRECV	1313	/* POSIX MQ send/receive record type */
#define AUDIT_MQ_NOTIFY		1314	/* POSIX MQ notify record type */
#define AUDIT_MQ_GETSETATTR	1315	/* POSIX MQ get/set attribute record type */
#define AUDIT_KERNEL_OTHER	1316	/* For use by 3rd party modules */
#define AUDIT_FD_PAIR		1317    /* audit record for pipe/socketpair */
#define AUDIT_OBJ_PID		1318	/* ptrace target */
#define AUDIT_TTY		1319	/* Input on an administrative TTY */
#define AUDIT_EOE		1320	/* End of multi-record event */
#define AUDIT_BPRM_FCAPS	1321	/* Information about fcaps increasing perms */
#define AUDIT_CAPSET		1322	/* Record showing argument to sys_capset */
#define AUDIT_MMAP		1323	/* Record showing descriptor and flags in mmap */
#define AUDIT_NETFILTER_PKT	1324	/* Packets traversing netfilter chains */
#define AUDIT_NETFILTER_CFG	1325	/* Netfilter chain modifications */

#define AUDIT_AVC		1400	/* SE Linux avc denial or grant */
#define AUDIT_SELINUX_ERR	1401	/* Internal SE Linux Errors */
#define AUDIT_AVC_PATH		1402	/* dentry, vfsmount pair from avc */
#define AUDIT_MAC_POLICY_LOAD	1403	/* Policy file load */
#define AUDIT_MAC_STATUS	1404	/* Changed enforcing,permissive,off */
#define AUDIT_MAC_CONFIG_CHANGE	1405	/* Changes to booleans */
#define AUDIT_MAC_UNLBL_ALLOW	1406	/* NetLabel: allow unlabeled traffic */
#define AUDIT_MAC_CIPSOV4_ADD	1407	/* NetLabel: add CIPSOv4 DOI entry */
#define AUDIT_MAC_CIPSOV4_DEL	1408	/* NetLabel: del CIPSOv4 DOI entry */
#define AUDIT_MAC_MAP_ADD	1409	/* NetLabel: add LSM domain mapping */
#define AUDIT_MAC_MAP_DEL	1410	/* NetLabel: del LSM domain mapping */
#define AUDIT_MAC_IPSEC_ADDSA	1411	/* Not used */
#define AUDIT_MAC_IPSEC_DELSA	1412	/* Not used  */
#define AUDIT_MAC_IPSEC_ADDSPD	1413	/* Not used */
#define AUDIT_MAC_IPSEC_DELSPD	1414	/* Not used */
#define AUDIT_MAC_IPSEC_EVENT	1415	/* Audit an IPSec event */
#define AUDIT_MAC_UNLBL_STCADD	1416	/* NetLabel: add a static label */
#define AUDIT_MAC_UNLBL_STCDEL	1417	/* NetLabel: del a static label */

#define AUDIT_FIRST_KERN_ANOM_MSG   1700
#define AUDIT_LAST_KERN_ANOM_MSG    1799
#define AUDIT_ANOM_PROMISCUOUS      1700 /* Device changed promiscuous mode */
#define AUDIT_ANOM_ABEND            1701 /* Process ended abnormally */
#define AUDIT_INTEGRITY_DATA	    1800 /* Data integrity verification */
#define AUDIT_INTEGRITY_METADATA    1801 /* Metadata integrity verification */
#define AUDIT_INTEGRITY_STATUS	    1802 /* Integrity enable status */
#define AUDIT_INTEGRITY_HASH	    1803 /* Integrity HASH type */
#define AUDIT_INTEGRITY_PCR	    1804 /* PCR invalidation msgs */
#define AUDIT_INTEGRITY_RULE	    1805 /* policy rule */

#define AUDIT_KERNEL		2000	/* Asynchronous audit record. NOT A REQUEST. */

/* Rule flags */
#define AUDIT_FILTER_USER	0x00	/* Apply rule to user-generated messages */
#define AUDIT_FILTER_TASK	0x01	/* Apply rule at task creation (not syscall) */
#define AUDIT_FILTER_ENTRY	0x02	/* Apply rule at syscall entry */
#define AUDIT_FILTER_WATCH	0x03	/* Apply rule to file system watches */
#define AUDIT_FILTER_EXIT	0x04	/* Apply rule at syscall exit */
#define AUDIT_FILTER_TYPE	0x05	/* Apply rule at audit_log_start */

#define AUDIT_NR_FILTERS	6

#define AUDIT_FILTER_PREPEND	0x10	/* Prepend to front of list */

/* Rule actions */
#define AUDIT_NEVER    0	/* Do not build context if rule matches */
#define AUDIT_POSSIBLE 1	/* Build context if rule matches  */
#define AUDIT_ALWAYS   2	/* Generate audit record if rule matches */

/* Rule structure sizes -- if these change, different AUDIT_ADD and
 * AUDIT_LIST commands must be implemented. */
#define AUDIT_MAX_FIELDS   64
#define AUDIT_MAX_KEY_LEN  256
#define AUDIT_BITMASK_SIZE 64
#define AUDIT_WORD(nr) ((__u32)((nr)/32))
#define AUDIT_BIT(nr)  (1 << ((nr) - AUDIT_WORD(nr)*32))

#define AUDIT_SYSCALL_CLASSES 16
#define AUDIT_CLASS_DIR_WRITE 0
#define AUDIT_CLASS_DIR_WRITE_32 1
#define AUDIT_CLASS_CHATTR 2
#define AUDIT_CLASS_CHATTR_32 3
#define AUDIT_CLASS_READ 4
#define AUDIT_CLASS_READ_32 5
#define AUDIT_CLASS_WRITE 6
#define AUDIT_CLASS_WRITE_32 7
#define AUDIT_CLASS_SIGNAL 8
#define AUDIT_CLASS_SIGNAL_32 9

/* This bitmask is used to validate user input.  It represents all bits that
 * are currently used in an audit field constant understood by the kernel.
 * If you are adding a new #define AUDIT_<whatever>, please ensure that
 * AUDIT_UNUSED_BITS is updated if need be. */
#define AUDIT_UNUSED_BITS	0x07FFFC00


/* Rule fields */
				/* These are useful when checking the
				 * task structure at task creation time
				 * (AUDIT_PER_TASK).  */
#define AUDIT_PID	0
#define AUDIT_UID	1
#define AUDIT_EUID	2
#define AUDIT_SUID	3
#define AUDIT_FSUID	4
#define AUDIT_GID	5
#define AUDIT_EGID	6
#define AUDIT_SGID	7
#define AUDIT_FSGID	8
#define AUDIT_LOGINUID	9
#define AUDIT_PERS	10
#define AUDIT_ARCH	11
#define AUDIT_MSGTYPE	12
#define AUDIT_SUBJ_USER	13	/* security label user */
#define AUDIT_SUBJ_ROLE	14	/* security label role */
#define AUDIT_SUBJ_TYPE	15	/* security label type */
#define AUDIT_SUBJ_SEN	16	/* security label sensitivity label */
#define AUDIT_SUBJ_CLR	17	/* security label clearance label */
#define AUDIT_PPID	18
#define AUDIT_OBJ_USER	19
#define AUDIT_OBJ_ROLE	20
#define AUDIT_OBJ_TYPE	21
#define AUDIT_OBJ_LEV_LOW	22
#define AUDIT_OBJ_LEV_HIGH	23

				/* These are ONLY useful when checking
				 * at syscall exit time (AUDIT_AT_EXIT). */
#define AUDIT_DEVMAJOR	100
#define AUDIT_DEVMINOR	101
#define AUDIT_INODE	102
#define AUDIT_EXIT	103
#define AUDIT_SUCCESS   104	/* exit >= 0; value ignored */
#define AUDIT_WATCH	105
#define AUDIT_PERM	106
#define AUDIT_DIR	107
#define AUDIT_FILETYPE	108

#define AUDIT_ARG0      200
#define AUDIT_ARG1      (AUDIT_ARG0+1)
#define AUDIT_ARG2      (AUDIT_ARG0+2)
#define AUDIT_ARG3      (AUDIT_ARG0+3)

#define AUDIT_FILTERKEY	210

#define AUDIT_NEGATE			0x80000000

/* These are the supported operators.
 *	4  2  1  8
 *	=  >  <  ?
 *	----------
 *	0  0  0	 0	00	nonsense
 *	0  0  0	 1	08	&  bit mask
 *	0  0  1	 0	10	<
 *	0  1  0	 0	20	>
 *	0  1  1	 0	30	!=
 *	1  0  0	 0	40	=
 *	1  0  0	 1	48	&=  bit test
 *	1  0  1	 0	50	<=
 *	1  1  0	 0	60	>=
 *	1  1  1	 1	78	all operators
 */
#define AUDIT_BIT_MASK			0x08000000
#define AUDIT_LESS_THAN			0x10000000
#define AUDIT_GREATER_THAN		0x20000000
#define AUDIT_NOT_EQUAL			0x30000000
#define AUDIT_EQUAL			0x40000000
#define AUDIT_BIT_TEST			(AUDIT_BIT_MASK|AUDIT_EQUAL)
#define AUDIT_LESS_THAN_OR_EQUAL	(AUDIT_LESS_THAN|AUDIT_EQUAL)
#define AUDIT_GREATER_THAN_OR_EQUAL	(AUDIT_GREATER_THAN|AUDIT_EQUAL)
#define AUDIT_OPERATORS			(AUDIT_EQUAL|AUDIT_NOT_EQUAL|AUDIT_BIT_MASK)

enum {
	Audit_equal,
	Audit_not_equal,
	Audit_bitmask,
	Audit_bittest,
	Audit_lt,
	Audit_gt,
	Audit_le,
	Audit_ge,
	Audit_bad
};

/* Status symbols */
				/* Mask values */
#define AUDIT_STATUS_ENABLED		0x0001
#define AUDIT_STATUS_FAILURE		0x0002
#define AUDIT_STATUS_PID		0x0004
#define AUDIT_STATUS_RATE_LIMIT		0x0008
#define AUDIT_STATUS_BACKLOG_LIMIT	0x0010
				/* Failure-to-log actions */
#define AUDIT_FAIL_SILENT	0
#define AUDIT_FAIL_PRINTK	1
#define AUDIT_FAIL_PANIC	2

/* distinguish syscall tables */
#define __AUDIT_ARCH_64BIT 0x80000000
#define __AUDIT_ARCH_LE	   0x40000000
#define AUDIT_ARCH_ALPHA	(EM_ALPHA|__AUDIT_ARCH_64BIT|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_ARM		(EM_ARM|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_ARMEB	(EM_ARM)
#define AUDIT_ARCH_CRIS		(EM_CRIS|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_FRV		(EM_FRV)
#define AUDIT_ARCH_H8300	(EM_H8_300)
#define AUDIT_ARCH_I386		(EM_386|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_IA64		(EM_IA_64|__AUDIT_ARCH_64BIT|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_M32R		(EM_M32R)
#define AUDIT_ARCH_M68K		(EM_68K)
#define AUDIT_ARCH_MIPS		(EM_MIPS)
#define AUDIT_ARCH_MIPSEL	(EM_MIPS|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_MIPS64	(EM_MIPS|__AUDIT_ARCH_64BIT)
#define AUDIT_ARCH_MIPSEL64	(EM_MIPS|__AUDIT_ARCH_64BIT|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_PARISC	(EM_PARISC)
#define AUDIT_ARCH_PARISC64	(EM_PARISC|__AUDIT_ARCH_64BIT)
#define AUDIT_ARCH_PPC		(EM_PPC)
#define AUDIT_ARCH_PPC64	(EM_PPC64|__AUDIT_ARCH_64BIT)
#define AUDIT_ARCH_S390		(EM_S390)
#define AUDIT_ARCH_S390X	(EM_S390|__AUDIT_ARCH_64BIT)
#define AUDIT_ARCH_SH		(EM_SH)
#define AUDIT_ARCH_SHEL		(EM_SH|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_SH64		(EM_SH|__AUDIT_ARCH_64BIT)
#define AUDIT_ARCH_SHEL64	(EM_SH|__AUDIT_ARCH_64BIT|__AUDIT_ARCH_LE)
#define AUDIT_ARCH_SPARC	(EM_SPARC)
#define AUDIT_ARCH_SPARC64	(EM_SPARCV9|__AUDIT_ARCH_64BIT)
#define AUDIT_ARCH_X86_64	(EM_X86_64|__AUDIT_ARCH_64BIT|__AUDIT_ARCH_LE)

#define AUDIT_PERM_EXEC		1
#define AUDIT_PERM_WRITE	2
#define AUDIT_PERM_READ		4
#define AUDIT_PERM_ATTR		8

struct audit_status {
	__u32		mask;		/* Bit mask for valid entries */
	__u32		enabled;	/* 1 = enabled, 0 = disabled */
	__u32		failure;	/* Failure-to-log action */
	__u32		pid;		/* pid of auditd process */
	__u32		rate_limit;	/* messages rate limit (per second) */
	__u32		backlog_limit;	/* waiting messages limit */
	__u32		lost;		/* messages lost */
	__u32		backlog;	/* messages waiting in queue */
};

struct audit_tty_status {
	__u32		enabled; /* 1 = enabled, 0 = disabled */
};

/* audit_rule_data supports filter rules with both integer and string
 * fields.  It corresponds with AUDIT_ADD_RULE, AUDIT_DEL_RULE and
 * AUDIT_LIST_RULES requests.
 */
struct audit_rule_data {
	__u32		flags;	/* AUDIT_PER_{TASK,CALL}, AUDIT_PREPEND */
	__u32		action;	/* AUDIT_NEVER, AUDIT_POSSIBLE, AUDIT_ALWAYS */
	__u32		field_count;
	__u32		mask[AUDIT_BITMASK_SIZE]; /* syscall(s) affected */
	__u32		fields[AUDIT_MAX_FIELDS];
	__u32		values[AUDIT_MAX_FIELDS];
	__u32		fieldflags[AUDIT_MAX_FIELDS];
	__u32		buflen;	/* total length of string fields */
	char		buf[0];	/* string fields buffer */
};

/* audit_rule is supported to maintain backward compatibility with
 * userspace.  It supports integer fields only and corresponds to
 * AUDIT_ADD, AUDIT_DEL and AUDIT_LIST requests.
 */
struct audit_rule {		/* for AUDIT_LIST, AUDIT_ADD, and AUDIT_DEL */
	__u32		flags;	/* AUDIT_PER_{TASK,CALL}, AUDIT_PREPEND */
	__u32		action;	/* AUDIT_NEVER, AUDIT_POSSIBLE, AUDIT_ALWAYS */
	__u32		field_count;
	__u32		mask[AUDIT_BITMASK_SIZE];
	__u32		fields[AUDIT_MAX_FIELDS];
	__u32		values[AUDIT_MAX_FIELDS];
};

#endif
