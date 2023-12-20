/*
 
 audio.c, part of the Pipmak Game Engine
 Copyright (c) 2006-2011 Christian Walther
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 */

/* $Id: audio.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "SDL_opengl.h"
#include "physfs.h"

#include "terminal.h"
#include "misc.h"


extern GLfloat azimuth, elevation;

static ALCdevice *audioDevice;
static ALCcontext *audioContext;


static size_t physfsOvRead(void *ptr, size_t size, size_t nmemb, void *datasource) {
	PHYSFS_sint64 result = PHYSFS_read((PHYSFS_file *)datasource, ptr, (PHYSFS_uint32)size, (PHYSFS_uint32)nmemb);
	if (result < 0) return 0;
	else return (size_t)result;
}

static int physfsOvSeek(void *datasource, ogg_int64_t offset, int whence) {
	PHYSFS_uint64 start = 0;
	if (whence == SEEK_CUR) start = PHYSFS_tell((PHYSFS_file *)datasource);
	else if (whence == SEEK_END) start = PHYSFS_fileLength((PHYSFS_file *)datasource);
	return (PHYSFS_seek((PHYSFS_file *)datasource, start + offset)) ? 0 : -1;
}

static int physfsOvClose(void *datasource) {
	PHYSFS_close((PHYSFS_file *)datasource);
	return 0;
}

static long physfsOvTell(void *datasource) {
	return (long)PHYSFS_tell((PHYSFS_file *)datasource);
}

static ov_callbacks physfsOvCallbacks = {
	physfsOvRead,
	physfsOvSeek,
	physfsOvClose,
	physfsOvTell
};


int audioInit() {
	audioDevice = alcOpenDevice(NULL);
	if (audioDevice == NULL) return 0;
	audioContext = alcCreateContext(audioDevice, NULL);
	if (audioContext == NULL) {
		alcCloseDevice(audioDevice);
		return 0;
	}
	alcMakeContextCurrent(audioContext);
	alDistanceModel(AL_NONE);
	return 1;
}

void audioQuit() {
	alcMakeContextCurrent(NULL);
	alcDestroyContext(audioContext);
	alcCloseDevice(audioDevice);
}

void updateListenerOrientation() {
	ALfloat ori[6];
	float az = azimuth*(float)M_PI/180, el = elevation*(float)M_PI/180;
	ori[0] = cosf(el)*sinf(az);
	ori[1] = sinf(el);
	ori[2] = -cosf(el)*cosf(az);
	ori[3] = 0;
	ori[4] = 1;
	ori[5] = 0;
	alListenerfv(AL_ORIENTATION, ori);
}


static int readSoundData(Sound *sound, ALvoid *data, ALsizei size) {
	int section, readBytes, result;
	readBytes = 0;
	while ((ALsizei)readBytes < size) {
		result = ov_read(sound->vorbisFile, (char *)data + readBytes, size - readBytes, (SDL_BYTEORDER == SDL_BIG_ENDIAN) ? 1 : 0, 2, 1, &section);
		if (result == 0) {
			if (sound->loop) ov_raw_seek_lap(sound->vorbisFile, 0);
			else break;
		}
		if (result < 0 && result != OV_HOLE) break;
		readBytes += result;
	}
	return readBytes;
}

static int staticLoader(void *sound) {
	int bufferSize;
	vorbis_info *info;
	ALvoid *data;
	
	info = ov_info(((Sound *)sound)->vorbisFile, -1);
	bufferSize = 2*info->channels*(int)ov_pcm_total(((Sound *)sound)->vorbisFile, -1);
	data = malloc(bufferSize);
	if (data == NULL) return 1;
	bufferSize = readSoundData((Sound *)sound, data, bufferSize);
	alBufferData(((Sound *)sound)->buffers[0], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data, bufferSize, info->rate);
	free(data);
	ov_clear(((Sound *)sound)->vorbisFile);
	free(((Sound *)sound)->vorbisFile);
	((Sound *)sound)->vorbisFile = NULL;
	alSourcei(((Sound *)sound)->source, AL_BUFFER, ((Sound *)sound)->buffers[0]);
	return 0;
}

static int streamingLoader(void *sound) {
#define STREAMING_BUFFER_SIZE (1*2*2*44100)
	ALint i;
	ALuint buffer;
	vorbis_info *info;
	ALvoid *data;
	int readBytes;
	int testRest; /*what to do after the job is done: 0 = go to sleep right away, 1 = rest for a while if playing, else sleep*/
	
	data = malloc(STREAMING_BUFFER_SIZE);
	if (data == NULL) return 1;
	SDL_LockMutex(((Sound *)sound)->mutex);
	info = ov_info(((Sound *)sound)->vorbisFile, -1);
	
	do {
		testRest = 1;
		alGetSourcei(((Sound *)sound)->source, AL_BUFFERS_QUEUED, &i);
		if (i == 0) { /*fill and queue initial buffers*/
			readBytes = readSoundData((Sound *)sound, data, STREAMING_BUFFER_SIZE);
			alBufferData(((Sound *)sound)->buffers[0], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data, readBytes, info->rate);
			readBytes = readSoundData((Sound *)sound, data, STREAMING_BUFFER_SIZE);
			alBufferData(((Sound *)sound)->buffers[1], (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data, readBytes, info->rate);
			alSourceQueueBuffers(((Sound *)sound)->source, 2, ((Sound *)sound)->buffers);
		}
		else { /*refill processed buffers*/
			alGetSourcei(((Sound *)sound)->source, AL_BUFFERS_PROCESSED, &i);
			while (i-- != 0) {
				readBytes = readSoundData((Sound *)sound, data, STREAMING_BUFFER_SIZE);
				if (readBytes == 0) {
					testRest = 0;
					break;
				}
				else {
					alSourceUnqueueBuffers(((Sound *)sound)->source, 1, &buffer);
					alBufferData(buffer, (info->channels > 1) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data, readBytes, info->rate);
					alSourceQueueBuffers(((Sound *)sound)->source, 1, &buffer);
				}
			}
		}
		SDL_CondSignal(((Sound *)sound)->cond); /*tell the main thread we're done, in case it's waiting for us*/
		if (testRest) {
			alGetSourcei(((Sound *)sound)->source, AL_SOURCE_STATE, &i);
			testRest = (i == AL_PLAYING);
		}
		if (testRest) {
			SDL_CondWaitTimeout(((Sound *)sound)->cond, ((Sound *)sound)->mutex, 400);
		}
		else {
			SDL_CondWait(((Sound *)sound)->cond, ((Sound *)sound)->mutex);
		}
	} while (!(((Sound *)sound)->loaderShouldExit));
	
	SDL_UnlockMutex(((Sound *)sound)->mutex);
	free(data);
	return 0;
}


