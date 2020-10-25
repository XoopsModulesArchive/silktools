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
** 1.36
** 2004/03/10 22:29:13
** thomasm
*/

/*
**
**  10/14/2002
** Suresh L. Konda
**
** fglob.c
**
** routines for globbing files in the special dir hierarchy we have for
** packed files.
**
** There are three externally visible routines:
**  fglobSetup(): setup stuff
**  fglobNext():  call repeatedly till no more files
**  fglobTeardown(): clean up
**  fglobValid(): 1 if user specified any params; 0 else.
**
**  There is also a set of GetXX routines to gain access to the internals
**  of the fglob data structure.
**  
** Details are given as block comments before these routines.
** 
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <glob.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <limits.h>

#include "utils.h"
#include "rwpack.h"		

/* Include site-specific declarations */
#define DECLARE_FGLOB_VARIABLES
#define DECLARE_SITE_VARIABLES
#define DECLARE_CLASSINFO_VARIABLES
#include "silk_site.h"


RCSIDENT("fglob.c,v 1.36 2004/03/10 22:29:13 thomasm Exp");

#ifndef	TEST_FGLOB
extern const char *appName;
#else
char *appName;
#endif

typedef char *charPtr;

/*
**  Structure for all pertinent information for a given find request.
*/

#define max(a, b) (((a) > (b)) ? (a) : (b))

typedef struct fglobListStruct {
  int used;			/* 1 if user specified ANY globbing params*/
  int classNumber;
  char *class;			/* The specified class */
  int numTypes;			/* Number of types designated. */
  int type[MAX_TYPES_PER_CLASS]; /* types specific to the class */
  int curType;
  char *types;
  int32_t initialized;		/* flag  the globber was initialized */
  int32_t stJD, endJD, curJD;	/* start, end,  and cur julian dates */
  int stHH, endHH, curHH, numHH;/* start, end, cur, and num hours */
  uint32_t numFiles;		/* # of files in current directory */
  char dirPath[PATH_MAX];	/* current directory path */
  charPtr (*fileNames)[1];	/* names of files in this directory */
  uint32_t nextFile;		/* index of the next file to be processed */
  uint8_t  allSensorsFlag;	/* set to 1 by default */
  uint8_t  sensorList[256];	/* indices of sensor ID's wanted  */
  uint8_t  numSensorsWanted;	/* number explicitly given */
  uint8_t  curSensor;		/* which one are we working on now */
} fglobListStruct;
typedef fglobListStruct *  fglobListStructPtr;

static fglobListStruct *fList = (fglobListStruct *)NULL;

/* FIXME. The description fo the type option should be constructed
 * dynamically from the type info array. */
static struct option fglobOptions[] =
{
  {"class", REQUIRED_ARG, 0, 2},
  {"type", REQUIRED_ARG, 0, 3},
  {"start-date", REQUIRED_ARG, 0, 0},
  {"end-date", REQUIRED_ARG, 0, 1},
#if (SENSOR_COUNT > 0)
  {"sensors", REQUIRED_ARG, 0, 4},
#endif
  {"verbose-fglob", NO_ARG, 0, 5},
  {"frail", NO_ARG, 0, 6},
  {0, 0, 0, 0}
};
static char *fglobHelp[] = {
  NULL, /* text is generated by help */
  NULL, /* text is generated by help */
  "YYYY/MM/DD[:HH]. Default start of today",
  "YYYY/MM/DD[:HH]. Default: start-date",
#if (SENSOR_COUNT > 0)
  "sensor id list. Default all",
#endif
  "report missing files",
  "die when encountering missing files",
  (char *)NULL
};

static int verbose = 0;
static int frail = 0;

static int optionCount = (sizeof(fglobOptions)/sizeof(struct option)) - 1;

#define RETVAL_SIZE 256

/* internal functions */
static void adjustCounters(void);
static int checkClass(char *cp);
static int checkSensors(void);
static int fglobHandler(clientData cData, int index, char *optarg);
static void fglobFreeFNames(void);
static int fglobGlobDir(char *);
static int checkTypes(void);
static int loadNextDir(void);
static int fglobInit(void);
static int parseSensorIDs(const char *);


