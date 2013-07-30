/* SSL socket module

   SSL support based on patches by Brian E Gallew and Laszlo Kovacs.
   Re-worked a bit by Bill Janssen to add server-side support and
   certificate decoding.  Chris Stawarz contributed some non-blocking
   patches.

   This module is imported by ssl.py. It should *not* be used
   directly.

   XXX should partial writes be enabled, SSL_MODE_ENABLE_PARTIAL_WRITE?

   XXX integrate several "shutdown modes" as suggested in
       http://bugs.python.org/issue8108#msg102867 ?
*/

#include "Python.h"

#ifdef WITH_THREAD
#include "pythread.h"
#define PySSL_BEGIN_ALLOW_THREADS_S(save) \
    do { if (_ssl_locks_count>0) { (save) = PyEval_SaveThread(); } } while (0)
#define PySSL_END_ALLOW_THREADS_S(save) \
    do { if (_ssl_locks_count>0) { PyEval_RestoreThread(save); } } while (0)
#define PySSL_BEGIN_ALLOW_THREADS { \
            PyThreadState *_save = NULL;  \
            PySSL_BEGIN_ALLOW_THREADS_S(_save);
#define PySSL_BLOCK_THREADS     PySSL_END_ALLOW_THREADS_S(_save);
#define PySSL_UNBLOCK_THREADS   PySSL_BEGIN_ALLOW_THREADS_S(_save);
#define PySSL_END_ALLOW_THREADS PySSL_END_ALLOW_THREADS_S(_save); }

#else   /* no WITH_THREAD */

#define PySSL_BEGIN_ALLOW_THREADS_S(save)
#define PySSL_END_ALLOW_THREADS_S(save)
#define PySSL_BEGIN_ALLOW_THREADS
#define PySSL_BLOCK_THREADS
#define PySSL_UNBLOCK_THREADS
#define PySSL_END_ALLOW_THREADS

#endif

enum py_ssl_error {
    /* these mirror ssl.h */
    PY_SSL_ERROR_NONE,
    PY_SSL_ERROR_SSL,
    PY_SSL_ERROR_WANT_READ,
    PY_SSL_ERROR_WANT_WRITE,
    PY_SSL_ERROR_WANT_X509_LOOKUP,
    PY_SSL_ERROR_SYSCALL,     /* look at error stack/return value/errno */
    PY_SSL_ERROR_ZERO_RETURN,
    PY_SSL_ERROR_WANT_CONNECT,
    /* start of non ssl.h errorcodes */
    PY_SSL_ERROR_EOF,         /* special case of SSL_ERROR_SYSCALL */
    PY_SSL_ERROR_NO_SOCKET,   /* socket has been GC'd */
    PY_SSL_ERROR_INVALID_ERROR_CODE
};

enum py_ssl_server_or_client {
    PY_SSL_CLIENT,
    PY_SSL_SERVER
};

enum py_ssl_cert_requirements {
    PY_SSL_CERT_NONE,
    PY_SSL_CERT_OPTIONAL,
    PY_SSL_CERT_REQUIRED
};

enum py_ssl_version {
#ifndef OPENSSL_NO_SSL2
    PY_SSL_VERSION_SSL2,
#endif
    PY_SSL_VERSION_SSL3=1,
    PY_SSL_VERSION_SSL23,
    PY_SSL_VERSION_TLS1
};

struct py_ssl_error_code {
    const char *mnemonic;
    int library, reason;
};

struct py_ssl_library_code {
    const char *library;
    int code;
};

/* Include symbols from _socket module */
#include "socketmodule.h"

static PySocketModule_APIObject PySocketModule;

#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

/* Include OpenSSL header files */
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/x509v3.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"

/* Include generated data (error codes) */
#include "_ssl_data.h"

/* SSL error object */
static PyObject *PySSLErrorObject;
static PyObject *PySSLZeroReturnErrorObject;
static PyObject *PySSLWantReadErrorObject;
static PyObject *PySSLWantWriteErrorObject;
static PyObject *PySSLSyscallErrorObject;
static PyObject *PySSLEOFErrorObject;

/* Error mappings */
static PyObject *err_codes_to_names;
static PyObject *err_names_to_codes;
static PyObject *lib_codes_to_names;

#ifdef WITH_THREAD

/* serves as a flag to see whether we've initialized the SSL thread support. */
/* 0 means no, greater than 0 means yes */

static unsigned int _ssl_locks_count = 0;

#endif /* def WITH_THREAD */

/* SSL socket object */

#define X509_NAME_MAXLEN 256

/* RAND_* APIs got added to OpenSSL in 0.9.5 */
#if OPENSSL_VERSION_NUMBER >= 0x0090500fL
# define HAVE_OPENSSL_RAND 1
#else
# undef HAVE_OPENSSL_RAND
#endif

/* SSL_CTX_clear_options() and SSL_clear_options() were first added in
 * OpenSSL 0.9.8m but do not appear in some 0.9.9-dev versions such the
 * 0.9.9 from "May 2008" that NetBSD 5.0 uses. */
#if OPENSSL_VERSION_NUMBER >= 0x009080dfL && OPENSSL_VERSION_NUMBER != 0x00909000L
# define HAVE_SSL_CTX_CLEAR_OPTIONS
#else
# undef HAVE_SSL_CTX_CLEAR_OPTIONS
#endif

/* In case of 'tls-unique' it will be 12 bytes for TLS, 36 bytes for
 * older SSL, but let's be safe */
#define PySSL_CB_MAXLEN 128

/* SSL_get_finished got added to OpenSSL in 0.9.5 */
#if OPENSSL_VERSION_NUMBER >= 0x0090500fL
# define HAVE_OPENSSL_FINISHED 1
#else
# define HAVE_OPENSSL_FINISHED 0
#endif

/* ECDH support got added to OpenSSL in 0.9.8 */
#if OPENSSL_VERSION_NUMBER < 0x0090800fL && !defined(OPENSSL_NO_ECDH)
# define OPENSSL_NO_ECDH
#endif

/* compression support got added to OpenSSL in 0.9.8 */
#if OPENSSL_VERSION_NUMBER < 0x0090800fL && !defined(OPENSSL_NO_COMP)
# define OPENSSL_NO_COMP
#endif


typedef struct {
    PyObject_HEAD
    SSL_CTX *ctx;
#ifdef OPENSSL_NPN_NEGOTIATED
    char *npn_protocols;
    int npn_protocols_len;
#endif
} PySSLContext;

typedef struct {
    PyObject_HEAD
    PyObject *Socket; /* weakref to socket on which we're layered */
    SSL *ssl;
    X509 *peer_cert;
    int shutdown_seen_zero;
    enum py_ssl_server_or_client socket_type;
} PySSLSocket;

static PyTypeObject PySSLContext_Type;
static PyTypeObject PySSLSocket_Type;

static PyObject *PySSL_SSLwrite(PySSLSocket *self, PyObject *args);
static PyObject *PySSL_SSLread(PySSLSocket *self, PyObject *args);
static int check_socket_and_wait_for_timeout(PySocketSockObject *s,
                                             int writing);
static PyObject *PySSL_peercert(PySSLSocket *self, PyObject *args);
static PyObject *PySSL_cipher(PySSLSocket *self);

#define PySSLContext_Check(v)   (Py_TYPE(v) == &PySSLContext_Type)
#define PySSLSocket_Check(v)    (Py_TYPE(v) == &PySSLSocket_Type)

typedef enum {
    SOCKET_IS_NONBLOCKING,
    SOCKET_IS_BLOCKING,
    SOCKET_HAS_TIMED_OUT,
    SOCKET_HAS_BEEN_CLOSED,
    SOCKET_TOO_LARGE_FOR_SELECT,
    SOCKET_OPERATION_OK
} timeout_state;

/* Wrap error strings with filename and line # */
#define STRINGIFY1(x) #x
#define STRINGIFY2(x) STRINGIFY1(x)
#define ERRSTR1(x,y,z) (x ":" y ": " z)
#define ERRSTR(x) ERRSTR1("_ssl.c", STRINGIFY2(__LINE__), x)


/*
 * SSL errors.
 */

PyDoc_STRVAR(SSLError_doc,
"An error occurred in the SSL implementation.");

PyDoc_STRVAR(SSLZeroReturnError_doc,
"SSL/TLS session closed cleanly.");

PyDoc_STRVAR(SSLWantReadError_doc,
"Non-blocking SSL socket needs to read more data\n"
"before the requested operation can be completed.");

PyDoc_STRVAR(SSLWantWriteError_doc,
"Non-blocking SSL socket needs to write more data\n"
"before the requested operation can be completed.");

PyDoc_STRVAR(SSLSyscallError_doc,
"System error when attempting SSL operation.");

PyDoc_STRVAR(SSLEOFError_doc,
"SSL/TLS connection terminated abruptly.");

static PyObject *
SSLError_str(PyOSErrorObject *self)
{
    if (self->strerror != NULL && PyUnicode_Check(self->strerror)) {
        Py_INCREF(self->strerror);
        return self->strerror;
    }
    else
        return PyObject_Str(self->args);
}

static PyType_Slot sslerror_type_slots[] = {
    {Py_tp_base, NULL},  /* Filled out in module init as it's not a constant */
    {Py_tp_doc, SSLError_doc},
    {Py_tp_str, SSLError_str},
    {0, 0},
};