#if 0
#pragma mark pipmak functions
#endif

static int loadsoundLua(lua_State *L) {
	Sound *sound;
	PHYSFS_file *file;
	const char *s;
	
	luaL_checktype(L, 1, LUA_TSTRING);
	absolutizePath(L, 0);
	s = lua_tostring(L, 1);
	sound = lua_newuserdata(L, sizeof(Sound));
	luaL_getmetatable(L, "pipmak-sound");
	lua_setmetatable(L, -2);
	sound->type = SOUND_STATIC;
	sound->loop = 0;
	sound->loaderShouldExit = 0;
	sound->loaderThread = NULL;
	sound->vorbisFile = NULL;
	sound->path = malloc(strlen(s) + 1);
	if (sound->path == NULL) luaL_error(L, "out of memory");
	strcpy(sound->path, s);
	alGenSources(1, &sound->source);
	alGenBuffers(2, sound->buffers);
	sound->vorbisFile = malloc(sizeof(OggVorbis_File));
	if (sound->vorbisFile == NULL) luaL_error(L, "out of memory");
	if ((file = PHYSFS_openRead(s)) == NULL) {
		free(sound->vorbisFile);
		sound->vorbisFile = NULL;
		luaL_error(L, "Error loading sound \"%s\": %s", s, PHYSFS_getLastError());
	}
	if (ov_open_callbacks(file, sound->vorbisFile, NULL, 0, physfsOvCallbacks) < 0) {
		free(sound->vorbisFile);
		sound->vorbisFile = NULL;
		PHYSFS_close(file);
		luaL_error(L, "Error loading sound \"%s\": not an Ogg Vorbis file", s);
	}
	
	if (ov_streams(sound->vorbisFile) > 1) luaL_error(L, "Error loading sound \"%s\": Ogg files containing multiple logical bitstreams are not supported", s);
	
	sound->duration = ov_time_total(sound->vorbisFile, -1);
	if (sound->duration > 10.0) {
		sound->type = SOUND_STREAMING;
		sound->mutex = SDL_CreateMutex();
		if (sound->mutex == NULL) luaL_error(L, "out of memory");
		sound->cond = SDL_CreateCond();
		if (sound->cond == NULL) luaL_error(L, "out of memory");
		sound->loaderThread = SDL_CreateThread(streamingLoader, "streamingLoader", sound);
	}
	else {
		sound->loaderThread = SDL_CreateThread(staticLoader, "staticLoader", sound);
	}
	
	return 1;
}