/*
** parseSensorIDs:
**	given a list (i.e., comma or - delimited set of numbers), set the
**	element in fList->sensorList[i] to the sensorID  if the sensor
**	is wanted where i = 0 .. (numSensorsWanted - 1).  This will allows
**	globbing for each specific sensor desired by looping through
**	the sensorList from 0 .. (numSensorsWanted - 1).
** Input:
**	char *
** Return:
**	0 if OK. 1 else;
** Side Effects:
**	fList->sensorList is modified along with numSensorsWanted and,
**	if all is well, allSensorsFlag is set to 0.
*/
static int parseSensorIDs(const char *sp) {
  unsigned int i, j;
  uint8_t count;
  uint8_t *parsedList;
  int duplicate;
  
  /* set default to all fields */
  memset(fList->sensorList, 0, sizeof(fList->sensorList));
  fList->numSensorsWanted = 0;

  parsedList = skParseNumberList(sp, 0, numSensors, &count);
  if (!parsedList) {
    return 1;
  }

  for (i = 0; i < count; ++i) {
    duplicate = 0;
    for (j = 0; j < fList->numSensorsWanted; ++j) {
      if (parsedList[i] == fList->sensorList[j]) {
        duplicate = 1;
        break;
      }
    }
    if (!duplicate) {
      fList->sensorList[fList->numSensorsWanted++] = parsedList[i];
    }
  }
  free(parsedList);

  return 0;			/* OK */
}


/*********************  exported functions ********************/
    
/*
**  fglobSetup:
**
**  Input:
**  	None.
**  Process:
**  	register fglob options and handler
**
**  Side Effects:
**  	global options setup for handling fglob  options
**	The fList struct is initialized to default values:
**		stJD = endJD = -1 
**  Return:
**  	0 if OK. 1 else.
**
*/
int fglobSetup(void) {
  int c;
  
  c = (sizeof(fglobHelp)/sizeof(char *) - 1);
  if ( c  != optionCount) {
    fprintf(stderr, "fglobSetup: mismatch in options and help %d vs %d\n",
	    optionCount, c);
    return 1;
  }
    
  if ( (fList = (fglobListStruct *)malloc(sizeof(fglobListStruct)))
       == (fglobListStruct *)NULL) {
    fprintf(stderr, "fglobSetup: Out of mem\n");
    return 1;
  }
  memset(fList, 0, sizeof(fglobListStruct));
  /* set start/end days/hours to be able to trap whether user gave
  **   it explicitly
  */
  fList->stHH = fList->endHH = fList->stJD = fList->endJD = -1;

  /* select all sensors by default */
  memset(fList->sensorList, 1, sizeof(fList->sensorList));
  fList->allSensorsFlag = 1;
  
  /* set default class depending on the situation */
  fList->classNumber = DEFAULT_CLASS;
  fList->class =  classInfo[fList->classNumber].className;
  
  /* Types will default later */

  if (optionsRegister((void *)fglobOptions, fglobHandler,
		      (clientData) fList)) {
    fprintf(stderr, "fglobSetup: Unable to register options\n");
    return 1;
  }
  return 0;			/* OK */
}


/*
** fglobUsage:
** 	print help
*/
void fglobUsage(void) {
  unsigned int j, k;
  int i = 0;

  fprintf(stderr, "\nfglob options:\n");

  /* CLASS: Read classInfo to get list of classes; use DEFAULT_CLASS
   * macro to print default class. */
  fprintf(stderr, "--%s %s: class (%s", fglobOptions[i].name,
          (fglobOptions[i].has_arg ? fglobOptions[i].has_arg == 1
           ? "Req. Arg" : "Opt. Arg" : "No Arg"),
          classInfo[0].className);
  for (j = 1; j < NUM_CLASSES; ++j) {
    fprintf(stderr, ",%s", classInfo[j].className);
  }
  fprintf(stderr, "). Default %s\n", classInfo[DEFAULT_CLASS].className);
  ++i;

  /* TYPE: For each class, read the list of types and default list of
   * types from classInfo.  The string names of the types are in the
   * outFInfo structure. */
  fprintf(stderr, "--%s %s: type varies by class:\n", fglobOptions[i].name,
          (fglobOptions[i].has_arg ? fglobOptions[i].has_arg == 1
           ? "Req. Arg" : "Opt. Arg" : "No Arg"));
  for (j = 0; j < NUM_CLASSES; ++j) {
    fprintf(stderr, "\t%s (%s", classInfo[j].className,
            outFInfo[classInfo[j].typeList[0]].dirPrefix);
    /* loop over all the types for the class */
    for (k = 1; k < classInfo[j].numTypes; ++k) {
      fprintf(stderr, ",%s", outFInfo[classInfo[j].typeList[k]].dirPrefix);
    }
    fprintf(stderr, "). Default %s",
            outFInfo[classInfo[j].defaultList[0]].dirPrefix);
    /* loop over all the defaults for the class */
    for (k = 1; k < classInfo[j].numDefaults; ++k) {
      fprintf(stderr, ",%s", outFInfo[classInfo[j].defaultList[k]].dirPrefix);
    }
    fprintf(stderr, "\n");
  }
  ++i;

  /* remaining options */
  for ( ; i < optionCount; i++ ) {
    fprintf(stderr, "--%s %s: %s\n", fglobOptions[i].name,
	    fglobOptions[i].has_arg ? fglobOptions[i].has_arg == 1
	    ? "Req. Arg" : "Opt. Arg" : "No Arg", fglobHelp[i]);
  }
  return;
}


