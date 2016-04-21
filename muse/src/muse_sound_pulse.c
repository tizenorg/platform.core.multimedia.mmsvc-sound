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
#include "muse_sound_tone.h"
#include <muse_core.h>
#include <muse_core_ipc.h>
#include <dlog.h>

#include <math.h>

#ifdef USE_PULSE

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MUSED_SOUND_PULSE"

static void _stream_drain_complete(pa_stream* s, int success, void *userdata);
static void _context_drain_complete(pa_context *c, void *userdata);

/* ------------------------------------------------- */
//#define DEBUG_TONE

#define CHECK_DEAD_GOTO(h, rerror, label)                               \
    do {                                                                \
        if (!(h)->s || !PA_CONTEXT_IS_GOOD(pa_context_get_state((h)->c)) || \
            !(h)->s || !PA_STREAM_IS_GOOD(pa_stream_get_state((h)->s))) { \
            if (((h)->c && pa_context_get_state((h)->c) == PA_CONTEXT_FAILED) || \
                ((h)->s && pa_stream_get_state((h)->s) == PA_STREAM_FAILED)) { \
                if (rerror)                                             \
                    *(rerror) = pa_context_errno((h)->c);         \
            } else                                                      \
                if (rerror)                                             \
                    *(rerror) = PA_ERR_BADSTATE;                        \
            goto label;                                                 \
        }                                                               \
    } while(false);

#define CHECK_SUCCESS_GOTO(h, rerror, expression, label)        \
    do {                                                        \
        if (!(expression)) {                                    \
            if (rerror)                                         \
                *(rerror) = pa_context_errno((h)->c);     \
            goto label;                                         \
        }                                                       \
    } while(false);

#define CHECK_VALIDITY_RETURN_ANY(rerror, expression, error, ret)       \
    do {                                                                \
        if (!(expression)) {                                            \
            if (rerror)                                                 \
                *(rerror) = error;                                      \
            return (ret);                                               \
        }                                                               \
    } while(false);

static int _simple_write(muse_sound_handle_s *h, const void *data, size_t length, int *rerror)
{
	//pa_assert(h);

	//CHECK_VALIDITY_RETURN_ANY(rerror, data, PA_ERR_INVALID, -1);
	//CHECK_VALIDITY_RETURN_ANY(rerror, length > 0, PA_ERR_INVALID, -1);

	pa_threaded_mainloop_lock(h->m);

	CHECK_DEAD_GOTO(h, rerror, unlock_and_fail);

	while (length > 0) {
		size_t l;
		int r;

		while (!(l = pa_stream_writable_size(h->s))) {
			pa_threaded_mainloop_wait(h->m);
			CHECK_DEAD_GOTO(h, rerror, unlock_and_fail);
		}

		CHECK_SUCCESS_GOTO(h, rerror, l != (size_t) -1, unlock_and_fail);

		if (l > length)
			l = length;

		r = pa_stream_write(h->s, data, l, NULL, 0LL, PA_SEEK_RELATIVE);
		CHECK_SUCCESS_GOTO(h, rerror, r >= 0, unlock_and_fail);
#ifdef DEBUG_TONE
		LOGE("write(%d) ret=%d", l, r);
#endif

		data = (const uint8_t*) data + l;
		length -= l;
	}

	pa_threaded_mainloop_unlock(h->m);
	return 0;

unlock_and_fail:
	pa_threaded_mainloop_unlock(h->m);
	return -1;
}