static PyType_Spec sslerror_type_spec = {
    "ssl.SSLError",
    sizeof(PyOSErrorObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    sslerror_type_slots
};

static void
fill_and_set_sslerror(PyObject *type, int ssl_errno, const char *errstr,
                      int lineno, unsigned long errcode)
{
    PyObject *err_value = NULL, *reason_obj = NULL, *lib_obj = NULL;
    PyObject *init_value, *msg, *key;
    _Py_IDENTIFIER(reason);
    _Py_IDENTIFIER(library);

    if (errcode != 0) {
        int lib, reason;

        lib = ERR_GET_LIB(errcode);
        reason = ERR_GET_REASON(errcode);
        key = Py_BuildValue("ii", lib, reason);
        if (key == NULL)
            goto fail;
        reason_obj = PyDict_GetItem(err_codes_to_names, key);
        Py_DECREF(key);
        if (reason_obj == NULL) {
            /* XXX if reason < 100, it might reflect a library number (!!) */
            PyErr_Clear();
        }
        key = PyLong_FromLong(lib);
        if (key == NULL)
            goto fail;
        lib_obj = PyDict_GetItem(lib_codes_to_names, key);
        Py_DECREF(key);
        if (lib_obj == NULL) {
            PyErr_Clear();
        }
        if (errstr == NULL)
            errstr = ERR_reason_error_string(errcode);
    }
    if (errstr == NULL)
        errstr = "unknown error";

    if (reason_obj && lib_obj)
        msg = PyUnicode_FromFormat("[%S: %S] %s (_ssl.c:%d)",
                                   lib_obj, reason_obj, errstr, lineno);
    else if (lib_obj)
        msg = PyUnicode_FromFormat("[%S] %s (_ssl.c:%d)",
                                   lib_obj, errstr, lineno);
    else
        msg = PyUnicode_FromFormat("%s (_ssl.c:%d)", errstr, lineno);

    if (msg == NULL)
        goto fail;
    init_value = Py_BuildValue("iN", ssl_errno, msg);
    err_value = PyObject_CallObject(type, init_value);
    Py_DECREF(init_value);
    if (err_value == NULL)
        goto fail;
    if (reason_obj == NULL)
        reason_obj = Py_None;
    if (_PyObject_SetAttrId(err_value, &PyId_reason, reason_obj))
        goto fail;
    if (lib_obj == NULL)
        lib_obj = Py_None;
    if (_PyObject_SetAttrId(err_value, &PyId_library, lib_obj))
        goto fail;
    PyErr_SetObject(type, err_value);
fail:
    Py_XDECREF(err_value);
}

static PyObject *
PySSL_SetError(PySSLSocket *obj, int ret, char *filename, int lineno)
{
    PyObject *type = PySSLErrorObject;
    char *errstr = NULL;
    int err;
    enum py_ssl_error p = PY_SSL_ERROR_NONE;
    unsigned long e = 0;

    assert(ret <= 0);
    e = ERR_peek_last_error();

    if (obj->ssl != NULL) {
        err = SSL_get_error(obj->ssl, ret);

        switch (err) {
        case SSL_ERROR_ZERO_RETURN:
            errstr = "TLS/SSL connection has been closed (EOF)";
            type = PySSLZeroReturnErrorObject;
            p = PY_SSL_ERROR_ZERO_RETURN;
            break;
        case SSL_ERROR_WANT_READ:
            errstr = "The operation did not complete (read)";
            type = PySSLWantReadErrorObject;
            p = PY_SSL_ERROR_WANT_READ;
            break;
        case SSL_ERROR_WANT_WRITE:
            p = PY_SSL_ERROR_WANT_WRITE;
            type = PySSLWantWriteErrorObject;
            errstr = "The operation did not complete (write)";
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            p = PY_SSL_ERROR_WANT_X509_LOOKUP;
            errstr = "The operation did not complete (X509 lookup)";
            break;
        case SSL_ERROR_WANT_CONNECT:
            p = PY_SSL_ERROR_WANT_CONNECT;
            errstr = "The operation did not complete (connect)";
            break;
        case SSL_ERROR_SYSCALL:
        {
            if (e == 0) {
                PySocketSockObject *s
                  = (PySocketSockObject *) PyWeakref_GetObject(obj->Socket);
                if (ret == 0 || (((PyObject *)s) == Py_None)) {
                    p = PY_SSL_ERROR_EOF;
                    type = PySSLEOFErrorObject;
                    errstr = "EOF occurred in violation of protocol";
                } else if (ret == -1) {
                    /* underlying BIO reported an I/O error */
                    Py_INCREF(s);
                    ERR_clear_error();
                    s->errorhandler();
                    Py_DECREF(s);
                    return NULL;
                } else { /* possible? */
                    p = PY_SSL_ERROR_SYSCALL;
                    type = PySSLSyscallErrorObject;
                    errstr = "Some I/O error occurred";
                }
            } else {
                p = PY_SSL_ERROR_SYSCALL;
            }
            break;
        }
        case SSL_ERROR_SSL:
        {
            p = PY_SSL_ERROR_SSL;
            if (e == 0)
                /* possible? */
                errstr = "A failure in the SSL library occurred";
            break;
        }
        default:
            p = PY_SSL_ERROR_INVALID_ERROR_CODE;
            errstr = "Invalid error code";
        }
    }
    fill_and_set_sslerror(type, p, errstr, lineno, e);
    ERR_clear_error();
    return NULL;
}

static PyObject *
_setSSLError (char *errstr, int errcode, char *filename, int lineno) {

    if (errstr == NULL)
        errcode = ERR_peek_last_error();
    else
        errcode = 0;
    fill_and_set_sslerror(PySSLErrorObject, errcode, errstr, lineno, errcode);
    ERR_clear_error();
    return NULL;
}

/*
 * SSL objects
 */

static PySSLSocket *
newPySSLSocket(SSL_CTX *ctx, PySocketSockObject *sock,
               enum py_ssl_server_or_client socket_type,
               char *server_hostname)
{
    PySSLSocket *self;

    self = PyObject_New(PySSLSocket, &PySSLSocket_Type);
    if (self == NULL)
        return NULL;

    self->peer_cert = NULL;
    self->ssl = NULL;
    self->Socket = NULL;

    /* Make sure the SSL error state is initialized */
    (void) ERR_get_state();
    ERR_clear_error();

    PySSL_BEGIN_ALLOW_THREADS
    self->ssl = SSL_new(ctx);
    PySSL_END_ALLOW_THREADS
    SSL_set_fd(self->ssl, sock->sock_fd);
#ifdef SSL_MODE_AUTO_RETRY
    SSL_set_mode(self->ssl, SSL_MODE_AUTO_RETRY);
#endif

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
    if (server_hostname != NULL)
        SSL_set_tlsext_host_name(self->ssl, server_hostname);
#endif

    /* If the socket is in non-blocking mode or timeout mode, set the BIO
     * to non-blocking mode (blocking is the default)
     */
    if (sock->sock_timeout >= 0.0) {
        BIO_set_nbio(SSL_get_rbio(self->ssl), 1);
        BIO_set_nbio(SSL_get_wbio(self->ssl), 1);
    }

    PySSL_BEGIN_ALLOW_THREADS
    if (socket_type == PY_SSL_CLIENT)
        SSL_set_connect_state(self->ssl);
    else
        SSL_set_accept_state(self->ssl);
    PySSL_END_ALLOW_THREADS

    self->socket_type = socket_type;
    self->Socket = PyWeakref_NewRef((PyObject *) sock, NULL);
    return self;
}

/* SSL object methods */

static PyObject *PySSL_SSLdo_handshake(PySSLSocket *self)
{
    int ret;
    int err;
    int sockstate, nonblocking;
    PySocketSockObject *sock
      = (PySocketSockObject *) PyWeakref_GetObject(self->Socket);

    if (((PyObject*)sock) == Py_None) {
        _setSSLError("Underlying socket connection gone",
                     PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
        return NULL;
    }
    Py_INCREF(sock);

    /* just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    /* Actually negotiate SSL connection */
    /* XXX If SSL_do_handshake() returns 0, it's also a failure. */
    do {
        PySSL_BEGIN_ALLOW_THREADS
        ret = SSL_do_handshake(self->ssl);
        err = SSL_get_error(self->ssl, ret);
        PySSL_END_ALLOW_THREADS
        if (PyErr_CheckSignals())
            goto error;
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PySocketModule.timeout_error,
                            ERRSTR("The handshake operation timed out"));
            goto error;
        } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
            PyErr_SetString(PySSLErrorObject,
                            ERRSTR("Underlying socket has been closed."));
            goto error;
        } else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
            PyErr_SetString(PySSLErrorObject,
                            ERRSTR("Underlying socket too large for select()."));
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    Py_DECREF(sock);
    if (ret < 1)
        return PySSL_SetError(self, ret, __FILE__, __LINE__);

    if (self->peer_cert)
        X509_free (self->peer_cert);
    PySSL_BEGIN_ALLOW_THREADS
    self->peer_cert = SSL_get_peer_certificate(self->ssl);
    PySSL_END_ALLOW_THREADS

    Py_INCREF(Py_None);
    return Py_None;

error:
    Py_DECREF(sock);
    return NULL;
}

static PyObject *
_create_tuple_for_attribute (ASN1_OBJECT *name, ASN1_STRING *value) {

    char namebuf[X509_NAME_MAXLEN];
    int buflen;
    PyObject *name_obj;
    PyObject *value_obj;
    PyObject *attr;
    unsigned char *valuebuf = NULL;

    buflen = OBJ_obj2txt(namebuf, sizeof(namebuf), name, 0);
    if (buflen < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail;
    }
    name_obj = PyUnicode_FromStringAndSize(namebuf, buflen);
    if (name_obj == NULL)
        goto fail;

    buflen = ASN1_STRING_to_UTF8(&valuebuf, value);
    if (buflen < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        Py_DECREF(name_obj);
        goto fail;
    }
    value_obj = PyUnicode_DecodeUTF8((char *) valuebuf,
                                     buflen, "strict");
    OPENSSL_free(valuebuf);
    if (value_obj == NULL) {
        Py_DECREF(name_obj);
        goto fail;
    }
    attr = PyTuple_New(2);
    if (attr == NULL) {
        Py_DECREF(name_obj);
        Py_DECREF(value_obj);
        goto fail;
    }
    PyTuple_SET_ITEM(attr, 0, name_obj);
    PyTuple_SET_ITEM(attr, 1, value_obj);
    return attr;

  fail:
    return NULL;
}

static PyObject *
_create_tuple_for_X509_NAME (X509_NAME *xname)
{
    PyObject *dn = NULL;    /* tuple which represents the "distinguished name" */
    PyObject *rdn = NULL;   /* tuple to hold a "relative distinguished name" */
    PyObject *rdnt;
    PyObject *attr = NULL;   /* tuple to hold an attribute */
    int entry_count = X509_NAME_entry_count(xname);
    X509_NAME_ENTRY *entry;
    ASN1_OBJECT *name;
    ASN1_STRING *value;
    int index_counter;
    int rdn_level = -1;
    int retcode;

    dn = PyList_New(0);
    if (dn == NULL)
        return NULL;
    /* now create another tuple to hold the top-level RDN */
    rdn = PyList_New(0);
    if (rdn == NULL)
        goto fail0;

    for (index_counter = 0;
         index_counter < entry_count;
         index_counter++)
    {
        entry = X509_NAME_get_entry(xname, index_counter);

        /* check to see if we've gotten to a new RDN */
        if (rdn_level >= 0) {
            if (rdn_level != entry->set) {
                /* yes, new RDN */
                /* add old RDN to DN */
                rdnt = PyList_AsTuple(rdn);
                Py_DECREF(rdn);
                if (rdnt == NULL)
                    goto fail0;
                retcode = PyList_Append(dn, rdnt);
                Py_DECREF(rdnt);
                if (retcode < 0)
                    goto fail0;
                /* create new RDN */
                rdn = PyList_New(0);
                if (rdn == NULL)
                    goto fail0;
            }
        }
        rdn_level = entry->set;

        /* now add this attribute to the current RDN */
        name = X509_NAME_ENTRY_get_object(entry);
        value = X509_NAME_ENTRY_get_data(entry);
        attr = _create_tuple_for_attribute(name, value);
        /*
        fprintf(stderr, "RDN level %d, attribute %s: %s\n",
            entry->set,
            PyBytes_AS_STRING(PyTuple_GET_ITEM(attr, 0)),
            PyBytes_AS_STRING(PyTuple_GET_ITEM(attr, 1)));
        */
        if (attr == NULL)
            goto fail1;
        retcode = PyList_Append(rdn, attr);
        Py_DECREF(attr);
        if (retcode < 0)
            goto fail1;
    }
    /* now, there's typically a dangling RDN */
    if (rdn != NULL) {
        if (PyList_GET_SIZE(rdn) > 0) {
            rdnt = PyList_AsTuple(rdn);
            Py_DECREF(rdn);
            if (rdnt == NULL)
                goto fail0;
            retcode = PyList_Append(dn, rdnt);
            Py_DECREF(rdnt);
            if (retcode < 0)
                goto fail0;
        }
        else {
            Py_DECREF(rdn);
        }
    }

    /* convert list to tuple */
    rdnt = PyList_AsTuple(dn);
    Py_DECREF(dn);
    if (rdnt == NULL)
        return NULL;
    return rdnt;

  fail1:
    Py_XDECREF(rdn);

  fail0:
    Py_XDECREF(dn);
    return NULL;
}