const luaL_reg pipmakAudioFuncs[] = {
	{"loadsound", loadsoundLua},
	{NULL, NULL}
};


#if 0
#pragma mark Sound methods
#endif

static int soundTostringLua(lua_State *L) {
	Sound *s;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	lua_pushfstring(L, "sound \"%s\"", s->path);
	return 1;
}

static int soundCollectLua(lua_State *L) {
	Sound *s;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	
	if (s->type == SOUND_STREAMING) {
		s->loaderShouldExit = 1;
		SDL_CondSignal(s->cond);
		SDL_WaitThread(s->loaderThread, NULL);
		SDL_DestroyCond(s->cond);
		SDL_DestroyMutex(s->mutex);
	}
	else {
		if (s->loaderThread != NULL) SDL_WaitThread(s->loaderThread, NULL);
	}
	
	alDeleteSources(1, &s->source);
	alDeleteBuffers(2, s->buffers);
	if (s->path != NULL) free(s->path);
	if (s->vorbisFile != NULL) {
		ov_clear(s->vorbisFile);
		free(s->vorbisFile);
	}
	return 0;
}

static int soundPlayLua(lua_State *L) {
	Sound *s;
	ALint i;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	if (s->type == SOUND_STREAMING) {
		alGetSourcei(s->source, AL_SOURCE_STATE, &i);
		switch (i) {
			case AL_PLAYING:
				alSourceStop(s->source);
			case AL_STOPPED:
				ov_raw_seek(s->vorbisFile, 0);
				alSourcei(s->source, AL_BUFFER, AL_NONE); /*unqueue all buffers*/
			case AL_INITIAL:
			case AL_PAUSED:
				SDL_LockMutex(s->mutex);
				SDL_CondSignal(s->cond); /*wake up loader*/
				SDL_CondWait(s->cond, s->mutex); /*wait until loader has queued initial buffers (if necessary), it will have gone to sleep when this returns because the source isn't playing yet*/
				alSourcePlay(s->source);
				SDL_CondSignal(s->cond); /*wake up loader again (it will stay awake this time since the source is playing now)*/
				SDL_UnlockMutex(s->mutex);
				break;
		}
	}
	else {
		if (s->loaderThread != NULL) {
			SDL_WaitThread(s->loaderThread, NULL);
			s->loaderThread = NULL;
		}
		alSourcePlay(s->source);
	}
	return 0;
}