/*
** fglobTeardown:
** 	free the elements in fList which were malloc'd. Dont free the
**	fList struct itself. Check for prev calls and return gracefully
** NOTE: Idempotency ensured by checking fList->class.
*/
void fglobTeardown(void) {
  if ( fList == NULL || fList->used == 0 ) {
    return;
  }
  fglobFreeFNames();
  free(fList);
  fList = NULL;
  return;
}


/*
**  fglobNext:
**
**  Return the next available qualified file in the current directory as
**  specified by the catenation of root with curYR, curMO, curDAY.
**
**  If we run out of files in that directory, they we try the next
**  oldest valid directory.  If we run out of valid directories, we are
**  done.
**
**  Input:
**  	fglobList *;
**  Output:
**  	char *. NULL if done.
*/
char *fglobNext(void) {
  register char *cp;
  static char *retVal = (char *)NULL;
  int rv;

  if (! fList) {
    fprintf(stderr, "fglob: fglobSetup() not called. Abort\n");

  }
  
  if (! fList->initialized) {
    if (fglobInit()) {
      return (char *)NULL;
    }
    fList->initialized = 1;
  }
  
  if (retVal == (char *)NULL) {
    if (NULL == (retVal = (char *)malloc(RETVAL_SIZE))) {
      fprintf(stderr, "fglob: out of memory\n");
      exit(1);
    }
  }
  
  if (fList->nextFile < fList->numFiles) {
    cp = (*fList->fileNames)[fList->nextFile];
    fList->nextFile++;
    /* prepend the directory IFF required */
    if (cp[0] != '/') {
      snprintf(retVal, RETVAL_SIZE, "%s/%s", fList->dirPath, cp);
    } else {
      retVal = cp;
    }
    
    return retVal;
  }
  /* done with all files in current directory. Next directory */
  do {
    if ( (rv = loadNextDir()) > 0) {
      return fglobNext();
    }
  } while (!frail && rv);

  /* no more dirs or files */
  return (char *)NULL;
}
  

/*
 *  fglobInit:
 *
 *  This is an internal function and should be called to initialize things
 *  based on the options given by the user.  Hence this must be called after
 *  the options are parsed. To make life easier for the application programmer,
 *  this routine is called by the first call to fglobNext().
 *
 *  Given a range of days in YYYY/MM/DD:HH:MM:SS - YYYY/MM/DD:HH:MM:SS
 *  format, a root directory which selects from a rw hieararchy,
 *  and a glob-spec to further differentiate between files within a leaf
 *  directory.
 *
 *  We do NOT store all the files at the start in a struct.  The search is
 *  always depth first.
 *
 *  The approach we take is to mark all possible values of yyyy,mm,dd as
 *  valid or not in a bitmap of days.  Then for the oldest valid
 *  yyyy/mm/dd we glob for the files specified by the flagsand return
 *  these one,by,one as requestted via fglobNext().
 *
 *  Return 0 for OK. 1 else;
 *
 */
