/*
** Copyright (C) 2003-2004 by Carnegie Mellon University.
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
** 1.13
** 2004/03/19 15:00:03
** thomasm
*/

#ifndef _SILK_SITE_H
#error "Never include silk_site_generic.h directly,include silk_site.h instead"
#endif

/*@unused@*/ static char rcsID_SILK_SITE_GENERIC_H[]
#ifdef __GNUC__
  __attribute__((__unused__))
#endif
  = "silk_site_generic.h,v 1.13 2004/03/19 15:00:03 thomasm Exp";


/* Indexes into the outFInfo array */
typedef enum {
  RW_IN,
  RW_OUT,
  RW_IN_WEB,
  RW_OUT_WEB
} flowtype_t;
#define NUM_FLOW_TYPES 4

#ifdef DECLARE_SITE_VARIABLES

#define OUT_F_INFO_ENTRY(dirPrefix,filePrefix,packType) \
  {"all", (dirPrefix), (filePrefix), (dirPrefix "/"), 1, (packType), \
   {{NULL,NULL,0,0},{NULL,NULL,0,0},{NULL,NULL,0,0},{NULL,NULL,0,0}}, 0}

fileTypeInfo outFInfo[NUM_FLOW_TYPES] = {
  /*
  **  className, dirPrefix, filePrefix, pathPrefix, filesPerDay,
  **  packType
  **  array[4] of streamInfo each consisting of:
  **      char *fName, FILE *outf, uint32_t count, uint32_t sTime;
  **  maxETime: end of NEXT hour
  */
  OUT_F_INFO_ENTRY("in", "in", FT_RWSPLIT),
  OUT_F_INFO_ENTRY("out", "out", FT_RWSPLIT),
  OUT_F_INFO_ENTRY("inweb", "iw", FT_RWWWW),
  OUT_F_INFO_ENTRY("outweb", "ow", FT_RWWWW)
};

/* compute number of file types */
uint32_t numFileTypes = sizeof(outFInfo)/sizeof(fileTypeInfo);
#endif /* DECLARE_SITE_VARIABLES */


/************** Classes */
#define NUM_CLASSES ((uint8_t)1)

#define DEFAULT_CLASS 0

#ifdef DECLARE_CLASSINFO_VARIABLES
/* Types associated with classes */
static classInfoStruct classInfo[NUM_CLASSES] = {
  {"all",  4, 2,
   {RW_IN, RW_OUT, RW_IN_WEB, RW_OUT_WEB, 0, 0, 0, 0, 0, 0, 0, 0},
   {RW_IN, RW_IN_WEB, 0, 0}}, /* 0 */
};
#endif /* DECLARE_CLASSINFO_VARIABLES */



/************ Sensors */

/* To customize for a site change SENSOR_COUNT to reflect all sensors
 * used at the site, and add the appropriate entries to sensorInfo.
 *
 * NOTE: All sensor names should begin with a capital letter, by
 *       convention an "S" is usually used.  The sensor name may be
 *       1-5 characters long and should only contain alpha-numerics;
 *       it must not contain underscore.
 */
   
/*
 * THESE ARE SAMPLE VALUES ONLY.  CHANGE THE STRINGS IN THE sensorInfo
 * STRUCT TO MATCH WHAT WAS USED IN THE "SENSOR_NAME" FIELD FOR EACH
 * "rwfpd" YOU HAVE DEPLOYED.  CHANGE THE VALUE OF "SENSOR_COUNT" AS
 * WELL.
 */
#define SENSOR_COUNT    15
#ifdef DECLARE_SENSORID_VARIABLES
uint32_t numSensors=SENSOR_COUNT;

