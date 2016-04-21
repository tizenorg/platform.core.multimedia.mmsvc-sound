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

#include "muse_sound_internal.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MUSED_SOUND"

static int _sound_start(muse_sound_handle_s * h)
{
#ifdef USE_OPENAL
	return alure_sound_start(h);
#endif
#ifdef USE_CANBERRA
	return canberra_sound_start(h);
#endif
#ifdef USE_PULSE
	return pulse_sound_start(h);
#endif
	return -1;
}

static int _sound_stop(muse_sound_handle_s * h)
{
#ifdef USE_OPENAL
	return alure_sound_stop(h);
#endif
#ifdef USE_CANBERRA
	return canberra_sound_stop(h);
#endif
#ifdef USE_PULSE
	return pulse_sound_stop(h);
#endif
	return -1;
}

static void _handle_dump(muse_sound_handle_s *h)
{
	if (h) {
		if (h->mode == MODE_WAV)
			LOGE("mode[%d] : path[%s], start[%p], stop[%p]", h->mode, h->file_path, h->start, h->stop);
		else
			LOGE("mode[%d] : tone[%d], duration[%d], start[%p], stop[%p]", h->mode, h->tone, h->duration, h->start, h->stop);
	}
}

static void _handle_free(muse_sound_handle_s *h)
{
	if (h) {
		if (h->file_path)
			free(h->file_path);
		free(h);
	}
}

static muse_sound_handle_s* _handle_new(muse_module_h module)
{
	muse_sound_handle_s *sound_h = NULL;
	char path[256] = { 0, };
	int mode;
	int tone = 0;
	int duration = 0;

	sound_h = (muse_sound_handle_s *)malloc(sizeof(muse_sound_handle_s));
	if (!sound_h) {
		LOGE("Failed to alloc handle!!!");
		return NULL;
	}
	memset (sound_h, 0, sizeof(muse_sound_handle_s));

	sound_h->module = module;

	/* Fill values */
	muse_sound_msg_get(mode, muse_core_client_get_msg(module));
	sound_h->mode = mode;

	if (mode == MODE_WAV) { /* wav */
		muse_sound_msg_get_string(path, muse_core_client_get_msg(module));
		sound_h->file_path = strdup(path);
	} else { /* tone */
		muse_sound_msg_get(tone, muse_core_client_get_msg(module));
		sound_h->tone = tone;

		muse_sound_msg_get(duration, muse_core_client_get_msg(module));
		sound_h->duration = duration;
	}

	/* Set interface */
#ifdef USE_PULSE
	sound_h->start = pulse_sound_start;
	sound_h->stop = pulse_sound_stop;
#endif
#ifdef USE_CANBERRA
	sound_h->start = canberra_sound_start;
	sound_h->stop = canberra_sound_stop;
#endif
#ifdef USE_OPENAL
	sound_h->start = alure_sound_start;
	sound_h->stop = alure_sound_stop;
#endif

	_handle_dump(sound_h);

	return sound_h;
}

int sound_dispatcher_start(muse_module_h module)
{
	muse_sound_api_e api = MUSE_SOUND_API_START;
	muse_sound_handle_s *sound_h = NULL;
	int ret = SOUND_ERROR_NONE;

	sound_h = _handle_new(module);
	if (sound_h) {
		if (sound_h->start(sound_h) != -1) {
			LOGE("Success to play!!!");
			muse_core_ipc_set_handle(module, (intptr_t)sound_h);
		} else {
			LOGE("Failed to play....");
			_handle_free(sound_h);
			ret = SOUND_ERROR_INVALID_OPERATION;
		}
	} else {
		ret = SOUND_ERROR_INVALID_OPERATION;
	}

	muse_sound_msg_return(api, ret, module);

	return MUSE_SOUND_ERROR_NONE;
}

int sound_dispatcher_dummy(muse_module_h module)
{
	LOGE("This must be not entered!!!");
	return MUSE_SOUND_ERROR_NONE;
}

int sound_dispatcher_stop(muse_module_h module)
{
	muse_sound_api_e api = MUSE_SOUND_API_STOP;
	muse_sound_handle_s *sound_h = NULL;
	int ret = SOUND_ERROR_NONE;

	/* get handle */
	sound_h = (muse_sound_handle_s *)muse_core_ipc_get_handle(module);
	if (sound_h) {
		sound_h->stop(sound_h);
	} else {
		ret = SOUND_ERROR_INVALID_OPERATION;
	}

	muse_sound_msg_return(api, ret, module);

	return MUSE_SOUND_ERROR_NONE;
}

int (*dispatcher[MUSE_SOUND_API_MAX]) (muse_module_h module) = {
	sound_dispatcher_start, /* MUSE_SOUND_API_START */
	sound_dispatcher_dummy, /* MUSE_SOUND_API_DUMMY */
	sound_dispatcher_stop, /* MUSE_SOUND_API_STOP */
};

/******************/
/* cmd dispatcher */
/******************/
static int sound_cmd_dispatcher_shutdown(muse_module_h module)
{
	muse_sound_handle_s *muse_sound = NULL;

	/* get handle */
	muse_sound = (muse_sound_handle_s *)muse_core_ipc_get_handle(module);
	if (!muse_sound) {
		return MUSE_SOUND_ERROR_INVALID;
	}

	/* Stop Sound */
	_sound_stop(muse_sound);

	/* clean-up */
	muse_core_ipc_set_handle(module, (intptr_t)NULL);
	_handle_free(muse_sound);

	LOGE("done");

	return MUSE_SOUND_ERROR_NONE;
}

int (*cmd_dispatcher[MUSE_MODULE_EVENT_MAX])(muse_module_h module) = {
	sound_cmd_dispatcher_shutdown, /* MUSE_MODULE_EVENT_SHUTDOWN */
	NULL, /* MUSE_MODULE_EVENT_DEBUG_INFO_DUMP */
};