static PyObject *
_get_peer_alt_names (X509 *certificate) {

    /* this code follows the procedure outlined in
       OpenSSL's crypto/x509v3/v3_prn.c:X509v3_EXT_print()
       function to extract the STACK_OF(GENERAL_NAME),
       then iterates through the stack to add the
       names. */

    int i, j;
    PyObject *peer_alt_names = Py_None;
    PyObject *v, *t;
    X509_EXTENSION *ext = NULL;
    GENERAL_NAMES *names = NULL;
    GENERAL_NAME *name;
    const X509V3_EXT_METHOD *method;
    BIO *biobuf = NULL;
    char buf[2048];
    char *vptr;
    int len;
    /* Issue #2973: ASN1_item_d2i() API changed in OpenSSL 0.9.6m */
#if OPENSSL_VERSION_NUMBER >= 0x009060dfL
    const unsigned char *p;
#else
    unsigned char *p;
#endif

    if (certificate == NULL)
        return peer_alt_names;

    /* get a memory buffer */
    biobuf = BIO_new(BIO_s_mem());

    i = -1;
    while ((i = X509_get_ext_by_NID(
                    certificate, NID_subject_alt_name, i)) >= 0) {

        if (peer_alt_names == Py_None) {
            peer_alt_names = PyList_New(0);
            if (peer_alt_names == NULL)
                goto fail;
        }

        /* now decode the altName */
        ext = X509_get_ext(certificate, i);
        if(!(method = X509V3_EXT_get(ext))) {
            PyErr_SetString
              (PySSLErrorObject,
               ERRSTR("No method for internalizing subjectAltName!"));
            goto fail;
        }

        p = ext->value->data;
        if (method->it)
            names = (GENERAL_NAMES*)
              (ASN1_item_d2i(NULL,
                             &p,
                             ext->value->length,
                             ASN1_ITEM_ptr(method->it)));
        else
            names = (GENERAL_NAMES*)
              (method->d2i(NULL,
                           &p,
                           ext->value->length));

        for(j = 0; j < sk_GENERAL_NAME_num(names); j++) {

            /* get a rendering of each name in the set of names */

            name = sk_GENERAL_NAME_value(names, j);
            if (name->type == GEN_DIRNAME) {

                /* we special-case DirName as a tuple of
                   tuples of attributes */

                t = PyTuple_New(2);
                if (t == NULL) {
                    goto fail;
                }

                v = PyUnicode_FromString("DirName");
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);

                v = _create_tuple_for_X509_NAME (name->d.dirn);
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 1, v);

            } else {

                /* for everything else, we use the OpenSSL print form */

                (void) BIO_reset(biobuf);
                GENERAL_NAME_print(biobuf, name);
                len = BIO_gets(biobuf, buf, sizeof(buf)-1);
                if (len < 0) {
                    _setSSLError(NULL, 0, __FILE__, __LINE__);
                    goto fail;
                }
                vptr = strchr(buf, ':');
                if (vptr == NULL)
                    goto fail;
                t = PyTuple_New(2);
                if (t == NULL)
                    goto fail;
                v = PyUnicode_FromStringAndSize(buf, (vptr - buf));
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 0, v);
                v = PyUnicode_FromStringAndSize((vptr + 1),
                                                (len - (vptr - buf + 1)));
                if (v == NULL) {
                    Py_DECREF(t);
                    goto fail;
                }
                PyTuple_SET_ITEM(t, 1, v);
            }

            /* and add that rendering to the list */

            if (PyList_Append(peer_alt_names, t) < 0) {
                Py_DECREF(t);
                goto fail;
            }
            Py_DECREF(t);
        }
        sk_GENERAL_NAME_pop_free(names, GENERAL_NAME_free);
    }
    BIO_free(biobuf);
    if (peer_alt_names != Py_None) {
        v = PyList_AsTuple(peer_alt_names);
        Py_DECREF(peer_alt_names);
        return v;
    } else {
        return peer_alt_names;
    }


  fail:
    if (biobuf != NULL)
        BIO_free(biobuf);

    if (peer_alt_names != Py_None) {
        Py_XDECREF(peer_alt_names);
    }

    return NULL;
}

static PyObject *
_decode_certificate(X509 *certificate) {

    PyObject *retval = NULL;
    BIO *biobuf = NULL;
    PyObject *peer;
    PyObject *peer_alt_names = NULL;
    PyObject *issuer;
    PyObject *version;
    PyObject *sn_obj;
    ASN1_INTEGER *serialNumber;
    char buf[2048];
    int len;
    ASN1_TIME *notBefore, *notAfter;
    PyObject *pnotBefore, *pnotAfter;

    retval = PyDict_New();
    if (retval == NULL)
        return NULL;

    peer = _create_tuple_for_X509_NAME(
        X509_get_subject_name(certificate));
    if (peer == NULL)
        goto fail0;
    if (PyDict_SetItemString(retval, (const char *) "subject", peer) < 0) {
        Py_DECREF(peer);
        goto fail0;
    }
    Py_DECREF(peer);

    issuer = _create_tuple_for_X509_NAME(
        X509_get_issuer_name(certificate));
    if (issuer == NULL)
        goto fail0;
    if (PyDict_SetItemString(retval, (const char *)"issuer", issuer) < 0) {
        Py_DECREF(issuer);
        goto fail0;
    }
    Py_DECREF(issuer);

    version = PyLong_FromLong(X509_get_version(certificate) + 1);
    if (PyDict_SetItemString(retval, "version", version) < 0) {
        Py_DECREF(version);
        goto fail0;
    }
    Py_DECREF(version);

    /* get a memory buffer */
    biobuf = BIO_new(BIO_s_mem());

    (void) BIO_reset(biobuf);
    serialNumber = X509_get_serialNumber(certificate);
    /* should not exceed 20 octets, 160 bits, so buf is big enough */
    i2a_ASN1_INTEGER(biobuf, serialNumber);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail1;
    }
    sn_obj = PyUnicode_FromStringAndSize(buf, len);
    if (sn_obj == NULL)
        goto fail1;
    if (PyDict_SetItemString(retval, "serialNumber", sn_obj) < 0) {
        Py_DECREF(sn_obj);
        goto fail1;
    }
    Py_DECREF(sn_obj);

    (void) BIO_reset(biobuf);
    notBefore = X509_get_notBefore(certificate);
    ASN1_TIME_print(biobuf, notBefore);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail1;
    }
    pnotBefore = PyUnicode_FromStringAndSize(buf, len);
    if (pnotBefore == NULL)
        goto fail1;
    if (PyDict_SetItemString(retval, "notBefore", pnotBefore) < 0) {
        Py_DECREF(pnotBefore);
        goto fail1;
    }
    Py_DECREF(pnotBefore);

    (void) BIO_reset(biobuf);
    notAfter = X509_get_notAfter(certificate);
    ASN1_TIME_print(biobuf, notAfter);
    len = BIO_gets(biobuf, buf, sizeof(buf)-1);
    if (len < 0) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto fail1;
    }
    pnotAfter = PyUnicode_FromStringAndSize(buf, len);
    if (pnotAfter == NULL)
        goto fail1;
    if (PyDict_SetItemString(retval, "notAfter", pnotAfter) < 0) {
        Py_DECREF(pnotAfter);
        goto fail1;
    }
    Py_DECREF(pnotAfter);

    /* Now look for subjectAltName */

    peer_alt_names = _get_peer_alt_names(certificate);
    if (peer_alt_names == NULL)
        goto fail1;
    else if (peer_alt_names != Py_None) {
        if (PyDict_SetItemString(retval, "subjectAltName",
                                 peer_alt_names) < 0) {
            Py_DECREF(peer_alt_names);
            goto fail1;
        }
        Py_DECREF(peer_alt_names);
    }

    BIO_free(biobuf);
    return retval;

  fail1:
    if (biobuf != NULL)
        BIO_free(biobuf);
  fail0:
    Py_XDECREF(retval);
    return NULL;
}


static PyObject *
PySSL_test_decode_certificate (PyObject *mod, PyObject *args) {

    PyObject *retval = NULL;
    PyObject *filename;
    X509 *x=NULL;
    BIO *cert;

    if (!PyArg_ParseTuple(args, "O&:test_decode_certificate",
                          PyUnicode_FSConverter, &filename))
        return NULL;

    if ((cert=BIO_new(BIO_s_file())) == NULL) {
        PyErr_SetString(PySSLErrorObject,
                        "Can't malloc memory to read file");
        goto fail0;
    }

    if (BIO_read_filename(cert, PyBytes_AsString(filename)) <= 0) {
        PyErr_SetString(PySSLErrorObject,
                        "Can't open file");
        goto fail0;
    }

    x = PEM_read_bio_X509_AUX(cert,NULL, NULL, NULL);
    if (x == NULL) {
        PyErr_SetString(PySSLErrorObject,
                        "Error decoding PEM-encoded file");
        goto fail0;
    }

    retval = _decode_certificate(x);
    X509_free(x);

  fail0:
    Py_DECREF(filename);
    if (cert != NULL) BIO_free(cert);
    return retval;
}


static PyObject *
PySSL_peercert(PySSLSocket *self, PyObject *args)
{
    PyObject *retval = NULL;
    int len;
    int verification;
    int binary_mode = 0;

    if (!PyArg_ParseTuple(args, "|p:peer_certificate", &binary_mode))
        return NULL;

    if (!self->peer_cert)
        Py_RETURN_NONE;

    if (binary_mode) {
        /* return cert in DER-encoded format */

        unsigned char *bytes_buf = NULL;

        bytes_buf = NULL;
        len = i2d_X509(self->peer_cert, &bytes_buf);
        if (len < 0) {
            PySSL_SetError(self, len, __FILE__, __LINE__);
            return NULL;
        }
        /* this is actually an immutable bytes sequence */
        retval = PyBytes_FromStringAndSize
          ((const char *) bytes_buf, len);
        OPENSSL_free(bytes_buf);
        return retval;

    } else {
        verification = SSL_CTX_get_verify_mode(SSL_get_SSL_CTX(self->ssl));
        if ((verification & SSL_VERIFY_PEER) == 0)
            return PyDict_New();
        else
            return _decode_certificate(self->peer_cert);
    }
}

PyDoc_STRVAR(PySSL_peercert_doc,
"peer_certificate([der=False]) -> certificate\n\
\n\
Returns the certificate for the peer.  If no certificate was provided,\n\
returns None.  If a certificate was provided, but not validated, returns\n\
an empty dictionary.  Otherwise returns a dict containing information\n\
about the peer certificate.\n\
\n\
If the optional argument is True, returns a DER-encoded copy of the\n\
peer certificate, or None if no certificate was provided.  This will\n\
return the certificate even if it wasn't validated.");