static int fglobInit(void) {
  int rv;
  unsigned int i, j;

  if (checkTypes() ) {
    /* error */
    return 1;			/* checkType() will dump the msg. */
  }

  /* now check if the sensor list is valid given the class etc. */
  if (checkSensors()) {
    return 1;
  }

  if (verbose || frail) {
    /* Ensure hour-by-hour processing */
    if (fList->stHH < 0) {
      /* yet another place where we implement the way times are
       * handled.  We need a subroutine call to make certain we do
       * this consistently.
       */
      fList->stHH = 0;
      fList->endHH = 23;
    } else if (fList->endHH < 0) {
      fList->endHH = fList->stHH;
    }

    /* Ensure sensor-by-sensor processing */
    if (fList->allSensorsFlag) {
      fList->numSensorsWanted = 0;
      fList->allSensorsFlag = 0;
      for (i = 0; i < numSensors; i++) {
        for (j = 0; j < sensorInfo[i].numClasses; j++) {
          if (sensorInfo[i].classNumbers[j] == fList->classNumber) {
            fList->sensorList[fList->numSensorsWanted] = i;
            ++fList->numSensorsWanted;
            break;
          }
        }
      }
    }
  }
  
  /* set all date/time values if user did not give start-time explicitly */
  if (fList->stJD < 0) {
    time_t ts;
    struct tm *tms;
    if (time(&ts) == (time_t)-1) {
      fprintf(stderr, "Unable to get current time of day: [%s]\n",
	      strerror(errno));
      return 0;
    }
    tms = localtime(&ts);
    fList->stJD =
      julianDate(tms->tm_year + 1900, tms->tm_mon + 1, tms->tm_mday);
    /* since the default is all hours in this day, leave xHH alone */
  } 

  if (fList->endJD < 0) {
    /* user did not give an explicit end time. Use start time */
    fList->endJD = fList->stJD;
  }

  /* set up hourly processing if required */
  if (fList->stHH > -1) {
    /* stHH explicitly given */
    fList->curHH = fList->stHH;	/* start here */
    if (fList->endHH < 0) {
      /* endHH not given but stHH is. Use the latter */
      /* NOTE: If we ever change this behaviour, change fglobGetEndHour()*/
      fList->endHH = fList->stHH;
    }
    fList->numHH = fList->endHH - fList->stHH + 1
		   + (fList->endJD - fList->stJD) * 24;
  } 

  fList->curJD = fList->stJD;
  fList->curType = 0;

  /* glob the first directory */
  do {
    if ( (rv = loadNextDir()) > 0) {
      return 0;
    }
  } while (!frail && rv);

  return 1;
}


/*
** fglobGlobDir:
**	glob files in dir based on date, time, root, and glob-spec
**  Inputs: char * pattern
**  Outputs: # of files that met the specs.
**
**  NOTES:
**  First cd to the target directory.
**  Return to the cwd.
**  
**  Return # of files that qualified if OK.
**
**  Side Effects: fList is modified.
**
*/
static int fglobGlobDir(char *pattern) {
  char cwd[PATH_MAX];
  extern int errno;
  glob_t globStruct;
  unsigned int i;
  char *cp, *new, *p;
  
  if ( getcwd(cwd, sizeof(cwd)) == (char *)NULL) {
    fprintf(stderr, "fglob: getcwd() error = [%s]\n", strerror(errno));
    return 0;
  }
  
  cp = strrchr(pattern, '/');
  *cp = '\0';
  if (chdir(pattern)) {
    *cp = '/';
    if (verbose) {
      fprintf(stderr, "Missing %s\n", pattern);
    }
    return -1;
  }
  *cp = '/';
  p = ++cp;
  /*  printf("fglobGlobDir: pattern = [%s]\n", pattern); */
  memset(&globStruct, 0, sizeof(globStruct));

  if ( (i = glob(p, GLOB_MARK|GLOB_NOSORT, NULL, &globStruct))) {  
    switch (i) {
    case GLOB_NOSPACE:
      fprintf(stderr, "Error globbing for [%s] out of memory\n", pattern);
      break;

#if SK_HAVE_GLOB_ABORTED
    case GLOB_ABORTED:
      fprintf(stderr, "Error globbing for [%s] read error\n", pattern);
      break;
#endif /* SK_HAVE_GLOB_ABORTED */

#if SK_HAVE_GLOB_NOMATCH
    case GLOB_NOMATCH:
#if 0      
      fprintf(stderr, "Error globbing for [%s] no matches found.\n", pattern);
#endif
      if (verbose) {
	fprintf(stderr, "Missing %s\n", pattern);
      }
      break;
#endif /* SK_HAVE_GLOB_NOMATCH */

    default:
      fprintf(stderr, "Error globbing for [%s] not implemented.\n", pattern);
      break;
    }
    goto badReturn;
  }
  
  fglobFreeFNames();	/* clear out previously obtained space */

  /* get space to stash the file names */
  fList->numFiles = globStruct.gl_pathc;
  if (fList->numFiles == 0) {
      goto badReturn;
  }
  fList->fileNames = (charPtr (*)[1])malloc(fList->numFiles * sizeof(char *));
  if (NULL == (fList->fileNames)) {
    fprintf(stderr, "fglob: out of memory\n");
    exit(1);
  }

  for (i = 0; i < fList->numFiles; i++) {
    /* there should only be files here */
    if (globStruct.gl_pathv[i][strlen(globStruct.gl_pathv[i]) - 1] == '/') {
      fprintf(stderr, "Bad logic: leaf dir %s has dir %s in it\n",
	      fList->dirPath, globStruct.gl_pathv[i]);
      goto badReturn;
    }

    /* if the dirPath is in the returned path, only save the file name */
    cp = strstr(globStruct.gl_pathv[i], fList->dirPath);
    if (cp) {
      MALLOCCOPY(new, cp);
    } else {
      MALLOCCOPY(new, globStruct.gl_pathv[i]);
    }
    /* and stash in array */
    (*fList->fileNames)[i] = new;
  }  /* for i */

  fList->nextFile = 0;		/* index of first file to process */
  globfree(&globStruct);
  chdir(cwd);
  return fList->numFiles;

 badReturn:
  globfree(&globStruct);
  chdir(cwd);
  return -1;
}