static gpointer _tone_thread_func(gpointer userdata)
{
	muse_sound_handle_s *h = (muse_sound_handle_s *)userdata;
	pa_operation *o;

	char *ptoneBuf = NULL;
	int prePlayingTime = 0;
	int duration = 0;
	int playingTime = 0;
	int toneSize = 0;
	int CurWaveIndex = 0;
	int numWave = 0;
	int waveRestPlayTime = 0;
	int CurIndex = 0;
	int CurArrayPlayCnt = 0;
	int is_cycle_ended = FALSE;
	double sample = 0;

	while (1) {
		TONE _TONE = tone_get(h->tone, CurIndex); /* Pop one of Tone Set */
		if (tone_get_wave_cnt_playing_time(h->duration, _TONE, &numWave, &waveRestPlayTime) != 0) {
			LOGE("tone_get_wave_cnt_playing_time return value error");
			goto exit;
		}

		if (tone_get_cur_index(_TONE, &CurArrayPlayCnt, &CurIndex) != 0) {
			LOGE("tone_get_cur_index return value error");
			goto exit;
		}

		LOGE("Tone[%d], Index[%d], TotalPlayTime(%d ms), playing_time: %5d, numWave = %3d, [%0.f, %0.f, %0.f]",
				h->tone, CurIndex, h->duration, _TONE.playingTime, numWave, _TONE.low_frequency, _TONE.middle_frequency, _TONE.high_frequency);

		if (_TONE.low_frequency == -1) { /* skip frequency which's value is -1*/
			if ((_TONE.loopCnt > 0) && (CurArrayPlayCnt == _TONE.loopCnt)) {
				is_cycle_ended = TRUE;
				LOGE("[is_cycle_ended]");
			}
			CurIndex = _TONE.loopIndx;
			continue;
		}

		/* Write pcm data */
		for (CurWaveIndex = 0; CurWaveIndex < numWave+1; CurWaveIndex++) {
			if (CurWaveIndex == numWave) { /* play the last tone set*/
				playingTime = waveRestPlayTime;
			} else {
				playingTime = MAX_DURATION;
			}
			duration = playingTime;

			if (playingTime == 0)
				break;

			if (prePlayingTime + playingTime > h->duration && h->duration != -1) {
				playingTime = h->duration - prePlayingTime;
			}

			/* Create Tone & Write */
			ptoneBuf = tone_create(&sample, _TONE, 1.0, &toneSize, is_cycle_ended);
			if (ptoneBuf == NULL) {
				LOGE("Tone Buffer is not allocated");
				goto exit;
			}
#ifdef DEBUG_TONE
			LOGE("[TONE] Play.....%dth %dms ----------------------------------- %d",
					CurWaveIndex, playingTime,  ((toneSize * playingTime / duration + 1) >> 1) << 1);
#endif
			_simple_write(h, ptoneBuf, ((toneSize * playingTime / duration + 1) >> 1) << 1, NULL);

			prePlayingTime += playingTime;
#ifdef DEBUG_TONE
			LOGE("    previous_sum: %d", prePlayingTime);
#endif
			g_free(ptoneBuf);
			ptoneBuf = NULL;

			/* check finish condition */
			if (prePlayingTime == h->duration || h->need_stop == TRUE) {
				LOGE("Finished.....on Total Playing Time : %d _TONE.playing_time: %d", prePlayingTime, _TONE.playingTime);

				o = pa_stream_drain(h->s, _stream_drain_complete, h);
				if (o)
					pa_operation_unref(o);

				LOGE("Finished.....quit loop");
				goto exit;
			}
#ifdef DEBUG_TONE
			LOGE("        [%d] CurIndex: %d previous_sum: %d", CurArrayPlayCnt, CurIndex, prePlayingTime);
#endif
		}
	}

exit :
	LOGE("Tone Thread Exit!!");
	return NULL;
}
/* ---------------------------------------------------------------------------------*/



/* Stream Callbacks */
static void _stream_state_callback(pa_stream *s, void *userdata)
{
	muse_sound_handle_s *h = (muse_sound_handle_s *)userdata;
	assert(s);

	LOGE("stream state callback : %p, %d", s, pa_stream_get_state(s));

	switch (pa_stream_get_state(s)) {
		case PA_STREAM_CREATING:
		case PA_STREAM_TERMINATED:
			break;
		case PA_STREAM_READY:
			if (IS_TONE(h->mode)) {
				LOGE("  Create Thread for TONE!");
				h->g_thread = g_thread_new("tone_thread", _tone_thread_func, h);
			}
			break;
		case PA_STREAM_FAILED:
		default:
			pa_threaded_mainloop_signal(h->m, 0);
			break;
	}
}