static PyObject *PySSL_cipher (PySSLSocket *self) {

    PyObject *retval, *v;
    const SSL_CIPHER *current;
    char *cipher_name;
    char *cipher_protocol;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    current = SSL_get_current_cipher(self->ssl);
    if (current == NULL)
        Py_RETURN_NONE;

    retval = PyTuple_New(3);
    if (retval == NULL)
        return NULL;

    cipher_name = (char *) SSL_CIPHER_get_name(current);
    if (cipher_name == NULL) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(retval, 0, Py_None);
    } else {
        v = PyUnicode_FromString(cipher_name);
        if (v == NULL)
            goto fail0;
        PyTuple_SET_ITEM(retval, 0, v);
    }
    cipher_protocol = SSL_CIPHER_get_version(current);
    if (cipher_protocol == NULL) {
        Py_INCREF(Py_None);
        PyTuple_SET_ITEM(retval, 1, Py_None);
    } else {
        v = PyUnicode_FromString(cipher_protocol);
        if (v == NULL)
            goto fail0;
        PyTuple_SET_ITEM(retval, 1, v);
    }
    v = PyLong_FromLong(SSL_CIPHER_get_bits(current, NULL));
    if (v == NULL)
        goto fail0;
    PyTuple_SET_ITEM(retval, 2, v);
    return retval;

  fail0:
    Py_DECREF(retval);
    return NULL;
}

#ifdef OPENSSL_NPN_NEGOTIATED
static PyObject *PySSL_selected_npn_protocol(PySSLSocket *self) {
    const unsigned char *out;
    unsigned int outlen;

    SSL_get0_next_proto_negotiated(self->ssl, 
                                   &out, &outlen);

    if (out == NULL)
        Py_RETURN_NONE;
    return PyUnicode_FromStringAndSize((char *) out, outlen);
}
#endif

static PyObject *PySSL_compression(PySSLSocket *self) {
#ifdef OPENSSL_NO_COMP
    Py_RETURN_NONE;
#else
    const COMP_METHOD *comp_method;
    const char *short_name;

    if (self->ssl == NULL)
        Py_RETURN_NONE;
    comp_method = SSL_get_current_compression(self->ssl);
    if (comp_method == NULL || comp_method->type == NID_undef)
        Py_RETURN_NONE;
    short_name = OBJ_nid2sn(comp_method->type);
    if (short_name == NULL)
        Py_RETURN_NONE;
    return PyUnicode_DecodeFSDefault(short_name);
#endif
}

static void PySSL_dealloc(PySSLSocket *self)
{
    if (self->peer_cert)        /* Possible not to have one? */
        X509_free (self->peer_cert);
    if (self->ssl)
        SSL_free(self->ssl);
    Py_XDECREF(self->Socket);
    PyObject_Del(self);
}

/* If the socket has a timeout, do a select()/poll() on the socket.
   The argument writing indicates the direction.
   Returns one of the possibilities in the timeout_state enum (above).
 */

static int
check_socket_and_wait_for_timeout(PySocketSockObject *s, int writing)
{
    fd_set fds;
    struct timeval tv;
    int rc;

    /* Nothing to do unless we're in timeout mode (not non-blocking) */
    if (s->sock_timeout < 0.0)
        return SOCKET_IS_BLOCKING;
    else if (s->sock_timeout == 0.0)
        return SOCKET_IS_NONBLOCKING;

    /* Guard against closed socket */
    if (s->sock_fd < 0)
        return SOCKET_HAS_BEEN_CLOSED;

    /* Prefer poll, if available, since you can poll() any fd
     * which can't be done with select(). */
#ifdef HAVE_POLL
    {
        struct pollfd pollfd;
        int timeout;

        pollfd.fd = s->sock_fd;
        pollfd.events = writing ? POLLOUT : POLLIN;

        /* s->sock_timeout is in seconds, timeout in ms */
        timeout = (int)(s->sock_timeout * 1000 + 0.5);
        PySSL_BEGIN_ALLOW_THREADS
        rc = poll(&pollfd, 1, timeout);
        PySSL_END_ALLOW_THREADS

        goto normal_return;
    }
#endif

    /* Guard against socket too large for select*/
    if (!_PyIsSelectable_fd(s->sock_fd))
        return SOCKET_TOO_LARGE_FOR_SELECT;

    /* Construct the arguments to select */
    tv.tv_sec = (int)s->sock_timeout;
    tv.tv_usec = (int)((s->sock_timeout - tv.tv_sec) * 1e6);
    FD_ZERO(&fds);
    FD_SET(s->sock_fd, &fds);

    /* See if the socket is ready */
    PySSL_BEGIN_ALLOW_THREADS
    if (writing)
        rc = select(s->sock_fd+1, NULL, &fds, NULL, &tv);
    else
        rc = select(s->sock_fd+1, &fds, NULL, NULL, &tv);
    PySSL_END_ALLOW_THREADS

#ifdef HAVE_POLL
normal_return:
#endif
    /* Return SOCKET_TIMED_OUT on timeout, SOCKET_OPERATION_OK otherwise
       (when we are able to write or when there's something to read) */
    return rc == 0 ? SOCKET_HAS_TIMED_OUT : SOCKET_OPERATION_OK;
}

static PyObject *PySSL_SSLwrite(PySSLSocket *self, PyObject *args)
{
    Py_buffer buf;
    int len;
    int sockstate;
    int err;
    int nonblocking;
    PySocketSockObject *sock
      = (PySocketSockObject *) PyWeakref_GetObject(self->Socket);

    if (((PyObject*)sock) == Py_None) {
        _setSSLError("Underlying socket connection gone",
                     PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
        return NULL;
    }
    Py_INCREF(sock);

    if (!PyArg_ParseTuple(args, "y*:write", &buf)) {
        Py_DECREF(sock);
        return NULL;
    }

    /* just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    sockstate = check_socket_and_wait_for_timeout(sock, 1);
    if (sockstate == SOCKET_HAS_TIMED_OUT) {
        PyErr_SetString(PySocketModule.timeout_error,
                        "The write operation timed out");
        goto error;
    } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
        PyErr_SetString(PySSLErrorObject,
                        "Underlying socket has been closed.");
        goto error;
    } else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
        PyErr_SetString(PySSLErrorObject,
                        "Underlying socket too large for select().");
        goto error;
    }
    do {
        PySSL_BEGIN_ALLOW_THREADS
        len = SSL_write(self->ssl, buf.buf, buf.len);
        err = SSL_get_error(self->ssl, len);
        PySSL_END_ALLOW_THREADS
        if (PyErr_CheckSignals()) {
            goto error;
        }
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PySocketModule.timeout_error,
                            "The write operation timed out");
            goto error;
        } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
            PyErr_SetString(PySSLErrorObject,
                            "Underlying socket has been closed.");
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);

    Py_DECREF(sock);
    PyBuffer_Release(&buf);
    if (len > 0)
        return PyLong_FromLong(len);
    else
        return PySSL_SetError(self, len, __FILE__, __LINE__);

error:
    Py_DECREF(sock);
    PyBuffer_Release(&buf);
    return NULL;
}

PyDoc_STRVAR(PySSL_SSLwrite_doc,
"write(s) -> len\n\
\n\
Writes the string s into the SSL object.  Returns the number\n\
of bytes written.");

static PyObject *PySSL_SSLpending(PySSLSocket *self)
{
    int count = 0;

    PySSL_BEGIN_ALLOW_THREADS
    count = SSL_pending(self->ssl);
    PySSL_END_ALLOW_THREADS
    if (count < 0)
        return PySSL_SetError(self, count, __FILE__, __LINE__);
    else
        return PyLong_FromLong(count);
}

PyDoc_STRVAR(PySSL_SSLpending_doc,
"pending() -> count\n\
\n\
Returns the number of already decrypted bytes available for read,\n\
pending on the connection.\n");

static PyObject *PySSL_SSLread(PySSLSocket *self, PyObject *args)
{
    PyObject *dest = NULL;
    Py_buffer buf;
    char *mem;
    int len, count;
    int buf_passed = 0;
    int sockstate;
    int err;
    int nonblocking;
    PySocketSockObject *sock
      = (PySocketSockObject *) PyWeakref_GetObject(self->Socket);

    if (((PyObject*)sock) == Py_None) {
        _setSSLError("Underlying socket connection gone",
                     PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
        return NULL;
    }
    Py_INCREF(sock);

    buf.obj = NULL;
    buf.buf = NULL;
    if (!PyArg_ParseTuple(args, "i|w*:read", &len, &buf))
        goto error;

    if ((buf.buf == NULL) && (buf.obj == NULL)) {
        dest = PyBytes_FromStringAndSize(NULL, len);
        if (dest == NULL)
            goto error;
        mem = PyBytes_AS_STRING(dest);
    }
    else {
        buf_passed = 1;
        mem = buf.buf;
        if (len <= 0 || len > buf.len) {
            len = (int) buf.len;
            if (buf.len != len) {
                PyErr_SetString(PyExc_OverflowError,
                                "maximum length can't fit in a C 'int'");
                goto error;
            }
        }
    }

    /* just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    /* first check if there are bytes ready to be read */
    PySSL_BEGIN_ALLOW_THREADS
    count = SSL_pending(self->ssl);
    PySSL_END_ALLOW_THREADS

    if (!count) {
        sockstate = check_socket_and_wait_for_timeout(sock, 0);
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PySocketModule.timeout_error,
                            "The read operation timed out");
            goto error;
        } else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
            PyErr_SetString(PySSLErrorObject,
                            "Underlying socket too large for select().");
            goto error;
        } else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
            count = 0;
            goto done;
        }
    }
    do {
        PySSL_BEGIN_ALLOW_THREADS
        count = SSL_read(self->ssl, mem, len);
        err = SSL_get_error(self->ssl, count);
        PySSL_END_ALLOW_THREADS
        if (PyErr_CheckSignals())
            goto error;
        if (err == SSL_ERROR_WANT_READ) {
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        } else if (err == SSL_ERROR_WANT_WRITE) {
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        } else if ((err == SSL_ERROR_ZERO_RETURN) &&
                   (SSL_get_shutdown(self->ssl) ==
                    SSL_RECEIVED_SHUTDOWN))
        {
            count = 0;
            goto done;
        } else {
            sockstate = SOCKET_OPERATION_OK;
        }
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            PyErr_SetString(PySocketModule.timeout_error,
                            "The read operation timed out");
            goto error;
        } else if (sockstate == SOCKET_IS_NONBLOCKING) {
            break;
        }
    } while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
    if (count <= 0) {
        PySSL_SetError(self, count, __FILE__, __LINE__);
        goto error;
    }

done:
    Py_DECREF(sock);
    if (!buf_passed) {
        _PyBytes_Resize(&dest, count);
        return dest;
    }
    else {
        PyBuffer_Release(&buf);
        return PyLong_FromLong(count);
    }

error:
    Py_DECREF(sock);
    if (!buf_passed)
        Py_XDECREF(dest);
    else
        PyBuffer_Release(&buf);
    return NULL;
}

PyDoc_STRVAR(PySSL_SSLread_doc,
"read([len]) -> string\n\
\n\
Read up to len bytes from the SSL socket.");