/*
** loadNextDir:
** 	Prepare pattern for globbing the appropriate directory with the
**	appropriate globbing pattern.  This depends on the settings for
**	hour and sensor.
**  	If fList->stHH is > -1 do hourly selection else daily selection.
**	If allSensorsFlag is 1 then select by sensor. Else all sensors.
**	All the dirty work is done by fglobGlobDir using the pattern.
** Inputs: none
** Outputs:  number of files loaded or 0 for end of all files
*/
static int loadNextDir(void) {
  register int numFiles;
  char pattern[256];
  int year, month, day;
  fileTypeInfo *info = &outFInfo[fList->type[fList->curType]];
  
  
  if (fList->curJD > fList->endJD) {
    return 0;			/* done */
  }

  /*  get the yr, mo, day from the curJD */
  gregorianDate(fList->curJD, &year, &month, &day);
  /* use pattern to hold the new directory spec before making a copy of it*/

  snprintf(pattern, sizeof(pattern), "%s/%s/%04d/%02d/%02d",
           silk_fglob_rootdir, info->pathPrefix, year, month, day);
  

  strcpy(fList->dirPath, pattern);

  if (fList->stHH < 0) {
    /* do entire day . Process the sensor stuff */
    /* NOTE: if we ever change this default behavior, change
     * fglobGetStartHour()*/
    if (fList->allSensorsFlag) {
      /* get all files for all sensors in the day */
      snprintf(pattern, sizeof(pattern), "%s/*", fList->dirPath);
    } else {
      /* specific sensor. */
      snprintf(pattern, sizeof(pattern), "%s/%s-%s*", fList->dirPath,
	       info->filePrefix,
	       sensorIdToName(fList->sensorList[fList->curSensor]));
    } /* specific sensor for all hours */
  } else {
    /* hourly stuff */
    if (fList->numHH <= 0) {
      return 0;
    }
    /*  check and deal with sensor specific stuff */

    /* AJK/mthomas: This used to only be executed when
     * fList->classNumber was either of the valid values but this is
     * ALWAYS the case (i.e., the classNumber is always one of the
     * valid classes in the class info array. FIXME. Validate this is
     * safe. */
       
    /* check what sensors are  wanted */
    if (fList->allSensorsFlag) {
      /* get all files for all sensors for this hour */
      snprintf(pattern, sizeof(pattern), "%s/*.%02d*", fList->dirPath,
               (fList->curHH % 24));
    } else {
      /* a specific sensor wanted */
      snprintf(pattern, sizeof(pattern), "%s/%s-%s_%04d%02d%02d.%02d*",
               fList->dirPath, info->filePrefix, 
               sensorIdToName(fList->sensorList[fList->curSensor]),
               year, month, day, (fList->curHH % 24));
    } /* specific sensor */

  }
  
  numFiles = fglobGlobDir(pattern);
  adjustCounters();
  return numFiles;
}


