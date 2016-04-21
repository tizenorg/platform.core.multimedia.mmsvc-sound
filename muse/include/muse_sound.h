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

#ifndef __MUSE_SOUND_H__
#define __MUSE_SOUND_H__

#ifdef _cplusplus
extern "C" {
#endif /* _cplusplus */

/**
  * @file muse_sound.h
  * @brief This file contains the muse SOUND API for framework, related structures and enumerations.
  */

#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Enumeration for the muse SOUND apis.
 */
typedef enum {
	MUSE_SOUND_API_START, //0
	MUSE_SOUND_API_DUMMY,
	MUSE_SOUND_API_STOP,
	MUSE_SOUND_API_MAX
} muse_sound_api_e;

/**
 * @brief Enumeration for the muse SOUND events.
 */
typedef enum {
	MUSE_SOUND_EVENT_TYPE_EOS,
	MUSE_SOUND_EVENT_TYPE_NUM
} muse_sound_event_e;

/**
 * @brief The structure type for muse camera errors.
 */
typedef enum {
	MUSE_SOUND_ERROR_INVALID = -1,
	MUSE_SOUND_ERROR_NONE = 1,
} muse_sound_error_e;

/**
 * @brief Definition for the callback event id.
 */
#define MUSE_SOUND_CB_EVENT	MUSE_SOUND_API_MAX + 1

/**
 * @brief Definition for the max message length.
 */
#define MUSE_SOUND_MSG_MAX_LENGTH		512

/**
 * @brief Definition for the wait time of the ipc callback.
 */
#define SOUND_CALLBACK_TIME_OUT 3


#ifdef __cplusplus
}
#endif /* _cplusplus */

#endif /* __MUSE_SOUND_H__ */
