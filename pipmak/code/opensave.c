/*
 
 opensave.c, part of the Pipmak Game Engine
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

/* $Id: opensave.c 232 2013-12-30 20:29:58Z cwalther $ */

#include "opensave.h"

#include <assert.h>

#include "SDL.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "physfs.h"

#include "nodes.h"
#include "misc.h"
#include "terminal.h"
#include "glstate.h"
#include "audio.h"
#include "tools.h"
#include "platform.h"

int luaopen_pack(lua_State *L);
void luaopen_pipmak(lua_State *L);

extern CNode *backgroundCNode, *thisCNode, *touchedNode;
extern lua_State *L;
extern GLfloat azimuth, elevation;
extern GLfloat verticalFOV;
extern float joystickSpeed;
extern GLint glTextureFilter;
extern SDL_Surface *screen;
extern Image *standardCursor;
extern Image *cursors[NUMBER_OF_CURSORS];


static int luaPanicHandler(lua_State *L) {
	errorMessage("Lua panic: %s\nThe application will quit now.", lua_tostring(L, -1));
	/*FIXME: maybe do something more graceful*/
	return 0;
}

/*
 * Try to load a Pipmak project (file or package) given its path or the path
 * to its main.lua in platform-dependent format. On success, exits all current
 * nodes, returns 1, and leaves the start node path on top of the Lua stack.
 * Errors in main.lua are reported on the Pipmak terminal and handled by falling
 * back to defaults, they are not considered as failure. If the given path is
 * not a directory or a supported archive file, or if main.lua doesn't exist,
 * returns 0, and everything is left in its previous state. 
 * If filename is NULL, no main.lua is read and default properties are set.
 * In that case, always returns 1 and pushes the default start node path "0".
 */