static void adjustCounters(void) {
  if (fList->stHH < 0) {
    /* by day */
    if (fList->allSensorsFlag == 0) {
      /* by sensor */
      if (fList->curSensor >= (fList->numSensorsWanted-1)) {
	/* by type */
	fList->curSensor = 0;
	if (fList->curType >= (fList->numTypes - 1)) {
	  /* no more sensors or types for this day - go to the next
	     day & first sensor and type */
	  fList->curType = 0;
	  fList->curJD++;
	  return;
	} else {
	  /* next type -- same day */
	  fList->curType++;
	  return;
	}
      } else {
	if (fList->curType >= (fList->numTypes - 1)) {
	  /* no more sensors or types for this day - go to the next
	     day & first sensor and type */
	  fList->curType = 0;
	  fList->curJD++;
	  return;
	} else {
	  fList->curSensor++;
	  return;
	}
      }
    } else {
      if (fList->curType >= (fList->numTypes - 1)) {
	/* all sensors -- go to the next day*/
	fList->curType = 0;
	fList->curJD++;
	return;
      } else {
	/* next type -- same sensors, same day */
	fList->curType++;
	return;
      }
    }
  }
  /* by the hour. check sensors */

  /* AJK/mthomas: This used to only be executed when
   * fList->classNumber was either of the valid values, but this is
   * ALWAYS the case (i.e., the classNumber is always one of the
   * valid classes in the class info array. FIXME. Validate this is
   * safe. */
  if (fList->allSensorsFlag == 0) {
    /* by sensor */
    if (fList->curSensor >= (fList->numSensorsWanted-1)) {
      /* by type */
      fList->curSensor = 0;
      if (fList->curType >= (fList->numTypes - 1)) {
        /*
        ** no more sensors for this hour.
        ** roll over to the next hour  appropriately & first sensor
        */
        fList->curType = 0;
        fList->curHH++;
        fList->numHH--;
        if ( (fList->curHH % 24) == 0) {
          fList->curJD++;
        }
        return;
      } else {
        /* just bump the type index */
        fList->curType++;
        return;
      }
    } else {
      /* just bump the sensor index */
      fList->curSensor++;
      return;
    }
  }

  
  if (fList->curType >= (fList->numTypes - 1)) {
    /* by type */
    /* either not [classes] or all sensors wanted. Roll over hours*/
    fList->curType = 0;
    fList->curHH++;
    fList->numHH--;
    if ( (fList->curHH % 24) == 0) {
      fList->curJD++;
    }
    return;
  }
  /* bump type */
  fList->curType++;
  return;
}


/*
 *
 *  free only the space used for fileNames. Check for previous calls and
 *  return gracefully
 *
 */
static void fglobFreeFNames(void) {
  register unsigned int i;
  if (! fList->fileNames) {
    return;
  }
  for(i = 0; i < fList->numFiles; i++) {
    free((*fList->fileNames)[i]);
  }
  free(fList->fileNames);
  fList->fileNames =  (charPtr (*)[1])NULL;
  return;
}


/*
** checkSensors:
** 	Check that the sensor list is good given the class/type.
** Inputs: None directly. Indirectly use the global fList;
** Outputs: 0 if ok. 1 else.
** Side Effects:
**	None.
*/
static int checkSensors(void) {
  int i, j;

  if (fList->allSensorsFlag) {
    return 0;
  }

  for (i = 0; i < fList->numSensorsWanted; i++) {
    sensorinfo_t *info = &sensorInfo[fList->sensorList[i]];
    int foundit = 0;
    for (j=0;j<info->numClasses;j++) {
      if (fList->classNumber == info->classNumbers[j]) {
        foundit = 1;
        break;
      }
    }
    if (!foundit) {
	fprintf(stderr, "Invalid sensor %u (%s) for class %s\n",
                fList->sensorList[i], info->sensorName, fList->class);
	return 1;
    }
  }

  return 0;
}


