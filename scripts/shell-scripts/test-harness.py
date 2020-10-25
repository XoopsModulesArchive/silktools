#######################################################################
#  Copyright (C) 2003 by Carnegie Mellon University.
#
# @OPENSOURCE_HEADER_START@
# 
# Use of the SILK system and related source code is subject to the terms 
# of the following licenses:
# 
# GNU Public License (GPL) Rights pursuant to Version 2, June 1991
# Government Purpose License Rights (GPLR) pursuant to DFARS 252.225-7013
# 
# NO WARRANTY
# 
# ANY INFORMATION, MATERIALS, SERVICES, INTELLECTUAL PROPERTY OR OTHER 
# PROPERTY OR RIGHTS GRANTED OR PROVIDED BY CARNEGIE MELLON UNIVERSITY 
# PURSUANT TO THIS LICENSE (HEREINAFTER THE "DELIVERABLES") ARE ON AN 
# "AS-IS" BASIS. CARNEGIE MELLON UNIVERSITY MAKES NO WARRANTIES OF ANY 
# KIND, EITHER EXPRESS OR IMPLIED AS TO ANY MATTER INCLUDING, BUT NOT 
# LIMITED TO, WARRANTY OF FITNESS FOR A PARTICULAR PURPOSE, 
# MERCHANTABILITY, INFORMATIONAL CONTENT, NONINFRINGEMENT, OR ERROR-FREE 
# OPERATION. CARNEGIE MELLON UNIVERSITY SHALL NOT BE LIABLE FOR INDIRECT, 
# SPECIAL OR CONSEQUENTIAL DAMAGES, SUCH AS LOSS OF PROFITS OR INABILITY 
# TO USE SAID INTELLECTUAL PROPERTY, UNDER THIS LICENSE, REGARDLESS OF 
# WHETHER SUCH PARTY WAS AWARE OF THE POSSIBILITY OF SUCH DAMAGES. 
# LICENSEE AGREES THAT IT WILL NOT MAKE ANY WARRANTY ON BEHALF OF 
# CARNEGIE MELLON UNIVERSITY, EXPRESS OR IMPLIED, TO ANY PERSON 
# CONCERNING THE APPLICATION OF OR THE RESULTS TO BE OBTAINED WITH THE 
# DELIVERABLES UNDER THIS LICENSE.
# 
# Licensee hereby agrees to defend, indemnify, and hold harmless Carnegie 
# Mellon University, its trustees, officers, employees, and agents from 
# all claims or demands made against them (and any related losses, 
# expenses, or attorney's fees) arising out of, or relating to Licensee's 
# and/or its sub licensees' negligent use or willful misuse of or 
# negligent conduct or willful misconduct regarding the Software, 
# facilities, or other rights or assistance granted by Carnegie Mellon 
# University under this License, including, but not limited to, any 
# claims of product liability, personal injury, death, damage to 
# property, or violation of any laws or regulations.
# 
# Carnegie Mellon University Software Engineering Institute authored 
# documents are sponsored by the U.S. Department of Defense under 
# Contract F19628-00-C-0003. Carnegie Mellon University retains 
# copyrights in all material produced under this contract. The U.S. 
# Government retains a non-exclusive, royalty-free license to publish or 
# reproduce these documents, or allow others to do so, for U.S. 
# Government purposes only pursuant to the copyright license under the 
# contract clause at 252.227.7013.
# 
# @OPENSOURCE_HEADER_END@
#
#######################################################################

#######################################################################
# This program accepts directory(s) as arguments.  For each directory,
# it looks for executable files in the directory named "*.test".  The
# "*.test" files are sorted in ASCII order and processed.
# 
# For each "BASENAME.test" file it finds, the program first checks to
# see if "BASENAME.pre" is an executable file.  If so, it is executed.
# If it exits with a non-zero status, the remainder of this test is
# skipped.  (I've used "BASENAME.pre" to whine at me if I try to run
# the test on a machine without access to the data).
# 
# mktemp() is used to generate two output file names (which I'll call
# tmp.out and tmp.err here): one for stdout and one for stderr.  The
# script checks to see if "BASENAME.in" exists, then it runs:
#   BASENAME.test < BASENAME.in 1> tmp.out 2> tmp.err
# if "BASENAME.in" exists, or
#   BASENAME.test 1> tmp.out 2> tmp.err
# if it does not.
# 
# If "BASENAME.out" exists, it is compared with tmp.out; if they
# differ, their differences are printed (I used "diff -c" since
# Solaris does not have unified diffs).  The same happens for
# "BASENAME.err".
# 
# If there are differences, the tmp are not removed so the user can do
# further investigation; otherwise the tmp files are removed and
# "BASENAME.post" is run if it exists.
# 
# We are done with that test script, so we go to the next script, or
# to the next directory....
#
# TO DO:
# * Should we have a separate BASENAME.clean in addition to
#   BASENAME.post and make BASENAME.post always be executed?
# * Would be nice if there were some way to pass environment between
#   the various scripts.
# * The testMap dictionary could be made into a class
#######################################################################