static void _stream_drain_complete(pa_stream* s, int success, void *userdata)
{
	pa_operation *o;
	muse_sound_handle_s *h = (muse_sound_handle_s *)userdata;

	LOGE("stream drain complete callback : %p, %d", s, success);

	if (!success) {
		LOGE("drain failed. s(%p), sucess(%d)", s, success);
		//pa_threaded_mainloop_signal(h->m, 0);
	}

	pa_stream_disconnect(h->s);
	pa_stream_unref(h->s);
	h->s = NULL;

	if (!(o = pa_context_drain(h->c, _context_drain_complete, h)))
		pa_context_disconnect(h->c);
	else {
		pa_operation_unref(o);
	}

	/* send EOS callback */
	LOGE("Send EOS to CALLBACK to client!!");
	muse_sound_msg_event(MUSE_SOUND_CB_EVENT, MUSE_SOUND_EVENT_TYPE_EOS, h->module);
}

static void _stream_moved_callback(pa_stream *s, void *userdata) {
	assert(s);
	LOGE("stream moved callback : %p", s);
}

static void _stream_underflow_callback(pa_stream *s, void *userdata) {
	muse_sound_handle_s *h = (muse_sound_handle_s *)userdata;
    assert(s);

	LOGE("stream underflow callback : %p, path(%s)", s, h->file_path);
}

static void _stream_buffer_attr_callback(pa_stream *s, void *userdata) {
	assert(s);

	LOGE("stream underflow callback : %p", s);
}

static void _stream_write_callback(pa_stream *s, size_t length, void *userdata)
{
	sf_count_t bytes = 0;
	void *data = NULL;
	size_t frame_size;
	pa_operation *o = NULL;
	muse_sound_handle_s *h = (muse_sound_handle_s *)userdata;

	if (!s || length <= 0) {
		LOGE("write error. stream(%p), length(%d)", s, length);
		return;
	}

	/* TONE */
	if (IS_TONE(h->mode)) {
#ifdef DEBUG_TONE
		LOGE("request length = %d", length);
#endif
		/* actual write will be done in tone thread */
		pa_threaded_mainloop_signal(h->m, 0);
		return;
	}

	/* WAVE */
	data = pa_xmalloc(length);

	frame_size = pa_frame_size(&h->spec);

	if ((bytes = sf_readf_short(h->sf, data, (sf_count_t)(length / frame_size))) > 0)
		bytes *= (sf_count_t)frame_size;

	LOGE("=== %lld / %d ===",bytes, length);

	if (bytes > 0)
		pa_stream_write(h->s, data, (size_t)bytes, pa_xfree, 0, PA_SEEK_RELATIVE);
	else
		pa_xfree(data);

	/* If No more data, drain stream */
	if (bytes < (sf_count_t)length) {
		LOGE("EOS!!!!! %lld/%d", bytes, length);

		pa_stream_set_write_callback(h->s, NULL, NULL);
		o = pa_stream_drain(h->s, _stream_drain_complete, h);
		if (o)
			pa_operation_unref(o);
	}
}

/* Context Callbacks */
static void _context_drain_complete(pa_context *c, void *userdata)
{
	 pa_context_disconnect(c);
}

static void _context_state_callback(pa_context *c, void *userdata) {
	muse_sound_handle_s *h = (muse_sound_handle_s *)userdata;
	assert(c);

	LOGE("context state callback : %p, %d", c, pa_context_get_state(c));

	switch (pa_context_get_state(c)) {
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			break;

		case PA_CONTEXT_READY: {
			//pa_stream_flags_t flags = 0;
			pa_stream_flags_t flags = PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE;
			pa_proplist *proplist = pa_proplist_new();

			if (h->stream_type)
				pa_proplist_sets(proplist, PA_PROP_MEDIA_ROLE, h->stream_type);
			if (h->stream_id != -1)
				pa_proplist_setf(proplist, PA_PROP_MEDIA_PARENT_ID, "%d", h->stream_id);

			/*
			pa_proplist_setf(proplist, PA_PROP_MEDIA_TIZEN_VOLUME_TYPE, "%d", param->vol_type);
			pa_proplist_setf(proplist, PA_PROP_MEDIA_TIZEN_GAIN_TYPE, "%d", param->vol_gain);
			pa_proplist_sets(proplist, PA_PROP_MEDIA_POLICY, policy);
			*/
			h->s = pa_stream_new_with_proplist(c, "MUSE", &h->spec, NULL, proplist);
			pa_proplist_free(proplist);
			if (!h->s) {
				LOGE("pa_stream_new failed. path(%s)", h->file_path);
				break;
			}

			pa_stream_set_state_callback(h->s, _stream_state_callback, h);
			pa_stream_set_write_callback(h->s, _stream_write_callback, h);

			pa_stream_set_moved_callback(h->s, _stream_moved_callback, h);
			pa_stream_set_underflow_callback(h->s, _stream_underflow_callback, h);
			pa_stream_set_buffer_attr_callback(h->s, _stream_buffer_attr_callback, h);

			pa_stream_connect_playback(h->s, NULL, NULL, flags, NULL, NULL);
			break;
		}

		case PA_CONTEXT_TERMINATED:
		case PA_CONTEXT_FAILED:
			/* terminate is called after drain.*/
			if (IS_TONE(h->mode))
				pa_threaded_mainloop_signal(h->m, 0);
			break;

		default:
			break;
	}
}