/*
** checkType:
** 	Check that the type of data is good given the class.
** Inputs: None
** Outputs: 0 if ok. 1 else.
** Side Effects:
*/
int checkTypes(void) {
  unsigned int i, flag;
  size_t n;
  char *cp;
  char *nextcomma;
  char *ctype;

  fList->numTypes = 0;
  
  if (fList->types == NULL) {
    for (i = 0; i < classInfo[fList->classNumber].numDefaults; ++i) {
      fList->type[i] = classInfo[fList->classNumber].defaultList[i];
    }
    fList->numTypes = classInfo[fList->classNumber].numDefaults;
    return 0;
  }

  if (strcmp(fList->types, "all") == 0) {

    for (i = 0; i < NUM_CLASSES; i++) {
      if (strcmp(fList->class, outFInfo[i].className) == 0) {
	fList->type[fList->numTypes++] = i;
      }
    }

    return 0;
  }

  ctype = fList->types;
  do {
    nextcomma = strchr(ctype, ',');
    n = nextcomma ? (size_t)(nextcomma - ctype) : strlen(ctype);
    flag = 0;
    for (i = 0; i < classInfo[fList->classNumber].numTypes; i++) 
    {
      int type = classInfo[fList->classNumber].typeList[i];

      /* Make sure the string matches the directory prefix for one of
       * the valid types for the class */
      if (!strncmp(outFInfo[type].dirPrefix, ctype,
		   max(n, strlen(outFInfo[type].dirPrefix)))) {
	fList->type[fList->numTypes++] = type;
	flag = 1;
	break;
      }

    }
    if (!flag) {
      fprintf(stderr, "Invalid type %s for class %s\n", ctype,
	      classInfo[fList->classNumber].className);
      fprintf(stderr, "Valid types for class %s are:\n", fList->class);
      for (i = 0; i < classInfo[fList->classNumber].numTypes; i++) {
	cp = outFInfo[classInfo[fList->classNumber].typeList[i]].dirPrefix;
	if (!cp) {
	  break;
	}
	fprintf(stderr, "\t%s\n", cp);
      }
  
      return 1;
      
    }
    ctype = nextcomma + 1;
  } while (nextcomma);

  return 0;
}


static int fglobHandler(clientData UNUSED(cData), int index, char *optarg) {
  
  fList->used = 1;
  switch (index) {
  case 0: /* start date */
    /* never returns on error */
    if  ( (fList->stJD = getJDFromDate(optarg, &fList->stHH)) < 0) {
      return 1;			/* error */
    }
    break;

  case 1: /* end date */
    /* never returns on error */
    if  ( (fList->endJD = getJDFromDate(optarg, &fList->endHH)) < 0) {
      return 1;			/* error */
    }
    if (fList->endJD < fList->stJD) {
      fprintf(stderr, "End date < start date.\n");
      return 1;			/* error */
    }
    break;

  case 2: /* class */
    if (checkClass(optarg)) {
      return 1;			/* error */
    }
    break;

  case 3: /* the type */
    /* for now, simply record the type into the struct. Check after finishing
       processing all options */
    fList->types = optarg;
    break;

  case 4: /* sensor list */
    if (parseSensorIDs(optarg)) {
      fprintf(stderr, "invalid sensor list %s\n", optarg);
      return 1;
    }
    fList->allSensorsFlag = 0;
    break;

  case 5:			/* verbose-fglob */
    verbose = 1;
    break;

  case 6:			/* frail */
    frail = 1;
    break;

  default:
    fprintf(stderr, "fglobHandler: invalid index %d\n", index);
    return 1;			/* error */

  } /* switch */


  return 0;			/* OK */
}


static int checkClass(char *cp) {
  register int i;
  unsigned int j;

  for (i = 0; i < NUM_CLASSES; i++) {
    if (0 == strcmp(cp, classInfo[i].className)) {
      fList->classNumber = i;
      fList->class = classInfo[i].className;
      for (j = 0; j < classInfo[i].numDefaults; ++j) {
        fList->type[j] = classInfo[i].defaultList[j];
      }
      fList->numTypes = classInfo[i].numDefaults;
      return 0;
    }
  }
  /* if we get here, we could not find the given class */
  fprintf(stderr, "invalid class %s\n", cp);
  return 1;
}


int fglobValid(void) {
  if (!fList) {
    return 0;
  }
  return fList->used;
}


