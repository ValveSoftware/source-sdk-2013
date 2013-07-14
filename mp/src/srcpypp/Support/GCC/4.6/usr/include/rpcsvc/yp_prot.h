/*
 * This file contains symbols and structures defining the rpc protocol
 * between the NIS clients and the NIS servers.  The servers
 * are the NIS database servers, and the NIS binders.
 */

#ifndef _RPCSVC_YP_PROT_H
#define _RPCSVC_YP_PROT_H

#include <features.h>

#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>

__BEGIN_DECLS

/*
 * The following procedures are supported by the protocol:
 *
 * YPPROC_NULL() returns () takes nothing, returns nothing.  This indicates
 * that the NIS server is alive.
 *
 * YPPROC_DOMAIN (char *) returns (bool_t) TRUE.  Indicates that the
 * responding NIS server does serve the named domain; FALSE indicates no
 * support.
 *
 * YPPROC_DOMAIN_NONACK (char *) returns (TRUE) if the NIS server does serve
 * the named domain, otherwise does not return.  Used in the broadcast case.
 *
 * YPPROC_MATCH (struct ypreq_key) returns (struct ypresp_val).  Returns the
 * right-hand value for a passed left-hand key, within a named map and
 * domain.
 *
 * YPPROC_FIRST (struct ypreq_nokey) returns (struct ypresp_key_val).
 * Returns the first key-value pair from a named domain and map.
 *
 * YPPROC_NEXT (struct ypreq_key) returns (struct ypresp_key_val).  Returns
 * the key-value pair following a passed key-value pair within a named
 * domain and map.
 *
 * YPPROC_XFR (struct ypreq_xfr) returns nothing.  Indicates to a server that
 * a map should be updated.
 *
 * YPPROC_CLEAR	takes nothing, returns nothing.  Instructs a NIS server to
 * close the current map, so that old versions of the disk file don't get
 * held open.
 *
 * YPPROC_ALL (struct ypreq_nokey), returns
 * 	union switch (bool_t more) {
 *		TRUE:	(struct ypresp_key_val);
 *		FALSE:	(struct) {};
 *	}
 *
 * YPPROC_MASTER (struct ypreq_nokey), returns (ypresp_master)
 *
 * YPPROC_ORDER (struct ypreq_nokey), returns (ypresp_order)
 *
 * YPPROC_MAPLIST (char *), returns (struct ypmaplist *)
 */

/* Program and version symbols, magic numbers */

#define YPPROG		100004
#define YPVERS		2
#define YPVERS_ORIG	1
#define YPMAXRECORD	1024
#define YPMAXDOMAIN	64 /* XXX orig. yp_prot.h defines 256 */
#define YPMAXMAP	64
#define YPMAXPEER	64 /* XXX orig. yp_prot.h defines 256 */

/* byte size of a large NIS packet */
#define YPMSGSZ		1600

typedef struct {
  u_int keydat_len;
  char *keydat_val;
} keydat_t;

typedef struct {
  u_int valdat_len;
  char *valdat_val;
} valdat_t;

struct ypmap_parms {
  char *domain;			/* Null string means not available */
  char *map;			/* Null string means not available */
  unsigned int ordernum;	/* 0 means not available */
  char *owner;			/* Null string means not available */
};

/*
 * Request parameter structures
 */

struct ypreq_key {
  const char *domain;
  const char *map;
  keydat_t keydat;
};

struct ypreq_nokey {
  char *domain;
  char *map;
};

struct ypreq_xfr {
  struct ypmap_parms map_parms;
  u_int transid;
  u_int proto;
  u_int port;
};

#define ypxfr_domain map_parms.domain
#define ypxfr_map map_parms.map
#define ypxfr_ordernum map_parms.ordernum
#define ypxfr_owner map_parms.owner

/* Return status values */

enum ypstat {
  YP_TRUE = 1,		/* General purpose success code */
#define YP_TRUE YP_TRUE
  YP_NOMORE = 2,	/* No more entries in map */
#define YP_NOMORE YP_NOMORE
  YP_FALSE = 0,		/* General purpose failure code */
#define YP_FALSE YP_FALSE
  YP_NOMAP = -1,	/* No such map in domain */
#define YP_NOMAP YP_NOMAP
  YP_NODOM = -2,	/* Domain not supported */
#define YP_NODOM YP_NODOM
  YP_NOKEY = -3,	/* No such key in map */
#define YP_NOKEY YP_NOKEY
  YP_BADOP = -4,	/* Invalid operation */
#define YP_BADOP YP_BADOP
  YP_BADDB = -5,	/* Server data base is bad */
#define YP_BADDB YP_BADDB
  YP_YPERR = -6,	/* NIS server error */
#define YP_YPERR YP_YPERR
  YP_BADARGS = -7,	/* Request arguments bad */
#define YP_BADARGS YP_BADARGS
  YP_VERS = -8,		/* NIS server version mismatch - server can't supply
			   requested service. */
#define YP_VERS YP_VERS
};

