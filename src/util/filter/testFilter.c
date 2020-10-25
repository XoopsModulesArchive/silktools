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
** 1.4
** 2004/03/10 22:23:52
** thomasm
*/

#include "silk.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "utils.h"
#include "filter.h"

int testParseStartTime();
int testParseEndTime();
int testParseMarks();
int testParseTime();
int testParseRange();
int testParseDecimalRange();
int testCheckAddress();
int testParseIP();
void printErrorBegin();
void printErrorEnd();


int main(void){

  printf("entering testing routine\n");


  if(!testParseStartTime()){
    printf("test parse single time failed.\n");
  }
  if(!testParseEndTime()){
    printf("test parse end time failed.\n\n");
  }
  if(!testParseMarks()){
    printf("test parse marks failed.\n");
  }
  if(!testParseTime()){
    printf("test parse time failed.\n");
  }  
  if(!testParseRange()){
    printf("test parse range failed.\n");
  }
  if(!testParseDecimalRange()){
    printf("test parse decimal rangefailed.\n");
  }
  if(!testCheckAddress()){
    printf("test check address failed.\n");
  }
  if(!testParseIP()){
    printf("test parse ip failed.\n");
  }

  return 0;
}

/*
 * checks to see if the parseSingleTime function functions correctly
 * NEGATIVE CASES:
 * CASE 1: invalid year NOT IMPLEMENTED
 * CASE 2: invalid month
 * CASE 3: invalid day
 * CASE 4: invalid hour
 * CASE 5: invalid minute
 * CASE 6: invalid second
 * CASE 7: invalid character in time string
*/
int testParseStartTime(){  
  char singleTime[19] = "";
  uint32_t resultTimeInt;
  uint32_t functionResult;
  uint32_t baseTimeInt;
  int overallResult;
  char *cYear;
  char *cMonth;
  char *cDay;
  char *cHour;
  char *cMinute;
  char *cSecond; 
  struct tm baseTime;

  overallResult = 1;

  /* initialize time vars */
  cYear = (char *)malloc(sizeof(char) * 4);
  strcpy(cYear, "2001");
  cMonth = (char *)malloc(sizeof(char) * 2);
  strcpy(cMonth,"10");
  cDay = (char *)malloc(sizeof(char) * 2);
  strcpy(cDay,"02");
  cHour = (char *)malloc(sizeof(char) * 2); 
  strcpy(cHour,"09");
  cMinute = (char *)malloc(sizeof(char) * 2);
  strcpy(cMinute,"34");
  cSecond = (char *)malloc(sizeof(char) * 2);
  strcpy(cSecond,"45");

  /*create time struct */

  baseTime.tm_year  = atoi(cYear)-1900;
  baseTime.tm_mon   = atoi(cMonth)-1;
  baseTime.tm_mday  = atoi(cDay);
  baseTime.tm_hour  = atoi(cHour);
  baseTime.tm_min   = atoi(cMinute);
  baseTime.tm_sec   = atoi(cSecond);
  baseTime.tm_isdst = 0;

  /*set base time (the time to test against) */
  baseTimeInt = SK_TIME_GM(&baseTime);

  /*load up singleTime array to reflect components above*/
  strcat(singleTime, cYear);
  strcat(singleTime, "/");
  strcat(singleTime, cMonth);
  strcat(singleTime, "/");
  strcat(singleTime, cDay);
  strcat(singleTime, ":");
  strcat(singleTime, cHour);
  strcat(singleTime, ":");
  strcat(singleTime, cMinute);
  strcat(singleTime, ":");
  strcat(singleTime, cSecond);

  functionResult = 1;
  /* positive: these tests should work */
  functionResult = parseStartTime(singleTime, &resultTimeInt);

  /*
  printf("time to test is: %s\n", singleTime);       
  printf("function result is %d and single time result is: %d\n", functionResult, resultTimeInt);
  printf("and original array is: %s\n", singleTime);
  printf("result time is: %d and base time is: %d.\n", resultTimeInt, baseTimeInt);
  */

  if(functionResult != 0){
    overallResult = 0;
    printf("parseStartTime reports a failed call.\n");
  }
  /*check for correctness */
  if(resultTimeInt != baseTimeInt){
    overallResult = 0;
    printf("parseStartTime generated incorrect output.\n");
  }

  /* negative: these tests should fail */
  
  printErrorBegin();

  /* CASE 1: invalid year NOT IMPLEMENTED
  strcpy(singleTime, "2002/12/10:10:14:15");
  functionResult = parseStartTime(singleTime, &resultTimeInt);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseStartTime incorrectly accepted an invalid year.\n");
  }
  */

  /* CASE 2: invalid month */
  strcpy(singleTime, "2001/13/10:10:14:15");
  functionResult = parseStartTime(singleTime, &resultTimeInt);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseStartTime incorrectly accepted an invalid month.\n");
  }

  /* CASE 3: invalid day */
  strcpy(singleTime, "2001/11/34:10:14:15");
  functionResult = parseStartTime(singleTime, &resultTimeInt);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseStartTime incorrectly accepted an invalid day.\n");
  }

  /* CASE 4: invalid hour */
  strcpy(singleTime, "2001/12/10:24:14:15");
  functionResult = parseStartTime(singleTime, &resultTimeInt);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseStartTime incorrectly accepted an invalid hour.\n");
  }

  /* CASE 5: invalid minute */
  strcpy(singleTime, "2001/12/10:10:61:15");
  functionResult = parseStartTime(singleTime, &resultTimeInt);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseStartTime incorrectly accepted an invalid minute.\n");
  }

  /* CASE 6: invalid second */
  strcpy(singleTime, "2001/12/10:10:14:61");
  functionResult = parseStartTime(singleTime, &resultTimeInt);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseStartTime incorrectly accepted an invalid second.\n");
  }
  /* CASE 7: invalid char in time string */
  strcpy(singleTime, "2001a/1/10:10:14:34");
  functionResult = parseStartTime(singleTime, &resultTimeInt);
  if(functionResult == 0){
    overallResult = 0;
    printf("\nparseStartTime incorrectly accepted an invalid character in the time string: %s.\n", singleTime);
  }

  printErrorEnd();

  free(cYear);
  free(cMonth);
  free(cDay);
  free(cHour);
  free(cMinute);
  free(cSecond);

  return overallResult;
}




