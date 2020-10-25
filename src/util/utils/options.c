/*
**  Copyright (C) 2001-2004 by Carnegie Mellon University.
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
** 1.10
** 2004/03/10 21:20:15
** thomasm
*/

/*
**  parseoptions.c
**
**  Suresh L Konda
**
**  12/4/2001
**
**  Routines to support long option parsing with multiple sets of options.
**
**  Four functions are exported:
**  optionsSetup();
**  optionsTeardown();
**  optionsRegister();
**  optionsParse();
**
**  Each client calls optionsRegister with:
**	1. a pointer to struct option []
**	2. a handler to process the option. The handler will be called with two
**	   arguments:
**	        1. the clientData
**		2. the original val value passed to the registry via
**		   options associated with this option
**		3. the optarg returned by getopt();
**	3. an opaque pointer to arbitrary data called clientData which
**	Error Return : 1
**
**  Once all clients have registered, then call optionsParse with argc, argv
**  which parses the options and calls the handler as required.
**
**  It returns -1 on error or optind if OK.  Thus, argv[optind] is the first
**  non-option argument given to the application.
**
**  Currently, we do NOT do flag versus val handling: flag is always
**  assumed to be NULL and val is the appropriate unique entity that
**  allows the handler to deal with the option to be parsed.  It is
**  suggested that the caller use a distinct index value in the val part.
**
**
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"



RCSIDENT("options.c,v 1.10 2004/03/10 21:20:15 thomasm Exp");



/*
  struct option has the following definition:
  struct option {
  char *name;
  int has_arg;
  int *flag;
  int val;
};
*/

typedef struct mapOptionStruct {
  int oIndex;			/* original index */
  clientData cData;	/* client data -- opaque pointer */
  optHandler handler;		/* handler */
} mapOptionsStruct;



/* locals */
static struct option (*gOptions)[1] = 0;
static mapOptionsStruct (*oMap)[1] = 0;
/* the following variable is set to 1 if the application registers a
 * --dynamic-library option OPT_DYNAMIC_LIBRARY.  When set to 1, the
 * user's options are rearranged to put the dynamic options to the
 * front.
*/
static int registeredDynamic = 0;
/* global option count */
static int gCount = 0;
/* number of clients */
static int numClients = 0;
static usageFn usageFunction;

static char **moveDynaLibsToFront(const int argc, char **argv);



int optionsSetup(usageFn fn) {
  usageFunction = fn;
  return 0;
}

void optionsTeardown(void) {
  if ( gOptions == 0) {
    return;
  }
  free(gOptions);
  free(oMap);
  gOptions = /* (struct option (*)[1]) */ NULL;
  oMap = /* (mapOptionsStruct (*)[1]) */ NULL;
  return;
}