static int soundPauseLua(lua_State *L) {
	Sound *s;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	alSourcePause(s->source);
	return 0;
}

static int soundStopLua(lua_State *L) {
	Sound *s;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	alSourceStop(s->source);
	return 0;
}

static int soundLoopLua(lua_State *L) {
	Sound *s;
	ALint old;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	alGetSourcei(s->source, AL_LOOPING, &old);
	if (!lua_isnone(L, 2)) {
		if (s->type == SOUND_STATIC) alSourcei(s->source, AL_LOOPING, lua_toboolean(L, 2));
		else s->loop = lua_toboolean(L, 2);
	}
	lua_pushboolean(L, old);
	return 1;
}

static int soundVolumeLua(lua_State *L) {
	Sound *s;
	ALfloat old, new;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	new = (ALfloat)luaL_optnumber(L, 2, -2893.0);
	alGetSourcef(s->source, AL_GAIN, &old);
	if (new != -2893.0) {
		alSourcef(s->source, AL_GAIN, new);
	}
	lua_pushnumber(L, old);
	return 1;
}

static int soundPitchLua(lua_State *L) {
	Sound *s;
	ALfloat old, new;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	new = (ALfloat)luaL_optnumber(L, 2, -2893.0);
	alGetSourcef(s->source, AL_PITCH, &old);
	if (new != -2893.0) {
		alSourcef(s->source, AL_PITCH, new);
	}
	lua_pushnumber(L, old);
	return 1;
}

static int soundLocationLua(lua_State *L) {
	Sound *s;
	ALfloat x, y, z;
	float oldaz, oldel, newaz = 0, newel = 0;
	int choice;
	
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	if (lua_isnoneornil(L, 2)) {
		choice = 0;
	}
	else if (lua_isboolean(L, 2) && !lua_toboolean(L, 2)) {
		choice = 1;
	}
	else {
		newaz = (float)luaL_checknumber(L, 2)*(float)M_PI/180;
		newel = (float)luaL_checknumber(L, 3)*(float)M_PI/180;
		choice = 2;
	}
	
	alGetSource3f(s->source, AL_POSITION, &x, &y, &z);
	if (x == 0 && y == 0 && z == 0) {
		lua_pushboolean(L, 0);
		lua_pushnil(L);
	}
	else {
		oldaz = atan2f(x, -z)/(float)M_PI*180;
		if (oldaz < 0) oldaz += 360;
		oldel = atan2f(y, sqrtf(x*x+z*z))/(float)M_PI*180;
		lua_pushnumber(L, oldaz);
		lua_pushnumber(L, oldel);
	}
	
	if (choice == 1) {
		alSource3f(s->source, AL_POSITION, 0, 0, 0);
	}
	else if (choice == 2) {
		alSource3f(s->source, AL_POSITION, cosf(newel)*sinf(newaz), sinf(newel), -cosf(newel)*cosf(newaz));
	}
	return 2;
}

static int soundPlayingLua(lua_State *L) {
	Sound *s;
	ALint i;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	alGetSourcei(s->source, AL_SOURCE_STATE, &i);
	lua_pushboolean(L, (i == AL_PLAYING));
	return 1;
}

static int soundDurationLua(lua_State *L) {
	Sound *s;
	s = (Sound*)luaL_checkudata(L, 1, "pipmak-sound");
	if (s == NULL) luaL_typerror(L, 1, "sound");
	lua_pushnumber(L, s->duration);
	return 1;
}

const luaL_reg soundFuncs[] = {
	{"__tostring", soundTostringLua},
	{"__gc", soundCollectLua},
	{"play", soundPlayLua},
	{"pause", soundPauseLua},
	{"stop", soundStopLua},
	{"loop", soundLoopLua},
	{"volume", soundVolumeLua},
	{"pitch", soundPitchLua},
	{"location", soundLocationLua},
	{"playing", soundPlayingLua},
	{"duration", soundDurationLua},
	{NULL, NULL}
};