/*
 * checks to see if the parseEndTime function functions correctly
 * NEGATIVE CASES:
 * CASE 1: invalid year - not implemented, users can search for > present time
 * CASE 2: invalid month
 * CASE 3: invalid day
 * CASE 4: invalid hour
 * CASE 5: invalid minute
 * CASE 6: invalid second
*/
int testParseEndTime(){  
  int result = 1; 
  char singleTime[19] = "";
  uint32_t resultTimeInt;
  uint32_t functionResult;
  uint32_t baseTimeInt;
  int overallResult;
  char *cYear;
  char *cMonth;
  char *cDay;
  char *cHour;
  char *cMinute;
  char *cSecond; 
  struct tm baseTime;

  overallResult = 1;

  /* initialize time vars */
  cYear = (char *)malloc(sizeof(char) * 4);
  strcpy(cYear, "2001");
  cMonth = (char *)malloc(sizeof(char) * 2);
  strcpy(cMonth,"10");
  cDay = (char *)malloc(sizeof(char) * 2);
  strcpy(cDay,"02");
  cHour = (char *)malloc(sizeof(char) * 2); 
  strcpy(cHour,"09");
  cMinute = (char *)malloc(sizeof(char) * 2);
  strcpy(cMinute,"34");
  cSecond = (char *)malloc(sizeof(char) * 2);
  strcpy(cSecond,"45");

  /*create time struct */

  baseTime.tm_year  = atoi(cYear)-1900;
  baseTime.tm_mon   = atoi(cMonth)-1;
  baseTime.tm_mday  = atoi(cDay);
  baseTime.tm_hour  = atoi(cHour);
  baseTime.tm_min   = atoi(cMinute);
  baseTime.tm_sec   = atoi(cSecond);
  baseTime.tm_isdst = 0;

  /*set base time (the time to test against) */
  baseTimeInt = SK_TIME_GM(&baseTime);


  functionResult = 1;
  /* positive: these tests should work */
  functionResult = parseEndTime(singleTime, &resultTimeInt, FILTER_USE_MAX);
  /*  printf("time to test is: %s\n", singleTime);       
  printf("function result is %d and single time result is: %d\n", functionResult, resultTimeInt);
  printf("and original array is: %s\n", singleTime);
  printf("result time is: %d and base time is: %d\n", resultTimeInt, baseTimeInt);*/
  if(functionResult != 0){
    overallResult = 0;
    printf("parseEndTime reports a failed call.\n");
  }
  /* negative: these tests should fail */

  printErrorBegin();
  
  /* CASE 1: year is incorrect NOT IMPLEMENTED
  strcpy(singleTime, "2002/12/10:10:14:15");
  functionResult = parseEndTime(singleTime, &resultTimeInt, FILTER_USE_MAX);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseEndTime incorrectly accepted an incorrect month.\n");
  }
  */
  /* CASE 2: month is incorrect */
  strcpy(singleTime, "2001/13/10:10:14:15");
  functionResult = parseEndTime(singleTime, &resultTimeInt, FILTER_USE_MAX);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseEndTime incorrectly accepted an incorrect month.\n");
  }

  /* CASE 3: day is incorrect */
  strcpy(singleTime, "2001/11/34:10:14:15");
  functionResult = parseEndTime(singleTime, &resultTimeInt, FILTER_USE_MAX);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseEndTime incorrectly accepted an incorrect day.\n");
  }

  /* CASE 4: hour is incorrect */
  strcpy(singleTime, "2001/13/10:24:14:15");
  functionResult = parseEndTime(singleTime, &resultTimeInt, FILTER_USE_MAX);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseEndTime incorrectly accepted an incorrect hour.\n");
  }

  /* CASE 5: minute is incorrect */
  strcpy(singleTime, "2001/13/10:10:61:15");
  functionResult = parseEndTime(singleTime, &resultTimeInt, FILTER_USE_MAX);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseEndTime incorrectly accepted an incorrect month.\n");
  }

  /* CASE 6: second is incorrect */
  strcpy(singleTime, "2001/13/10:10:14:61");
  functionResult = parseEndTime(singleTime, &resultTimeInt, FILTER_USE_MAX);
  if(functionResult == 0){
    overallResult = 0;
    printf("parseEndTime incorrectly accepted an incorrect second.\n");
  }

  printErrorEnd();

  free(cYear);
  free(cMonth);
  free(cDay);
  free(cHour);
  free(cMinute);
  free(cSecond);

  return result;
}



