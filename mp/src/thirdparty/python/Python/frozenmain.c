
/* Python interpreter main program for frozen scripts */

#include "Python.h"
#include <locale.h>

#ifdef MS_WINDOWS
extern void PyWinFreeze_ExeInit(void);
extern void PyWinFreeze_ExeTerm(void);
extern int PyInitFrozenExtensions(void);
#endif

/* Main program */

int
Py_FrozenMain(int argc, char **argv)
{
    char *p;
    int i, n, sts;
    int inspect = 0;
    int unbuffered = 0;
    char *oldloc;
    wchar_t **argv_copy = PyMem_Malloc(sizeof(wchar_t*)*argc);
    /* We need a second copies, as Python might modify the first one. */
    wchar_t **argv_copy2 = PyMem_Malloc(sizeof(wchar_t*)*argc);

    Py_FrozenFlag = 1; /* Suppress errors from getpath.c */

    if ((p = Py_GETENV("PYTHONINSPECT")) && *p != '\0')
        inspect = 1;
    if ((p = Py_GETENV("PYTHONUNBUFFERED")) && *p != '\0')
        unbuffered = 1;

    if (unbuffered) {
        setbuf(stdin, (char *)NULL);
        setbuf(stdout, (char *)NULL);
        setbuf(stderr, (char *)NULL);
    }

    if (!argv_copy) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    oldloc = setlocale(LC_ALL, NULL);
    setlocale(LC_ALL, "");
    for (i = 0; i < argc; i++) {
#ifdef HAVE_BROKEN_MBSTOWCS
        size_t argsize = strlen(argv[i]);
#else
        size_t argsize = mbstowcs(NULL, argv[i], 0);
#endif
        size_t count;
        if (argsize == (size_t)-1) {
            fprintf(stderr, "Could not convert argument %d to string\n", i);
            return 1;
        }
        argv_copy[i] = PyMem_Malloc((argsize+1)*sizeof(wchar_t));
        argv_copy2[i] = argv_copy[i];
        if (!argv_copy[i]) {
            fprintf(stderr, "out of memory\n");
            return 1;
        }
        count = mbstowcs(argv_copy[i], argv[i], argsize+1);
        if (count == (size_t)-1) {
            fprintf(stderr, "Could not convert argument %d to string\n", i);
            return 1;
        }
    }
    setlocale(LC_ALL, oldloc);

#ifdef MS_WINDOWS
    PyInitFrozenExtensions();
#endif /* MS_WINDOWS */
    Py_SetProgramName(argv_copy[0]);
    Py_Initialize();
#ifdef MS_WINDOWS
    PyWinFreeze_ExeInit();
#endif

    if (Py_VerboseFlag)
        fprintf(stderr, "Python %s\n%s\n",
            Py_GetVersion(), Py_GetCopyright());

    PySys_SetArgv(argc, argv_copy);

    n = PyImport_ImportFrozenModule("__main__");
    if (n == 0)
        Py_FatalError("__main__ not frozen");
    if (n < 0) {
        PyErr_Print();
        sts = 1;
    }
    else
        sts = 0;

    if (inspect && isatty((int)fileno(stdin)))
        sts = PyRun_AnyFile(stdin, "<stdin>") != 0;

#ifdef MS_WINDOWS
    PyWinFreeze_ExeTerm();
#endif
    Py_Finalize();
    for (i = 0; i < argc; i++) {
        PyMem_Free(argv_copy2[i]);
    }
    PyMem_Free(argv_copy);
    PyMem_Free(argv_copy2);
    return sts;
}