static PyObject *PySSL_SSLshutdown(PySSLSocket *self)
{
    int err, ssl_err, sockstate, nonblocking;
    int zeros = 0;
    PySocketSockObject *sock
      = (PySocketSockObject *) PyWeakref_GetObject(self->Socket);

    /* Guard against closed socket */
    if ((((PyObject*)sock) == Py_None) || (sock->sock_fd < 0)) {
        _setSSLError("Underlying socket connection gone",
                     PY_SSL_ERROR_NO_SOCKET, __FILE__, __LINE__);
        return NULL;
    }
    Py_INCREF(sock);

    /* Just in case the blocking state of the socket has been changed */
    nonblocking = (sock->sock_timeout >= 0.0);
    BIO_set_nbio(SSL_get_rbio(self->ssl), nonblocking);
    BIO_set_nbio(SSL_get_wbio(self->ssl), nonblocking);

    while (1) {
        PySSL_BEGIN_ALLOW_THREADS
        /* Disable read-ahead so that unwrap can work correctly.
         * Otherwise OpenSSL might read in too much data,
         * eating clear text data that happens to be
         * transmitted after the SSL shutdown.
         * Should be safe to call repeatedly everytime this
         * function is used and the shutdown_seen_zero != 0
         * condition is met.
         */
        if (self->shutdown_seen_zero)
            SSL_set_read_ahead(self->ssl, 0);
        err = SSL_shutdown(self->ssl);
        PySSL_END_ALLOW_THREADS
        /* If err == 1, a secure shutdown with SSL_shutdown() is complete */
        if (err > 0)
            break;
        if (err == 0) {
            /* Don't loop endlessly; instead preserve legacy
               behaviour of trying SSL_shutdown() only twice.
               This looks necessary for OpenSSL < 0.9.8m */
            if (++zeros > 1)
                break;
            /* Shutdown was sent, now try receiving */
            self->shutdown_seen_zero = 1;
            continue;
        }

        /* Possibly retry shutdown until timeout or failure */
        ssl_err = SSL_get_error(self->ssl, err);
        if (ssl_err == SSL_ERROR_WANT_READ)
            sockstate = check_socket_and_wait_for_timeout(sock, 0);
        else if (ssl_err == SSL_ERROR_WANT_WRITE)
            sockstate = check_socket_and_wait_for_timeout(sock, 1);
        else
            break;
        if (sockstate == SOCKET_HAS_TIMED_OUT) {
            if (ssl_err == SSL_ERROR_WANT_READ)
                PyErr_SetString(PySocketModule.timeout_error,
                                "The read operation timed out");
            else
                PyErr_SetString(PySocketModule.timeout_error,
                                "The write operation timed out");
            goto error;
        }
        else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
            PyErr_SetString(PySSLErrorObject,
                            "Underlying socket too large for select().");
            goto error;
        }
        else if (sockstate != SOCKET_OPERATION_OK)
            /* Retain the SSL error code */
            break;
    }

    if (err < 0) {
        Py_DECREF(sock);
        return PySSL_SetError(self, err, __FILE__, __LINE__);
    }
    else
        /* It's already INCREF'ed */
        return (PyObject *) sock;

error:
    Py_DECREF(sock);
    return NULL;
}

PyDoc_STRVAR(PySSL_SSLshutdown_doc,
"shutdown(s) -> socket\n\
\n\
Does the SSL shutdown handshake with the remote end, and returns\n\
the underlying socket object.");

#if HAVE_OPENSSL_FINISHED
static PyObject *
PySSL_tls_unique_cb(PySSLSocket *self)
{
    PyObject *retval = NULL;
    char buf[PySSL_CB_MAXLEN];
    int len;

    if (SSL_session_reused(self->ssl) ^ !self->socket_type) {
        /* if session is resumed XOR we are the client */
        len = SSL_get_finished(self->ssl, buf, PySSL_CB_MAXLEN);
    }
    else {
        /* if a new session XOR we are the server */
        len = SSL_get_peer_finished(self->ssl, buf, PySSL_CB_MAXLEN);
    }

    /* It cannot be negative in current OpenSSL version as of July 2011 */
    assert(len >= 0);
    if (len == 0)
        Py_RETURN_NONE;

    retval = PyBytes_FromStringAndSize(buf, len);

    return retval;
}

PyDoc_STRVAR(PySSL_tls_unique_cb_doc,
"tls_unique_cb() -> bytes\n\
\n\
Returns the 'tls-unique' channel binding data, as defined by RFC 5929.\n\
\n\
If the TLS handshake is not yet complete, None is returned");

#endif /* HAVE_OPENSSL_FINISHED */

static PyMethodDef PySSLMethods[] = {
    {"do_handshake", (PyCFunction)PySSL_SSLdo_handshake, METH_NOARGS},
    {"write", (PyCFunction)PySSL_SSLwrite, METH_VARARGS,
     PySSL_SSLwrite_doc},
    {"read", (PyCFunction)PySSL_SSLread, METH_VARARGS,
     PySSL_SSLread_doc},
    {"pending", (PyCFunction)PySSL_SSLpending, METH_NOARGS,
     PySSL_SSLpending_doc},
    {"peer_certificate", (PyCFunction)PySSL_peercert, METH_VARARGS,
     PySSL_peercert_doc},
    {"cipher", (PyCFunction)PySSL_cipher, METH_NOARGS},
#ifdef OPENSSL_NPN_NEGOTIATED
    {"selected_npn_protocol", (PyCFunction)PySSL_selected_npn_protocol, METH_NOARGS},
#endif
    {"compression", (PyCFunction)PySSL_compression, METH_NOARGS},
    {"shutdown", (PyCFunction)PySSL_SSLshutdown, METH_NOARGS,
     PySSL_SSLshutdown_doc},
#if HAVE_OPENSSL_FINISHED
    {"tls_unique_cb", (PyCFunction)PySSL_tls_unique_cb, METH_NOARGS,
     PySSL_tls_unique_cb_doc},
#endif
    {NULL, NULL}
};

static PyTypeObject PySSLSocket_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ssl._SSLSocket",                  /*tp_name*/
    sizeof(PySSLSocket),                /*tp_basicsize*/
    0,                                  /*tp_itemsize*/
    /* methods */
    (destructor)PySSL_dealloc,          /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_reserved*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                 /*tp_flags*/
    0,                                  /*tp_doc*/
    0,                                  /*tp_traverse*/
    0,                                  /*tp_clear*/
    0,                                  /*tp_richcompare*/
    0,                                  /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    PySSLMethods,                       /*tp_methods*/
};


/*
 * _SSLContext objects
 */

static PyObject *
context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"protocol", NULL};
    PySSLContext *self;
    int proto_version = PY_SSL_VERSION_SSL23;
    SSL_CTX *ctx = NULL;

    if (!PyArg_ParseTupleAndKeywords(
        args, kwds, "i:_SSLContext", kwlist,
        &proto_version))
        return NULL;

    PySSL_BEGIN_ALLOW_THREADS
    if (proto_version == PY_SSL_VERSION_TLS1)
        ctx = SSL_CTX_new(TLSv1_method());
    else if (proto_version == PY_SSL_VERSION_SSL3)
        ctx = SSL_CTX_new(SSLv3_method());
#ifndef OPENSSL_NO_SSL2
    else if (proto_version == PY_SSL_VERSION_SSL2)
        ctx = SSL_CTX_new(SSLv2_method());
#endif
    else if (proto_version == PY_SSL_VERSION_SSL23)
        ctx = SSL_CTX_new(SSLv23_method());
    else
        proto_version = -1;
    PySSL_END_ALLOW_THREADS

    if (proto_version == -1) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid protocol version");
        return NULL;
    }
    if (ctx == NULL) {
        PyErr_SetString(PySSLErrorObject,
                        "failed to allocate SSL context");
        return NULL;
    }

    assert(type != NULL && type->tp_alloc != NULL);
    self = (PySSLContext *) type->tp_alloc(type, 0);
    if (self == NULL) {
        SSL_CTX_free(ctx);
        return NULL;
    }
    self->ctx = ctx;
#ifdef OPENSSL_NPN_NEGOTIATED
    self->npn_protocols = NULL;
#endif
    /* Defaults */
    SSL_CTX_set_verify(self->ctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_options(self->ctx,
                        SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);

#define SID_CTX "Python"
    SSL_CTX_set_session_id_context(self->ctx, (const unsigned char *) SID_CTX,
                                   sizeof(SID_CTX));
#undef SID_CTX

    return (PyObject *)self;
}

static void
context_dealloc(PySSLContext *self)
{
    SSL_CTX_free(self->ctx);
#ifdef OPENSSL_NPN_NEGOTIATED
    PyMem_Free(self->npn_protocols);
#endif
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
set_ciphers(PySSLContext *self, PyObject *args)
{
    int ret;
    const char *cipherlist;

    if (!PyArg_ParseTuple(args, "s:set_ciphers", &cipherlist))
        return NULL;
    ret = SSL_CTX_set_cipher_list(self->ctx, cipherlist);
    if (ret == 0) {
        /* Clearing the error queue is necessary on some OpenSSL versions,
           otherwise the error will be reported again when another SSL call
           is done. */
        ERR_clear_error();
        PyErr_SetString(PySSLErrorObject,
                        "No cipher can be selected.");
        return NULL;
    }
    Py_RETURN_NONE;
}

#ifdef OPENSSL_NPN_NEGOTIATED
/* this callback gets passed to SSL_CTX_set_next_protos_advertise_cb */
static int
_advertiseNPN_cb(SSL *s, 
                 const unsigned char **data, unsigned int *len, 
                 void *args)
{
    PySSLContext *ssl_ctx = (PySSLContext *) args;

    if (ssl_ctx->npn_protocols == NULL) {
        *data = (unsigned char *) "";
        *len = 0;
    } else {
        *data = (unsigned char *) ssl_ctx->npn_protocols;
        *len = ssl_ctx->npn_protocols_len;
    }

    return SSL_TLSEXT_ERR_OK;
}
/* this callback gets passed to SSL_CTX_set_next_proto_select_cb */
static int
_selectNPN_cb(SSL *s, 
              unsigned char **out, unsigned char *outlen,
              const unsigned char *server, unsigned int server_len,
              void *args)
{
    PySSLContext *ssl_ctx = (PySSLContext *) args;

    unsigned char *client = (unsigned char *) ssl_ctx->npn_protocols;
    int client_len;

    if (client == NULL) {
        client = (unsigned char *) "";
        client_len = 0;
    } else {
        client_len = ssl_ctx->npn_protocols_len;
    }

    SSL_select_next_proto(out, outlen,
                          server, server_len,
                          client, client_len);

    return SSL_TLSEXT_ERR_OK;
}
#endif

static PyObject *
_set_npn_protocols(PySSLContext *self, PyObject *args)
{
#ifdef OPENSSL_NPN_NEGOTIATED
    Py_buffer protos;

    if (!PyArg_ParseTuple(args, "y*:set_npn_protocols", &protos))
        return NULL;

    if (self->npn_protocols != NULL) {
        PyMem_Free(self->npn_protocols);
    }

    self->npn_protocols = PyMem_Malloc(protos.len);
    if (self->npn_protocols == NULL) {
        PyBuffer_Release(&protos);
        return PyErr_NoMemory();
    }
    memcpy(self->npn_protocols, protos.buf, protos.len);
    self->npn_protocols_len = (int) protos.len;

    /* set both server and client callbacks, because the context can
     * be used to create both types of sockets */
    SSL_CTX_set_next_protos_advertised_cb(self->ctx,
                                          _advertiseNPN_cb,
                                          self);
    SSL_CTX_set_next_proto_select_cb(self->ctx,
                                     _selectNPN_cb,
                                     self);

    PyBuffer_Release(&protos);
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "The NPN extension requires OpenSSL 1.0.1 or later.");
    return NULL;
#endif
}

static PyObject *
get_verify_mode(PySSLContext *self, void *c)
{
    switch (SSL_CTX_get_verify_mode(self->ctx)) {
    case SSL_VERIFY_NONE:
        return PyLong_FromLong(PY_SSL_CERT_NONE);
    case SSL_VERIFY_PEER:
        return PyLong_FromLong(PY_SSL_CERT_OPTIONAL);
    case SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT:
        return PyLong_FromLong(PY_SSL_CERT_REQUIRED);
    }
    PyErr_SetString(PySSLErrorObject,
                    "invalid return value from SSL_CTX_get_verify_mode");
    return NULL;
}