/* tests against the parseMarks() function 
 * failure cases:
 * 1)Values are not in sequence
 * 2)Value included is not allowable (out of range)
 * 3)Improper character included in mark list
 * 4)Values equal - this is currently not a failure case and is permitted by parseMarks
*/
int testParseMarks(){
  int result;
  uint32_t testBuffer[MAX_ADDRESSES];
 
  char testString[10] = {"3,4-9"};  
  result = 1;
  
  /* POSITIVE CASES */
  
  /* TEST 1: 250 should be testBuffer[7]=67108864  mask: 1 shifted 26 times: 0000 0100  0000 0000  0000 0000  0000 0000 */
  testBuffer[7]=0;
  strcpy(testString, "250");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result != 0){
    printf("parseMarks() choked on parsing the string 250\n");
    return 0;
  }
  if(testBuffer[7] != 67108864){
    printf("parseMarks() incorrectly parsed the string 250\n");
    return 0;
  }

  /* TEST 2: 43  should be testBuffer[1]=2048  mask: 1 shifted 11 times: 0000 0000  0000 0000  0000 1000  0000 0000 */
  testBuffer[1]=0;
  strcpy(testString, "43");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result != 0){
    printf("parseMarks() choked on parsing the string 250\n");
    return 0;
  }
  if(testBuffer[1] != 2048){
    printf("parseMarks() incorrectly parsed the string 43\n");
    return 0;
  }

  /* TEST 3: 43,250  should be testBuffer[1]=2048,testBuffer[7]=67108864 */
  testBuffer[1]=0;
  testBuffer[7]=0;
  strcpy(testString, "43,250");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result != 0){
    printf("parseMarks() choked on parsing the string 250\n");
    return 0;
  }
  if(testBuffer[1] != 2048 || testBuffer[7] != 67108864){
    printf("parseMarks() incorrectly parsed the string 43,250\n");
    return 0;
  }

  /* TEST 4: 42,43  should be testBuffer[1]=2048+1024=3072  
   * mask: 1 shifted 11 times:      0000 0000  0000 0000  0000 1000  0000 0000 
   * and 1 shifted  10 times gives: 0000 0000  0000 0000  0000 1100  0000 0000
   */
  testBuffer[1]=0;
  strcpy(testString, "42,43");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result != 0){
    printf("parseMarks() choked on parsing the string 42,43\n");
    return 0;
  }
  if(testBuffer[1] != 3072){
    printf("parseMarks() incorrectly parsed the string 42,43\n");
    return 0;
  }

  /* TEST 5: 40-43  should be testBuffer[1]=2048+1024+512+256=3840
   * mask: 1 shifted 11 times: (43)      0000 0000  0000 0000  0000 1000  0000 0000 
   * and 1 shifted  10 times:  (42)      0000 0000  0000 0000  0000 1100  0000 0000
   * and 1 shifted  9 times:   (41)      0000 0000  0000 0000  0000 1110  0000 0000
   * and 1 shifted  8 times:   (40)      0000 0000  0000 0000  0000 1111  0000 0000
   */
  testBuffer[1]=0;
  strcpy(testString, "40-43");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result != 0){
    printf("parseMarks() choked on parsing the string 40-43\n");
    return 0;
  }
  if(testBuffer[1] != 3840){
    printf("parseMarks() incorrectly parsed the string 40-43\n");
    return 0;
  }

  /* TEST 6: 31-32  should be testBuffer[0]=2147483648, testBuffer[1]=1
   * mask: testBuffer[0] 1 shifted 31 times: (31)      1000 0000  0000 0000  0000 0000  0000 0000    
   * mask: testBuffer[1] 1 shifted 0 times:  (32)      0000 0000  0000 0000  0000 0000  0000 0001 
   */
  testBuffer[1]=0;
  testBuffer[0]=0;
  strcpy(testString, "31-32");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result != 0){
    printf("parseMarks() choked on parsing the string 31-32\n");
    return 0;
  }

  if(testBuffer[1] != 1 || testBuffer[0] != 2147483648u){
    printf("parseMarks() incorrectly parsed the string 31-32\n");
    return 0;
  }

  /* TEST 7: 31-32,34  should be testBuffer[0]=2147483648, testBuffer[1]=5
   * mask: testBuffer[0] 1 shifted 31 times: (31)      1000 0000  0000 0000  0000 0000  0000 0000    
   * mask: testBuffer[1] 1 shifted 0 times:  (32)      0000 0000  0000 0000  0000 0000  0000 0001    
   * mask: testBuffer[1] 1 shifted 2 times:  (34)      0000 0000  0000 0000  0000 0000  0000 0101 
   */
  testBuffer[1]=0;
  testBuffer[0]=0;
  strcpy(testString, "31-32,34");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result != 0){
    printf("parseMarks() choked on parsing the string 31-32,34\n");
    return 0;
  }
  if(testBuffer[1] != 5 || testBuffer[0] != 2147483648u){
    printf("parseMarks() incorrectly parsed the string 31-32,34\n");
    return 0;
  }



  /* Negative cases */

  printErrorBegin();

  /* Case 1: should this fail?  */
  strcpy(testString, "134-123");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  /* AJY 12.5.01 COMMENTED OUT, WAITING FOR CODE MOD. */
  if(result == 0){
    printf("Case 1 in parseMarks fails: values are not in sequence but are accepted\n");
    return 0;
  }
  

  /* Case 2: value out of range */
  strcpy(testString, "1-256");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result == 0){
    printf("Case 2 in parseMarks fails: values are out of range but are accepted\n");
    return 0;
  }

  /* Case 3: unacceptable value */
  strcpy(testString, "3,a");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result == 0){
    printf("Case 3 in parseMarks fails: values are not allowable but are accepted\n");
    return 0;
  }

  /* Case 4a: comma-seperated values equal */
  /* AJY: this is acceptable.  test left here, commented, for extensibility
  strcpy(testString, "3,3");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result == 0){
    printf("Case 4a in parseMarks fails: comma-seperated values are equal e.g. 3,3 but are accepted\n");
    return 0;
  }
  */

  /* Case 4: hyphen-seperated values equal */
  /* AJY: this is acceptable.  test left here, commented, for extensibility
  strcpy(testString, "3-3");
  result = parseMarks(testString, testBuffer, (MAX_ADDRESSES * 32 - 1));
  if(result == 0){
    printf("Case 4b in parseMarks fails: values are equal e.g. 3-3 but are accepted\n");
    return 0;
  }
  */

  printErrorEnd();

  return 1;
}

