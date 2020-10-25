/*
** Copyright (C) 2001-2004 by Carnegie Mellon University.
**
** @OPENSOURCE_HEADER_START@
** 
** Use of the SILK system and related source code is subject to the terms 
** of the following licenses:
** 
** GNU Public License (GPL) Rights pursuant to Version 2, June 1991
** Government Purpose License Rights (GPLR) pursuant to DFARS 252.225-7013
** 
** NO WARRANTY
** 
** ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER 
** PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY 
** PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN 
** "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY 
** KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT 
** LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE, 
** MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE 
** OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT, 
** SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY 
** TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF 
** WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES. 
** LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF 
** CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON 
** CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE 
** DELIVERABLES UNDER THIS LICENSE.
** 
** Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie 
** Mellon University, its trustees, officers, employees, and agents from 
** all claims or demands made against them (and any related losses, 
** expenses, or attorney's fees) arising out of, or relating to Licensee's 
** and/or its sub licensees' negligent use or willful misuse of or 
** negligent conduct or willful misconduct regarding the Software, 
** facilities, or other rights or assistance granted by Carnegie Mellon 
** University under this License, including, but not limited to, any 
** claims of product liability, personal injury, death, damage to 
** property, or violation of any laws or regulations.
** 
** Carnegie Mellon University Software Engineering Institute authored 
** documents are sponsored by the U.S. Department of Defense under 
** Contract F19628-00-C-0003. Carnegie Mellon University retains 
** copyrights in all material produced under this contract. The U.S. 
** Government retains a non-exclusive, royalty-free license to publish or 
** reproduce these documents, or allow others to do so, for U.S. 
** Government purposes only pursuant to the copyright license under the 
** contract clause at 252.227.7013.
** 
** @OPENSOURCE_HEADER_END@
*/

/*
**
** 1.10
** 2004/03/10 22:33:37
** thomasm
*/


/*
**  dynlib - glue code between a run-time dynamic library and an
**  application.  See dynlib.h for the details.
*/

#include "silk.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <limits.h>

#include "utils.h"
#include "dynlib.h"

RCSIDENT("dynlib.c,v 1.10 2004/03/10 22:33:37 thomasm Exp");


/*
 * Representation for a symbol name in the dynlib or plugin.  id is a
 * unique integer for the symbol, name is a string representing the
 * symbol, must_exist is non-zero if the plugin must have a the the
 * named symbol.
*/
typedef struct _dynlibSymbolStruct {
  dynlibSymbolId id;
  int8_t must_exist;
  char *name;
} dynlibSymbolStruct;

static const dynlibSymbolStruct dynlibSyms[] = {
  {DYNLIB_SETUP,       1, "setup"},
  {DYNLIB_TEARDOWN,    1, "teardown"},
  {DYNLIB_INITIALIZE,  0, "initialize"},
  {DYNLIB_OPT_USAGE,   0, "optionsUsage"},
  {DYNLIB_EXCL_FILTER, 0, "check"},
  {DYNLIB_SHAR_FILTER, 0, "filter"},
  {DYNLIB_CUT,         0, "cut"},
#if 0 /* NOT_YET_IMPLEMENTED */
  {DYNLIB_COUNT,       0, "count"},
  {DYNLIB_SET,         0, "set"},
  {DYNLIB_TOTAL,       0, "total"},
#endif
};
static const int dynlibSymsCount=sizeof(dynlibSyms)/sizeof(dynlibSymbolStruct);

/* Name of envar that if set will enable debugging output */
#define DYNLIB_DEBUG_ENVAR "SILK_DYNLIB_DEBUG"

/* This variable is 1 to print debugging output, 0 to not debug, -1 if
 * the above envar is not yet checked */
static int dynlibDebug = -1;


/* Copy the last dlerror() info the dlISP */
#define dynlibCacheError(dlISP) \
  {strncpy((dlISP)->dlLastError, dlerror(), DYNLIB_DLERR_LENGTH-1); \
   (dlISP)->dlLastError[DYNLIB_DLERR_LENGTH-1] = '\0';}


/* Return 1 if the named envar is defined in the environment and has a
 * non-empty value; 0 otherwise */
static int dynlibCheckDebugEnvar(const char *env_name) {
  char *env_value;

  env_value = getenv(env_name);
  if ((NULL != env_value) && ('\0' != env_value[0])) {
    return 1;
  }
  return 0;
}


/* Create the data structure pointer */
dynlibInfoStructPtr dynlibSetup(dynlibSymbolId appType) {
  dynlibInfoStructPtr dlISP;

  /* Check the envar if we haven't */
  if (dynlibDebug < 0) {
    dynlibDebug = dynlibCheckDebugEnvar(DYNLIB_DEBUG_ENVAR);
  }

  /* Create the dlISP */
  dlISP = (dynlibInfoStructPtr)malloc(sizeof(dynlibInfoStruct));
  memset(dlISP, 0, sizeof(dynlibInfoStruct));
  dlISP->dlType = appType;
  return dlISP;
}