static int
set_verify_mode(PySSLContext *self, PyObject *arg, void *c)
{
    int n, mode;
    if (!PyArg_Parse(arg, "i", &n))
        return -1;
    if (n == PY_SSL_CERT_NONE)
        mode = SSL_VERIFY_NONE;
    else if (n == PY_SSL_CERT_OPTIONAL)
        mode = SSL_VERIFY_PEER;
    else if (n == PY_SSL_CERT_REQUIRED)
        mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    else {
        PyErr_SetString(PyExc_ValueError,
                        "invalid value for verify_mode");
        return -1;
    }
    SSL_CTX_set_verify(self->ctx, mode, NULL);
    return 0;
}

static PyObject *
get_options(PySSLContext *self, void *c)
{
    return PyLong_FromLong(SSL_CTX_get_options(self->ctx));
}

static int
set_options(PySSLContext *self, PyObject *arg, void *c)
{
    long new_opts, opts, set, clear;
    if (!PyArg_Parse(arg, "l", &new_opts))
        return -1;
    opts = SSL_CTX_get_options(self->ctx);
    clear = opts & ~new_opts;
    set = ~opts & new_opts;
    if (clear) {
#ifdef HAVE_SSL_CTX_CLEAR_OPTIONS
        SSL_CTX_clear_options(self->ctx, clear);
#else
        PyErr_SetString(PyExc_ValueError,
                        "can't clear options before OpenSSL 0.9.8m");
        return -1;
#endif
    }
    if (set)
        SSL_CTX_set_options(self->ctx, set);
    return 0;
}

typedef struct {
    PyThreadState *thread_state;
    PyObject *callable;
    char *password;
    Py_ssize_t size;
    int error;
} _PySSLPasswordInfo;

static int
_pwinfo_set(_PySSLPasswordInfo *pw_info, PyObject* password,
            const char *bad_type_error)
{
    /* Set the password and size fields of a _PySSLPasswordInfo struct
       from a unicode, bytes, or byte array object.
       The password field will be dynamically allocated and must be freed
       by the caller */
    PyObject *password_bytes = NULL;
    const char *data = NULL;
    Py_ssize_t size;

    if (PyUnicode_Check(password)) {
        password_bytes = PyUnicode_AsEncodedString(password, NULL, NULL);
        if (!password_bytes) {
            goto error;
        }
        data = PyBytes_AS_STRING(password_bytes);
        size = PyBytes_GET_SIZE(password_bytes);
    } else if (PyBytes_Check(password)) {
        data = PyBytes_AS_STRING(password);
        size = PyBytes_GET_SIZE(password);
    } else if (PyByteArray_Check(password)) {
        data = PyByteArray_AS_STRING(password);
        size = PyByteArray_GET_SIZE(password);
    } else {
        PyErr_SetString(PyExc_TypeError, bad_type_error);
        goto error;
    }

    free(pw_info->password);
    pw_info->password = malloc(size);
    if (!pw_info->password) {
        PyErr_SetString(PyExc_MemoryError,
                        "unable to allocate password buffer");
        goto error;
    }
    memcpy(pw_info->password, data, size);
    pw_info->size = size;

    Py_XDECREF(password_bytes);
    return 1;

error:
    Py_XDECREF(password_bytes);
    return 0;
}

static int
_password_callback(char *buf, int size, int rwflag, void *userdata)
{
    _PySSLPasswordInfo *pw_info = (_PySSLPasswordInfo*) userdata;
    PyObject *fn_ret = NULL;

    PySSL_END_ALLOW_THREADS_S(pw_info->thread_state);

    if (pw_info->callable) {
        fn_ret = PyObject_CallFunctionObjArgs(pw_info->callable, NULL);
        if (!fn_ret) {
            /* TODO: It would be nice to move _ctypes_add_traceback() into the
               core python API, so we could use it to add a frame here */
            goto error;
        }

        if (!_pwinfo_set(pw_info, fn_ret,
                         "password callback must return a string")) {
            goto error;
        }
        Py_CLEAR(fn_ret);
    }

    if (pw_info->size > size) {
        PyErr_Format(PyExc_ValueError,
                     "password cannot be longer than %d bytes", size);
        goto error;
    }

    PySSL_BEGIN_ALLOW_THREADS_S(pw_info->thread_state);
    memcpy(buf, pw_info->password, pw_info->size);
    return pw_info->size;

error:
    Py_XDECREF(fn_ret);
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info->thread_state);
    pw_info->error = 1;
    return -1;
}

static PyObject *
load_cert_chain(PySSLContext *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"certfile", "keyfile", "password", NULL};
    PyObject *certfile, *keyfile = NULL, *password = NULL;
    PyObject *certfile_bytes = NULL, *keyfile_bytes = NULL;
    pem_password_cb *orig_passwd_cb = self->ctx->default_passwd_callback;
    void *orig_passwd_userdata = self->ctx->default_passwd_callback_userdata;
    _PySSLPasswordInfo pw_info = { NULL, NULL, NULL, 0, 0 };
    int r;

    errno = 0;
    ERR_clear_error();
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
        "O|OO:load_cert_chain", kwlist,
        &certfile, &keyfile, &password))
        return NULL;
    if (keyfile == Py_None)
        keyfile = NULL;
    if (!PyUnicode_FSConverter(certfile, &certfile_bytes)) {
        PyErr_SetString(PyExc_TypeError,
                        "certfile should be a valid filesystem path");
        return NULL;
    }
    if (keyfile && !PyUnicode_FSConverter(keyfile, &keyfile_bytes)) {
        PyErr_SetString(PyExc_TypeError,
                        "keyfile should be a valid filesystem path");
        goto error;
    }
    if (password && password != Py_None) {
        if (PyCallable_Check(password)) {
            pw_info.callable = password;
        } else if (!_pwinfo_set(&pw_info, password,
                                "password should be a string or callable")) {
            goto error;
        }
        SSL_CTX_set_default_passwd_cb(self->ctx, _password_callback);
        SSL_CTX_set_default_passwd_cb_userdata(self->ctx, &pw_info);
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_use_certificate_chain_file(self->ctx,
        PyBytes_AS_STRING(certfile_bytes));
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    if (r != 1) {
        if (pw_info.error) {
            ERR_clear_error();
            /* the password callback has already set the error information */
        }
        else if (errno != 0) {
            ERR_clear_error();
            PyErr_SetFromErrno(PyExc_IOError);
        }
        else {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
        }
        goto error;
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_use_PrivateKey_file(self->ctx,
        PyBytes_AS_STRING(keyfile ? keyfile_bytes : certfile_bytes),
        SSL_FILETYPE_PEM);
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    Py_CLEAR(keyfile_bytes);
    Py_CLEAR(certfile_bytes);
    if (r != 1) {
        if (pw_info.error) {
            ERR_clear_error();
            /* the password callback has already set the error information */
        }
        else if (errno != 0) {
            ERR_clear_error();
            PyErr_SetFromErrno(PyExc_IOError);
        }
        else {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
        }
        goto error;
    }
    PySSL_BEGIN_ALLOW_THREADS_S(pw_info.thread_state);
    r = SSL_CTX_check_private_key(self->ctx);
    PySSL_END_ALLOW_THREADS_S(pw_info.thread_state);
    if (r != 1) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        goto error;
    }
    SSL_CTX_set_default_passwd_cb(self->ctx, orig_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(self->ctx, orig_passwd_userdata);
    free(pw_info.password);
    Py_RETURN_NONE;

error:
    SSL_CTX_set_default_passwd_cb(self->ctx, orig_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(self->ctx, orig_passwd_userdata);
    free(pw_info.password);
    Py_XDECREF(keyfile_bytes);
    Py_XDECREF(certfile_bytes);
    return NULL;
}

static PyObject *
load_verify_locations(PySSLContext *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"cafile", "capath", NULL};
    PyObject *cafile = NULL, *capath = NULL;
    PyObject *cafile_bytes = NULL, *capath_bytes = NULL;
    const char *cafile_buf = NULL, *capath_buf = NULL;
    int r;

    errno = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
        "|OO:load_verify_locations", kwlist,
        &cafile, &capath))
        return NULL;
    if (cafile == Py_None)
        cafile = NULL;
    if (capath == Py_None)
        capath = NULL;
    if (cafile == NULL && capath == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "cafile and capath cannot be both omitted");
        return NULL;
    }
    if (cafile && !PyUnicode_FSConverter(cafile, &cafile_bytes)) {
        PyErr_SetString(PyExc_TypeError,
                        "cafile should be a valid filesystem path");
        return NULL;
    }
    if (capath && !PyUnicode_FSConverter(capath, &capath_bytes)) {
        Py_XDECREF(cafile_bytes);
        PyErr_SetString(PyExc_TypeError,
                        "capath should be a valid filesystem path");
        return NULL;
    }
    if (cafile)
        cafile_buf = PyBytes_AS_STRING(cafile_bytes);
    if (capath)
        capath_buf = PyBytes_AS_STRING(capath_bytes);
    PySSL_BEGIN_ALLOW_THREADS
    r = SSL_CTX_load_verify_locations(self->ctx, cafile_buf, capath_buf);
    PySSL_END_ALLOW_THREADS
    Py_XDECREF(cafile_bytes);
    Py_XDECREF(capath_bytes);
    if (r != 1) {
        if (errno != 0) {
            ERR_clear_error();
            PyErr_SetFromErrno(PyExc_IOError);
        }
        else {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
        }
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
load_dh_params(PySSLContext *self, PyObject *filepath)
{
    FILE *f;
    DH *dh;

    f = _Py_fopen(filepath, "rb");
    if (f == NULL) {
        if (!PyErr_Occurred())
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, filepath);
        return NULL;
    }
    errno = 0;
    PySSL_BEGIN_ALLOW_THREADS
    dh = PEM_read_DHparams(f, NULL, NULL, NULL);
    fclose(f);
    PySSL_END_ALLOW_THREADS
    if (dh == NULL) {
        if (errno != 0) {
            ERR_clear_error();
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, filepath);
        }
        else {
            _setSSLError(NULL, 0, __FILE__, __LINE__);
        }
        return NULL;
    }
    if (SSL_CTX_set_tmp_dh(self->ctx, dh) == 0)
        _setSSLError(NULL, 0, __FILE__, __LINE__);
    DH_free(dh);
    Py_RETURN_NONE;
}

static PyObject *
context_wrap_socket(PySSLContext *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"sock", "server_side", "server_hostname", NULL};
    PySocketSockObject *sock;
    int server_side = 0;
    char *hostname = NULL;
    PyObject *hostname_obj, *res;

    /* server_hostname is either None (or absent), or to be encoded
       using the idna encoding. */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!i|O!:_wrap_socket", kwlist,
                                     PySocketModule.Sock_Type,
                                     &sock, &server_side,
                                     Py_TYPE(Py_None), &hostname_obj)) {
        PyErr_Clear();
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!iet:_wrap_socket", kwlist,
            PySocketModule.Sock_Type,
            &sock, &server_side,
            "idna", &hostname))
            return NULL;
