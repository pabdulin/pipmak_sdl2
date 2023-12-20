/*
 
 audio.h, part of the Pipmak Game Engine
 Copyright (c) 2006 Christian Walther
 
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

/* $Id: audio.h 204 2008-10-07 18:00:01Z cwalther $ */

#ifndef AUDIO_H_SEEN
#define AUDIO_H_SEEN

#if defined(MACOSX)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <Vorbis/vorbisfile.h>
#elif defined(WIN32)
#include <al.h>
#include <alc.h>
#include <vorbis/vorbisfile.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <vorbis/vorbisfile.h>
#endif

#include "SDL.h"
#include "SDL_thread.h"
#include "lua.h"
#include "lauxlib.h"


extern const luaL_reg pipmakAudioFuncs[];
extern const luaL_reg soundFuncs[];

typedef struct Sound {
	enum { SOUND_STATIC, SOUND_STREAMING } type;
	char *path;
	ALuint source;
	ALuint buffers[2];
	double duration;
	unsigned loop:1, loaderShouldExit:1;
	SDL_Thread *loaderThread;
	SDL_mutex *mutex; /*used by cond, held by the loader thread while it is working*/
	SDL_cond *cond; /*used by the main thread to wake up the loader thread, and by the loader thread to signal the main thread that it is done*/
	OggVorbis_File *vorbisFile;
} Sound;

int audioInit();
void audioQuit();
void updateListenerOrientation();

#endif /*AUDIO_H_SEEN*/