/* 
 * checks the parseTime() method 
 * NEGATIVE CASES:
 * CASE 1: Start time > End Time
 * CASE 2: Start time incorrectly formatted (we know that specific format errors will be caught in testParseStartTime() and do not have to check this.
 * CASE 3: Invalid characters in input
*/
int testParseTime(){
  int result;
  char testString[42];
  valueRange tgtTime;
  uint32_t uiResult;

  uiResult = 1;
  result = 1;
  tgtTime.max = 0;
  tgtTime.min = 0;  

  strcpy(testString, "2001/11/23:17:44:56-2001/11/24:17:44:57");
  /*  printf("about to test on string %s\n", testString); */
  uiResult=parseTime(testString, &tgtTime);
  if(uiResult != 0){
    printf("parseTime failed and should have passed.\n");
    return 0;
  }



  /* NEGATIVE CASES */

  printErrorBegin();

  /* CASE 1: Start Time > End Time */
  strcpy(testString, "2001/11/23:17:44:56-2001/11/22:17:44:57");
  uiResult=parseTime(testString, &tgtTime);
  if(uiResult == 0){
    printf("parseTime incorrectly accepted a range where the starting time is greater than the ending time.\n");
    return 0;
  }

  /* CASE 3: Invalid characters in the input: something other than {0-9,:,/} */
  strcpy(testString, "2001/11/23:17:44:56-200a/11/22:17:44:57");
  uiResult=parseTime(testString, &tgtTime);
  if(uiResult == 0){
    printf("parse time incorrectly accepted a range containing an invalid character.\n");
    return 0;
  }
  
  /* END NEGATIVE CASES */
  printErrorEnd();

  return 1;
}

