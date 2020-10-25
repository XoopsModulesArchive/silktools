#ifndef _DYNLIB_H
#define _DYNLIB_H
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
** 1.14
** 2004/01/15 17:57:29
** thomasm
*/

/*@unused@*/ static char rcsID_DYNLIB_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "dynlib.h,v 1.14 2004/01/15 17:57:29 thomasm Exp";



#define DYNLIB_FAILED	1
#define DYNLIB_WILLPROCESS 2
#define DYNLIB_WONTPROCESS 3


/*
 *  dynlib provides an interface to C-plugins---dynamic libraries
 *  loaded at run time.
 *
 *  The application that wants to use a plugin does the following:
 *
 *  1. Calls dynlibSetup(appType) where appType is one of the symbols
 *  listed in dynlibSymbolId and describes the type of the
 *  application; this returns an interface pointer, dlISP.
 *
 *  2. Calls dynlibCheck(dlISP, path) where path is the path to the
 *  dynamic library.  If path does not contain a slash, dynlibCheck()
 *  will first look for the plugin in in
 *  $SILK_PATH/SILK_SUBDIR_PLUGINS/path.  If that fails (or if path
 *  contains a slash), the path is handed to dlopen() unaltered.
 *  dlopen() may consult LD_LIBRARY_PATH (or similar).  dynlibCheck()
 *  will verify that the plugin has all the required subroutines, and
 *  it will call the plugin's setup() method which may add options
 *  handlers.
 *
 *  2a. Should the application desire to print any options associated
 *  with the plugin, it may call dynlibOptionsUsage(dlISP) which will
 *  call the plugin's optionsUage() method, if it exists.
 *
 *  3. Once the application has called the options parser, it can use
 *  dynlibCheckActive(dlISP) to see if the plugin should be called as
 *  part of the application's processing--some applications only
 *  become active when the user specifies an option related to the
 *  plugin.
 *
 *  4. If the plugin is active, dynlibGetRWProcessor(dlISP) returns a
 *  pointer to the function that the application should call.  For a
 *  filtering application, dynlibGetRWProcessor() returns a pointer to
 *  the plugin's filter() subroutine.
 *
 *  5. Once processing is completed, the application should call
 *  dynlibTeardown(dlISP) which will free the dlISP data structure and
 *  call teardown() on the plugin.
 *
 *
 *  The interface expects the following C subroutines to exist in the
 *  plugin:
 *
 *  - int setup(dynlibInfoStructPtr dlISP, dynlibSymbolId appType);
 *
 *    This subroutine should set up the plugin; if the plugin has its
 *    own options, they need to be registered now.  setup() should
 *    return one of:
 *
 *      DYNLIB_FAILED - if processing fails
 *      DYNLIB_WONTPROCESS - if application should do normal output
 *      DYNLIB_WILLPROCESS - if this plugin takes over output
 *
 *  - void teardown(dynlibSymbolId appType);
 *
 *    Any cleanup the plugin requires should happen in this
 *    subroutine.
 *
 *  In addition, the following subroutines MAY exist in the plugin:
 *
 *  - int initialize(dynlibInfoStructPtr dlISP, dynlibSymbolId appType);
 *
 *    This subroutine should do any "expensive" initialization
 *    required by the plugin in preparation for calling one of the
 *    interface subroutines listed below.
 *
 *  - void optionsUsage(dynlibSymbolId appType);
 *
 *    If the plugin has registered options, this subroutine should
 *    print a them and a short usage message to stderr.
 *
 *  To be used by an application type, the plugin must provide the
 *  necessary interface subroutine:
 *
 *  DYNLIB_EXCL_FILTER: used by rwfilter
 *    int check(rwRecPtr rwRec);
 *
 *    This subroutine takes over all determination as to whether the
 *    rwRec should pass or fail the filter.  The subroutine should
 *    return 0 if the rwRec passes the filter; 1 otherwise.
 *
 *  DYNLIB_SHAR_FILTER: used by rwfilter
 *    int filter(rwRecPtr rwRec);
 *
 *    This subroutine works in tadem with the standard filtering rules
 *    of rwfilter (and other shared filter plugins) to determine
 *    whether the rwRec should pass the filter.  This subroutine may
 *    not see all rwRec records since they may be filtered by one of
 *    the standard rules.  The subroutine should return 0 if the rwRec
 *    passes the filter; 1 otherwise.
 *
 *    The dynlib plug-in code will call this filter() routine ONCE per
 *    rw-record.  If the user has specified multiple filter-rules
 *    switches which a single plug-in handles, the plugin must process
 *    all the switches before returning.
 *
 *  DYNLIB_CUT: used by rwcut
 *    int cut(unsigned int field, char *out, size_t len_out, rwRecPtr rwrec)
 *
 *  Arguments:
 *      which field to output (indexed from 1)
 *      a string buffer to hold the output
 *      the length of the buffer
 *      a RW record
 *  Returns:
 *      if field is zero, the number of supported fields
 *      if out is NULL and rwrec is NULL, the size of buffer needed
 *         for a title for this field
 *      if out is NULL and rwrec is non-NULL, the size of buffer
 *         needed for the value to be output for this field
 *      if out is non-NULL and rwrec is NULL, outputs the title,
 *         returns length of buffer necessary to hold full string
 *      if out is non-NULL and rwrec is non-NULL, outputs the value,
 *         returns length of buffer necessary to hold full string
 *
 *
 *  DYNLIB_COUNT: used by rwcount
 *    int count(rwRecPtr rwRec)
 *
 *    TO BE DECIDED
 *
 *  DYNLIB_SET: used by rwset
 *    int set(rwRecPtr rwRec)
 *
 *    TO BE DECIDED
 *
 *  DYNLIB_TOTAL: used by rwtotal
 *    int total(rwRecPtr rwRec)
 *
 *    TO BE DECIDED
 *
*/

