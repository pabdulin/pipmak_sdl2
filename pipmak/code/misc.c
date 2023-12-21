/*
 
 misc.c, part of the Pipmak Game Engine
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

/* $Id: misc.c 209 2008-10-19 09:32:27Z cwalther $ */

#include "misc.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "SDL.h"
#include "physfs.h"

#include "terminal.h"
#include "glstate.h"
#include "config.h"
#include "audio.h"


extern lua_State *L;
extern struct MouseModeStackEntry *topMouseMode;
extern int mouseX, mouseY, mouseWarpStartX, mouseWarpStartY;
extern Uint32 thisRedrawTime, mouseWarpEndTime;
extern GLfloat azimuth, elevation;
extern GLfloat verticalFOV;
extern SDL_Window *sdl2Window;
extern SDL_Rect screenSize;
extern CNode *backgroundCNode, *thisCNode;


typedef struct {
	PHYSFS_file *handle;
	char buffer[1024];
} physfsLuaChunkreaderState;

static const char * physfsLuaChunkreader(lua_State *L, void *data, size_t *size) {
	physfsLuaChunkreaderState *state = (physfsLuaChunkreaderState *)data;
	if (PHYSFS_eof(state->handle)) return NULL;
	*size = (size_t)PHYSFS_read(state->handle, state->buffer, 1, 1024);
	if (*size <= 0) return NULL;
	else return state->buffer;
}

int loadLuaChunkFromPhysfs(lua_State *L, const char *filename) {
	physfsLuaChunkreaderState state;
	int r;
	state.handle = PHYSFS_openRead(filename);
	if (state.handle == NULL) {
		lua_pushfstring(L, "\"%s\": %s", filename, PHYSFS_getLastError());
		return -1;
	}
	lua_pushfstring(L, "@%s", filename);
	r = lua_load(L, physfsLuaChunkreader, &state, lua_tostring(L, -1));
	lua_remove(L, -2);
	PHYSFS_close(state.handle);
	return r;
}

/*
 * Load and run a Lua chunk from a physfs file. Returns 1 on success.
 * Returns 0 and prints an error message to the pipmak terminal in case
 * of file loading, Lua compilation, or Lua runtime errors.
 */

