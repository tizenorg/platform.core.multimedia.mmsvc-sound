/*
* Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#include "muse_sound_tone.h"
#include <dlog.h>

#include <math.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MUSED_SOUND_TONE"

/* ------------------------------------------------- */
//#define DEBUG_TONE

static const int TONE_SEGMENT[][MM_SOUND_TONE_NUM] =
{
	{941,	1336,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 0 key: 1336Hz, 941Hz

	{697,	1209,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 1 key: 1209Hz, 697Hz

	{697,	1336,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 2 key: 1336Hz, 697Hz

	{697,	1477,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, // 3 key: 1477Hz, 697Hz

	{770,	1209,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 4 key: 1209Hz, 770Hz

	{770,	1336,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 5 key: 1336Hz, 770Hz

	{770,	1477,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 6 key: 1477Hz, 770Hz

	{852,	1209,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 7 key: 1209Hz, 852Hz

	{852,	1336,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 8 key: 1336Hz, 852Hz

	{852,	1477,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// 9 key: 1477Hz, 852Hz

	{941,	1209,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// * key: 1209Hz, 941Hz

	{941,	1477,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// # key: 1477Hz, 941Hz

	{697,	1633,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// A key: 1633Hz, 697Hz

	{770,	1633,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// B key: 1633Hz, 770Hz

	{852,	1633,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},// C key: 1633Hz, 852Hz

	{941,	1633,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, // D key: 1633Hz, 941Hz

	{425,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},  //Call supervisory tone, Dial tone: CEPT: 425Hz, continuous

	{350,	440, 0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},  //Call supervisory tone, Dial tone: ANSI (IS-95): 350Hz+440Hz, continuous

	{400,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},  //Call supervisory tone, Dial tone: JAPAN: 400Hz, continuous

	{425,	0,	0,	500,	0,	0,
		0,	0,	0,	500,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},//Call supervisory tone, Busy: CEPT: 425Hz, 500ms ON, 500ms OFF...

	{480,	620, 0,	500,	0,	0,
		0,	0,	 0,	500,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Busy: ANSI (IS-95): 480Hz+620Hz, 500ms ON, 500ms OFF...

	{400,	0,	0,	500,	0,	0,
		0,	0,	 0,	500,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Busy: JAPAN: 400Hz, 500ms ON, 500ms OFF...

	{425,	0,	0,	200,	0,	0,
		0,	0,	0,	200,	0,	0,
	-1,	-1,	-1,	-1,	1,	0}, //Call supervisory tone, Congestion: CEPT, JAPAN: 425Hz, 200ms ON, 200ms OFF

	{480,	620, 0,	250,	0,	0,
		0,	0,	0,	250,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Congestion: ANSI (IS-95): 480Hz+620Hz, 250ms ON, 250ms OFF...

	{425,	0,	0,	200,	0,	0,
	-1,	-1,	-1,	-1,	1,	0}, //Call supervisory tone, Radio path acknowlegment : CEPT, ANSI: 425Hz, 200ms ON

	{400,	0,	0,	1000,	0,	0,
		0,	0,	0,	2000,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Radio path acknowlegment : JAPAN: 400Hz, 1s ON, 2s OFF...

	{425,	0,	0,	200,	0,	0,
		0,	0,	0,	200,	0,	0,
		-1,	-1,	-1,	-1,	3,	0}, //Call supervisory tone, Radio path not available: 425Hz, 200ms ON, 200 OFF 3 bursts

	{950, 1400, 1800,	330,	0,	0,
	0,	0,	0,	1000,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Error/Special info: 950Hz+1400Hz+1800Hz, 330ms ON, 1s OFF...

	{425,	0,	0,	200,	0,	0,
		0,	0,	0,	600,	0,	0,
	 425,	0,	0,	200,	0,	0,
	 	0,	0,	0,	3000,0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Call Waiting: CEPT, JAPAN: 425Hz, 200ms ON, 600ms OFF, 200ms ON, 3s OFF...

	{440,	0,	0,	300,	0,	0,
	0,	0,	0,	9700,	0,	0,
	440,	0,	0,	100,	0,	0,
	0,	0,	0,	100,	0,	0,
	440,	0,	0,	100,	0,	0,
	0,	0,	0,	9700,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Call Waiting: ANSI (IS-95): 440 Hz, 300 ms ON, 9.7 s OFF, (100 ms ON, 100 ms OFF, 100 ms ON, 9.7s OFF ...)

	{425,	0,	0,	1000,	0,	0,
		0,	0,	0,	4000,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Ring Tone: CEPT, JAPAN: 425Hz, 1s ON, 4s OFF...

	{440,  480,	0, 	2000,	0,	0,
		0,	  0,	0,	4000,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //Call supervisory tone, Ring Tone: ANSI (IS-95): 440Hz + 480Hz, 2s ON, 4s OFF...

	{400,   1200,	  0, 	35,	0,	 0,
	-1,	-1,	-1,	-1,	1,	0}, // General beep: 400Hz+1200Hz, 35ms ON

	{1200,	0,	0, 100,	0,	0,
		0,	0,	0, 100,	0,	0,
		-1,	-1,	-1,	-1,	2,	0}, //Proprietary tone, positive acknowlegement: 1200Hz, 100ms ON, 100ms OFF 2 bursts

	{300, 400, 500,  400,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //Proprietary tone, negative acknowlegement: 300Hz+400Hz+500Hz, 400ms ON

	{400, 1200,   0,  200,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //Proprietary tone, prompt tone: 400Hz+1200Hz, 200ms ON

	{400,	1200,	0,	35,	0,	0,
		0,	0,	0,	200,		0,	0,
	400,		1200,	0,	35,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //Proprietary tone, general double beep: twice 400Hz+1200Hz, 35ms ON, 200ms OFF, 35ms ON

	{440,	0,	0,	250,	0,	0,
	 620,	0,	0,	250,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //Call supervisory tone (IS-95), intercept tone: alternating 440 Hz and 620 Hz tones, each on for 250 ms

	{440,	0,	0,	250, 0,	0,
	620,		0,	0,	250, 0,	0,
		-1,	-1,	-1,	-1,	8,	0}, //Call supervisory tone (IS-95), abbreviated intercept: intercept tone limited to 4 seconds

	{480,	620,	0,	250,	0,	0,
	     0,	0,   	0,	250,	0,	0,
		-1,	-1,	-1,	-1,	8,	0 }, //Call supervisory tone (IS-95), abbreviated congestion: congestion tone limited to 4 seconds

	{350,	440,	0,	100,	0,	0,
		0,	0,	0,	100,	0,	0,
		-1,	-1,	-1,	-1,	3,	0}, //Call supervisory tone (IS-95), confirm tone: a 350 Hz tone added to a 440 Hz tone repeated 3 times in a 100 ms on, 100 ms off cycle

	{480,	0,	0,	100,	0,	0,
		0,	0,	0,	100,	0,	0,
		-1,	-1,	-1,	-1,	4,	0}, //Call supervisory tone (IS-95), pip tone: four bursts of 480 Hz tone (0.1 s on, 0.1 s off).

	{ 425,	0,	0,	-1,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //425Hz continuous

	{440,	480,	0,	2000,	0,	0,
		0,	0,	0,	4000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA USA Ringback: 440Hz+480Hz 2s ON, 4000 OFF ...

	{440,	0,	0,	250,	0,	0,
	620,		0,	0,	250,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA Intercept tone: 440Hz 250ms ON, 620Hz 250ms ON ...

	{440,	0,	0,	250,	0,	0,
	620,		0,	0,	250,	0,	0,
		-1,	-1,	-1,	-1,	1,	0 }, //CDMA Abbr Intercept tone: 440Hz 250ms ON, 620Hz 250ms ON

	{480,	620,	0,	 250,	0,	0,
		0,	0,	0,	 250,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 }, //CDMA Reorder tone: 480Hz+620Hz 250ms ON, 250ms OFF...

	{480,	620,	0,	250,	0,	0,
		0,	0,	0,	250,	0,	0,
		-1,	-1,	-1,	-1,	8,	0}, //CDMA Abbr Reorder tone: 480Hz+620Hz 250ms ON, 250ms OFF repeated for 8 times

	{480,	620,	0,	500,	0,	0,
		0,	0,	0,	500,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA Network Busy tone: 480Hz+620Hz 500ms ON, 500ms OFF continuous

	{350,	440,	0,	100,	0,	0,
		0,	0,	0,	100,	0,	0,
		-1,	-1,	-1,	-1,	3,	0}, //CDMA Confirm tone: 350Hz+440Hz 100ms ON, 100ms OFF repeated for 3 times

	{660, 1000,	0,	500,	0,	0,
		0,	0,	0,	100,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA answer tone: silent tone - defintion Frequency 0, 0ms ON, 0ms OFF

	{440,	0,	0,	300,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA Network Callwaiting tone: 440Hz 300ms ON

	{480,	0,	0,	100,	0,	0,
		0,	0,	0,	100,	0,	0,
		-1,	-1,	-1,	-1,	4,	0}, //CDMA PIP tone: 480Hz 100ms ON, 100ms OFF repeated for 4 times

	{2090,	0,	0,	32,	0,	0,
	2556,	0,	0,	64,	19,	0,
	2090,	0,	0,	32,	0,	0,
	2556,	0,	0,	48,	0,	0,
	0,	0,	 0,	4000,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //ISDN Call Signal Normal tone: {2091Hz 32ms ON, 2556 64ms ON} 20 times, 2091 32ms ON, 2556 48ms ON, 4s OFF

	{2091,	0,	0,	32,	0,	0,
	2556,	0,	0,	64,	7,	0,
	2091,	0,	0,	32,	0,	0,
	0,	0,	0,	400,		0,	0,
	2091,	0,	0,	32,	0,	0,
	2556,	0,	0,	64,	7,	4,
	2091,	0,	0,	32,	0,	0,
	0,	0,	0,	4000,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //ISDN Call Signal Intergroup tone: {2091Hz 32ms ON, 2556 64ms ON} 8 times, 2091Hz 32ms ON, 400ms OFF, {2091Hz 32ms ON, 2556Hz 64ms ON} 8times, 2091Hz 32ms ON, 4s OFF

	{2091,	0,	0,	32,	0,	0,
	2556,	0,	0,	64,	3,	0,
	2091,	0,	0,	32,	0,	0,
		0, 	0,	0,	200,	0,	0,
	2091,	0,	0,	32,	0,	0,
	2556,	0,	0,	64,	3,	4,
	2091,	0,	0,	32,	0,	0,
		0,	0,	0,	200,	0,	0,
		-1,	-1,	-1,	-1,	1,	0},//ISDN Call Signal SP PRI tone:{2091Hz 32ms ON, 2556 64ms ON} 4 times 2091Hz 16ms ON, 200ms OFF, {2091Hz 32ms ON, 2556Hz 64ms ON} 4 times, 2091Hz 16ms ON, 200ms OFF

	{0,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	1,	0}, //ISDN Call sign PAT3 tone: silent tone

	{2091,	0,	0,	32,	0,	0,
	2556,	0,	0,	64,	4,	0,
	2091,	0,	0,	20,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //ISDN Ping Ring tone: {2091Hz 32ms ON, 2556Hz 64ms ON} 5 times 2091Hz 20ms ON

	{0,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	1,	0}, //ISDN Pat5 tone: silent tone

	{0,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	1,	0}, //ISDN Pat6 tone: silent tone

	{0,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	1,	0}, //ISDN Pat7 tone: silent tone

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	39,	0,
		0,	0,	0, 4000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //TONE_CDMA_HIGH_L tone: {3700Hz 25ms, 4000Hz 25ms} 40 times 4000ms OFF, Repeat ....

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	39,	0,
		0,	0,	0, 4000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0},//TONE_CDMA_MED_L tone: {2600Hz 25ms, 2900Hz 25ms} 40 times 4000ms OFF, Repeat ....

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	39,	0,
		0,	0,	0,4000,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //TONE_CDMA_LOW_L tone: {1300Hz 25ms, 1450Hz 25ms} 40 times, 4000ms OFF, Repeat ....

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	15,	0,
	0,	0,	0, 400,	0,	0,
	-1,	-1,	-1,	-1,	0,	0},//CDMA HIGH SS tone: {3700Hz 25ms, 4000Hz 25ms} repeat 16 times, 400ms OFF, repeat ....

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	15,	0,
		0,	0,	0,400,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //CDMA MED SS tone: {2600Hz 25ms, 2900Hz 25ms} repeat 16 times, 400ms OFF, repeat ....

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	15,	0,
		0,	0,	0,	400,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //CDMA LOW SS tone: {1300z 25ms, 1450Hz 25ms} repeat 16 times, 400ms OFF, repeat ....

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	0,
		0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	3,
		0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	15,	6,
		0,	0,	0,	4000,0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA HIGH SSL tone: {3700Hz 25ms, 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} repeat 8 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} repeat 16 times, 4000ms OFF, repeat ...

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	0,
		0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	3,
		0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	15,	6,
		0,	0,	0,	4000,0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA MED SSL tone: {2600Hz 25ms, 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} repeat 8 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} repeat 16 times, 4000ms OFF, repeat ...

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	0,
		0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	3,
		0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	15,	6,
		0,	0,	0,	4000,0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA LOW SSL tone: {1300Hz 25ms, 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} repeat 8 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} repeat 16 times, 4000ms OFF, repeat ...

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	19,	0,
	0,	0,	0,	1000,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	19,	3,
	0,	0,	0,	3000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 },//CDMA HIGH SS2 tone: {3700Hz 25ms, 4000Hz 25ms} 20 times, 1000ms OFF, {3700Hz 25ms, 4000Hz 25ms} 20 times, 3000ms OFF, repeat ....

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	19,	0,
	0,	0,	0,	1000,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	19,	3,
	0,	0,	0,	3000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA MED SS2 tone: {2600Hz 25ms, 2900Hz 25ms} 20 times, 1000ms OFF, {2600Hz 25ms, 2900Hz 25ms} 20 times, 3000ms OFF, repeat ....

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	19,	0,
	0,	0,	0,	1000,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	19,	3,
	0,	0,	0,	3000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 }, //CDMA LOW SS2 tone: {1300Hz 25ms, 1450Hz 25ms} 20 times, 1000ms OFF, {1300Hz 25ms, 1450Hz 25ms} 20 times, 3000ms OFF, repeat ....

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	9,	0,
		0,	0,	0,	500,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	19,	3,
		0,	0,	0,	500,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	9,	6,
	0,	0,	0,	3000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA HIGH SLS tone: {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 20 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 3000ms OFF, REPEAT....

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	9,	0,
		0,	0,	0,	500,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	19,	3,
		0,	0,	0,	500,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	9,	6,
	0,	0,	0,	3000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA MED SLS tone: {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 20 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 3000ms OFF, REPEAT....

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	9,	0,
	0,	0,	0,	500,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	19,	3,
	0,	0,	0,	500,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	9,	6,
	0,	0,	0,	3000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA LOW SLS tone: {1300Hz 25ms, 1450Hz 25ms} 10 times, 500ms OFF, {1300Hz 25ms, 1450Hz 25ms} 20 times, 500ms OFF, {1300Hz 25ms, 1450Hz 25ms} 10 times, 3000ms OFF, REPEAT....

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	9,	0,
		0,	0,	0,	500,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	9,	3,
		0,	0,	0,	500,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	9,	6,
	0,	0,	0,	2500,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, ////CDMA HIGH S X4 tone: {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 500ms OFF, {3700Hz 25ms, 4000Hz 25ms} 10 times, 2500ms OFF, REPEAT....

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	9,	0,
		0,	0,	0,	500,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	9,	4,
		0,	0,	0,	500,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	9,	6,
	0,	0,	0,	2500,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 },//CDMA MED S X4 tone: {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 2500ms OFF, REPEAT....

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	9,	0,
		0,	0,	0,	500,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	9,	3,
		0,	0,	0,	500,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	9,	6,
	0,	0,	0,	2500,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA LOW S X4 tone: {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 500ms OFF, {2600Hz 25ms, 2900Hz 25ms} 10 times, 2500ms OFF, REPEAT....

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	19,	0,
	0,	0,	0,	2000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 },//CDMA HIGH PBX L: {3700Hz 25ms, 4000Hz 25ms}20 times, 2000ms OFF, REPEAT....

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	19,	0,
	0,	0,	0,	2000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA MED PBX L: {2600Hz 25ms, 2900Hz 25ms}20 times, 2000ms OFF, REPEAT....

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	19,	0,
	0,	0,	0,	2000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 },//CDMA LOW PBX L: {1300Hz 25ms,1450Hz 25ms}20 times, 2000ms OFF, REPEAT....

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	3,
	0,	0,	0,	2000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA HIGH PBX SS tone: {3700Hz 25ms, 4000Hz 25ms} 8 times 200 ms OFF, {3700Hz 25ms 4000Hz 25ms}8 times, 2000ms OFF, REPEAT....

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	3,
	0,	0,	0,	2000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 }, //CDMA MED PBX SS tone: {2600Hz 25ms, 2900Hz 25ms} 8 times 200 ms OFF, {2600Hz 25ms 2900Hz 25ms}8 times, 2000ms OFF, REPEAT....

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	3,
	0,	0,	0,	2000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 },//CDMA LOW PBX SS tone: {1300Hz 25ms, 1450Hz 25ms} 8 times 200 ms OFF, {1300Hz 25ms 1450Hz 25ms}8 times, 2000ms OFF, REPEAT....

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	3,
	0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	15,	6,
	0,	0,	0,	1000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA HIGH PBX SSL tone:{3700Hz 25ms, 4000Hz 25ms} 8 times 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 16 times, 1000ms OFF, REPEAT....//

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	3,
	0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	15,	6,
	0,	0,	0,	1000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA MED PBX SSL tone:{2600Hz 25ms, 2900Hz 25ms} 8 times 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 16 times, 1000ms OFF, REPEAT....//

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	3,
	0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	15,	6,
	0,	0,	0,	1000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA LOW PBX SSL tone:{1300Hz 25ms, 1450Hz 25ms} 8 times 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 16 times, 1000ms OFF, REPEAT....//

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	15,	0,
	0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	3,
	0,	0,	0,	1000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 },//CDMA HIGH PBX SLS tone:{3700Hz 25ms, 4000Hz 25ms} 8 times 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 16 times, 200ms OFF, {3700Hz 25ms, 4000Hz 25ms} 8 times, 1000ms OFF, REPEAT.... //

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	15,	0,
		0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	3,
		0,	0,	0,	1000,0,	0,
		-1,	-1,	-1,	-1,	0,	0 }, //CDMA HIGH PBX SLS tone:{2600Hz 25ms, 2900Hz 25ms} 8 times 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 16 times, 200ms OFF, {2600Hz 25ms, 2900Hz 25ms} 8 times, 1000ms OFF, REPEAT....//

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	15,	0,
	0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	3,
	0,	0,	0,	1000,	0,	0,
		-1,	-1,	-1,	-1,	0,	0 }, //CDMA HIGH PBX SLS tone:{1300Hz 25ms, 1450Hz 25ms} 8 times 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 16 times, 200ms OFF, {1300Hz 25ms, 1450Hz 25ms} 8 times, 1000ms OFF, REPEAT....//

	{3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	3,
	0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	6,
	0,	0,	0,	200,	0,	0,
	3700,	0,	0,	25,	0,	0,
	4000,	0,	0,	25,	7,	9,
	0,	0,	0,	800,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA HIGH PBX X S4 tone: {3700Hz 25ms 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms 4000Hz 25ms} 8 times, 200ms OFF, {3700Hz 25ms 4000Hz 25ms} 8 times, 800ms OFF, REPEAT...

	{2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	0,
	0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	3,
	0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	6,
	0,	0,	0,	200,	0,	0,
	2600,	0,	0,	25,	0,	0,
	2900,	0,	0,	25,	7,	9,
	0,	0,	0,	800,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA MED PBX X S4 tone: {2600Hz 25ms 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms 2900Hz 25ms} 8 times, 200ms OFF, {2600Hz 25ms 2900Hz 25ms} 8 times, 800ms OFF, REPEAT...

	{1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	0,
		0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	3,
		0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	6,
		0,	0,	0,	200,	0,	0,
	1300,	0,	0,	25,	0,	0,
	1450,	0,	0,	25,	7,	9,
		0,	0,	0,	800,	0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA LOW PBX X S4 tone: {1300Hz 25ms 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms 1450Hz 25ms} 8 times, 200ms OFF, {1300Hz 25ms 1450Hz 25ms} 8 times, 800ms OFF, REPEAT...

	{1109,	0,	0,	62,	0,	0,
		784,	0,	0,	62,	0,	0,
		740,	0,	0,	62,	0,	0,
		622,	0,	0,	62,	0,	0,
	1109,	0,	0,	62,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA Alert Network Lite tone: 1109Hz 62ms ON, 784Hz 62ms ON, 740Hz 62ms ON 622Hz 62ms ON, 1109Hz 62ms ON

	{1245,	0,	0,	62,	0,	0,
	659,	0,	0,	62,	0,	0,
	1245,	0,	0,	62,	0,	0,
	659,	0,	0,	62,	0,	0,
	1245,	0,	0,	62,	0,	0,
	659,	0,	0,	62,	0,	0,
	1245,	0,	0,	62,	0,	0,
		-1,	-1,	-1,	-1,	1,	0},//CDMA Alert Auto Redial tone: {1245Hz 62ms ON, 659Hz 62ms ON} 3 times, 1245 62ms ON//

	{1150,	770, 0,  400, 0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA One Min Beep tone: 1150Hz+770Hz 400ms ON//

	{941, 1477,	0,  120, 0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA KEYPAD Volume key lite tone: 941Hz+1477Hz 120ms ON

	{587,	0,	 0, 375, 0,	0,
	1175,	0,	 0, 125, 0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA PRESSHOLDKEY LITE tone: 587Hz 375ms ON, 1175Hz 125ms ON

	{587,	0,	0,	62, 0,	0,
	784,		0,	0,	62, 0,	0,
	831,		0,	0,	62, 0,	0,
	784,		0,	0,	62, 0,	0,
	1109,	0,	0,	62, 0,	0,
	784, 	0,	0,	62, 0,	0,
	831,		0,	0,	62, 0,	0,
	784, 	0,	0,	62, 0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA ALERT INCALL LITE tone: 587Hz 62ms, 784 62ms, 831Hz 62ms, 784Hz 62ms, 1109 62ms, 784Hz 62ms, 831Hz 62ms, 784Hz 62ms

	{941,	0,	0,	125,	0,	0,
		0,	0,	0,	10,	0,	0,
		941,	0,	0,	125,	0,	0,
		0,	0,	0,	10,	0,	0,
	1245,	0,	0,	62,	0,	0,
		0,	0,	0,	10,	0,	0,
		0,	0,	0,	4990, 0,	0,
		-1,	-1,	-1,	-1,	0,	0}, //CDMA EMERGENCY RINGBACK tone: {941Hz 125ms ON, 10ms OFF} 3times 4990ms OFF, REPEAT...

	{1319,	0,	0,	125,	0,	0,
		0,	0,	0,	125,	0,	0,
		-1,	-1,	-1,	-1,	3,	0 }, //CDMA ALERT CALL GUARD tone: {1319Hz 125ms ON, 125ms OFF} 3 times

	{1047,	0,	0,	125,	0,	0,
		370,	0,	0,	125,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA SOFT ERROR LITE tone: 1047Hz 125ms ON, 370Hz 125ms

	{1480,	0,	0,	125,	0,	0,
	1397,	0,	0,	125,	0,	0,
	784,	0,	0,	125,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA CALLDROP LITE tone: 1480Hz 125ms, 1397Hz 125ms, 784Hz 125ms//

	{425,	0,	0,	125,	0,	0,
		0,	0,	0,	125,	0,	0,
		-1,	-1,	-1,	-1,	1,	0},//CDMA_NETWORK_BUSY_ONE_SHOT tone: 425Hz 500ms ON, 500ms OFF

	{1150,	770,	0,	400,	0,	0,
		-1,	-1,	-1,	-1,	1,	0}, //CDMA_ABBR_ALERT tone: 1150Hz+770Hz 400ms ON

	{0,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	1,	0}, //CDMA_SIGNAL_OFF - silent tone

	{100,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //100Hz continuous

	{200,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //200Hz continuous

	{300,	0,	0,	-1,	0,	0,
	-1,	-1,	-1,	-1,	0,	0}, //300Hz continuous
};

char* tone_create(double *sample, TONE _TONE, double volume, int *toneSize, int is_cycle_ended)
{
	short *pbuf;
	double i = 0;
	double amplitude, f1, f2, f3;
	int quota= 0;
	float low_frequency;
	float middle_frequency;
	float high_frequency;
	int sample_size = 0;
	char* buffer;

	if (is_cycle_ended) {
		low_frequency = 0;
		middle_frequency = 0;
		high_frequency = 0;
	} else {
		low_frequency = _TONE.low_frequency;
		middle_frequency = _TONE.middle_frequency;
		high_frequency = _TONE.high_frequency;
	}

	if (sample == NULL) {
		LOGE("Sample buffer is not allocated");
		return NULL;
	}

	/* Create a buffer for the tone */
	if ((_TONE.playingTime >  MAX_DURATION) || (_TONE.playingTime == -1) ) {
		*toneSize = ((MAX_DURATION / 1000.) * SAMPLERATE * SAMPLE_SIZE * CHANNELS) / 8;
	} else	 {
		*toneSize = ((_TONE.playingTime / 1000.) * SAMPLERATE * SAMPLE_SIZE * CHANNELS) / 8;
	}
	*toneSize = ((*toneSize+1)>>1)<<1;
	sample_size = (*toneSize) / (SAMPLE_SIZE / 8);

#ifdef DEBUG_TONE
	LOGE("%0.f, %0.f, %0.f _TONE.playing_time: %d toneSize: %d",
			low_frequency, middle_frequency, high_frequency, _TONE.playingTime, *toneSize);
#endif

	buffer = malloc(*toneSize);
	if (buffer == NULL) {
		LOGE("Buffer is not allocated");
		return NULL;
	} else {
		pbuf = (short*)buffer;

		if (_TONE.low_frequency > 0) {
			quota++;
		}
		if (_TONE.middle_frequency > 0) {
			quota++;
		}
		if (_TONE.high_frequency > 0) {
			quota++;
		}

		for (i = 0; i < sample_size; i++) {
			/*
			 * We add the fundamental frequencies together.
			 */

			f1 = sin (2 * M_PI * low_frequency     	* ((*sample) / SAMPLERATE));
			f2 = sin (2 * M_PI * middle_frequency	* ((*sample) / SAMPLERATE));
			f3 = sin (2 * M_PI *high_frequency	* ((*sample) / SAMPLERATE));

			if (f1 + f2 + f3 != 0) {
				amplitude = (f1 + f2 + f3) / quota;
				/* Adjust the volume */
				amplitude *= volume;

				/* Make the [-1:1] interval into a [-32767:32767] interval */
				amplitude *= 32767;
			} else {
				amplitude = 0;
			}

			/* Store it in the data buffer */
			*(pbuf++) = (short) amplitude;

			(*sample)++;
		}
	}
	return buffer;
}

TONE tone_get(int key, int CurIndex)
{
	TONE _TONE;

	_TONE.low_frequency		= TONE_SEGMENT[key][CurIndex * TONE_COLUMN + LOW_FREQUENCY];
	_TONE.middle_frequency	= TONE_SEGMENT[key][CurIndex * TONE_COLUMN + MIDDLE_FREQUENCY];
	_TONE.high_frequency	= TONE_SEGMENT[key][CurIndex * TONE_COLUMN + HIGH_FREQUENCY];
	_TONE.playingTime		= TONE_SEGMENT[key][CurIndex * TONE_COLUMN + PLAYING_TIME];
	_TONE.loopCnt			= TONE_SEGMENT[key][CurIndex * TONE_COLUMN + LOOP_COUNT];
	_TONE.loopIndx			= TONE_SEGMENT[key][CurIndex * TONE_COLUMN + LOOP_INDEX];

	return _TONE;
}

int tone_get_wave_cnt_playing_time(int toneTime, TONE _TONE, int *waveCnt, int *waveRestPlayTime)
{
	int ret = 0;
	if (waveCnt == NULL || waveRestPlayTime == NULL) {
		LOGE("waveCnt || waveRestPlayTime buffer is NULL");
		return -1;
	}
	/*Set wave count and wave playing time*/
	if ( _TONE.playingTime == -1) {
		*waveCnt = abs(toneTime) / MAX_DURATION;
		*waveRestPlayTime =  abs(toneTime) % MAX_DURATION;
	} else {
		*waveCnt = _TONE.playingTime / MAX_DURATION;
		*waveRestPlayTime =  _TONE.playingTime % MAX_DURATION;
	}
	return ret;
}

int tone_get_cur_index(TONE _TONE, int *CurArrayPlayCnt, int *CurIndex)
{
	int ret = 0;
	if (CurArrayPlayCnt == NULL || CurIndex == NULL) {
		LOGE("CurArrayPlayCnt || CurIndex buffer is NULL");
		return -1;
	}

	if (_TONE.loopCnt != 0 && *CurArrayPlayCnt <= _TONE.loopCnt) {
		(*CurArrayPlayCnt)++;
		if (*CurArrayPlayCnt >_TONE.loopCnt) {
			(*CurIndex)++;
			*CurArrayPlayCnt = 0;
		} else {
			*CurIndex = _TONE.loopIndx;
		}
	} else {
		(*CurIndex)++;
	}
#ifdef DEBUG_TONE
	LOGE("[%d] CurIndex: %d", *CurArrayPlayCnt, *CurIndex);
#endif
	return ret;
}

/* ---------------------------------------------------------------------------------*/