/* A list of application types.  Keep SETUP at first position and
 * TEARDOWN at last position.  One of these should be passed to
 * dynlibSetup(). */
typedef enum _dynlibSymbolId {
  DYNLIB_SETUP,		/* internal use -- must be first*/
  DYNLIB_OPT_USAGE,	/* internal use */
  DYNLIB_INITIALIZE,    /* internal use */
  DYNLIB_EXCL_FILTER,	/* rwfilter: override other filter rules  */
  DYNLIB_SHAR_FILTER,	/* rwfilter: augemnt other filter rules */
  DYNLIB_CUT,		/* rwcut: output of data */
  DYNLIB_COUNT,		/* rwcount: to be decided */
  DYNLIB_SET,		/* rwset: to be decided */
  DYNLIB_TOTAL,		/* rwtotal: to be decided */
  DYNLIB_TEARDOWN	/* internal use -- must be last */
} dynlibSymbolId;


/* Pointer to function returning int---the actual argument list varies
 * by function.  We store an array of these in the dynlibInfoStruct,
 * and it is the return type of dynlibGetRWProcessor().
*/
typedef int (*dynlibFuncPtr)();


/* Length of dlerror() message we cache */
#define DYNLIB_DLERR_LENGTH 2048


/* The structure that holds information about the plugin.  Do not use
 * directly; use the accessor functions/macros below. */
typedef struct dynlibInfoStruct {
  char *dlPath;			/* path to .so file */
  char dlLastError[DYNLIB_DLERR_LENGTH]; /* most recent value of dlerror() */
  void *dlHandle;		/* handle returned by dlopen */
  int dlStatus;			/* one of failed/will|wont process  */
  int dlActive;			/* non-zero if this dynlib should be called */
  int dlDoNotClose;             /* 1 to not call dlclose()--Oracle issue */
  int dlInitialized;            /* 1 if the initialized() routine was called */
  dynlibSymbolId dlType;	/* type of application or library */
  dynlibFuncPtr func[1+DYNLIB_TEARDOWN-DYNLIB_SETUP];	/* function ptrs */
} dynlibInfoStruct;
typedef dynlibInfoStruct * dynlibInfoStructPtr;


/*
**  dynlibGetPath
**	Return the path to the dynamic library
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	A \0 terminated string.  String should be considered
**	read-only.  Returns the path passed to dlopen to open
**      the dynamic library.
**  Side Effects:
**	None.
*/
#define dynlibGetPath(dlISP) (((dynlibInfoStructPtr)(dlISP))->dlPath)


/*
**  dynlibSetup:
**	get space and initialize struct
**  Inputs:
** 	The type of application
**  Outputs:
**	dynlibInfoStructPtr dlISP;
*/
dynlibInfoStructPtr dynlibSetup(dynlibSymbolId appType);


/*
**  dynlibCheck:
**	check all stuff about potential dynamic lib: load the library
**	and check that it has the necessary functions.  To find Call
**	the dynamic library's setup() subroutine which may register
**	options
**  Input:
**	dynlibInfoStructPtr dlISP
**	path to dyn lib
**  Output:
**	0 if ok; 1 else
**  Side Effects:
**	struct dlISP filled.
*/
int dynlibCheck(dynlibInfoStructPtr dlISP, const char *dlPath);