uint32_t fglobGetEpochStart(void) { 
  int32_t y, m, d;
  static struct tm initTm;

  gregorianDate(fList->stJD, &y, &m, &d);
  initTm.tm_sec = 0;
  initTm.tm_min = 0;
  initTm.tm_hour = fList->stHH;
  initTm.tm_mday = d;
  initTm.tm_mon = m - 1;
  initTm.tm_year = y - 1900;
  return mktime(&initTm);
}


uint32_t fglobGetEpochEnd(void) { 
  int32_t y, m, d;
  static struct tm initTm;

  gregorianDate(fList->endJD, &y, &m, &d);
  initTm.tm_sec = 59;
  initTm.tm_min = 59;
  initTm.tm_hour = fList->endHH;
  initTm.tm_mday = d;
  initTm.tm_mon = m - 1;
  initTm.tm_year = y - 1900;
  return mktime(&initTm);
}


uint32_t fglobGetStartHour(void) {
  if (fList->stHH < 0) {
    return 0;
  } else {
    return fList->stHH;
  }
}


uint32_t fglobGetEndHour(void) {
  if (fList->endHH < 0) {
    return fList->stHH;
  } else {
    return fList->endHH;
  }
}


uint32_t fglobGetStartJulianDate(void) {
  return fList->stJD;
}


uint32_t fglobGetEndJulianDate(void) {
  return fList->endJD;
}


char * fglobGetClass(void) {
  return fList->class;
}


char * fglobGetType(void) {
  return fList->types;
}


/* public setting routine */
int fglobSetStartDate(char *arg) {
  return fglobHandler(NULL, 0, arg);
}


int fglobSetEndDate(char *arg) {
  return fglobHandler(NULL, 1, arg);
}

  
int fglobSetClass(char *arg) {
  return fglobHandler(NULL, 2, arg);
}


int fglobSetType(char *arg) {
  return fglobHandler(NULL, 3, arg);
}


int fglobSetSensors(char *arg) {
  return fglobHandler(NULL, 4, arg);
}


#ifdef	TEST_FGLOB

int main(int argc, char **argv) {
  char *fname;
  int numFiles;
  char arg[128];

  skAppRegister(argv[0]);
#if	0
  if (argc < 2) {
    fglobUsage();
    exit(1);
  }
#endif
  if (optionsSetup(fglobUsage)) {
    fprintf(stderr, "%s: unable to setup options module\n", skAppName());
    exit(1);/* never returns */
  }
  
  if ( fglobSetup()){
    fprintf(stderr, "fglob: error\n");
    exit(1);
  }
  numFiles = 0;
#if	1
  /* for testing command line options parsing */
  if (optionsParse(argc, argv) < 0) {
    fprintf(stderr, "optionsParse error\n");
    fglobUsage();
    exit(1);
  }
#else
  /* for testing application passed options parsing */
  fprintf(stdout, "StartDate? ");fgets(arg, sizeof(arg), stdin);
  if (strlen(arg) > 1) {
    arg[strlen(arg) - 1] = '\0';
    fglobSetStartDate(arg);
  }
  fprintf(stdout, "EndDate? ");fgets(arg, sizeof(arg), stdin);
  if (strlen(arg) > 1) {
    arg[strlen(arg) - 1] = '\0';
    fglobSetEndDate(arg);
  }
  fprintf(stdout, "Class? ");fgets(arg, sizeof(arg), stdin);
  if (strlen(arg) > 1) {
    arg[strlen(arg) - 1] = '\0';
    fglobSetClass(arg);
  }
  fprintf(stdout, "Type? ");fgets(arg, sizeof(arg), stdin);
  if (strlen(arg) > 1) {
    arg[strlen(arg) - 1] = '\0';
    fglobSetType(arg);
  }
  fprintf(stdout, "SensorList? ");fgets(arg, sizeof(arg), stdin);
  if (strlen(arg) > 1) {
    arg[strlen(arg) - 1] = '\0';
    fglobSetSensors(arg);
  }
#endif
  
  while( ( fname = fglobNext()) != (char *)NULL) {
    fprintf(stdout, "%s\n", fname);
    numFiles++;
  }
  fprintf(stdout, "globbed %d files\n", numFiles);
  fglobTeardown();
  exit(0);
}
#endif