/* Load library; call its setup()--which should register options--and
 * set dlStatus to the return value of setup() */
int dynlibCheck(dynlibInfoStructPtr dlISP, const char *dlPath) {
  int i;
  char *name;
  const char* silkPath;
  char pluginPath[PATH_MAX+1];

  /* check input */
  if (dlISP == NULL || dlPath == NULL) {
    return 1;
  }

  /* put pluginPath into known state */
  pluginPath[0] = '\0';

  /* if dlPath does not contain a slash, first look for the plugin in
   * the SILK_SUBDIR_PLUGINS subdirectory of the environment variable
   * named by ENV_SILK_PATH.  If the plugin does not exist there, pass
   * the dlPath as given to dlopen() which will use LD_LIBRARY_PATH or
   * equivalent.
   */
  if ((!strchr(dlPath, '/')) && (silkPath = getenv(ENV_SILK_PATH))) {
    snprintf(pluginPath, PATH_MAX, "%s/%s/%s",
             silkPath, SILK_SUBDIR_PLUGINS, dlPath);
    pluginPath[PATH_MAX] = '\0';
    if (!fileExists(pluginPath)) {
      /* file does not exist.  Fall back to LD_LIBRARY_PATH */
      pluginPath[0] = '\0';
    }
  }

  if ('\0' == pluginPath[0]) {
    strncpy(pluginPath, dlPath, PATH_MAX);
    pluginPath[PATH_MAX] = '\0';
  }

  /* try to open library */
  dlISP->dlHandle = dlopen(pluginPath, RTLD_NOW);
  if (!dlISP->dlHandle) {
    dynlibCacheError(dlISP);
    if (dynlibDebug > 0) {
      fprintf(stderr, "dlopen warning: %s\n", dynlibLastError(dlISP));
    }
    return 1;
  }

  /* verify existence of functions in the plugin: all in dynlibSyms
   * with a non-zero value for "must_exist" must exist, as well as the
   * function for this app type  */
  for (i = 0; i < dynlibSymsCount; ++i) {
    name = dynlibSyms[i].name;
    dlISP->func[dynlibSyms[i].id] =(dynlibFuncPtr)dlsym(dlISP->dlHandle, name);
    if ((dynlibSyms[i].must_exist || dynlibSyms[i].id == dlISP->dlType) &&
        (NULL == dlISP->func[dynlibSyms[i].id])) {
      dynlibCacheError(dlISP);
      fprintf(stderr, "dynlib: error finding symbol '%s': %s\n",
              name, dynlibLastError(dlISP));
      return 1;
    }
  }

  /* call the dynlib's setup() func */
  dlISP->dlStatus = (*(dlISP->func[DYNLIB_SETUP]))(dlISP, dlISP->dlType);
  if (DYNLIB_FAILED == dlISP->dlStatus) {
    if (dynlibDebug > 0) {
      fprintf(stderr, "dynlib: setup() for %s returned FAILED\n", pluginPath);
    }
    return 1;
  }

  if (NULL == (dlISP->dlPath = strdup(pluginPath))) {
    fprintf(stderr, "dynlib: out of memory!\n");
    return 1;
  }

  /* all OK */
  return 0;
}


/* Call the dynamic library's initialize() routine to do any
 * "expensive" initialization */
int dynlibInitialize(dynlibInfoStructPtr dlISP) {
  int status = 0;

  if (dlISP->dlInitialized) {
    /* Been here before */
    return status;
  }
  if (dlISP->func[DYNLIB_INITIALIZE]) {
    /* Call plugin's initialize() function */
    status = (*(dlISP->func[DYNLIB_INITIALIZE]))(dlISP, dlISP->dlType);
  }
  if (0 == status) {
    /* Either initialize() returned o.k., or there was no initialize()
     * function to call */
    dlISP->dlInitialized = 1;
  }

  return status;
}


/* Print to STDERR the usage for this dynamic library */
void dynlibOptionsUsage(dynlibInfoStructPtr dlISP) {
  if (dlISP->func[DYNLIB_OPT_USAGE]) {
    (*(dlISP->func[DYNLIB_OPT_USAGE]))(dlISP->dlType);
  }
}


/* Get the processing function, caller should check dlActive first */
dynlibFuncPtr dynlibGetRWProcessor(dynlibInfoStructPtr dlISP) {
  return (dynlibFuncPtr)dlISP->func[dlISP->dlType];
}


/* Call the dynlib's teardown() method; free memory */
void dynlibTeardown(dynlibInfoStructPtr dlISP) {
  if (!dlISP) {
    return;
  }
  if (dlISP->dlDoNotClose) {
    return;
  }
  if (dlISP->dlPath) {
    (dlISP->func[DYNLIB_TEARDOWN])(dlISP->dlType);
    dlclose(dlISP->dlHandle);
    free(dlISP->dlPath);
    dlISP->dlPath = (char *)NULL;
  }
  free(dlISP);
  dlISP = NULL;
  return;
}