/*
 * Response parameter structures
 */

typedef enum ypstat ypstat;

struct ypresp_val {
  ypstat status;
  valdat_t valdat;
};

struct ypresp_key_val {
  ypstat status;
#ifdef STUPID_SUN_BUG
  /* This is the form as distributed by Sun.  But even the Sun NIS
     servers expect the values in the other order.  So their
     implementation somehow must change the order internally.  We
     don't want to follow this bad example since the user should be
     able to use rpcgen on this file.  */
  keydat_t keydat;
  valdat_t valdat;
#else
  valdat_t valdat;
  keydat_t keydat;
#endif
};

struct ypresp_master {
  ypstat status;
  char *master;
};

struct ypresp_order {
  ypstat status;
  u_int ordernum;
};

struct ypmaplist {
  char *map;
#define ypml_name map
  struct ypmaplist *next;
#define ypml_next next
};

struct ypresp_maplist {
  ypstat status;
  struct ypmaplist *list;
};

/*
 * Procedure symbols.  YPPROC_NULL, YPPROC_DOMAIN, and YPPROC_DOMAIN_NONACK
 * must keep the same values (0, 1, and 2) that they had in the first version
 * of the protocol.
 */

#define YPPROC_NULL	0
#define YPPROC_DOMAIN	1
#define YPPROC_DOMAIN_NONACK 2
#define YPPROC_MATCH	3
#define YPPROC_FIRST	4
#define YPPROC_NEXT	5
#define YPPROC_XFR	6
#define YPPROC_CLEAR	7
#define YPPROC_ALL	8
#define YPPROC_MASTER	9
#define YPPROC_ORDER	10
#define YPPROC_MAPLIST	11
#define	YPPROC_NEWXFR	12

/*
 *		Protocol between clients and NIS binder servers
 */

/*
 * The following procedures are supported by the protocol:
 *
 * YPBINDPROC_NULL() returns ()
 * 	takes nothing, returns nothing
 *
 * YPBINDPROC_DOMAIN takes (char *) returns (struct ypbind_resp)
 *
 * YPBINDPROC_SETDOM takes (struct ypbind_setdom) returns nothing
 */

/* Program and version symbols, magic numbers */

#define YPBINDPROG		100007
#define YPBINDVERS		2
#define YPBINDVERS_ORIG		1

/* Procedure symbols */

#define YPBINDPROC_NULL		0
#define YPBINDPROC_DOMAIN	1
#define YPBINDPROC_SETDOM	2
/*
 * Response structure and overall result status codes.  Success and failure
 * represent two separate response message types.
 */

enum ypbind_resptype {YPBIND_SUCC_VAL = 1, YPBIND_FAIL_VAL = 2};

struct ypbind_binding {
  struct in_addr ypbind_binding_addr;	        /* In network order */
  unsigned short int ypbind_binding_port;	/* In network order */
};

struct ypbind_resp {
  enum ypbind_resptype ypbind_status;
  union {
    u_int ypbind_error;
    struct ypbind_binding ypbind_bindinfo;
  } ypbind_respbody;
};


/* Detailed failure reason codes for response field ypbind_error*/

#define YPBIND_ERR_ERR 1		/* Internal error */
#define YPBIND_ERR_NOSERV 2		/* No bound server for passed domain */
#define YPBIND_ERR_RESC 3		/* System resource allocation failure */

/*
 * Request data structure for ypbind "Set domain" procedure.
 */
struct ypbind_setdom {
  char *ypsetdom_domain;
  struct ypbind_binding ypsetdom_binding;
  u_int ypsetdom_vers;
};
#define ypsetdom_addr ypsetdom_binding.ypbind_binding_addr
#define ypsetdom_port ypsetdom_binding.ypbind_binding_port

/*
 *		Protocol between clients (ypxfr, only) and yppush
 *		yppush speaks a protocol in the transient range, which
 *		is supplied to ypxfr as a command-line parameter when it
 *		is activated by ypserv.
 */
#define YPPUSHVERS		1
#define YPPUSHVERS_ORIG		1

/* Procedure symbols */

#define YPPUSHPROC_NULL		0
#define YPPUSHPROC_XFRRESP	1

/* Status values for yppushresp_xfr.status */