static int _pa_connect_start_play(muse_sound_handle_s *h)
{
	if (!(h->m = pa_threaded_mainloop_new())) {
		LOGE("mainloop create failed");
		goto error;
	}

	if (!(h->c = pa_context_new(pa_threaded_mainloop_get_api(h->m), NULL))) {
		LOGE("context create failed");
		goto error;
	}

	pa_context_set_state_callback(h->c, _context_state_callback, h);

	pa_threaded_mainloop_lock(h->m);

	if (pa_threaded_mainloop_start(h->m) < 0) {
		LOGE("mainloop start failed");
		goto error;
	}

	if (pa_context_connect(h->c, NULL, 0, NULL) < 0) {
		LOGE("context connect failed");
		goto error;
	}

	pa_threaded_mainloop_unlock(h->m);

	return 0;

error:
	if (h->c) {
		pa_context_unref(h->c);
		h->c = NULL;
	}
	if (h->m) {
		pa_threaded_mainloop_free(h->m);
		h->m = NULL;
	}

	return -1;
}

static int _pa_stop_disconnect(muse_sound_handle_s *h)
{
	if (h->m) {
		pa_threaded_mainloop_lock(h->m);

		if (h->s) {
			pa_stream_disconnect(h->s);
			pa_stream_unref(h->s);
			h->s = NULL;
		}

		if (h->c) {
			pa_context_unref(h->c);
			h->c = NULL;
		}

		pa_threaded_mainloop_unlock(h->m);

		if (h->m) {
			pa_threaded_mainloop_free(h->m);
			h->m = NULL;
		}
	}
	return 0;
}

static int _prepare_sound(muse_sound_handle_s *h)
{
	if (h->mode & MODE_WAV) {
		memset(&h->si, 0, sizeof(SF_INFO));

		h->sf = sf_open(h->file_path, SFM_READ, &h->si);
		if (!h->sf) {
			LOGE("sf_open error. path(%s)", h->file_path);
			return -1;
		}

		h->spec.rate = h->si.samplerate;
		h->spec.channels = h->si.channels;
		h->spec.format = PA_SAMPLE_S16LE;

		LOGE("SF_INFO : frames = %lld, samplerate = %d, channels = %d, format = 0x%X, sections = %d, seekable = %d",
				h->si.frames, h->si.samplerate, h->si.channels, h->si.format, h->si.sections, h->si.seekable);

	} else if (IS_TONE(h->mode)) {
		/* FIXED SETTING */
		h->spec.rate = SAMPLERATE;
		h->spec.channels = CHANNELS;
		h->spec.format = PA_SAMPLE_S16LE;
	}
	return 0;
}

static void _unprepare_sound(muse_sound_handle_s *h)
{
	if (h->sf) {
		sf_close(h->sf);
		h->sf = NULL;
	}
}

int pulse_sound_start(void *handle)
{
	muse_sound_handle_s *h = (muse_sound_handle_s *)handle;
	int ret = -1;

	if (_prepare_sound(h) == 0) {
		ret = _pa_connect_start_play(h);
	}

	return ret;
}

int pulse_sound_stop(void *handle)
{
	muse_sound_handle_s *h = (muse_sound_handle_s *)handle;
	int ret = 0;

	if (IS_TONE(h->mode)) {
		h->need_stop = TRUE;
		g_thread_join(h->g_thread);
	}

	ret = _pa_stop_disconnect(h);

	_unprepare_sound(h);

	return ret;
}

#endif
