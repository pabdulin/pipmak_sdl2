/*
 
 misc.h, part of the Pipmak Game Engine
 Copyright (c) 2004-2008 Christian Walther
 
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

/* $Id: misc.h 209 2008-10-19 09:32:27Z cwalther $ */

#ifndef MISC_H_SEEN
#define MISC_H_SEEN

#include "lua.h"

#include "nodes.h"


enum TransitionState { TRANSITION_IDLE, TRANSITION_PENDING, TRANSITION_RUNNING };
enum TransitionEffect { TRANSITION_DISSOLVE, TRANSITION_ROTATE, TRANSITION_WIPE };
enum TransitionDirection { TRANSITION_LEFT, TRANSITION_RIGHT, TRANSITION_UP, TRANSITION_DOWN, TRANSITION_DEFAULT };

enum MouseMode { MOUSE_MODE_JOYSTICK, MOUSE_MODE_DIRECT, NUMBER_OF_MOUSE_MODES };
struct MouseModeStackEntry { struct MouseModeStackEntry *next; enum MouseMode mode; };
typedef struct MouseModeStackEntry * MouseModeToken;

enum DisruptiveInstruction { INSTR_NONE, INSTR_OPENSAVEDGAME, INSTR_OPENPROJECT };


int loadLuaChunkFromPhysfs(lua_State *L, const char *filename);
int runLuaChunkFromPhysfs(lua_State *L, const char *filename);
int pcallWithBacktrace(lua_State *L, int nargs, int nresults, CNode *node);
int strEndsWith(const char *str, const char *end);
char *directoryFromPath(const char *path);
float smoothCubic(float x);
int updateFile(const char *path, int line, const char *pattern, const char *replacement, ...);
void setStandardMouseMode(enum MouseMode newMode);
MouseModeToken pushMouseMode(enum MouseMode mode);
int popMouseMode(MouseModeToken token);
void trimMouseModeStack();
void absolutizePath(lua_State *L, int relativeToParent);
void xYtoAzEl(int x, int y, float *az, float *el);
void quit(int status);

#endif /*MISC_H_SEEN*/
