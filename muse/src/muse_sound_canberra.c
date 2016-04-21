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

#ifdef USE_CANBERRA
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MUSED_SOUND_CANBERRA"

static void _callback(ca_context *c, uint32_t id, int error, void *userdata)
{
	muse_sound_handle_s * h = (muse_sound_handle_s *)userdata;
	LOGE("callback called for id %u, error '%s', userdata=%p\n", id, ca_strerror(error), userdata);

	if (error != CA_ERROR_CANCELED)
		use_sound_msg_event(MUSE_SOUND_CB_EVENT, MUSE_SOUND_EVENT_TYPE_EOS, h->module);
}

int canberra_sound_start(void *handle)
{
	muse_sound_handle_s *h = (muse_sound_handle_s *)handle;
	int ret;
	ca_proplist *p = NULL;

	/* Context create & open */
	ret = ca_context_create(&h->c);
	LOGE("create: %s\n", ca_strerror(ret));
	if (ret < 0)
		goto error;

	/* Prepare proplist */
	ret = ca_proplist_create(&p);
	if (ret < 0)
		goto error;

	ca_proplist_sets(p, CA_PROP_EVENT_ID, "muse wav");
	ca_proplist_sets(p, CA_PROP_MEDIA_FILENAME, h->file_path);
	ca_proplist_sets(p, CA_PROP_MEDIA_NAME, "sound from muse");

	/* Play using proplist */
	ret = ca_context_play_full(h->c, 0, p, _callback, h);
	LOGE("play_full: %s\n", ca_strerror(ret));

	/* Clean up */
	ca_proplist_destroy(p);

	return 0;
error:
	if (p)
		ca_proplist_destroy(p);
	if (h->c) {
		ca_context_destroy(h->c);
		h->c = NULL;
	}
	return -1;
}

int canberra_sound_stop(void *handle)
{
	muse_sound_handle_s *h = (muse_sound_handle_s *)handle;
	int ret;

	if (!h->c)
		return -1;

	/* Stop one sound */
	ret = ca_context_cancel(h->c, 0);
	LOGE("cancel: %s\n", ca_strerror(ret));

	ret = ca_context_destroy(h->c);
	LOGE("destroy: %s\n", ca_strerror(ret));

	h->c = NULL;
	return 0;
}

#endif
