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
#include "muse_sound.h"
#include "muse_sound_msg.h"
#include "muse_sound_internal.h"
#include <muse_core.h>
#include <muse_core_ipc.h>
#include <dlog.h>

#ifdef USE_OPENAL

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MUSED_SOUND_ALURE"

static void _eos_callback(void *user_data, ALuint source)
{
	muse_sound_handle_s * h = (muse_sound_handle_s *)user_data;
	LOGE("eos callback %p, %u\n", user_data, source);

	muse_sound_msg_event(MUSE_SOUND_CB_EVENT, MUSE_SOUND_EVENT_TYPE_EOS, h->module);
}

int alure_sound_start(muse_sound_handle_s * h)
{
	/* Initialize Device */
	if (!alureInitDevice(NULL, NULL)) {
		LOGE("Failed to open OpenAL device: %s\n", alureGetErrorString());
		return -1;
	}

	/* Generate a single source, */
	alGenSources(1, &h->source);

	/* Create an AL buffer from the given sound file. */
	h->buffer = alureCreateBufferFromFile(h->file_path);
	if (!h->buffer) {
		LOGE("Could not load %s: %s", h->file_path, alureGetErrorString());
		goto start_error;
	}

	/* attach the buffer to it and start playing. */
	alSourcei(h->source, AL_BUFFER, h->buffer);

	/* start playing */
	if (alurePlaySource(h->source, _eos_callback, h) == AL_FALSE) {
		LOGE("Failed to start source!");
		goto start_error;
	}

	/* Set update interval for auto update : this is needed for EOS callback */
	alureUpdateInterval(0.1);

	return 0;

start_error:
	alDeleteSources(1, &h->source);
	alureShutdownDevice();

	return -1;
}

int alure_sound_stop(muse_sound_handle_s * h)
{
	alureUpdateInterval(0);
	alureStopSource(h->source, FALSE);
	alDeleteSources(1, &h->source);
	alureShutdownDevice();

	return 0;
}

#endif