#######################################################################
#
# 1.4
# 2003/12/15 20:11:03
# thomasm
#
#######################################################################

import sys;
import os;
import string;
import tempfile;


#######################################################################
# mySpawn(command)
#
# Python 1.5 doesn't have spawnvp; so fake our own.  command is a
# tuple; command[0] is used as the name of the program.
#######################################################################
def mySpawn(command):
    pid = os.fork();
    if 0 == pid:
        # child
        os.execvp(command[0], command);
        sys.exit(1); # not reached
    (pid, status) = os.waitpid(pid, 0);
    return status;

#######################################################################
# isTest(fileName)
#
# Returns 1 if fileName is a valid testing script: has the testExt
# extension, does not begin with ".", is an executable file.
#######################################################################
def isTest(fileName):
    global testExt;
    # check for . at start
    if (string.find(fileName, '.') == 0):
        return 0;
    # check for .test extension
    if (testExt == os.path.splitext(fileName)[-1]):
        return os.access(fileName, os.F_OK|os.X_OK);
    return 0;

#######################################################################
# prepTest(testPath)
#
# Return a dictionary with informataion about testPath.  The
# dictionary contains the paths to the test's input, expected output &
# error, and files where the generated output and error should go.
# The dictionary also contains paths to the pre-processing and
# post-processing scripts.  If any of the files do not exist, or if
# any script isn't executable, its value is the empty string.  In
# addition, the pre-processing script is run.  If it exits with a
# non-zero status, an empty dictionary is returned and all further
# processing of this test should be aborted.
#######################################################################
def prepTest(testPath):
    # the resulting dictionary
    testMap = {'test' : testPath,
               'doCleanup': 1};
    print "EXAMINING %s" % testPath;
    (dir, basename) = os.path.split(testPath);
    basename = os.path.splitext(basename)[0];
    testMap['dir'] =  dir;
    testMap['basename'] = basename;
    # do we have a preprocessing script
    preProc = os.path.join(dir, "%s.pre" % basename);
    if os.access(preProc, os.F_OK | os.X_OK):
        testMap['preProc'] = preProc;
    # does the test require input?
    input = os.path.join(dir, "%s.in" % basename);
    if os.path.exists(input):
        testMap['input'] = input;
    # what's our output stream supposed to be?
    expectedOut = os.path.join(dir, "%s.out" % basename);
    if os.path.exists(expectedOut):
        testMap['expectedOut'] = expectedOut;
    # what's our error stream supposed to be?
    expectedErr = os.path.join(dir, "%s.err" % basename);
    if os.path.exists(expectedErr):
        testMap['expectedErr'] = expectedErr;
    # any postprocessing?
    postProc = os.path.join(dir, "%s.post" % basename);
    if os.access(postProc, os.F_OK | os.X_OK):
        testMap['postProc'] = postProc;
    # return it
    return testMap;

#######################################################################
# preProcess(testMap)
#
# Run the preprocessing script, if any.  Return 1 if the test passes
# (exits with status == 0); return 0 otherwise.
#######################################################################
def preProcess(testMap):
    status = 0;
    if testMap.has_key('preProc'):
        print "  PREPROCESSING";
        status = os.system(testMap['preProc']);
        if status != 0:
            print "  Preprocessing returned nonzero.  Aborting test";
    return (0 == status);