/*
 * checks the parseRange() method. 
 * NEGATIVE (failure) CASES:
 * CASE 1: Range string is improperly formatted. Proper format: {x-y | x,y e Nn} 
 * CASE 1a: Range string is missing a dash
 * CASE 1b: Range string is missing min. range e.g. -y
 * CASE 1c: Range string is missing max. range e.g. x-
 * CASE 1d: Range string contains improper chars e.g. x or y ne Nn
 * CASE 2: Range string is semantically incorrect: x>y
 * 
*/
int testParseRange(){
  char rangeString[5] = {"1-2"};
  valueRange tgtRange;
  uint32_t uiResult;
  
  uiResult = 1;
 
  /*positive tests: tests should pass */

  printErrorBegin();
  /*NEGATIVE TESTS BEGIN */
  /* CASE 1a: Range string is missing a dash */
  strcpy(rangeString, "34");
  uiResult = parseRange(rangeString, &tgtRange);
  if(uiResult == 0){
    printf("error: parseRange accepted a single value with no hyphen when it should have failed.\n");
    return 0;
  }
  /* CASE 1b: Range string is missing min. range e.g. -y */
  strcpy(rangeString, "-7");
  uiResult = parseRange(rangeString, &tgtRange);
  if(uiResult == 0){
    printf("error: parseRange accepted a malformed string (-y) when it should have failed.\n");
   return 0;
  } 
  /* CASE 1c: Range string is missing max. range e.g. x- */
  strcpy(rangeString, "4-");
  uiResult = parseRange(rangeString, &tgtRange);
  if(uiResult == 0){
    printf("error: parseRange accepted a malformed string (x-) when it should have failed.\n");
    return 0;
  } 
  /* CASE 1d: Range string x-y contains improper chars e.g. y ne Nn */
  strcpy(rangeString, "4-r");
  uiResult = parseRange(rangeString, &tgtRange);  
  if(uiResult == 0){
    printf("error: parseRange accepted a malformed string (-y) when it should have failed.\n");
    return 0;
  } 
  /* CASE 1d1: Range string x-y contains improper chars e.g. x ne Nn */
  strcpy(rangeString, "r-7");
  uiResult = parseRange(rangeString, &tgtRange);  
  /* AJY COMMENTED OUT 12.6.01 WAITING FOR CLARIFICATION 
  if(uiResult == 0){
    printf("error: parseRange accepted a malformed string (x-) when it should have failed.\n");
    return 0;
  } 
  */
  /* CASE 2: Range string is semantically incorrect: x>y */
  strcpy(rangeString, "65-12");
  uiResult = parseRange(rangeString, &tgtRange);  
  if(uiResult == 0){
    printf("error: parseRange accepted an incorrect value (x-y s.t. x>y) when it should have failed.\n");
    return 0;
  }   

  printErrorEnd();

  return 1;
}