int openProject(const char *filename) {
	int r;
	char *myfilename;
	const char *title;
	const char *startnodepath;
	char **pfslist, **pfslisti;
	
	if (filename != NULL && strEndsWith(filename, "main.lua")) {
		myfilename = directoryFromPath(filename);
	}
	else {
		myfilename = (char *)filename;
	}
		
	pfslist = PHYSFS_getSearchPath();
	for (pfslisti = pfslist + 1; *pfslisti != NULL; pfslisti++) { /*remove all but the resources (first entry)*/
		PHYSFS_removeFromSearchPath(*pfslisti);
	}
	
	if (myfilename != NULL) {
		if (PHYSFS_addToSearchPath(myfilename, 1)) { /*test whether it is a proper project*/
			r = PHYSFS_exists("main.lua");
			PHYSFS_removeFromSearchPath(myfilename);
		}
		else r = 0;
	}
	else r = 1;
	
	for (pfslisti = pfslist + 1; *pfslisti != NULL; pfslisti++) { /*restore previous search path: to clean up after ourselves in case of failure, or to set the stage for leaving the old node in case of success*/
		PHYSFS_addToSearchPath(*pfslisti, 1);
	}
	
	if (r) { /*project is ok*/
		while (backgroundCNode != NULL) leaveNode(backgroundCNode);
		
		for (pfslisti = pfslist + 1; *pfslisti != NULL; pfslisti++) { /*remove old project again*/
			PHYSFS_removeFromSearchPath(*pfslisti);
		}
		if (myfilename != NULL) PHYSFS_addToSearchPath(myfilename, 1);
		
		/*start from a clean state*/
		assert(thisCNode == NULL); /*the call stack of this function never goes through Lua, calls come straight through C from the main loop*/
		assert(touchedNode == NULL); /*should be guaranteed by leaveNode()*/
		freeAutofreedCNodes();
		
		if (L != NULL) lua_close(L);
		L = lua_open();
		lua_atpanic(L, luaPanicHandler);
		luaopen_base(L); lua_pop(L, 1);
		luaopen_string(L); lua_pop(L, 1);
		luaopen_table(L); lua_pop(L, 1);
		luaopen_math(L); lua_pop(L, 1);
		
		luaopen_io(L); lua_pop(L, 3); /* io, file metatable, os */
		/* remove the io table and parts of the os table to keep the project sandboxed */
		lua_pushliteral(L, LUA_IOLIBNAME); lua_pushnil(L); lua_rawset(L, LUA_GLOBALSINDEX);
		lua_pushliteral(L, LUA_OSLIBNAME); lua_rawget(L, LUA_GLOBALSINDEX);
		lua_pushliteral(L, "execute"); lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "exit"); lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "getenv"); lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "remove"); lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "rename"); lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "setlocale"); lua_pushnil(L); lua_rawset(L, -3);
		lua_pushliteral(L, "tmpname"); lua_pushnil(L); lua_rawset(L, -3);
		lua_pop(L, 1); /* os */
		
		luaopen_pipmak(L);
		luaopen_pack(L); lua_pop(L, 1);
		luaopen_debug(L); lua_pop(L, 1); /*for the _TRACEBACK function*/
		luaopen_loadlib(L);
		if (!runLuaChunkFromPhysfs(L, "resources/defaults.lua")) {
			errorMessage("Oops, we hit a bug in Pipmak! Please report this. The application will quit now.\n\n%s", terminalLastLine());
			quit(1);
		}
		
		lua_pushliteral(L, "state");
		lua_newtable(L);
		lua_rawset(L, LUA_GLOBALSINDEX);
		
		azimuth = elevation = 0;
		updateListenerOrientation();
		standardCursor = cursors[CURSOR_HAND];
		trimMouseModeStack();
		
		lua_pushliteral(L, "pipmak_internal");
		lua_rawget(L, LUA_GLOBALSINDEX);
		lua_pushliteral(L, "loadmain");
		lua_rawget(L, -2);
		lua_pushboolean(L, (myfilename == NULL)); /*defaults only*/
		if (pcallWithBacktrace(L, 1, 0, NULL) != 0) {
			terminalPrintf("Error running Lua file %s", lua_tostring(L, -1)); /*this only catches errors in defaults.lua, not those in main.lua*/
			lua_pop(L, 1);
		}
		/*Lua stack: pipmak_internal*/
		
		lua_pushliteral(L, "project");
		lua_rawget(L, -2);
		
		lua_pushliteral(L, "title");
		lua_rawget(L, -2);
		title = lua_tostring(L, -1);
		SDL_WM_SetCaption(title, title);
		lua_pop(L, 1);
		
		lua_pushliteral(L, "onopenproject");
		lua_rawget(L, -2);
		if (lua_isfunction(L, -1)) {
			if (pcallWithBacktrace(L, 0, 0, NULL) != 0) {
				terminalPrintf("Error running openproject handler:\n%s", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		else lua_pop(L, 1);
		
		lua_pushliteral(L, "startnode");
		lua_rawget(L, -2);
		startnodepath = lua_tostring(L, -1); /*convert it to a string if it's a number*/
		if (startnodepath == NULL || *startnodepath == '\0') {
			lua_pop(L, 1);
			lua_pushliteral(L, "0");
		}
		
		lua_remove(L, -2); /*project*/
		lua_remove(L, -2); /*pipmak_internal*/
		/*Lua stack: startnode*/
		
		lua_pushstring(L, "pipmak-projectpath");
		lua_pushstring(L, myfilename);
		lua_rawset(L, LUA_REGISTRYINDEX);
	}
	
	PHYSFS_freeList(pfslist);
	
	if (myfilename != filename) free(myfilename);
	
	return r;
}

int openAndEnterProject(const char *filename) {
	if (openProject(filename)) {
		enterNode(NULL, NULL);
		lua_pop(L, 1); /*startnode*/
		return 1;
	}
	if (L == NULL) {
		/*opening the project didn't work and no project was open before: load defaults*/
		openProject(NULL);
		assert(L != NULL);
		enterNode(NULL, NULL);
		lua_pop(L, 1); /*startnode*/
	}
	return 0;
}

/*
 * Try to load a saved game given its path in platform-dependent format. On
 * success, opens the corresponding project, restores the saved state, and
 * returns 1. If the given path is not a proper saved game, quietly returns 0.
 * If the project can't be opened, reports on the Pipmak terminal and returns
 * 0. In both failure cases, all external state is left unchanged.
 */
int openSavedGame(const char *filename) {
	FILE *f;
	luaL_Buffer b;
	size_t n;
	int i, r, w, h;
	Uint32 videoflags;
	
	f = fopen(filename, "rb");
	if (f != NULL) {
		lua_pushliteral(L, "deserialize");
		lua_rawget(L, LUA_GLOBALSINDEX);
		luaL_buffinit(L, &b);
		do {
			n = fread(luaL_prepbuffer(&b), 1, LUAL_BUFFERSIZE, f);
			luaL_addsize(&b, n);
		} while (n == LUAL_BUFFERSIZE);
		luaL_pushresult(&b);
		if (lua_pcall(L, 1, 1, 0) != 0) {
			lua_pop(L, 1); /*error message*/
			r = 0;
		}
		else {
			/*Lua stack: deserialized table*/
			lua_pushliteral(L, "pipmak_internal");
			lua_rawget(L, LUA_GLOBALSINDEX);
			lua_pushliteral(L, "project");
			lua_rawget(L, -2);
			lua_pushliteral(L, "title");
			lua_rawget(L, -2);
			lua_pushliteral(L, "title");
			lua_rawget(L, -5);
			/*Lua stack: deserialized table, pipmak_internal, project, project title, deserialized title*/
			if (lua_equal(L, -1, -2)) { /*the saved game's project is already open, reload it from its current path*/
				lua_pushliteral(L, "pipmak-projectpath");
				lua_rawget(L, LUA_REGISTRYINDEX);
			}
			else {
				lua_pushliteral(L, "path");
				lua_rawget(L, -6);
			}
			/*Lua stack: deserialized table, pipmak_internal, project, project title, deserialized title, path*/
			r = openProject(lua_tostring(L, -1));
			if (r) {
				lua_pop(L, 1); /*startnode*/
				/*read the file again since openProject() killed the Lua state*/
				rewind(f);
				lua_pushliteral(L, "deserialize");
				lua_rawget(L, LUA_GLOBALSINDEX);
				luaL_buffinit(L, &b);
				do {
					n = fread(luaL_prepbuffer(&b), 1, LUAL_BUFFERSIZE, f);
					luaL_addsize(&b, n);
				} while (n == LUAL_BUFFERSIZE);
				luaL_pushresult(&b);
				if (lua_pcall(L, 1, 1, 0) != 0) r = 0;
			}
			else {
				terminalPrintf("Can't open the project \"%s\" which used to be at %s.\n• If it is on a CD, make sure that the CD is inserted.\n• If you moved it, open the project first and then the saved game.", lua_tostring(L, -2), lua_tostring(L, -1));
				lua_pop(L, 5); /*path, titles, project, pipmak_internal*/
			}
			/*Lua stack: deserialized table*/
			
			if (r) {
				while (backgroundCNode != NULL) leaveNode(backgroundCNode);
				assert(thisCNode == NULL); /*the call stack of this function never goes through Lua, calls come straight through C from the main loop*/
				assert(touchedNode == NULL); /*should be guaranteed by leaveNode()*/
				freeAutofreedCNodes();
				
				lua_pushliteral(L, "state");
				lua_pushvalue(L, -1);
				lua_rawget(L, -3);
				lua_rawset(L, LUA_GLOBALSINDEX);
				
				lua_pushliteral(L, "az"); lua_rawget(L, -2); azimuth = (GLfloat)lua_tonumber(L, -1); lua_pop(L, 1);
				lua_pushliteral(L, "el"); lua_rawget(L, -2); elevation = (GLfloat)lua_tonumber(L, -1); lua_pop(L, 1);
				updateListenerOrientation();
				lua_pushliteral(L, "joyspeed"); lua_rawget(L, -2); joystickSpeed = (float)lua_tonumber(L, -1); lua_pop(L, 1);
				lua_pushliteral(L, "w"); lua_rawget(L, -2); w = (int)lua_tonumber(L, -1); lua_pop(L, 1);
				lua_pushliteral(L, "h"); lua_rawget(L, -2); h = (int)lua_tonumber(L, -1); lua_pop(L, 1);
				lua_pushliteral(L, "fov"); lua_rawget(L, -2); if (lua_isnumber(L, -1)) verticalFOV = (GLfloat)lua_tonumber(L, -1); lua_pop(L, 1);
				
				videoflags = SDL_WINDOW_OPENGL;
				lua_pushliteral(L, "full");
				lua_rawget(L, -2);
				if (lua_toboolean(L, -1)) videoflags |= SDL_WINDOW_FULLSCREEN;
				else videoflags |= SDL_WINDOW_RESIZABLE;
				lua_pop(L, 1);
				
				lua_pushliteral(L, "texfilter"); lua_rawget(L, -2); glTextureFilter = (GLint)lua_tonumber(L, -1); lua_pop(L, 1);
				
				terminalClear();
				cleanupGL();
				screen = 0 = 0; SDL_SetVideoMode(w, h, 0, videoflags);
				if (screen == NULL) {
					screen = 0 = 0; SDL_SetVideoMode(640, 480, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
					terminalPrintf("Error switching to full screen: %s", SDL_GetError());
				}
				if (screen->flags & SDL_WINDOW_FULLSCREEN) SDL_SetRelativeMouseMode(SDL_TRUE);
				setupGL();
				
				trimMouseModeStack();
				lua_pushliteral(L, "mouse"); lua_rawget(L, -2); setStandardMouseMode((int)lua_tonumber(L, -1)); lua_pop(L, 1);
				
				lua_pushliteral(L, "node"); lua_rawget(L, -2); enterNode(NULL, NULL); lua_pop(L, 1);
				
				lua_pushliteral(L, "overlays");
				lua_rawget(L ,-2);
				if (lua_istable(L, -1)) {
					for (i = 1; ; i++) {
						lua_rawgeti(L, -1, i);
						if (lua_isstring(L, -1)) {
							lua_tostring(L, -1); /*convert it to a string if it's a number (saved game from an older version)*/
							overlayNode(0);
							lua_pop(L, 1);
						}
						else {
							lua_pop(L, 1);
							break;
						}
					}
				}
				lua_pop(L, 1);
				
				lua_pop(L, 1);
			}
		}
		fclose(f);
	}
	else {
		r = 0;
	}
	return r;
}