#######################################################################
# doTest(testMap)
#
# Run the testing program given in the dictionary testMap.  Redirect
# the stdout and stderr into temporary files, and compare the
# generated output and error with what was expected.
#######################################################################
def doTest(testMap):
    # place for generated output and error
    generatedOut = tempfile.mktemp('.%s-out' % testMap['basename']);
    generatedErr = tempfile.mktemp('.%s-err' % testMap['basename']);
    testMap['generatedOut'] = generatedOut;
    testMap['generatedErr'] = generatedErr;
    # do we have input
    input = "";
    if testMap.has_key('input'):
        input = '<'+testMap['input'];
    # command to run
    cmd = "%s %s 1>%s 2>%s" \
          % (testMap['test'], input, generatedOut, generatedErr);
    #print "DEBUG: %s" % cmd;
    print "  RUNNING TEST\n\tOUTPUT TO: %s\n\tERRORS TO: %s" % \
          (generatedOut, generatedErr);
    status = os.system(cmd);
    if (0 != status):
        # Check low byte to see if signal received
        if (status & 0xFF):
            print "  ERROR: command killed on signal\nABORTING ALL TESTS";
            sys.exit(1);
        print "  ERROR: command returned nonzero: %d" % (status >> 8);
        testMap['doCleanup'] = 0;
        return;
    # Check output
    if testMap.has_key('expectedOut'):
        expectedOut = testMap['expectedOut'];
    else:
        expectedOut = '';
    if 0 != compareFiles('stdout', generatedOut, expectedOut):
        testMap['doCleanup'] = 0;
    # Check error stream
    if testMap.has_key('expectedErr'):
        expectedErr = testMap['expectedErr'];
    else:
        expectedErr = '';
    if 0 != compareFiles('stderr', generatedErr, expectedErr):
        testMap['doCleanup'] = 0;
    return;

#######################################################################
# compareFiles(type, generated, expected)
#
# Compare the contents of the files 'generated' and 'expected'.
# 'type' is a string descripting the type of output (stdout or
# stderr).  Return -1 if generated does not exist.  Return 1 if
# expected does not exist or if its contents differ from generated.
# Return 0 if the files are identical.  If the files exist but differ,
# run 'diff' on their contents.
#######################################################################
def compareFiles(type, generated, expected):
    print "    %s CHECK" % string.upper(type);
    if not os.path.exists(generated):
        print "      ERROR: generated %s file '%s' missing" % (type,generated);
        return -1;
    if 0 == len(expected):
        print "      WARN: no expected %s provided" % type;
        print "            generated stream in '%s'" % generated;
        return 1;
    # following requires Python2.x
    #if os.spawnlp(os.P_WAIT, 'cmp', 'cmp', '-s', expected, generated)
    if mySpawn(('cmp', '-s', expected, generated)):
        print "      ERROR: differences found";
        cmd = ['diff', '-c', expected, generated];
        print "%s" % string.join(cmd);
        # following requires Python2.x
        #os.spawnvp(os.P_WAIT, 'diff', cmd);
        mySpawn(cmd);
        return 1;
    return 0;

#######################################################################
# cleanUp(testMap)
#
# If the 'doCleanup' flag in testMap is non-zero, delete the generated
# output and error files listed in testMap and run its post-processing
# script.
#######################################################################
def cleanUp(testMap):
    if 0 == testMap['doCleanup']:
        return;
    print "  POSTPROCESSING/CLEANUP";
    if testMap.has_key('postProc'):
        # following requires Python2
        #os.spawnvp(os.P_WAIT, testMap['postProc'], testMap['postProc']);
        mySpawn((testMap['postProc']));
    #print "DEBUG: out: '%s' err: '%s'" \
    #      % (testMap['generatedOut'], testMap['generatedErr']);
    os.unlink(testMap['generatedOut']);
    os.unlink(testMap['generatedErr']);
    return;

#######################################################################
# runTestsInDir(dirname)
#
# Find all executable *.test files in 'dirname' and run them,
# capturing the stdout and stderr generated by the script and
# comparing these streams with files containing expected output and
# error.
#######################################################################
def runTestsInDir(dirname):
    olddir = os.getcwd();
    try:
        os.chdir(dirname);
        testfiles = filter(isTest, os.listdir('.'));
        testfiles.sort();
        #print "DEBUG: ", string.join(testfiles);
        for tst in testfiles:
            #print "DEBUG: %s" % tst;
            testMap = prepTest(os.path.join('.', tst));
            if preProcess(testMap):
                doTest(testMap);
                cleanUp(testMap);
    finally:
        os.chdir(olddir);
    return;

#######################################################################
# main()
#
# For each dir in argv, run the tests in that directory.
#######################################################################
def main():
    global testExt;
    testExt = '.test';
    if len(sys.argv) < 2:
        print "Usage: %s <dir> [<dir> ...]\n Run tests in <dir>" % sys.argv[0];
        sys.exit(1);
    for dir in sys.argv[1:]:
        if os.path.isdir(dir):
            runTestsInDir(dir);
        else:
            print "Warning: '%s' is not a directory...skipping" % dir;
    return;

#######################################################################
if __name__ == "__main__":
    main();