#ifndef SSL_CTRL_SET_TLSEXT_HOSTNAME
        PyMem_Free(hostname);
        PyErr_SetString(PyExc_ValueError, "server_hostname is not supported "
                        "by your OpenSSL library");
        return NULL;
#endif
    }

    res = (PyObject *) newPySSLSocket(self->ctx, sock, server_side,
                                      hostname);
    if (hostname != NULL)
        PyMem_Free(hostname);
    return res;
}

static PyObject *
session_stats(PySSLContext *self, PyObject *unused)
{
    int r;
    PyObject *value, *stats = PyDict_New();
    if (!stats)
        return NULL;

#define ADD_STATS(SSL_NAME, KEY_NAME) \
    value = PyLong_FromLong(SSL_CTX_sess_ ## SSL_NAME (self->ctx)); \
    if (value == NULL) \
        goto error; \
    r = PyDict_SetItemString(stats, KEY_NAME, value); \
    Py_DECREF(value); \
    if (r < 0) \
        goto error;

    ADD_STATS(number, "number");
    ADD_STATS(connect, "connect");
    ADD_STATS(connect_good, "connect_good");
    ADD_STATS(connect_renegotiate, "connect_renegotiate");
    ADD_STATS(accept, "accept");
    ADD_STATS(accept_good, "accept_good");
    ADD_STATS(accept_renegotiate, "accept_renegotiate");
    ADD_STATS(accept, "accept");
    ADD_STATS(hits, "hits");
    ADD_STATS(misses, "misses");
    ADD_STATS(timeouts, "timeouts");
    ADD_STATS(cache_full, "cache_full");

#undef ADD_STATS

    return stats;

error:
    Py_DECREF(stats);
    return NULL;
}

static PyObject *
set_default_verify_paths(PySSLContext *self, PyObject *unused)
{
    if (!SSL_CTX_set_default_verify_paths(self->ctx)) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    Py_RETURN_NONE;
}

#ifndef OPENSSL_NO_ECDH
static PyObject *
set_ecdh_curve(PySSLContext *self, PyObject *name)
{
    PyObject *name_bytes;
    int nid;
    EC_KEY *key;

    if (!PyUnicode_FSConverter(name, &name_bytes))
        return NULL;
    assert(PyBytes_Check(name_bytes));
    nid = OBJ_sn2nid(PyBytes_AS_STRING(name_bytes));
    Py_DECREF(name_bytes);
    if (nid == 0) {
        PyErr_Format(PyExc_ValueError,
                     "unknown elliptic curve name %R", name);
        return NULL;
    }
    key = EC_KEY_new_by_curve_name(nid);
    if (key == NULL) {
        _setSSLError(NULL, 0, __FILE__, __LINE__);
        return NULL;
    }
    SSL_CTX_set_tmp_ecdh(self->ctx, key);
    EC_KEY_free(key);
    Py_RETURN_NONE;
}
#endif

static PyGetSetDef context_getsetlist[] = {
    {"options", (getter) get_options,
                (setter) set_options, NULL},
    {"verify_mode", (getter) get_verify_mode,
                    (setter) set_verify_mode, NULL},
    {NULL},            /* sentinel */
};

static struct PyMethodDef context_methods[] = {
    {"_wrap_socket", (PyCFunction) context_wrap_socket,
                       METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_ciphers", (PyCFunction) set_ciphers,
                    METH_VARARGS, NULL},
    {"_set_npn_protocols", (PyCFunction) _set_npn_protocols,
                           METH_VARARGS, NULL},
    {"load_cert_chain", (PyCFunction) load_cert_chain,
                        METH_VARARGS | METH_KEYWORDS, NULL},
    {"load_dh_params", (PyCFunction) load_dh_params,
                       METH_O, NULL},
    {"load_verify_locations", (PyCFunction) load_verify_locations,
                              METH_VARARGS | METH_KEYWORDS, NULL},
    {"session_stats", (PyCFunction) session_stats,
                      METH_NOARGS, NULL},
    {"set_default_verify_paths", (PyCFunction) set_default_verify_paths,
                                 METH_NOARGS, NULL},
#ifndef OPENSSL_NO_ECDH
    {"set_ecdh_curve", (PyCFunction) set_ecdh_curve,
                       METH_O, NULL},
#endif
    {NULL, NULL}        /* sentinel */
};

static PyTypeObject PySSLContext_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_ssl._SSLContext",                        /*tp_name*/
    sizeof(PySSLContext),                      /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)context_dealloc,               /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_reserved*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    0,                                         /*tp_getattro*/
    0,                                         /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    0,                                         /*tp_doc*/
    0,                                         /*tp_traverse*/
    0,                                         /*tp_clear*/
    0,                                         /*tp_richcompare*/
    0,                                         /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    0,                                         /*tp_iternext*/
    context_methods,                           /*tp_methods*/
    0,                                         /*tp_members*/
    context_getsetlist,                        /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    0,                                         /*tp_dictoffset*/
    0,                                         /*tp_init*/
    0,                                         /*tp_alloc*/
    context_new,                               /*tp_new*/
};



#ifdef HAVE_OPENSSL_RAND

/* helper routines for seeding the SSL PRNG */
static PyObject *
PySSL_RAND_add(PyObject *self, PyObject *args)
{
    char *buf;
    int len;
    double entropy;

    if (!PyArg_ParseTuple(args, "s#d:RAND_add", &buf, &len, &entropy))
        return NULL;
    RAND_add(buf, len, entropy);
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(PySSL_RAND_add_doc,
"RAND_add(string, entropy)\n\
\n\
Mix string into the OpenSSL PRNG state.  entropy (a float) is a lower\n\
bound on the entropy contained in string.  See RFC 1750.");

static PyObject *
PySSL_RAND(int len, int pseudo)
{
    int ok;
    PyObject *bytes;
    unsigned long err;
    const char *errstr;
    PyObject *v;

    bytes = PyBytes_FromStringAndSize(NULL, len);
    if (bytes == NULL)
        return NULL;
    if (pseudo) {
        ok = RAND_pseudo_bytes((unsigned char*)PyBytes_AS_STRING(bytes), len);
        if (ok == 0 || ok == 1)
            return Py_BuildValue("NO", bytes, ok == 1 ? Py_True : Py_False);
    }
    else {
        ok = RAND_bytes((unsigned char*)PyBytes_AS_STRING(bytes), len);
        if (ok == 1)
            return bytes;
    }
    Py_DECREF(bytes);

    err = ERR_get_error();
    errstr = ERR_reason_error_string(err);
    v = Py_BuildValue("(ks)", err, errstr);
    if (v != NULL) {
        PyErr_SetObject(PySSLErrorObject, v);
        Py_DECREF(v);
    }
    return NULL;
}

static PyObject *
PySSL_RAND_bytes(PyObject *self, PyObject *args)
{
    int len;
    if (!PyArg_ParseTuple(args, "i:RAND_bytes", &len))
        return NULL;
    return PySSL_RAND(len, 0);
}

PyDoc_STRVAR(PySSL_RAND_bytes_doc,
"RAND_bytes(n) -> bytes\n\
\n\
Generate n cryptographically strong pseudo-random bytes.");

static PyObject *
PySSL_RAND_pseudo_bytes(PyObject *self, PyObject *args)
{
    int len;
    if (!PyArg_ParseTuple(args, "i:RAND_pseudo_bytes", &len))
        return NULL;
    return PySSL_RAND(len, 1);
}

PyDoc_STRVAR(PySSL_RAND_pseudo_bytes_doc,
"RAND_pseudo_bytes(n) -> (bytes, is_cryptographic)\n\
\n\
Generate n pseudo-random bytes. is_cryptographic is True if the bytes\
generated are cryptographically strong.");

static PyObject *
PySSL_RAND_status(PyObject *self)
{
    return PyLong_FromLong(RAND_status());
}

PyDoc_STRVAR(PySSL_RAND_status_doc,
"RAND_status() -> 0 or 1\n\
\n\
Returns 1 if the OpenSSL PRNG has been seeded with enough data and 0 if not.\n\
It is necessary to seed the PRNG with RAND_add() on some platforms before\n\
using the ssl() function.");

static PyObject *
PySSL_RAND_egd(PyObject *self, PyObject *args)
{
    PyObject *path;
    int bytes;

    if (!PyArg_ParseTuple(args, "O&:RAND_egd",
                          PyUnicode_FSConverter, &path))
        return NULL;

    bytes = RAND_egd(PyBytes_AsString(path));
    Py_DECREF(path);
    if (bytes == -1) {
        PyErr_SetString(PySSLErrorObject,
                        "EGD connection failed or EGD did not return "
                        "enough data to seed the PRNG");
        return NULL;
    }
    return PyLong_FromLong(bytes);
}

PyDoc_STRVAR(PySSL_RAND_egd_doc,
"RAND_egd(path) -> bytes\n\
\n\
Queries the entropy gather daemon (EGD) on the socket named by 'path'.\n\
Returns number of bytes read.  Raises SSLError if connection to EGD\n\
fails or if it does provide enough data to seed PRNG.");

#endif



/* List of functions exported by this module. */

static PyMethodDef PySSL_methods[] = {
    {"_test_decode_cert",       PySSL_test_decode_certificate,
     METH_VARARGS},
#ifdef HAVE_OPENSSL_RAND
    {"RAND_add",            PySSL_RAND_add, METH_VARARGS,
     PySSL_RAND_add_doc},
    {"RAND_bytes",          PySSL_RAND_bytes, METH_VARARGS,
     PySSL_RAND_bytes_doc},
    {"RAND_pseudo_bytes",   PySSL_RAND_pseudo_bytes, METH_VARARGS,
     PySSL_RAND_pseudo_bytes_doc},
    {"RAND_egd",            PySSL_RAND_egd, METH_VARARGS,
     PySSL_RAND_egd_doc},
    {"RAND_status",         (PyCFunction)PySSL_RAND_status, METH_NOARGS,
     PySSL_RAND_status_doc},
#endif
    {NULL,                  NULL}            /* Sentinel */
};


#ifdef WITH_THREAD

/* an implementation of OpenSSL threading operations in terms
   of the Python C thread library */

static PyThread_type_lock *_ssl_locks = NULL;

static unsigned long _ssl_thread_id_function (void) {
    return PyThread_get_thread_ident();
}

static void _ssl_thread_locking_function
    (int mode, int n, const char *file, int line) {
    /* this function is needed to perform locking on shared data
       structures. (Note that OpenSSL uses a number of global data
       structures that will be implicitly shared whenever multiple
       threads use OpenSSL.) Multi-threaded applications will
       crash at random if it is not set.

       locking_function() must be able to handle up to
       CRYPTO_num_locks() different mutex locks. It sets the n-th
       lock if mode & CRYPTO_LOCK, and releases it otherwise.

       file and line are the file number of the function setting the
       lock. They can be useful for debugging.
    */

    if ((_ssl_locks == NULL) ||
        (n < 0) || ((unsigned)n >= _ssl_locks_count))
        return;

    if (mode & CRYPTO_LOCK) {
        PyThread_acquire_lock(_ssl_locks[n], 1);
    } else {
        PyThread_release_lock(_ssl_locks[n]);
    }
}

static int _setup_ssl_threads(void) {

    unsigned int i;

    if (_ssl_locks == NULL) {
        _ssl_locks_count = CRYPTO_num_locks();
        _ssl_locks = (PyThread_type_lock *)
            malloc(sizeof(PyThread_type_lock) * _ssl_locks_count);
        if (_ssl_locks == NULL)
            return 0;
        memset(_ssl_locks, 0,
               sizeof(PyThread_type_lock) * _ssl_locks_count);
        for (i = 0;  i < _ssl_locks_count;  i++) {
            _ssl_locks[i] = PyThread_allocate_lock();
            if (_ssl_locks[i] == NULL) {
                unsigned int j;
                for (j = 0;  j < i;  j++) {
                    PyThread_free_lock(_ssl_locks[j]);
                }
                free(_ssl_locks);
                return 0;
            }
        }
        CRYPTO_set_locking_callback(_ssl_thread_locking_function);
        CRYPTO_set_id_callback(_ssl_thread_id_function);
    }
    return 1;
}

#endif  /* def HAVE_THREAD */

PyDoc_STRVAR(module_doc,
"Implementation module for SSL socket operations.  See the socket module\n\
for documentation.");


static struct PyModuleDef _sslmodule = {
    PyModuleDef_HEAD_INIT,
    "_ssl",
    module_doc,
    -1,
    PySSL_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


static void
parse_openssl_version(unsigned long libver,
                      unsigned int *major, unsigned int *minor,
                      unsigned int *fix, unsigned int *patch,
                      unsigned int *status)
{
    *status = libver & 0xF;
    libver >>= 4;
    *patch = libver & 0xFF;
    libver >>= 8;
    *fix = libver & 0xFF;
    libver >>= 8;
    *minor = libver & 0xFF;
    libver >>= 8;
    *major = libver & 0xFF;
}

PyMODINIT_FUNC
PyInit__ssl(void)
{
    PyObject *m, *d, *r;
    unsigned long libver;
    unsigned int major, minor, fix, patch, status;
    PySocketModule_APIObject *socket_api;
    struct py_ssl_error_code *errcode;
    struct py_ssl_library_code *libcode;

    if (PyType_Ready(&PySSLContext_Type) < 0)
        return NULL;
    if (PyType_Ready(&PySSLSocket_Type) < 0)
        return NULL;

    m = PyModule_Create(&_sslmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);

    /* Load _socket module and its C API */
    socket_api = PySocketModule_ImportModuleAndAPI();
    if (!socket_api)
        return NULL;
    PySocketModule = *socket_api;

    /* Init OpenSSL */
    SSL_load_error_strings();
    SSL_library_init();
#ifdef WITH_THREAD
    /* note that this will start threading if not already started */
    if (!_setup_ssl_threads()) {
        return NULL;
    }
#endif
    OpenSSL_add_all_algorithms();

    /* Add symbols to module dict */
    sslerror_type_slots[0].pfunc = PyExc_OSError;
    PySSLErrorObject = PyType_FromSpec(&sslerror_type_spec);
    if (PySSLErrorObject == NULL)
        return NULL;

    PySSLZeroReturnErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLZeroReturnError", SSLZeroReturnError_doc,
        PySSLErrorObject, NULL);
    PySSLWantReadErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLWantReadError", SSLWantReadError_doc,
        PySSLErrorObject, NULL);
    PySSLWantWriteErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLWantWriteError", SSLWantWriteError_doc,
        PySSLErrorObject, NULL);
    PySSLSyscallErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLSyscallError", SSLSyscallError_doc,
        PySSLErrorObject, NULL);
    PySSLEOFErrorObject = PyErr_NewExceptionWithDoc(
        "ssl.SSLEOFError", SSLEOFError_doc,
        PySSLErrorObject, NULL);
    if (PySSLZeroReturnErrorObject == NULL
        || PySSLWantReadErrorObject == NULL
        || PySSLWantWriteErrorObject == NULL
        || PySSLSyscallErrorObject == NULL
        || PySSLEOFErrorObject == NULL)
        return NULL;
    if (PyDict_SetItemString(d, "SSLError", PySSLErrorObject) != 0
        || PyDict_SetItemString(d, "SSLZeroReturnError", PySSLZeroReturnErrorObject) != 0
        || PyDict_SetItemString(d, "SSLWantReadError", PySSLWantReadErrorObject) != 0
        || PyDict_SetItemString(d, "SSLWantWriteError", PySSLWantWriteErrorObject) != 0
        || PyDict_SetItemString(d, "SSLSyscallError", PySSLSyscallErrorObject) != 0
        || PyDict_SetItemString(d, "SSLEOFError", PySSLEOFErrorObject) != 0)
        return NULL;
    if (PyDict_SetItemString(d, "_SSLContext",
                             (PyObject *)&PySSLContext_Type) != 0)
        return NULL;
    if (PyDict_SetItemString(d, "_SSLSocket",
                             (PyObject *)&PySSLSocket_Type) != 0)
        return NULL;
    PyModule_AddIntConstant(m, "SSL_ERROR_ZERO_RETURN",
                            PY_SSL_ERROR_ZERO_RETURN);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_READ",
                            PY_SSL_ERROR_WANT_READ);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_WRITE",
                            PY_SSL_ERROR_WANT_WRITE);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_X509_LOOKUP",
                            PY_SSL_ERROR_WANT_X509_LOOKUP);
    PyModule_AddIntConstant(m, "SSL_ERROR_SYSCALL",
                            PY_SSL_ERROR_SYSCALL);
    PyModule_AddIntConstant(m, "SSL_ERROR_SSL",
                            PY_SSL_ERROR_SSL);
    PyModule_AddIntConstant(m, "SSL_ERROR_WANT_CONNECT",
                            PY_SSL_ERROR_WANT_CONNECT);
    /* non ssl.h errorcodes */
    PyModule_AddIntConstant(m, "SSL_ERROR_EOF",
                            PY_SSL_ERROR_EOF);
    PyModule_AddIntConstant(m, "SSL_ERROR_INVALID_ERROR_CODE",
                            PY_SSL_ERROR_INVALID_ERROR_CODE);
    /* cert requirements */
    PyModule_AddIntConstant(m, "CERT_NONE",
                            PY_SSL_CERT_NONE);
    PyModule_AddIntConstant(m, "CERT_OPTIONAL",
                            PY_SSL_CERT_OPTIONAL);
    PyModule_AddIntConstant(m, "CERT_REQUIRED",
                            PY_SSL_CERT_REQUIRED);

    /* protocol versions */