int optionsRegister(void *inOptions, optHandler handler, clientData cData) {
  struct option (*options)[1] =  (struct option (*)[1])inOptions;
  int i,j;
  int optCount;
  int prevGCount;

  /* count the options */
  for (j = 0; (*options)[j].name; j++)
    ;
  optCount = j;

  /*
   * Get spaces.  In gOptions, leave additional space for our
   * internal help option and for the null entry.  This means that in
   * the end we will be wasting  numClients*2 - 1 entries.
   */
  gOptions = (struct option (*)[1])realloc(gOptions, sizeof(struct option)
					   * (gCount + 2 + optCount));
  if (gOptions == NULL) {
    fprintf(stderr, "Cannot realloc gOptions\n");
    exit(1);
  }
  oMap = (mapOptionsStruct (*)[1])realloc(oMap, sizeof(mapOptionsStruct)
					  * (gCount + 1 + optCount));
  if (oMap == NULL) {
    fprintf(stderr, "Cannot realloc oMap\n");
    exit(1);
  }

  /* check for dups */
  prevGCount = gCount;
  for (j = 0; j < optCount; j++) {
    if (strcmp((*options)[j].name, "help") == 0 ) {
      /* this is an error since the usage function is supposed to be used */
      fprintf(stderr, "optionsRegister: help option found. Invalid\n");
      return 1;
    }
    if (strcmp((*options)[j].name, OPT_DYNAMIC_LIBRARY) == 0) {
      /* will need to rearrange the user's options to put anything
       * matching OPT_DYNAMIC_LIBRARY to the front to be processed
       * first.
       */
      registeredDynamic = 1;
    }
    for(i = 0; i < prevGCount; i++) {
      if (strcmp((*gOptions)[i].name, (*options)[j].name) == 0 ) {
        /* a real name clash */
        fprintf(stderr, "Name clash %s\n", (*options)[j].name);
        return 1;		/* failure */
      }
    }
    /* a clean new entry. record */
    (*gOptions)[gCount].name = (*options)[j].name;
    (*gOptions)[gCount].has_arg = (*options)[j].has_arg;
    (*gOptions)[gCount].flag = (*options)[j].flag;
    (*gOptions)[gCount].val = gCount; /* internal global val is the index into the global array */

    (*oMap)[gCount].oIndex =  (*options)[j].val; /* original val to be returned with handler */
    (*oMap)[gCount].handler = handler;
    (*oMap)[gCount].cData = cData;
    gCount++;
  }
  /* set the sentinal for gOptions */
  memset(&(*gOptions)[gCount], 0, sizeof(struct option));
  numClients++;
  return 0;
}


/*
 *  optionsParse:
 *  	Adjust the global options array to allow for the help
 *  	option. If help is selected by the user, call the stashed
 *  	usageFunction.  Parse input options given a set of
 *  	pre-registered options and their handlers.  For each
 *  	legitimate option, call the handler.
 *  SideEffects:
 *  	The individual handlers update whatever datastruture they wish
 *  	to via the clientData argument to the handler.
 *  Return:
 *  	optind which points at the first non-option argument passed if
 *  	all is OK.  If not OK, the return -1 for error.
*/
int optionsParse(int argc, char **argv) {
  int done = 0;
  int c;

  opterr = 1;

  if (registeredDynamic) {
    argv = moveDynaLibsToFront(argc, argv);
  }

  /* set up the options for help */
  (*gOptions)[gCount].name = "help";
  (*gOptions)[gCount].has_arg = NO_ARG;
  (*gOptions)[gCount].flag = 0;
  (*gOptions)[gCount].val = HELPVALUE;
  gCount++;
  memset(&(*gOptions)[gCount], 0, sizeof(struct option));

  while (! done) {
    int option_index;
#ifdef SK_NEED_GETOPT
    c = _getopt_internal(argc, argv, "", (const struct option *)gOptions,
			 &option_index, 1);
#else
    c = getopt_long_only(argc, argv, "", (const struct option *)gOptions,
			 &option_index);
#endif
    switch (c) {

    case '?':
      /*fprintf(stderr, "Invalid or ambiguous option\n"); */
      return -1;

    case HELPVALUE:
      usageFunction();
      return -1;		/* treat like an error */

    case -1:
      done = 1;
      break;

    default:
      /* a legit value: call the handler */
      if ((*oMap)[c].handler((*oMap)[c].cData, (*oMap)[c].oIndex, optarg)) {
        /* handler signaled error */
        return -1;
      }
      break;
    }
  }
  return optind;
}