/* sensorinfo_t is defined in silk_site.h */
sensorinfo_t sensorInfo[SENSOR_COUNT+1] = {
  /*   0 */ { "S0",   1, {DEFAULT_CLASS, 0} },
  /*   1 */ { "S1",   1, {DEFAULT_CLASS, 0} },
  /*   2 */ { "S2",   1, {DEFAULT_CLASS, 0} },
  /*   3 */ { "S3",   1, {DEFAULT_CLASS, 0} },
  /*   4 */ { "S4",   1, {DEFAULT_CLASS, 0} },
  /*   5 */ { "S5",   1, {DEFAULT_CLASS, 0} },
  /*   6 */ { "S6",   1, {DEFAULT_CLASS, 0} },
  /*   7 */ { "S7",   1, {DEFAULT_CLASS, 0} },
  /*   8 */ { "S8",   1, {DEFAULT_CLASS, 0} },
  /*   9 */ { "S9",   1, {DEFAULT_CLASS, 0} },
  /*  10 */ { "S10",  1, {DEFAULT_CLASS, 0} },
  /*  11 */ { "S11",  1, {DEFAULT_CLASS, 0} },
  /*  12 */ { "S12",  1, {DEFAULT_CLASS, 0} },
  /*  13 */ { "S13",  1, {DEFAULT_CLASS, 0} },
  /*  14 */ { "S14",  1, {DEFAULT_CLASS, 0} },

#if 0
  /*  15 */ { "S15",  1, {DEFAULT_CLASS, 0} },
  /*  16 */ { "S16",  1, {DEFAULT_CLASS, 0} },
  /*  17 */ { "S17",  1, {DEFAULT_CLASS, 0} },
  /*  18 */ { "S18",  1, {DEFAULT_CLASS, 0} },
  /*  19 */ { "S19",  1, {DEFAULT_CLASS, 0} },
  /*  20 */ { "S20",  1, {DEFAULT_CLASS, 0} },
  /*  21 */ { "S21",  1, {DEFAULT_CLASS, 0} },
  /*  22 */ { "S22",  1, {DEFAULT_CLASS, 0} },
  /*  23 */ { "S23",  1, {DEFAULT_CLASS, 0} },
  /*  24 */ { "S24",  1, {DEFAULT_CLASS, 0} },
  /*  25 */ { "S25",  1, {DEFAULT_CLASS, 0} },
  /*  26 */ { "S26",  1, {DEFAULT_CLASS, 0} },
  /*  27 */ { "S27",  1, {DEFAULT_CLASS, 0} },
  /*  28 */ { "S28",  1, {DEFAULT_CLASS, 0} },
  /*  29 */ { "S29",  1, {DEFAULT_CLASS, 0} },
  /*  30 */ { "S30",  1, {DEFAULT_CLASS, 0} },
  /*  31 */ { "S31",  1, {DEFAULT_CLASS, 0} },
  /*  32 */ { "S32",  1, {DEFAULT_CLASS, 0} },
  /*  33 */ { "S33",  1, {DEFAULT_CLASS, 0} },
  /*  34 */ { "S34",  1, {DEFAULT_CLASS, 0} },
  /*  35 */ { "S35",  1, {DEFAULT_CLASS, 0} },
  /*  36 */ { "S36",  1, {DEFAULT_CLASS, 0} },
  /*  37 */ { "S37",  1, {DEFAULT_CLASS, 0} },
  /*  38 */ { "S38",  1, {DEFAULT_CLASS, 0} },
  /*  39 */ { "S39",  1, {DEFAULT_CLASS, 0} },
  /*  40 */ { "S40",  1, {DEFAULT_CLASS, 0} },
  /*  41 */ { "S41",  1, {DEFAULT_CLASS, 0} },
  /*  42 */ { "S42",  1, {DEFAULT_CLASS, 0} },
  /*  43 */ { "S43",  1, {DEFAULT_CLASS, 0} },
  /*  44 */ { "S44",  1, {DEFAULT_CLASS, 0} },
  /*  45 */ { "S45",  1, {DEFAULT_CLASS, 0} },
  /*  46 */ { "S46",  1, {DEFAULT_CLASS, 0} },
  /*  47 */ { "S47",  1, {DEFAULT_CLASS, 0} },
  /*  48 */ { "S48",  1, {DEFAULT_CLASS, 0} },
  /*  49 */ { "S49",  1, {DEFAULT_CLASS, 0} },
  /*  50 */ { "S50",  1, {DEFAULT_CLASS, 0} },
  /*  51 */ { "S51",  1, {DEFAULT_CLASS, 0} },
  /*  52 */ { "S52",  1, {DEFAULT_CLASS, 0} },
  /*  53 */ { "S53",  1, {DEFAULT_CLASS, 0} },
  /*  54 */ { "S54",  1, {DEFAULT_CLASS, 0} },
  /*  55 */ { "S55",  1, {DEFAULT_CLASS, 0} },
  /*  56 */ { "S56",  1, {DEFAULT_CLASS, 0} },
  /*  57 */ { "S57",  1, {DEFAULT_CLASS, 0} },
  /*  58 */ { "S58",  1, {DEFAULT_CLASS, 0} },
  /*  59 */ { "S59",  1, {DEFAULT_CLASS, 0} },
  /*  60 */ { "S60",  1, {DEFAULT_CLASS, 0} },
  /*  61 */ { "S61",  1, {DEFAULT_CLASS, 0} },
  /*  62 */ { "S62",  1, {DEFAULT_CLASS, 0} },
  /*  63 */ { "S63",  1, {DEFAULT_CLASS, 0} },
  /*  64 */ { "S64",  1, {DEFAULT_CLASS, 0} },
  /*  65 */ { "S65",  1, {DEFAULT_CLASS, 0} },
  /*  66 */ { "S66",  1, {DEFAULT_CLASS, 0} },
  /*  67 */ { "S67",  1, {DEFAULT_CLASS, 0} },
  /*  68 */ { "S68",  1, {DEFAULT_CLASS, 0} },
  /*  69 */ { "S69",  1, {DEFAULT_CLASS, 0} },
  /*  70 */ { "S70",  1, {DEFAULT_CLASS, 0} },
  /*  71 */ { "S71",  1, {DEFAULT_CLASS, 0} },
  /*  72 */ { "S72",  1, {DEFAULT_CLASS, 0} },
  /*  73 */ { "S73",  1, {DEFAULT_CLASS, 0} },
  /*  74 */ { "S74",  1, {DEFAULT_CLASS, 0} },
  /*  75 */ { "S75",  1, {DEFAULT_CLASS, 0} },
  /*  76 */ { "S76",  1, {DEFAULT_CLASS, 0} },
  /*  77 */ { "S77",  1, {DEFAULT_CLASS, 0} },
  /*  78 */ { "S78",  1, {DEFAULT_CLASS, 0} },
  /*  79 */ { "S79",  1, {DEFAULT_CLASS, 0} },
  /*  80 */ { "S80",  1, {DEFAULT_CLASS, 0} },
  /*  81 */ { "S81",  1, {DEFAULT_CLASS, 0} },
  /*  82 */ { "S82",  1, {DEFAULT_CLASS, 0} },
  /*  83 */ { "S83",  1, {DEFAULT_CLASS, 0} },
  /*  84 */ { "S84",  1, {DEFAULT_CLASS, 0} },
  /*  85 */ { "S85",  1, {DEFAULT_CLASS, 0} },
  /*  86 */ { "S86",  1, {DEFAULT_CLASS, 0} },
  /*  87 */ { "S87",  1, {DEFAULT_CLASS, 0} },
  /*  88 */ { "S88",  1, {DEFAULT_CLASS, 0} },
  /*  89 */ { "S89",  1, {DEFAULT_CLASS, 0} },
  /*  90 */ { "S90",  1, {DEFAULT_CLASS, 0} },
  /*  91 */ { "S91",  1, {DEFAULT_CLASS, 0} },
  /*  92 */ { "S92",  1, {DEFAULT_CLASS, 0} },
  /*  93 */ { "S93",  1, {DEFAULT_CLASS, 0} },
  /*  94 */ { "S94",  1, {DEFAULT_CLASS, 0} },
  /*  95 */ { "S95",  1, {DEFAULT_CLASS, 0} },
  /*  96 */ { "S96",  1, {DEFAULT_CLASS, 0} },
  /*  97 */ { "S97",  1, {DEFAULT_CLASS, 0} },
  /*  98 */ { "S98",  1, {DEFAULT_CLASS, 0} },
  /*  99 */ { "S99",  1, {DEFAULT_CLASS, 0} },
  /* 100 */ { "S100", 1, {DEFAULT_CLASS, 0} },
  /* 101 */ { "S101", 1, {DEFAULT_CLASS, 0} },
  /* 102 */ { "S102", 1, {DEFAULT_CLASS, 0} },
  /* 103 */ { "S103", 1, {DEFAULT_CLASS, 0} },
  /* 104 */ { "S104", 1, {DEFAULT_CLASS, 0} },
  /* 105 */ { "S105", 1, {DEFAULT_CLASS, 0} },
  /* 106 */ { "S106", 1, {DEFAULT_CLASS, 0} },
  /* 107 */ { "S107", 1, {DEFAULT_CLASS, 0} },
  /* 108 */ { "S108", 1, {DEFAULT_CLASS, 0} },
  /* 109 */ { "S109", 1, {DEFAULT_CLASS, 0} },
  /* 110 */ { "S110", 1, {DEFAULT_CLASS, 0} },
  /* 111 */ { "S111", 1, {DEFAULT_CLASS, 0} },
  /* 112 */ { "S112", 1, {DEFAULT_CLASS, 0} },
  /* 113 */ { "S113", 1, {DEFAULT_CLASS, 0} },
  /* 114 */ { "S114", 1, {DEFAULT_CLASS, 0} },
  /* 115 */ { "S115", 1, {DEFAULT_CLASS, 0} },
  /* 116 */ { "S116", 1, {DEFAULT_CLASS, 0} },
  /* 117 */ { "S117", 1, {DEFAULT_CLASS, 0} },
  /* 118 */ { "S118", 1, {DEFAULT_CLASS, 0} },
  /* 119 */ { "S119", 1, {DEFAULT_CLASS, 0} },
  /* 120 */ { "S120", 1, {DEFAULT_CLASS, 0} },
  /* 121 */ { "S121", 1, {DEFAULT_CLASS, 0} },
  /* 122 */ { "S122", 1, {DEFAULT_CLASS, 0} },
  /* 123 */ { "S123", 1, {DEFAULT_CLASS, 0} },
  /* 124 */ { "S124", 1, {DEFAULT_CLASS, 0} },
  /* 125 */ { "S125", 1, {DEFAULT_CLASS, 0} },
  /* 126 */ { "S126", 1, {DEFAULT_CLASS, 0} },
  /* 127 */ { "S127", 1, {DEFAULT_CLASS, 0} },
  /* 128 */ { "S128", 1, {DEFAULT_CLASS, 0} },
  /* 129 */ { "S129", 1, {DEFAULT_CLASS, 0} },
  /* 130 */ { "S130", 1, {DEFAULT_CLASS, 0} },
  /* 131 */ { "S131", 1, {DEFAULT_CLASS, 0} },
  /* 132 */ { "S132", 1, {DEFAULT_CLASS, 0} },
  /* 133 */ { "S133", 1, {DEFAULT_CLASS, 0} },
  /* 134 */ { "S134", 1, {DEFAULT_CLASS, 0} },
  /* 135 */ { "S135", 1, {DEFAULT_CLASS, 0} },
  /* 136 */ { "S136", 1, {DEFAULT_CLASS, 0} },
  /* 137 */ { "S137", 1, {DEFAULT_CLASS, 0} },
  /* 138 */ { "S138", 1, {DEFAULT_CLASS, 0} },
  /* 139 */ { "S139", 1, {DEFAULT_CLASS, 0} },
  /* 140 */ { "S140", 1, {DEFAULT_CLASS, 0} },
  /* 141 */ { "S141", 1, {DEFAULT_CLASS, 0} },
  /* 142 */ { "S142", 1, {DEFAULT_CLASS, 0} },
  /* 143 */ { "S143", 1, {DEFAULT_CLASS, 0} },
  /* 144 */ { "S144", 1, {DEFAULT_CLASS, 0} },
  /* 145 */ { "S145", 1, {DEFAULT_CLASS, 0} },
  /* 146 */ { "S146", 1, {DEFAULT_CLASS, 0} },
  /* 147 */ { "S147", 1, {DEFAULT_CLASS, 0} },
  /* 148 */ { "S148", 1, {DEFAULT_CLASS, 0} },
  /* 149 */ { "S149", 1, {DEFAULT_CLASS, 0} },
  /* 150 */ { "S150", 1, {DEFAULT_CLASS, 0} },
  /* 151 */ { "S151", 1, {DEFAULT_CLASS, 0} },
  /* 152 */ { "S152", 1, {DEFAULT_CLASS, 0} },
  /* 153 */ { "S153", 1, {DEFAULT_CLASS, 0} },
  /* 154 */ { "S154", 1, {DEFAULT_CLASS, 0} },
  /* 155 */ { "S155", 1, {DEFAULT_CLASS, 0} },
  /* 156 */ { "S156", 1, {DEFAULT_CLASS, 0} },
  /* 157 */ { "S157", 1, {DEFAULT_CLASS, 0} },
  /* 158 */ { "S158", 1, {DEFAULT_CLASS, 0} },
  /* 159 */ { "S159", 1, {DEFAULT_CLASS, 0} },
  /* 160 */ { "S160", 1, {DEFAULT_CLASS, 0} },
  /* 161 */ { "S161", 1, {DEFAULT_CLASS, 0} },
  /* 162 */ { "S162", 1, {DEFAULT_CLASS, 0} },
  /* 163 */ { "S163", 1, {DEFAULT_CLASS, 0} },
  /* 164 */ { "S164", 1, {DEFAULT_CLASS, 0} },
  /* 165 */ { "S165", 1, {DEFAULT_CLASS, 0} },
  /* 166 */ { "S166", 1, {DEFAULT_CLASS, 0} },
  /* 167 */ { "S167", 1, {DEFAULT_CLASS, 0} },
  /* 168 */ { "S168", 1, {DEFAULT_CLASS, 0} },
  /* 169 */ { "S169", 1, {DEFAULT_CLASS, 0} },
  /* 170 */ { "S170", 1, {DEFAULT_CLASS, 0} },
  /* 171 */ { "S171", 1, {DEFAULT_CLASS, 0} },
  /* 172 */ { "S172", 1, {DEFAULT_CLASS, 0} },
  /* 173 */ { "S173", 1, {DEFAULT_CLASS, 0} },
  /* 174 */ { "S174", 1, {DEFAULT_CLASS, 0} },
  /* 175 */ { "S175", 1, {DEFAULT_CLASS, 0} },
  /* 176 */ { "S176", 1, {DEFAULT_CLASS, 0} },
  /* 177 */ { "S177", 1, {DEFAULT_CLASS, 0} },
  /* 178 */ { "S178", 1, {DEFAULT_CLASS, 0} },
  /* 179 */ { "S179", 1, {DEFAULT_CLASS, 0} },
  /* 180 */ { "S180", 1, {DEFAULT_CLASS, 0} },
  /* 181 */ { "S181", 1, {DEFAULT_CLASS, 0} },
  /* 182 */ { "S182", 1, {DEFAULT_CLASS, 0} },
  /* 183 */ { "S183", 1, {DEFAULT_CLASS, 0} },
  /* 184 */ { "S184", 1, {DEFAULT_CLASS, 0} },
  /* 185 */ { "S185", 1, {DEFAULT_CLASS, 0} },
  /* 186 */ { "S186", 1, {DEFAULT_CLASS, 0} },
  /* 187 */ { "S187", 1, {DEFAULT_CLASS, 0} },
  /* 188 */ { "S188", 1, {DEFAULT_CLASS, 0} },
  /* 189 */ { "S189", 1, {DEFAULT_CLASS, 0} },
  /* 190 */ { "S190", 1, {DEFAULT_CLASS, 0} },
  /* 191 */ { "S191", 1, {DEFAULT_CLASS, 0} },
  /* 192 */ { "S192", 1, {DEFAULT_CLASS, 0} },
  /* 193 */ { "S193", 1, {DEFAULT_CLASS, 0} },
  /* 194 */ { "S194", 1, {DEFAULT_CLASS, 0} },
  /* 195 */ { "S195", 1, {DEFAULT_CLASS, 0} },
  /* 196 */ { "S196", 1, {DEFAULT_CLASS, 0} },
  /* 197 */ { "S197", 1, {DEFAULT_CLASS, 0} },
  /* 198 */ { "S198", 1, {DEFAULT_CLASS, 0} },
  /* 199 */ { "S199", 1, {DEFAULT_CLASS, 0} },
  /* 200 */ { "S200", 1, {DEFAULT_CLASS, 0} },
  /* 201 */ { "S201", 1, {DEFAULT_CLASS, 0} },
  /* 202 */ { "S202", 1, {DEFAULT_CLASS, 0} },
  /* 203 */ { "S203", 1, {DEFAULT_CLASS, 0} },
  /* 204 */ { "S204", 1, {DEFAULT_CLASS, 0} },
  /* 205 */ { "S205", 1, {DEFAULT_CLASS, 0} },
  /* 206 */ { "S206", 1, {DEFAULT_CLASS, 0} },
  /* 207 */ { "S207", 1, {DEFAULT_CLASS, 0} },
  /* 208 */ { "S208", 1, {DEFAULT_CLASS, 0} },
  /* 209 */ { "S209", 1, {DEFAULT_CLASS, 0} },
  /* 210 */ { "S210", 1, {DEFAULT_CLASS, 0} },
  /* 211 */ { "S211", 1, {DEFAULT_CLASS, 0} },
  /* 212 */ { "S212", 1, {DEFAULT_CLASS, 0} },
  /* 213 */ { "S213", 1, {DEFAULT_CLASS, 0} },
  /* 214 */ { "S214", 1, {DEFAULT_CLASS, 0} },
  /* 215 */ { "S215", 1, {DEFAULT_CLASS, 0} },
  /* 216 */ { "S216", 1, {DEFAULT_CLASS, 0} },
  /* 217 */ { "S217", 1, {DEFAULT_CLASS, 0} },
  /* 218 */ { "S218", 1, {DEFAULT_CLASS, 0} },
  /* 219 */ { "S219", 1, {DEFAULT_CLASS, 0} },
  /* 220 */ { "S220", 1, {DEFAULT_CLASS, 0} },
  /* 221 */ { "S221", 1, {DEFAULT_CLASS, 0} },
  /* 222 */ { "S222", 1, {DEFAULT_CLASS, 0} },
  /* 223 */ { "S223", 1, {DEFAULT_CLASS, 0} },
  /* 224 */ { "S224", 1, {DEFAULT_CLASS, 0} },
  /* 225 */ { "S225", 1, {DEFAULT_CLASS, 0} },
  /* 226 */ { "S226", 1, {DEFAULT_CLASS, 0} },
  /* 227 */ { "S227", 1, {DEFAULT_CLASS, 0} },
  /* 228 */ { "S228", 1, {DEFAULT_CLASS, 0} },
  /* 229 */ { "S229", 1, {DEFAULT_CLASS, 0} },
  /* 230 */ { "S230", 1, {DEFAULT_CLASS, 0} },
  /* 231 */ { "S231", 1, {DEFAULT_CLASS, 0} },
  /* 232 */ { "S232", 1, {DEFAULT_CLASS, 0} },
  /* 233 */ { "S233", 1, {DEFAULT_CLASS, 0} },
  /* 234 */ { "S234", 1, {DEFAULT_CLASS, 0} },
  /* 235 */ { "S235", 1, {DEFAULT_CLASS, 0} },
  /* 236 */ { "S236", 1, {DEFAULT_CLASS, 0} },
  /* 237 */ { "S237", 1, {DEFAULT_CLASS, 0} },
  /* 238 */ { "S238", 1, {DEFAULT_CLASS, 0} },
  /* 239 */ { "S239", 1, {DEFAULT_CLASS, 0} },
  /* 240 */ { "S240", 1, {DEFAULT_CLASS, 0} },
  /* 241 */ { "S241", 1, {DEFAULT_CLASS, 0} },
  /* 242 */ { "S242", 1, {DEFAULT_CLASS, 0} },
  /* 243 */ { "S243", 1, {DEFAULT_CLASS, 0} },
  /* 244 */ { "S244", 1, {DEFAULT_CLASS, 0} },
  /* 245 */ { "S245", 1, {DEFAULT_CLASS, 0} },
  /* 246 */ { "S246", 1, {DEFAULT_CLASS, 0} },
  /* 247 */ { "S247", 1, {DEFAULT_CLASS, 0} },
  /* 248 */ { "S248", 1, {DEFAULT_CLASS, 0} },
  /* 249 */ { "S249", 1, {DEFAULT_CLASS, 0} },
  /* 250 */ { "S250", 1, {DEFAULT_CLASS, 0} },
  /* 251 */ { "S251", 1, {DEFAULT_CLASS, 0} },
  /* 252 */ { "S252", 1, {DEFAULT_CLASS, 0} },
  /* 253 */ { "S253", 1, {DEFAULT_CLASS, 0} },
  /* 254 */ { "S254", 1, {DEFAULT_CLASS, 0} },
#endif /* 0 */    

  /*sentinel*/ { NULL,   0, {0, 0}}
};

#endif /* DECLARE_SENSOR_VARIABLES */
