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

#ifndef __MUSE_SOUND_INTERNAL_H__
#define __MUSE_SOUND_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <tizen.h>

#include <muse_core.h>
#include <muse_core_ipc.h>
#include "muse_sound.h"
#include "muse_sound_msg.h"
#include <dlog.h>
#include <glib.h>


#define IS_WAV(mode)    (mode & MODE_WAV)
#define IS_TONE(mode)    (mode & MODE_TONE)

//#define USE_OPENAL
//#define USE_CANBERRA
#define USE_PULSE

#ifdef USE_OPENAL
#include <AL/alure.h>
#endif
#ifdef USE_CANBERRA
#include <canberra.h>
#endif
#ifdef USE_PULSE
#include <pulse/pulseaudio.h>
#include <sndfile.h>
#endif

/* FIXME : remove redundant declaration???? */
typedef enum {
    SOUND_ERROR_NONE                 = TIZEN_ERROR_NONE,                  /**< Successful */
    SOUND_ERROR_INVALID_PARAMETER    = TIZEN_ERROR_INVALID_PARAMETER,     /**< Invalid parameter */
    SOUND_ERROR_INVALID_OPERATION    = TIZEN_ERROR_INVALID_OPERATION,     /**< Invalid operation */
} sound_error_e;

typedef enum {
    WAV_PLAYER_ERROR_FORMAT_NOT_SUPPORTED = TIZEN_ERROR_WAV_PLAYER | 0x01,     /**< Format not supported */
    WAV_PLAYER_ERROR_NOT_SUPPORTED_TYPE   = TIZEN_ERROR_WAV_PLAYER | 0x02      /**< Not supported (Since 3.0) */
} wav_player_error_e;

typedef enum {
	TONE_PLAYER_ERROR_NOT_SUPPORTED_TYPE = TIZEN_ERROR_TONE_PLAYER | 0x01       /**< Not supported (Since 3.0) */
} tone_player_error_e;


/**
 * @brief The structure type for the muse sound.
 */
typedef struct {
#ifdef USE_OPENAL
	ALuint buffer;
	ALuint source;
#endif

#ifdef USE_CANBERRA
	ca_context *c;
	int id;
#endif
#ifdef USE_PULSE
	pa_threaded_mainloop *m;
	pa_context *c;
	pa_stream* s;
	pa_sample_spec spec;
	SNDFILE* sf;
	SF_INFO si;
#endif

	int mode; /* wav(0) or tone(1) */

	char* stream_type;
	int stream_id;

	/* wav */
	char * file_path;

	/* tone */
	GThread *g_thread;
	int tone;
	int duration;
	gboolean need_stop;

	/* interface */
	int (*start)(void *);
	int (*stop)(void *);

	muse_module_h module;
} muse_sound_handle_s;

#ifdef USE_OPENAL
int alure_sound_start(muse_sound_handle_s * h);
int alure_sound_stop(muse_sound_handle_s * h);
#endif
#ifdef USE_CANBERRA
int canberra_sound_start(muse_sound_handle_s * h);
int canberra_sound_stop(muse_sound_handle_s * h);
#endif
#ifdef USE_PULSE
int pulse_sound_start(muse_sound_handle_s * h);
int pulse_sound_stop(muse_sound_handle_s * h);
#endif

#ifdef __cplusplus
}
#endif

#endif /*__MUSE_SOUND_INTERNAL_H__*/