/* checks the parseDecimalRange() method in filteropts.c
 *
*/
int testParseDecimalRange(){
  char rangeString[9] = {".11-.12"};
  valueRange tgtRange;
  uint32_t uiResult;
  
  uiResult = 1;
 
  /*positive tests: tests should pass */

  printErrorBegin();
  /*NEGATIVE TESTS BEGIN */
  /* CASE 1a: Range string is missing a dash */
  strcpy(rangeString, ".34");
  uiResult = parseDecimalRange(rangeString, &tgtRange);

  if(uiResult == 0){
    printf("error: parseDecimalRange accepted a single value with no hyphen when it should have failed.\n");
    return 0;
  }
  /* CASE 1b: Range string is missing min. range e.g. -y */
  strcpy(rangeString, "-.07");
  uiResult = parseDecimalRange(rangeString, &tgtRange);

  if(uiResult == 0){
    printf("error: parseDecimalRange accepted a malformed string (-y) when it should have failed.\n");
   return 0;
  } 
  strcpy(rangeString, ".4-");
  uiResult = parseDecimalRange(rangeString, &tgtRange);
  if(uiResult == 0){
    printf("error: parseDecimalRange accepted a malformed string (x-) when it should have failed.\n");
    return 0;
  } 
  
  /* CASE 1d: Range string x-y contains improper chars e.g. y ne Nn */
  strcpy(rangeString, ".4-r");
  uiResult = parseDecimalRange(rangeString, &tgtRange);  
  if(uiResult == 0){
    printf("error: parseDecimalRange accepted a malformed string (-y) when it should have failed.\n");
    return 0;
  } 
  /* CASE 1d1: Range string x-y contains improper chars e.g. x ne Nn */
  strcpy(rangeString, "r-.7");
  uiResult = parseDecimalRange(rangeString, &tgtRange);  
  /* AJY COMMENTED OUT 12.6.01 WAITING FOR CLARIFICATION 
  if(uiResult == 0){
    printf("error: parseDecimalRange accepted a malformed string (x-) when it should have failed.\n");
    return 0;
  } 
  */
  /* CASE 2: Range string is semantically incorrect: x>y */
  strcpy(rangeString, ".65-.12");
  uiResult = parseDecimalRange(rangeString, &tgtRange);  
  if(uiResult == 0){
    printf("error: parseDecimalRange accepted an incorrect value (x-y s.t. x>y) when it should have failed.\n");
    return 0;
  }   

  printErrorEnd();

  return 1;
}


/* tests the checkAddress() method in filtercheck.c
 * 
 *
 * typedef struct {
 *  uint32_t octets[4][MAX_ADDRESSES];
 * } addressRange;
 *
 * NEGATIVE CASES
 * open q - do negative cases exist for this?
*/
int testCheckAddress(){
  addressRange arRange;
  uint32_t targetAddress;
  int result;
  int swapFlag;

  result=1;
  swapFlag=0;

  /* POSITIVE CASE: check address for correctness - say address is: 0.0.0.1, which translates to the assignment below */
  
  arRange.octets[0][0]=2; /* mask: 1 shifted 1 time */
  arRange.octets[1][0]=1; /* mask: 1 shifted 0 times */
  arRange.octets[2][0]=1;
  arRange.octets[3][0]=1;  

  /* target address is: 0000 0000   0000 0000    0000 0000   0000 0001.  obviously, 1 in decimal */

  targetAddress=1;

  result = checkAddress(targetAddress, &arRange);
  if(! result){
    printf("checkAddress failed.  result is: %d\n", result);
  }

  /* assume a target address of 0.0.0.8 */

  arRange.octets[0][0]=256; /* mask: 1 shifted 8 times: 23 0's1 0000 0000 */
  arRange.octets[1][0]=1; /* mask: 1 shifted 0 times */
  arRange.octets[2][0]=1;
  arRange.octets[3][0]=1;  

  /* target address is: 0000 0000   0000 0000    0000 0000   0000 1000.  obviously, 8 in decimal */

  targetAddress=8;

  result = checkAddress(targetAddress, &arRange);
  if(! result){
    printf("checkAddress failed.  result is: %d\n", result);
  }

  /* assume a target address of 250.43.96.14 */

  arRange.octets[3][7]=67108864; /* mask: 1 shifted 26 times: 0000 0100  0000 0000  0000 0000  0000 0000 */
  arRange.octets[2][1]=2048;     /* mask: 1 shifted 11 times: 0000 0000  0000 0000  0000 1000  0000 0000 */
  arRange.octets[1][3]=1;        /* mask: 1 shifted 0 times:  0000 0000  0000 0000  0000 0000  0000 0000 */
  arRange.octets[0][0]=16384;    /* mask: 1 shifted 14 times: 0000 0000  0000 0000  0100 0000  0000 0000 */  

  /* target address is: 1111 1010   0010 1011    0110 0000   0000 1110.  =4197146638 in decimal */

  targetAddress=4197146638u;

  result = checkAddress(targetAddress, &arRange);
  if(! result){
    printf("checkAddress failed.  result is: %d\n", result);
  }

  /* NEGATIVE CASES - what does a negative case for checkAddress() consist of?  */


  return 1;
}