/*
**  dynlibInitialize:
**      Call the dynamic library's initalize() subroutine.  The idea
**      is to put expensive initialization code in the initialize()
**      routine, and only call it once the application knows the
**      dynamic library will be used.
**
**      Once the initialize() routine returns 0, this wrapper will not
**      call it again; if you must call initialize() multiple times,
**      use the dynlibClearInitialized() macro to clear the
**      'initialized()-was-called' flag.
**  Input:
**	dynlibInfoStructPtr dlISP
**  Output:
**      Returns 0 if
**        -- initialized() does not exist, or
**        -- initialized() returns 0, or
**        -- initialized() previously returned 0
**      Otherwise returns status of call to initialize()
**  Side Effects:
**      Depends on shared library
*/
int dynlibInitialize(dynlibInfoStructPtr dlISP);


/*
**  dynlibOptionsUsage:
**	Calls the dynamic library's optionsUsage() subroutine
**  Input:
**	dynlibInfoStructPtr dlISP
**  Output:
**	none
**  Side Effects:
**	Depends on shared library
*/
void dynlibOptionsUsage(dynlibInfoStructPtr dlISP);


/*
**  dynlibGetRWProcessor:
**	Returns a pointer to the function applicable to this app-type;
**	e.g., for DYNLIB_CUT returns a pointer to cut().  The
**	existence of the function was checked in dynlibCheck
**  Input:
**	dynlibInfoStructPtr dlISP
**  Output:
**	function ptr
**  Side Effects:
**	None.
*/
dynlibFuncPtr dynlibGetRWProcessor(dynlibInfoStructPtr dlISP);

/*
**  dynlibTeardown:
**	close and free mem
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	None
*/
void dynlibTeardown(dynlibInfoStructPtr dlISP);


/*
**  dynlibCheckLoaded
**	Returns status of loading and checking the plugin
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	Return 1 if the plugin was successfully loaded and checked, 0
**	otherwise.
**  Side Effects:
**	None.
**  Notes:
**	dlISP may be NULL
*/
#define dynlibCheckLoaded(dlISP) \
    (NULL!=(dlISP) && NULL!=(((dynlibInfoStructPtr)(dlISP))->dlPath))


/*
**  dynlibGetAppType
**	Returns the application type as set in dynlibSetup()
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	The dynlibSymbolId for this application
**  Side Effects:
**	None.
*/
#define dynlibGetAppType(dlISP) (((dynlibInfoStructPtr)(dlISP))->dlType)


/*
**  dynlibGetStatus
**	Return the status of the call to dynlibCheck()
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	An integer value of:
**
**      DYNLIB_FAILED - if processing fails
**      DYNLIB_WONTPROCESS - if application should do normal output
**      DYNLIB_WILLPROCESS - if this plugin takes over output
**  Side Effects:
**	None.
*/
#define dynlibGetStatus(dlISP)   (((dynlibInfoStructPtr)(dlISP))->dlStatus)


/*
**  dynlibCheckActive
**	Return non-zero if this plugin will be processing the
**	rwRecPtr's and thus should be called.
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	The state of the active flag
**  Side Effects:
**	None.
**  Notes:
**	dlISP may be NULL
*/
#define dynlibCheckActive(dlISP) \
    ((NULL==(dlISP)) ? 0 : ((dynlibInfoStructPtr)(dlISP))->dlActive)


/*
**  dynlibMarkActive
**	Mark the plugin as being active: i.e., the plugin will be
**	processing the rwRecPtr's
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	None.
**  Side Effects:
**	None.
*/
#define dynlibMakeActive(dlISP) {((dynlibInfoStructPtr)(dlISP))->dlActive = 1;}


/*
**  dynlibClearInitialized
**	Clear the flag that indicates that the initialize() routine
**	has been run.  See dynlibInitialize() for details.
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	None.
**  Side Effects:
**	None.
*/
#define dynlibClearInitialized(dlISP) \
    {((dynlibInfoStructPtr)(dlISP))->dlInitialized = 0;}


/*
**  dynlibSetDoNotClose
**	Set the do-not-close flag to TRUE.  This will prevent
**	dlclose() from being called on the plugin.  We do this do work
**	around an Oracle problem with unloading libraries.
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	None.
**  Side Effects:
**	None.
*/
#define dynlibSetDoNotClose(dlISP) \
    {((dynlibInfoStructPtr)(dlISP))->dlDoNotClose = 1;}


/*
**  dynlibLastError
**	Return the most recent value from dlerror()
**  Inputs:
** 	dynlibInfoStructPtr dlISP
**  Outputs:
**	A \0 terminated string.  String should be considered
**	read-only.  The string only contains a valid error message
**	immediately after an error occurred.  At other times, the
**	string may be empty or contain a previous error.
**  Side Effects:
**	None.
*/
#define dynlibLastError(dlISP) (((dynlibInfoStructPtr)(dlISP))->dlLastError)


#endif /*  _DYNLIB_H */