/*
 * moveDynaLibsToFront
 *      Rearrange argv so that dynamic library options appear at front
 * Arguments:
 *      argc - int - argument count
 *      argv - char** - arguments
 * Returns:
 *      A char** to use in place of argv as the argument array.  If no
 *      dynamic libraries were found, the returned argv will be same
 *      argv that was passed into the function; otherwise, a new argv
 *      array is created and returned, but it still references the
 *      values in the argv passed into the function.
 * Side Effects:
 *      None
 * Notes:
 *      Only a single option name (but multiple occurances of it) are
 *      moved by this function.  The name of that option is given in
 *      the OPT_DYNAMIC_LIBRARY macro defined in utils.h.  This
 *      function may malloc memory for the returned argv that never
 *      gets freed.
*/
static char **moveDynaLibsToFront(const int argc, char **argv)
{
  int dynlib_count = 0;        /* count of options to be moved */
  int dynlib_moved = 0;        /* count of options actually moved */
  const int max_args = 256;    /* max # of args that may be moved */
  uint8_t is_dynlib[max_args]; /* flags options to be moved */
  char **new_argv;             /* return value */
  int c;
  char *p;
  char *eq_pos;               /* location of = in option */
  const int arg_limit = (argc > max_args ? max_args : argc);

  memset(is_dynlib, 0, sizeof(is_dynlib));

  /* in this first pass, find the options that need to move and set
   * their values in is_dynlib[] to 1 */
  for (c = 1; c < arg_limit; ++c) {
    p = argv[c];
    if ('-' != *p) {
      /* ignore arguments that don't begin with a - */
      continue;
    }
    ++p;
    if ('\0' == *p) {
      /* ignore options that are only a - */
      continue;
    }
    if ('-' == *p) {
      /* options may begin with a - or -- */
      ++p;
      if ('\0' == *p) {
        /* getopt stops processing on a -- */
        break;
      }
    }
    if ('=' == *p) {
      /* ignore --= */
      continue;
    }
    eq_pos = strchr(p, '=');
    if (eq_pos) {
      /* maybe need to move a single argument */
      if (0 == strncmp(OPT_DYNAMIC_LIBRARY, p, eq_pos-p)) {
        is_dynlib[c] = 1;
        ++dynlib_count;
      }
    } else {
      /* maybe need to move two arguments */
      if ((0 ==strncmp(OPT_DYNAMIC_LIBRARY,p,strlen(p))) && (c+1 < arg_limit)){
        is_dynlib[c] = 1;
        is_dynlib[c+1] = 1;
        ++c;
        dynlib_count += 2;
      }
    }
  }

  /* nothing to do */
  if (0 == dynlib_count) {
    return argv;
  }

  /* make a second pass thru argv and copy the options into new_argv */
  new_argv = (char**)calloc(argc, sizeof(char*));
  if (new_argv == NULL) {
    /* alloc failed; return current argv */
    return argv;
  }
  new_argv[0] = argv[0];
  for (c = 1; c < arg_limit; ++c) {
    if (is_dynlib[c]) {
      ++dynlib_moved;
      new_argv[dynlib_moved] = argv[c];
    } else {
      new_argv[dynlib_count - dynlib_moved + c] = argv[c];
    }
  }
  /* if we have more than max_args args, copy them into new_argv[] as
   * well */
  if (argc > max_args) {
    memcpy(new_argv + dynlib_count - dynlib_moved + arg_limit,
           argv + arg_limit, (argc - max_args)*sizeof(char*));
  }

  return new_argv;
}


/*
** checkArg
**	check if the given argument is correct depending on the argType
** Inputs:
**	char *arg; int argType
** Output:
**	0 if ok. 1 else
*/
int checkArg(const char *arg, int argType) {
  register const char *cp;

  cp = arg;
  switch (argType) {
  case INTEGER_ONLY:
    while (*cp) {
      if (! isdigit((int)*cp)) {
	if (*cp != '-') return 1;
      }
      cp++;
    }
    return 0;
    
  case UNSIGNED_ONLY:
    while (*cp) {
      if (! isdigit((int)*cp)) {
	return 1;
      }
      cp++;
    }
    return 0;

  case FLOAT_ONLY:
    while (*cp) {
      if (! isdigit((int)*cp)) {
	if (strchr("+-.", *cp)) {
	  continue;		/* OK in a float */
	} else {
	  return 1;
	}
      }
      cp++;
    }
    return 0;

  default:
    fprintf(stderr, "Invalid argType %d\n", argType);
    return 1;
  }
}