int runLuaChunkFromPhysfs(lua_State *L, const char *filename) {
	if (loadLuaChunkFromPhysfs(L, filename) != 0) {
		terminalPrintf("Error loading Lua file %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return 0;
	}
	else if (pcallWithBacktrace(L, 0, 0, thisCNode) != 0) {
		terminalPrintf("Error running Lua file %s", lua_tostring(L, -1));
		lua_pop(L, 1);
		return 0;
	}
	else {
		return 1;
	}
}

/*
 * Replacement for lua_pcall() that appends a backtrace to error messages.
 * Taken with minor modifications from the interactive Lua interpreter (lua.c).
 */
int pcallWithBacktrace(lua_State *L, int nargs, int nresults, CNode *node) {
	int status;
	int base = lua_gettop(L) - nargs;  /* function index */
	CNode *savedNode = thisCNode;
	thisCNode = node;
	lua_pushliteral(L, "_TRACEBACK");
	lua_rawget(L, LUA_GLOBALSINDEX);  /* get traceback function (from debug library) */
	lua_insert(L, base);  /* put it under chunk and args */
	status = lua_pcall(L, nargs, nresults, base);
	lua_remove(L, base);  /* remove traceback function */
	thisCNode = savedNode;
	return status;
}

int strEndsWith(const char *str, const char *end) {
	const char *p = str;
	const char *q = end;
	if (*p == '\0' || *q == '\0') return 0;
	while (*p != '\0') p++;
	while (*q != '\0') q++;
	while (p >= str && q >= end && *p == *q) {
		p--;
		q--;
	}
	return (q < end);
}

char *directoryFromPath(const char *path) {
	char *r, *s, *t;
	const char *d = PHYSFS_getDirSeparator();
	r = malloc(strlen(path)+1);
	if (r == NULL) return NULL;
	strcpy(r, path);
	t = r-1;
	do {
		s = t;
		t = strstr(s+1, d);
	} while (t != NULL);
	if (s < r) {
		free(r);
		return NULL;
	}
	else {
		*s = '\0';
		return r;
	}
}

float smoothCubic(float x) {
	if (x <= 0) return 0;
	else if (x >= 1) return 1;
	else return (-2*x + 3)*x*x;
}

/*
 * Perform Lua's string.gsub() pattern substitution on line <line> of file
 * <path> (as determined by pipmak_internal.whereami()). <replacement> is a
 * printf-style format string into which the subsequent arguments are
 * substituted before it is fed to gsub(). Returns 1 on success, returns 0 and
 * prints error messages to the Pipmak terminal on failure.
 */
int updateFile(const char *path, int line, const char *pattern, const char *replacement, ...) {
	char *buffer = NULL, *bp, *linestart;
	int currentline, ok = 0;
	PHYSFS_file *file;
	PHYSFS_sint64 length = 0;
	va_list ap;
	va_start(ap, replacement);
	/*determine whether we can write to the project*/
	if (!PHYSFS_setWriteDir(PHYSFS_getRealDir(path))) {
		terminalPrintf("This project is not writable: %s", PHYSFS_getLastError());
		return 0;
	}
	/*read the whole file into memory because opening it for writing will discard the previous contents*/
	file = PHYSFS_openRead(path);
	if (file != NULL) {
		length = PHYSFS_fileLength(file);
		if (length > 0) {
			buffer = malloc((size_t)length);
			if (buffer != NULL) {
				if (PHYSFS_read(file, buffer, 1, (PHYSFS_uint32)length) == length) {
					ok = 1;
				}
			}
		}
		PHYSFS_close(file);
	}
	/*process it line by line*/
	if (!ok) {
		terminalPrintf("Error reading file %s: %s", path, PHYSFS_getLastError());
	}
	else {
		ok = 0;
		file = PHYSFS_openWrite(path);
		if (file == NULL) {
			terminalPrintf("Error writing to file %s: %s", path, PHYSFS_getLastError());
		}
		else {
			currentline = 1;
			bp = linestart = buffer;
			while (bp < buffer + length) {
				if (*bp == '\n' || bp == buffer + length - 1) { /*conveniently, Lua only considers LFs when counting lines*/
					if (currentline == line) {
						lua_pushliteral(L, "string");
						lua_rawget(L, LUA_GLOBALSINDEX);
						lua_pushliteral(L, "gsub");
						lua_rawget(L, -2);
						lua_pushlstring(L, linestart, (PHYSFS_uint32)(bp - linestart + 1));
						lua_pushstring(L, pattern);
						lua_pushvfstring(L, replacement, ap);
						lua_pushnumber(L, 1.0);
						if (pcallWithBacktrace(L, 4, 2, NULL) == 0) {
							if (lua_tonumber(L, -1) == 1.0) {
								ok = 1;
							}
							else {
								terminalPrintf("Line %d of file %s is not of the expected form, I refuse to overwrite it.", line, path);
							}
							lua_pop(L, 1); /*number*/
						}
						else {
							terminalPrintf("Lua error updating file %s: %s\nThis is a bug in Pipmak, please report.", path, lua_tostring(L, -1));
							lua_pop(L, 1); /*error message*/
							lua_pushlstring(L, linestart, bp - linestart + 1);
						}
						PHYSFS_write(file, lua_tostring(L, -1), 1, (PHYSFS_uint32)lua_strlen(L, -1));
						lua_pop(L, 2); /*line, string table*/
					}
					else {
						PHYSFS_write(file, linestart, 1, bp - linestart + 1);
					}
					currentline++;
					linestart = bp + 1;
				}
				bp++;
			}
			if (currentline < line) terminalPrintf("Error updating line %d of file %s: file has only %d lines", line, path, currentline);
			PHYSFS_close(file);
		}
	}
	if (buffer != NULL) free(buffer);
	va_end(ap);
	return ok;
}

static void handleMouseModeChange(enum MouseMode oldMode, enum MouseMode newMode) {
	if (newMode != oldMode) {
		if (newMode == MOUSE_MODE_DIRECT) {
			SDL_GetRelativeMouseState(NULL, NULL); /*reset accumulated relative motion*/
			mouseWarpStartX = mouseX;
			mouseWarpStartY = mouseY;
			mouseWarpEndTime = thisRedrawTime + MOUSE_WARP_DURATION;
		}
		else { /*newMode == MOUSE_MODE_JOYSTICK*/
			SDL_WarpMouseInWindow(sdl2Window, mouseX, mouseY);
		}
	}
}

void setStandardMouseMode(enum MouseMode newMode) {
	struct MouseModeStackEntry *entry = topMouseMode;
	while (entry->next != NULL) entry = entry->next;
	if (entry == topMouseMode) handleMouseModeChange(entry->mode, newMode);
	entry->mode = newMode;
}

MouseModeToken pushMouseMode(enum MouseMode mode) {
	MouseModeToken entry;
	entry = malloc(sizeof(struct MouseModeStackEntry));
	if (entry != NULL) {
		entry->next = topMouseMode;
		entry->mode = mode;
		handleMouseModeChange(topMouseMode->mode, mode);
		topMouseMode = entry;
	}
	return entry;
}

int popMouseMode(MouseModeToken token) {
	MouseModeToken prev;
	if (token == NULL) return 0;
	if (token == topMouseMode) {
		if (topMouseMode->next == NULL) return 0; /*can't pop the last entry*/
		topMouseMode = topMouseMode->next;
		handleMouseModeChange(token->mode, topMouseMode->mode);
	}
	else {
		prev = topMouseMode;
		while (prev != NULL && prev->next != token) prev = prev->next;
		if (prev == NULL) return 0;
		prev->next = token->next;
	}
	free(token);
	return 1;
}

void trimMouseModeStack() {
	struct MouseModeStackEntry *entry = topMouseMode, *next;
	enum MouseMode oldMode = topMouseMode->mode;
	while (entry->next != NULL) {
		next = entry->next;
		free(entry);
		entry = next;
	}
	topMouseMode = entry;
	handleMouseModeChange(oldMode, topMouseMode->mode);
}

void absolutizePath(lua_State *L, int relativeToParent) {
	const char *s;
	char *t, *p;
	const char *parts[4];
	const char **part;
	int popcount;
	s = lua_tostring(L, -1);
	if (*s == '/') { /*absolute path*/
		parts[0] = s+1;
		parts[1] = NULL;
		popcount = 1; /*argument*/
		t = malloc(lua_strlen(L, -1));
	}
	else if (thisCNode != NULL) { /*relative path*/
		lua_rawgeti(L, LUA_REGISTRYINDEX, thisCNode->noderef);
		lua_pushliteral(L, "path"); lua_rawget(L, -2);
		parts[0] = lua_tostring(L, -1);
		parts[1] = relativeToParent ? ".." : "";
		parts[2] = lua_tostring(L, -3);
		parts[3] = NULL;
		popcount = 3; /*node path, node, argument*/
		t = malloc(lua_strlen(L, -1) + 4 + lua_strlen(L, -3) + 1);
	}
	else { /*in main.lua, absolute path*/
		parts[0] = s;
		parts[1] = NULL;
		popcount = 1; /*argument*/
		t = malloc(lua_strlen(L, -1) + 1);
	}
	if (t == NULL) luaL_error(L, "out of memory");
	p = t;
	for (part = parts; *part != NULL; part++) {
		s = *part;
		while (*s == '/') s++;
		do {
			if (*s == '.' && *(s+1) == '.' && (*(s+2) == '/' || *(s+2) == '\0')) {
				if (p > t) {
					do p--; while (p > t && *(p-1) != '/');
				}
				s += 2;
			}
			else {
				while (*s != '\0' && *s != '/') {
					*p++ = *s++;
				}
				if (p > t && *(p-1) != '/') *p++ = '/';
			}
			while (*s == '/') s++;
		} while (*s != '\0');
	}
	if (p > t) {
		p--;
		assert(*p == '/');
	}
	*p = '\0';
	lua_pop(L, popcount);
	lua_pushlstring(L, t, p-t);
	free(t);
}

void xYtoAzEl(int x, int y, float *az, float *el) {
	SDL_Rect *screen = &screenSize;
	x -= screen->w/2;
	y -= screen->h/2;
	if (x == 0 && y == 0) {
		*az = azimuth;
		*el = elevation;
	}
	else {
		float d, sinaz, cosaz, sinel, cosel, tx, ty, tz;
		d = screen->h/(2*tanf(verticalFOV*(float)M_PI/180/2));
		sinaz = sinf(azimuth*(float)M_PI/180);
		cosaz = cosf(azimuth*(float)M_PI/180);
		sinel = sinf(elevation*(float)M_PI/180);
		cosel = cosf(elevation*(float)M_PI/180);
		tx = d*cosel*sinaz + x*cosaz + y*sinel*sinaz;
		ty = d*sinel - y*cosel;
		tz = -d*cosel*cosaz + x*sinaz - y*sinel*cosaz;
		
		*az = atan2f(-tx, tz)*180/(float)M_PI + 180;
		*el = 180/(float)M_PI*atan2f(ty, sqrtf(tx*tx + tz*tz));
	}
}

void quit(int status) {
	CNode *node, *nextnode;
	cleanupGL();
	node = backgroundCNode;
	while (node != NULL) {
		nextnode = node->next;
		freeCNode(node);
		node = nextnode;
	}
	if (L != NULL) lua_close(L);
	audioQuit();
	quitTerminal();
	PHYSFS_deinit();
	SDL_Quit();
	if (topMouseMode != NULL) {
		trimMouseModeStack();
		free(topMouseMode);
	}
	exit(status);
}