#ifndef OPENSSL_NO_SSL2
    PyModule_AddIntConstant(m, "PROTOCOL_SSLv2",
                            PY_SSL_VERSION_SSL2);
#endif
    PyModule_AddIntConstant(m, "PROTOCOL_SSLv3",
                            PY_SSL_VERSION_SSL3);
    PyModule_AddIntConstant(m, "PROTOCOL_SSLv23",
                            PY_SSL_VERSION_SSL23);
    PyModule_AddIntConstant(m, "PROTOCOL_TLSv1",
                            PY_SSL_VERSION_TLS1);

    /* protocol options */
    PyModule_AddIntConstant(m, "OP_ALL",
                            SSL_OP_ALL & ~SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS);
    PyModule_AddIntConstant(m, "OP_NO_SSLv2", SSL_OP_NO_SSLv2);
    PyModule_AddIntConstant(m, "OP_NO_SSLv3", SSL_OP_NO_SSLv3);
    PyModule_AddIntConstant(m, "OP_NO_TLSv1", SSL_OP_NO_TLSv1);
    PyModule_AddIntConstant(m, "OP_CIPHER_SERVER_PREFERENCE",
                            SSL_OP_CIPHER_SERVER_PREFERENCE);
    PyModule_AddIntConstant(m, "OP_SINGLE_DH_USE", SSL_OP_SINGLE_DH_USE);
#ifdef SSL_OP_SINGLE_ECDH_USE
    PyModule_AddIntConstant(m, "OP_SINGLE_ECDH_USE", SSL_OP_SINGLE_ECDH_USE);
#endif
#ifdef SSL_OP_NO_COMPRESSION
    PyModule_AddIntConstant(m, "OP_NO_COMPRESSION",
                            SSL_OP_NO_COMPRESSION);
#endif

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
    r = Py_True;
#else
    r = Py_False;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_SNI", r);

#if HAVE_OPENSSL_FINISHED
    r = Py_True;
#else
    r = Py_False;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_TLS_UNIQUE", r);

#ifdef OPENSSL_NO_ECDH
    r = Py_False;
#else
    r = Py_True;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_ECDH", r);

#ifdef OPENSSL_NPN_NEGOTIATED
    r = Py_True;
#else
    r = Py_False;
#endif
    Py_INCREF(r);
    PyModule_AddObject(m, "HAS_NPN", r);

    /* Mappings for error codes */
    err_codes_to_names = PyDict_New();
    err_names_to_codes = PyDict_New();
    if (err_codes_to_names == NULL || err_names_to_codes == NULL)
        return NULL;
    errcode = error_codes;
    while (errcode->mnemonic != NULL) {
        PyObject *mnemo, *key;
        mnemo = PyUnicode_FromString(errcode->mnemonic);
        key = Py_BuildValue("ii", errcode->library, errcode->reason);
        if (mnemo == NULL || key == NULL)
            return NULL;
        if (PyDict_SetItem(err_codes_to_names, key, mnemo))
            return NULL;
        if (PyDict_SetItem(err_names_to_codes, mnemo, key))
            return NULL;
        Py_DECREF(key);
        Py_DECREF(mnemo);
        errcode++;
    }
    if (PyModule_AddObject(m, "err_codes_to_names", err_codes_to_names))
        return NULL;
    if (PyModule_AddObject(m, "err_names_to_codes", err_names_to_codes))
        return NULL;

    lib_codes_to_names = PyDict_New();
    if (lib_codes_to_names == NULL)
        return NULL;
    libcode = library_codes;
    while (libcode->library != NULL) {
        PyObject *mnemo, *key;
        key = PyLong_FromLong(libcode->code);
        mnemo = PyUnicode_FromString(libcode->library);
        if (key == NULL || mnemo == NULL)
            return NULL;
        if (PyDict_SetItem(lib_codes_to_names, key, mnemo))
            return NULL;
        Py_DECREF(key);
        Py_DECREF(mnemo);
        libcode++;
    }
    if (PyModule_AddObject(m, "lib_codes_to_names", lib_codes_to_names))
        return NULL;
    
    /* OpenSSL version */
    /* SSLeay() gives us the version of the library linked against,
       which could be different from the headers version.
    */
    libver = SSLeay();
    r = PyLong_FromUnsignedLong(libver);
    if (r == NULL)
        return NULL;
    if (PyModule_AddObject(m, "OPENSSL_VERSION_NUMBER", r))
        return NULL;
    parse_openssl_version(libver, &major, &minor, &fix, &patch, &status);
    r = Py_BuildValue("IIIII", major, minor, fix, patch, status);
    if (r == NULL || PyModule_AddObject(m, "OPENSSL_VERSION_INFO", r))
        return NULL;
    r = PyUnicode_FromString(SSLeay_version(SSLEAY_VERSION));
    if (r == NULL || PyModule_AddObject(m, "OPENSSL_VERSION", r))
        return NULL;

    libver = OPENSSL_VERSION_NUMBER;
    parse_openssl_version(libver, &major, &minor, &fix, &patch, &status);
    r = Py_BuildValue("IIIII", major, minor, fix, patch, status);
    if (r == NULL || PyModule_AddObject(m, "_OPENSSL_API_VERSION", r))
        return NULL;

    return m;
}