enum yppush_status {
  YPPUSH_SUCC = 1,		/* Success */
#define YPPUSH_SUCC	YPPUSH_SUCC
  YPPUSH_AGE = 2,		/* Master's version not newer */
#define YPPUSH_AGE	YPPUSH_AGE
  YPPUSH_NOMAP = -1,		/* Can't find server for map */
#define YPPUSH_NOMAP 	YPPUSH_NOMAP
  YPPUSH_NODOM = -2,		/* Domain not supported */
#define YPPUSH_NODOM 	YPPUSH_NODOM
  YPPUSH_RSRC = -3,		/* Local resouce alloc failure */
#define YPPUSH_RSRC 	YPPUSH_RSRC
  YPPUSH_RPC = -4,		/* RPC failure talking to server */
#define YPPUSH_RPC 	YPPUSH_RPC
  YPPUSH_MADDR = -5,		/* Can't get master address */
#define YPPUSH_MADDR	YPPUSH_MADDR
  YPPUSH_YPERR = -6,		/* NIS server/map db error */
#define YPPUSH_YPERR 	YPPUSH_YPERR
  YPPUSH_BADARGS = -7,		/* Request arguments bad */
#define YPPUSH_BADARGS 	YPPUSH_BADARGS
  YPPUSH_DBM = -8,		/* Local dbm operation failed */
#define YPPUSH_DBM	YPPUSH_DBM
  YPPUSH_FILE = -9,		/* Local file I/O operation failed */
#define YPPUSH_FILE	YPPUSH_FILE
  YPPUSH_SKEW = -10,		/* Map version skew during transfer */
#define YPPUSH_SKEW	YPPUSH_SKEW
  YPPUSH_CLEAR = -11,		/* Can't send "Clear" req to local ypserv */
#define YPPUSH_CLEAR	YPPUSH_CLEAR
  YPPUSH_FORCE = -12,		/* No local order number in map - use -f flag*/
#define YPPUSH_FORCE	YPPUSH_FORCE
  YPPUSH_XFRERR = -13,		/* ypxfr error */
#define YPPUSH_XFRERR	YPPUSH_XFRERR
  YPPUSH_REFUSED = -14,		/* Transfer request refused by ypserv */
#define YPPUSH_REFUSED	YPPUSH_REFUSED
  YPPUSH_NOALIAS = -15		/* Alias not found for map or domain */
#define	YPPUSH_NOALIAS	YPPUSH_NOALIAS
};
typedef enum yppush_status yppush_status;

struct yppushresp_xfr {
  u_int transid;
  yppush_status status;
};

struct ypresp_all {
  bool_t more;
  union {
    struct ypresp_key_val val;
  } ypresp_all_u;
};

extern bool_t xdr_ypreq_key (XDR *__xdrs, struct ypreq_key * __objp);
extern bool_t xdr_ypreq_nokey (XDR *__xdrs, struct ypreq_nokey * __objp);
extern bool_t xdr_ypreq_xfr (XDR *__xdrs, struct ypreq_xfr * __objp);
extern bool_t xdr_ypresp_val (XDR *__xdrs, struct ypresp_val * __objp);
extern bool_t xdr_ypresp_key_val (XDR *__xdrs, struct ypresp_key_val * __objp);
extern bool_t xdr_ypbind_resp (XDR *__xdrs, struct ypbind_resp * __objp);
extern bool_t xdr_ypbind_setdom (XDR *__xdrs, struct ypbind_setdom * __objp);
extern bool_t xdr_ypmap_parms (XDR *__xdrs, struct ypmap_parms * __objp);
extern bool_t xdr_yppushresp_xfr (XDR *__xdrs, struct yppushresp_xfr * __objp);
extern bool_t xdr_ypresp_order (XDR *__xdrs, struct ypresp_order  * __objp);
extern bool_t xdr_ypresp_master (XDR *__xdrs, struct ypresp_master * __objp);
extern bool_t xdr_ypall (XDR *__xdrs, struct ypall_callback * __objp);
extern bool_t xdr_ypresp_maplist (XDR *__xdrs, struct ypresp_maplist * __objp);
extern bool_t xdr_ypbind_binding (XDR *__xdrs, struct ypbind_binding * __objp);
extern bool_t xdr_ypbind_resptype (XDR *__xdrs, enum ypbind_resptype * __objp);
extern bool_t xdr_ypstat (XDR *__xdrs, enum ypbind_resptype * __objp);
extern bool_t xdr_ypresp_all (XDR *__xdrs, struct ypresp_all  * __objp);
extern bool_t xdr_domainname (XDR *__xdrs, char ** __objp);

__END_DECLS

#endif	/* _RPCSVC_YP_PROT_H */