/* tests the parseIP() method
 * CASE 1: First octect is incorrect
 * CASE 2: Second octect is incorrect
 * CASE 3: Third octect is incorrect
 * CASE 4: Fourth octect is incorrect
 * CASE a: octect below 0
 * CASE b: octect above 255
 * CASE c: octect contains improper character
 * For other cases, see testParseMarks()
 * CASE 5: missing octect 
 *
 * filter rules def'n:
 * uint8_t totalAddresses[PAIRS];
 * uint8_t negateIP[PAIRS];
 * addressRange IP[PAIRS][MAX_IP_RULES];
 * uint32_t srcPorts[MAX_PORTS];
 * uint32_t dstPorts[MAX_PORTS];
 * uint32_t protocols[MAX_PROTOCOLS];
 * valueRange sTime, eTime, bytes, packets, flows, duration;
 * valueRange packetsPerFlow, bytesPerPacket, bytesPerFlow, bytesPerSecond;
 *
 * uint8_t flags[NUM_RULE_CLASSES];
 *
 * typedef struct {
 *  uint32_t octets[4][MAX_ADDRESSES];
 * } addressRange;
 * 
*/
int testParseIP(){
  int result, place;
  filterRules frRules;
  int currentIP;
  char vanillaIPString[16] = {"100.100.100.100"};
  char ipString[30] = {"100.100.100.100"};
  result = 1;
  currentIP = 0;
  place = 0;
  /* Positive Tests.  These should pass.  */

  /* Production test - checks for correctness.  Below this, uncommenting display code follows, which will result in visually displaying the ip addr's that are stored. */
  
 /* PRODUCTION TEST BEGIN */

  /* assume a target address of 250.43.96.14 */
  strcpy(vanillaIPString, "250.43.96.14");
  result = parseIP(vanillaIPString, place, &frRules);

  if(result != 0){
    printf("parseIP failed on positive testing.");
    return 0;
  }  

  currentIP = frRules.totalAddresses[place];

  /*
  display of the proper state of the addressRange (arRange below) member of the filterRules (frRules) struct
  arRange.octets[0][7]=67108864;  mask: 1 shifted 26 times: 0000 0100  0000 0000  0000 0000  0000 0000
  arRange.octets[1][1]=2048;      mask: 1 shifted 11 times: 0000 0000  0000 0000  0000 1000  0000 0000 
  arRange.octets[2][3]=1;         mask: 1 shifted 0 times:  0000 0000  0000 0000  0000 0000  0000 0000 
  arRange.octets[3][0]=16384;     mask: 1 shifted 14 times: 0000 0000  0000 0000  0100 0000  0000 0000   

  target address is: 1111 1010   0010 1011    0110 0000   0000 1110.  =4197146638 in decimal 

  targetAddress=4197146638;
  */

  currentIP--; //bring it to the recently added ip

  if(frRules.IP[place][currentIP].octets[0][7] != 67108864 || frRules.IP[place][currentIP].octets[1][1] != 2048 || frRules.IP[place][currentIP].octets[2][3] != 1 || frRules.IP[place][currentIP].octets[3][0] != 16384){
    printf("the parseIP() method failed to parse the ip correctly.\n");
    return 0;
  }



  /* more complex case: comma-seperated list
   * assume a target address of 43,250.43.96.14 */
  place++;
  strcpy(vanillaIPString, "43,250.43.96.14");
  result = parseIP(vanillaIPString, place, &frRules);
  if(result != 0){
    printf("parseIP failed on positive testing of string 43,250.43.96.14.");
    return 0;
  }  

  currentIP = frRules.totalAddresses[place];
  currentIP--;

  if(frRules.IP[place][currentIP].octets[0][7] != 67108864 || frRules.IP[place][currentIP].octets[0][1] != 2048 || frRules.IP[place][currentIP].octets[1][1] != 2048 || frRules.IP[place][currentIP].octets[2][3] != 1 || frRules.IP[place][currentIP].octets[3][0] != 16384){
    printf("the parseIP() method failed to parse the ip 43,250.43.96.14 correctly.\n");
    return 0;
  }

  /* PRODUCTION TEST END */  
  /* BEGIN DISPLAY CODE */
  /*
  currentIP = frRules.totalAddresses[place]; 
  printf("currentip is: %d\n", currentIP);
  
  for(i = 0;i<4;i++){
    for(j = 0;j<8;j++){
      printf("fr rules i: %d j: %d, value: %d\n", i, j, frRules.IP[place][currentIP].octets[i][j]);
    }
  }
  result = parseIP(vanillaIPString, place, &frRules);
  if(result!=0){
    return 0;
  }
  currentIP = frRules.totalAddresses[place];
  currentIP--; 
  printf("currentip is: %d place is: %d\n", currentIP, place);
  for(i = 0;i<4;i++){
    for(j = 0;j<8;j++){
      printf("fr rules i: %d j: %d, value: %d\n", i, j, frRules.IP[place][currentIP].octets[i][j]);
    }
  }
  printf("run 1, ip string is: %s\n", ipString); 
  place++;
  result = parseIP(ipString, place, &frRules);
  printf("result is: %d\n", result);
  if(result != 0){
    return 0;
  }

  currentIP = frRules.totalAddresses[place];
  currentIP--; 
  printf("currentip is: %d place is: %d\n", currentIP, place);
  for(i = 0;i<4;i++){
    for(j = 0;j<8;j++){
      printf("fr rules i: %d j: %d, value: %d\n", i, j, frRules.IP[place][currentIP].octets[i][j]);
    }
  }
  
  strcpy(ipString, ipTest);
  printf("run 2, ip string is: %s\n", ipString); 
  place++;
  result = parseIP(ipString, place, &frRules);
  
  printf("result is: %d\n", result);

  if(result != 0){
    return 0;
  }

  currentIP = frRules.totalAddresses[place];
  currentIP--;
 
  printf("currentip is: %d place is: %d\n", currentIP, place);
 
  for(i = 0;i<4;i++){
    for(j = 0;j<8;j++){
      printf("fr rules i: %d j: %d, value: %d\n", i, j, frRules.IP[place][currentIP].octets[i][j]);
    }
  }
  */
  /* END DISPLAY CODE.  LEAVE COMMENTED OUT FOR PROPER OUTPUT */ 
  

  /* Negative Tests.  These should fail. */
  place++;
  strcpy(ipString, "256.100.100.100");
  printErrorBegin();
  result = parseIP(ipString, place, &frRules);
  if(result == 0){
    return 0;
  }
  
  /* CASE 1a: First octect incorrect - below 0 */
  strcpy(ipString, "-5.100.100.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the first octect\n");
    return 0;
  }
  /* CASE 1b: First octect incorrect - value above 255 */
  strcpy(ipString, "256.100.100.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the first octect\n");
    return 0;
  }
  /* CASE 1c: First octect incorrect - invalid character */
  strcpy(ipString, "25z.100.100.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting an improper character in the first octect\n");
    return 0;
  }
  /* CASE 2a: Second octect incorrect - below 0 */
  strcpy(ipString, "100.-5.100.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the second octect\n");
    return 0;
  }
  /* CASE 2b: Second octect incorrect - value above 255 */
  strcpy(ipString, "100.256.100.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the second octect\n");
    return 0;
  }
  /* CASE 2c: Second octect incorrect - invalid character */
  strcpy(ipString, "25.x0.100.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting an improper character in the second octect\n");
    return 0;
  }
  /* CASE 3a: Third octect incorrect - below 0 */
  strcpy(ipString, "100.100.-134.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the third octect\n");
    return 0;
  }
  /* CASE 3b: Third octect incorrect - value above 255 */
  strcpy(ipString, "250.100.257.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the third octect\n");
    return 0;
  }
  /* CASE 3c: Third octect incorrect - invalid character */
  strcpy(ipString, "25.100.t00.100");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting an improper character in the third octect\n");
    return 0;
  }
  /* CASE 4a: Fourth octect incorrect - below 0 */
  strcpy(ipString, "1.100.100.-1");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the fourth octect\n");
    return 0;
  }
  /* CASE 4b: Fourth octect incorrect - value above 255 */
  strcpy(ipString, "200.100.100.300");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting a negative value in the fourth octect\n");
    return 0;
  }
  /* CASE 4c: Fourth octect incorrect - invalid character */
  strcpy(ipString, "25.100.100.10b");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    printf("parseIP failed by accepting an improper character in the fourth octect\n");
    return 0;
  }

  /* QUESTION: x.x.x-.x or x.x.x,.x is allowed, irregardless of the position (y.x.x.x or x.y.x.x etc) of the "dangling" dash or comma 
  printf("\ntesting hanging dash\n");
  strcpy(ipString, "100.100.100.250");
  result=parseIP(ipString, place, &frRules);
  if(result == 0){
    return 0;
  }
  */
  printErrorEnd();


  return 1;
}



void printErrorBegin(){
  printf("\n-------------------------begin error messages-------------------------------------\n");
}

void printErrorEnd(){
  printf("\n-------------------------end error messages---------------------------------------\n");
}











