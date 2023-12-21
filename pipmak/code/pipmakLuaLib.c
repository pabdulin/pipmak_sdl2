/*
 
 pipmakLuaLib.c, part of the Pipmak Game Engine
 Copyright (c) 2004-2011 Christian Walther
 
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

/* $Id: pipmakLuaLib.c 228 2011-04-30 19:40:44Z cwalther $ */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "lua.h"
#include "lauxlib.h"
#include "physfs.h"
#include "physfsrwops.h"

#include "nodes.h"
#include "images.h"
#include "tools.h"
#include "misc.h"
#include "terminal.h"
#include "glstate.h"
#include "platform.h"
#include "config.h"
#include "version.h"
#include "audio.h"
#include "textedit.h"

extern CNode *thisCNode, *backgroundCNode, *touchedNode;
extern Image *currentCursor, *standardCursor;
extern int mouseX, mouseY;
extern int mouseButton;
extern float clickH, clickV;
extern Uint32 thisRedrawTime;
extern enum TransitionState transitionState;
extern enum TransitionEffect currentTransitionEffect;
extern enum TransitionDirection currentTransitionDirection;
extern Uint32 transitionDuration;
extern float transitionParameter;
extern struct MouseModeStackEntry *topMouseMode;
extern SDL_Rect screenSize;
extern SDL_Window *sdl2Window;
extern GLint glTextureFilter;
extern GLenum glTextureTarget;
extern int showControls;
extern float joystickSpeed;
extern enum DisruptiveInstruction disruptiveInstruction;
extern char *disruptiveInstructionArgumentCharP;
extern char *disruptiveInstructionPostCode;
extern GLfloat azimuth, elevation;
extern GLfloat verticalFOV;
extern Image *imageListStart;
extern Image *cursors[NUMBER_OF_CURSORS];
extern TTF_Font *verafont;
extern SDL_Rect desktopsize;    /* initialized in main.c */
extern int cpTermToStdout;    /* initialized in main.c */


static Image *checkCursor(lua_State *L, int idx) {
	Image *i = (Image*)luaL_checkudata(L, idx, "pipmak-image");
	if (i == NULL) luaL_typerror(L, idx, "cursor");
	return i;
}

static int copyPhysfsFile(const char *src, const char *dest) {
	PHYSFS_file *srcfile, *destfile;
	int ok = 0;
	srcfile = PHYSFS_openRead(src);
	if (srcfile != NULL) {
		destfile = PHYSFS_openWrite(dest);
		if (destfile != NULL) {
			char buffer[1024];
			PHYSFS_uint32 count;
			while (!PHYSFS_eof(srcfile)) {
				count = (PHYSFS_uint32)PHYSFS_read(srcfile, buffer, 1, 1024);
				PHYSFS_write(destfile, buffer, 1, count);
			}
			ok = 1;
			PHYSFS_close(destfile);
		}
		PHYSFS_close(srcfile);
	}
	return ok;
}

/* pipmak_internal functions ------------------------------*/

static int dofile(lua_State *L, int propagateError) {
	const char *s;
	int n;
	if (!lua_isstring(L, -1)) {
		luaL_typerror(L, 1, lua_typename(L, LUA_TSTRING));
	}
	absolutizePath(L, 0);
	s = lua_tostring(L, -1);
	n = lua_gettop(L);
	if (propagateError) {
		if (loadLuaChunkFromPhysfs(L, s) != 0) {
			luaL_error(L, lua_tostring(L, -1));
		}
		lua_call(L, 0, LUA_MULTRET);
	}
	else {
		if (loadLuaChunkFromPhysfs(L, s) != 0) {
			terminalPrintf("Error loading Lua file %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			lua_pushboolean(L, 0);
			return 1;
		}
		else if (pcallWithBacktrace(L, 0, LUA_MULTRET, thisCNode) != 0) {
			terminalPrintf("Error running Lua file %s", lua_tostring(L, -1));
			lua_pop(L, 1);
			lua_pushboolean(L, 0);
			return 0;
		}
		lua_pushboolean(L, 1);
		lua_insert(L, n+1);
	}
	lua_remove(L, n); /*s*/
	return lua_gettop(L) - n + 1;
}

static int internalDofileLua(lua_State *L) {
	return dofile(L, 0);
}

static int internalUpdatepatchLua(lua_State *L) {
	if (thisCNode != NULL && thisCNode->complete) thisCNode->updatePatch(thisCNode);
	return 0;
}

static int internalUpdatepanelpositionLua(lua_State *L) {
	CNode *node;
	Uint32 duration = (Uint32)(1000*luaL_optnumber(L, 2, 0.0));
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushliteral(L, "cnode");
	lua_rawget(L, 1);
	node = lua_touserdata(L, -1);
	if (node != NULL && node->type == NODE_TYPE_PANEL) node->private.panel.updatePosition(node, duration);
	return 0;
}

static int internalSetpanelbboxLua(lua_State *L) {
	CNode *node;
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushliteral(L, "cnode");
	lua_rawget(L, 1);
	node = lua_touserdata(L, -1);
	if (node != NULL && node->type == NODE_TYPE_PANEL) {
		node->private.panel.bbminx = luaL_checkint(L, 2);
		node->private.panel.bbmaxx = luaL_checkint(L, 3);
		node->private.panel.bbminy = luaL_checkint(L, 4);
		node->private.panel.bbmaxy = luaL_checkint(L, 5);
	}
	return 0;
}

static int internalVersionLua(lua_State *L) {
	int v = (int)floor(100*luaL_checknumber(L, 1));
	if (v > 100*PIPMAK_VERSION) {
		terminalPrintf("This project was made for Pipmak version %d.%d.%d, while you are using an older version (%d.%d.%d).\nSome things may not work as expected. Get a newer version of Pipmak at http://pipmak.sourceforge.net/.", v/100, (v/10)%10, v%10, (int)PIPMAK_VERSION, ((int)(10*PIPMAK_VERSION))%10, ((int)(100*PIPMAK_VERSION))%10);
	}
	else if (v/10 < (int)(10*PIPMAK_VERSION)) {
		terminalPrintf("This project was made for Pipmak version %d.%d.%d, while you are using a newer version (%d.%d.%d).\nSome things may not work as expected.", v/100, (v/10)%10, v%10, (int)PIPMAK_VERSION, ((int)(10*PIPMAK_VERSION))%10, ((int)(100*PIPMAK_VERSION))%10);
	}
	return 0;
}

static int internalToolLua(lua_State *L) {
	int new, old = (backgroundCNode == NULL || backgroundCNode == NULL) ? 0 : backgroundCNode->tool->tag;
	if (!lua_isnoneornil(L, 1)) {
		new = (int)lua_tonumber(L, 1);
		if (new < 0 || new > NUMBER_OF_TOOLS || !lua_isnumber(L, 1)) {
			terminalPrintf("Unknown tool \"%s\"", lua_tostring(L, 1));
		}
		else {
			toolChoose(newTool(new%NUMBER_OF_TOOLS, backgroundCNode));
		}
	}
	lua_pushnumber(L, old);
	return 1;
}

static int internalTexrectLua(lua_State *L) {
	int oldstate = (glTextureTarget == GL_TEXTURE_RECTANGLE_NV);
	if (!lua_isnoneornil(L, 1)) {
		terminalClear();
		cleanupGL();
		glTextureTarget = (lua_toboolean(L, 1)) ? GL_TEXTURE_RECTANGLE_NV : GL_TEXTURE_2D;
		setupGL();
		printf("before: %d, after: %d\n", oldstate, (glTextureTarget == GL_TEXTURE_RECTANGLE_NV));
	}
	lua_pushboolean(L, oldstate);
	return 1;
}

static int internalWhereamiLua(lua_State *L) {
	lua_Debug ar;
	lua_getstack(L, 2, &ar);
	lua_getinfo(L, "Sl", &ar);
	if (ar.source[0] == '@') lua_pushstring(L, ar.source + 1);
	else lua_pushliteral(L, "");
	lua_pushnumber(L, ar.currentline);
	return 2;
}

/* Does the same thing as the 'error' function from the base library except
 * that the error message is printed to the Pipmak terminal and control
 * returns to the caller.
 */
static int internalWarningLua(lua_State *L) {
	int level = luaL_optint(L, 2, 1);
	luaL_checkany(L, 1);
	if (!lua_isstring(L, 1) || level == 0)
		lua_pushvalue(L, 1);  /* propagate error message without changes */
	else {  /* add extra information */
		luaL_where(L, level);
		lua_pushvalue(L, 1);
		lua_concat(L, 2);
	}
	terminalPrint(lua_tostring(L, -1), 0);
	return 0;
}

static int internalOpenfileLua(lua_State *L) {
	const char *p;
	if (!lua_isstring(L, 1)) {
		lua_pushliteral(L, "node.lua");
		absolutizePath(L, 0);
	}
	p = PHYSFS_getRealDir(lua_tostring(L, -1));
	if (p != NULL) openFile(p, lua_tostring(L, -1));
	else terminalPrintf("File %s not found.", lua_tostring(L, -1));
	return 0;
}

static void checkinstanceof(lua_State *L, int idx, const char *metatable, const char *expected) {
	const char *s;
	lua_pushvalue(L, idx);
	while (lua_getmetatable(L, -1)) {
		lua_remove(L, -2);
		lua_pushvalue(L, -1);
		lua_rawget(L, LUA_REGISTRYINDEX);
		s = lua_tostring(L, -1);
		if (s != NULL && strcmp(s, metatable) == 0) {
			lua_pop(L, 2);
			return;
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	luaL_typerror(L, idx, expected);
}

static int internalPretendnodeLua(lua_State *L) {
	CNode *node;
	checkinstanceof(L, 1, "pipmak-node", "node");
	lua_pushliteral(L, "cnode"); lua_rawget(L, 1); node = lua_touserdata(L, -1); lua_pop(L, 1);
	if (node != NULL) thisCNode = node;
	return 0;
}

static int internalSpecialcursorLua(lua_State *L) {
	Image *m;
	int i;
	i = luaL_checkint(L, 1);
	m = (Image*)luaL_checkudata(L, 2, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 2, "cursor");
	if (i >= 0 && i < NUMBER_OF_CURSORS) cursors[i] = m;
	return 0;
}

static int setwindowedLua(lua_State *L);
static int internalNewprojectLua(lua_State *L) {
	char *path;

	// TODO(pabdulin): check
	Uint32 screen_flags = SDL_GetWindowFlags(sdl2Window);
	if (screen_flags & SDL_WINDOW_FULLSCREEN) setwindowedLua(L);
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);
	path = newProjectPath();
	SDL_ShowCursor(SDL_DISABLE);
	
	if (path != NULL) {
		char *dir = directoryFromPath(path);
		if (dir != NULL) {
			char *filename = path + strlen(dir) + strlen(PHYSFS_getDirSeparator());
			
			if (PHYSFS_setWriteDir(dir) && PHYSFS_mkdir(filename) && PHYSFS_setWriteDir(path)) {
				PHYSFS_file *file = PHYSFS_openWrite("main.lua");
				if (file != NULL) {
					lua_pushfstring(L, "version (%f)%s%stitle ", PIPMAK_VERSION, lineEnding, lineEnding);
					
					lua_pushliteral(L, "string"); lua_rawget(L, LUA_GLOBALSINDEX);
					lua_pushliteral(L, "format"); lua_rawget(L, -2);
					lua_remove(L, -2); /*string table*/
					lua_pushliteral(L, "%q");
					lua_pushlstring(L, filename, strlen(filename) - (strEndsWith(filename, ".pipmak") ? 7 : 0));
					lua_call(L, 2, 1);
					
					lua_pushfstring(L, "%sstartnode (1)%s", lineEnding, lineEnding);
					
					lua_concat(L, 3);
					
					PHYSFS_write(file, lua_tostring(L, -1), 1, (PHYSFS_uint32)lua_strlen(L, -1));
					PHYSFS_close(file);
					lua_pop(L, 1);
					
					if (PHYSFS_mkdir("1")) {
						if (copyPhysfsFile("resources/missing.png", "1/face.png")) {
							file = PHYSFS_openWrite("1/node.lua");
							if (file != NULL) {
								lua_pushfstring(L, "cubic {%s\t\"face.png\", --front%s\t\"face.png\", --right%s\t\"face.png\", --back%s\t\"face.png\", --left%s\t\"face.png\", --top%s\t\"face.png\" --bottom%s}%s", lineEnding, lineEnding, lineEnding, lineEnding, lineEnding, lineEnding, lineEnding, lineEnding);
								PHYSFS_write(file, lua_tostring(L, -1), 1, (PHYSFS_uint32)lua_strlen(L, -1));
								PHYSFS_close(file);
								lua_pop(L, 1);
								
								disruptiveInstruction = INSTR_OPENPROJECT;
								disruptiveInstructionArgumentCharP = malloc(strlen(path) + 1);
								if (disruptiveInstructionArgumentCharP != NULL) strcpy(disruptiveInstructionArgumentCharP, path);
								disruptiveInstructionPostCode = malloc(24);
								if (disruptiveInstructionPostCode != NULL) strcpy(disruptiveInstructionPostCode, "pipmak.overlaynode(-20)");
							}
							else terminalPrintf("Could not write node.lua: %s", PHYSFS_getLastError());
						}
						else terminalPrintf("Could not copy face.png: %s", PHYSFS_getLastError());
						
					}
					else terminalPrintf("Could not create folder \"1\" in %s: %s", path, PHYSFS_getLastError());
					
				}
				else terminalPrintf("Could not write main.lua: %s", PHYSFS_getLastError());
				
			}
			else terminalPrintf("Could not create project %s in %s: %s", filename, dir, PHYSFS_getLastError());
			free(dir);
		}
		else terminalPrintf("Error examining path %s", path);
		free(path);
	}
	return 0;
}

static const luaL_reg pipmakInternalFuncs[] = {
	{"dofile", internalDofileLua},
	{"updatepatch", internalUpdatepatchLua},
	{"updatepanelposition", internalUpdatepanelpositionLua},
	{"setpanelbbox", internalSetpanelbboxLua},
	{"version", internalVersionLua},
	{"tool", internalToolLua},
	{"texrect", internalTexrectLua},
	{"whereami", internalWhereamiLua},
	{"warning", internalWarningLua},
	{"openfile", internalOpenfileLua},
	{"pretendnode", internalPretendnodeLua},
	{"specialcursor", internalSpecialcursorLua},
	{"newproject", internalNewprojectLua},
	{NULL, NULL}
};

/* Pipmak functions ---------------------------------------*/

static int printLua(lua_State *L) {
	int i;
	int n = lua_gettop(L);
	lua_pushstring(L, "tostring");
	lua_gettable(L, LUA_GLOBALSINDEX);
	for (i = 1; i <= n; i++) {
		lua_pushvalue(L, n+1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
	}
	lua_concat(L, n);
	terminalPrint(lua_tostring(L, -1), 0);
	return 0;
}

static int printinplaceLua(lua_State *L) {
	int i;
	int n = lua_gettop(L);
	lua_pushstring(L, "tostring");
	lua_gettable(L, LUA_GLOBALSINDEX);
	for (i = 1; i <= n; i++) {
		lua_pushvalue(L, n+1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
	}
	lua_concat(L, n);
	terminalPrint(lua_tostring(L, -1), 1);
	return 0;
}

static int dofileLua(lua_State *L) {
	return dofile(L, 1);
}

static int getmousemodeLua(lua_State *L) {
	struct MouseModeStackEntry *entry = topMouseMode;
	while (entry->next != NULL) entry = entry->next;
	lua_pushnumber(L, entry->mode);
	return 1;
}

static int setmousemodeLua(lua_State *L) {
	int c = (int)lua_tonumber(L, 1);
	if (c < 0 || c >= NUMBER_OF_MOUSE_MODES || !lua_isnumber(L, 1)) {
		terminalPrintf("Unknown mouse mode \"%s\" - use pipmak.joystick or pipmak.direct", lua_tostring(L, 1));
	}
	else {
		setStandardMouseMode(c);
	}
	return 0;
}

static int pushmousemodeLua(lua_State *L) {
	int c = (int)lua_tonumber(L, 1);
	if (c < 0 || c >= NUMBER_OF_MOUSE_MODES || !lua_isnumber(L, 1)) {
		terminalPrintf("Unknown mouse mode \"%s\" - use pipmak.joystick or pipmak.direct", lua_tostring(L, 1));
		lua_pushnil(L);
	}
	else {
		MouseModeToken token = pushMouseMode(c);
		if (token == NULL) luaL_error(L, "out of memory");
		lua_pushlightuserdata(L, token);
	}
	return 1;
}

static int popmousemodeLua(lua_State *L) {
	MouseModeToken token;
	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	token = lua_touserdata(L, 1);
	lua_pushboolean(L, popMouseMode(token));
	return 1;
}

static int setcursorLua(lua_State * L) {
	currentCursor = checkCursor(L, 1);
	return 0;
}

static int setstandardcursorLua(lua_State *L) {
	standardCursor = checkCursor(L, 1);
	return 0;
}

static int clicklocLua(lua_State *L) {
	lua_pushnumber(L, clickH);
	lua_pushnumber(L, clickV);
	return 2;
}

static int mouselocLua(lua_State *L) {
	float h, v;
	if (!(thisCNode != NULL && thisCNode->complete)) luaL_error(L, "mouseloc is unavailable until the current node is completely loaded");
	thisCNode->mouseXYtoHV(thisCNode, mouseX, mouseY, &h, &v);
	lua_pushnumber(L, h);
	lua_pushnumber(L, v);
	return 2;
}

static int mouseminusclicklocLua(lua_State *L) {
	float h, v;
	if (!(thisCNode != NULL && thisCNode->complete)) luaL_error(L, "mouseminusclickloc is unavailable until the current node is completely loaded");
	thisCNode->mouseXYtoHV(thisCNode, mouseX, mouseY, &h, &v);
	lua_pushnumber(L, h - clickH);
	lua_pushnumber(L, v - clickV);
	return 2;
}

static int mouseisdownLua(lua_State *L) {
	lua_pushboolean(L, mouseButton != 0);
	return 1;
}

static int getimageLua(lua_State *L) { /*serves as both pipmak.getimage and pipmak.loadcursor*/
	Image *image;
	int hotx = -1, hoty = -1;
	luaL_checktype(L, 1, LUA_TSTRING);
	if (lua_gettop(L) > 1) {
		hotx = luaL_checkint(L, 2);
		hoty = luaL_checkint(L, 3);
		lua_pop(L, 2);
	}
	absolutizePath(L, 0);
	/*Lua stack: path*/
	
	lua_pushliteral(L, "pipmak-imagecache");
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, -2);
	lua_rawget(L, -2);
	/*Lua stack: path, imagecache, cached image or nil*/
	if (lua_isnil(L, -1)) {
		const char *s;
		
		lua_pop(L, 1); /*nil*/
		s = lua_tostring(L, -2);
		
		image = lua_newuserdata(L, sizeof(Image));
		/*Lua stack: path, imagecache, image*/
		image->path = malloc(strlen(s) + 1);
		if (image->path == NULL) {
			luaL_error(L, "out of memory");
		}
		strcpy(image->path, s);
		image->isFile = 1;
		image->textureID = 0;
		image->texrefcount = 0;
		image->datarefcount = 0;
		image->data = NULL;
		image->drawingColor.r = image->drawingColor.g = image->drawingColor.b = 0;
		image->drawingColor.a = 255; /*used as alpha here*/
		
		image->prev = NULL;
		image->next = imageListStart;
		if (imageListStart != NULL) imageListStart->prev = image;
		imageListStart = image;
		
		luaL_getmetatable(L, "pipmak-image");
		lua_setmetatable(L, -2);
		
		getImageData(image, 1);
		
		lua_pushvalue(L, -3); /*path*/
		lua_pushvalue(L, -2); /*image*/
		lua_rawset(L, -4);
		lua_remove(L, -2); /*imagecache*/
		lua_remove(L, -2); /*path*/
	}
	else {
		IFDEBUG(printf("debug: got image %s from cache\n", lua_tostring(L, -3));)
		image = lua_touserdata(L, -1);
		assert(image->isFile);
		if (image->prev != NULL) { /*move to the start of the linked list*/
			image->prev->next = image->next;
			if (image->next != NULL) image->next->prev = image->prev;
			image->prev = NULL;
			image->next = imageListStart;
			imageListStart = image;
			image->next->prev = image;
		}
		lua_remove(L, -2); /*imagecache*/
		lua_remove(L, -2); /*path*/
	}
	image->cursorHotX = hotx;
	image->cursorHotY = hoty;
	return 1;
}

static int newimageLua(lua_State *L) {
	Image *image, *srcimg = NULL;
	int w, h;
	
	if (lua_type(L, 1) == LUA_TSTRING) {
		lua_pushcfunction(L, getimageLua);
		lua_pushvalue(L, 1);
		lua_call(L, 1, 1);
		lua_replace(L, 1);
	}
	srcimg = (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (srcimg != NULL) {
		w = srcimg->w;
		h = srcimg->h;
	}
	else {
		w = luaL_checkint(L, 1);
		if (w < 1 || w >= 32768) luaL_error(L, "image width out of range: %d", w);
		h = luaL_checkint(L, 2);
		if (h < 1 || h >= 32768) luaL_error(L, "image height out of range: %d", h);
	}
	image = lua_newuserdata(L, sizeof(Image));
	image->path = malloc(24);
	if (image->path == NULL) {
		luaL_error(L, "out of memory");
	}
	sprintf(image->path, "dyn-%p", image);
	image->isFile = 0;
	image->textureID = 0;
	image->texrefcount = 0;
	image->datarefcount = 0;
	image->w = w;
	image->h = h;
	image->bpp = 32;
	image->data = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, BYTEORDER_DEPENDENT_RGBA_MASKS);
	image->datasize = image->data->h*image->data->pitch;
	image->drawingColor.r = image->drawingColor.g = image->drawingColor.b = 0;
	image->drawingColor.a = 255; /*used as alpha here*/
	
	if (srcimg != NULL) {
		getImageData(srcimg, 1);
		SDL_SetAlpha(srcimg->data, 0, 255);
		SDL_BlitSurface(srcimg->data, NULL, image->data, NULL);
	}
	
	image->prev = NULL;
	image->next = imageListStart;
	if (imageListStart != NULL) imageListStart->prev = image;
	imageListStart = image;
	
	luaL_getmetatable(L, "pipmak-image");
	lua_setmetatable(L, -2);
	
	return 1;
}

static int flushimagecacheLua(lua_State *L) {
	lua_pushliteral(L, "pipmak-imagecache");
	lua_newtable(L);
	lua_newtable(L);
	lua_pushliteral(L, "__mode");
	lua_pushliteral(L, "v");
	lua_rawset(L, -3);
	lua_setmetatable(L, -2);
	lua_rawset(L, LUA_REGISTRYINDEX);
	return 0;
}

static int getcurrentnodeLua(lua_State *L) {
	if (backgroundCNode == NULL) {
		lua_pushliteral(L, "0");
	}
	else {
		lua_rawgeti(L, LUA_REGISTRYINDEX, backgroundCNode->noderef);
		lua_pushliteral(L, "path"); lua_rawget(L, -2);
	}
	return 1;
}

static int thisnodeLua(lua_State *L) {
	if (thisCNode == NULL) lua_pushnil(L);
	else lua_rawgeti(L, LUA_REGISTRYINDEX, thisCNode->noderef);
	return 1;
}

static int backgroundnodeLua(lua_State *L) {
	if (backgroundCNode == NULL) lua_pushnil(L);
	else lua_rawgeti(L, LUA_REGISTRYINDEX, backgroundCNode->noderef);
	return 1;
}

static int gotonodeLua(lua_State *L) {
	CNode *replaced, *prev;
	IFDEBUG(Uint32 profiletime;)
	luaL_checkstring(L, 1);
	replaced = (lua_toboolean(L, 2)) ? backgroundCNode : thisCNode;
	lua_settop(L, 1);
	absolutizePath(L, 1);
	IFDEBUG(profiletime = SDL_GetTicks();)
	if (transitionState != TRANSITION_PENDING) {
		captureScreenGL(0);
		currentTransitionEffect = TRANSITION_DISSOLVE;
		transitionDuration = 500;
		transitionParameter = 1;
		transitionState = TRANSITION_PENDING;
	}
	if (replaced != NULL && !replaced->entered) replaced = NULL;
	prev = (replaced != NULL) ? replaced->prev : NULL;
	enterNode(prev, replaced);
	if (touchedNode == NULL) updateTouchedNode();
	IFDEBUG(printf("%p -> %2s: %4d ms\n", replaced, lua_tostring(L, 1), SDL_GetTicks() - profiletime);)
	return 0;
}

static int overlaynodeLua(lua_State *L) {
	CNode *node;
	luaL_checkstring(L, 1);
	lua_settop(L, 2);
	lua_insert(L, -2); /*swap path and dontsave*/
	absolutizePath(L, 1);
	node = overlayNode(lua_toboolean(L, 1));
	if (node != NULL) lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	else lua_pushnil(L);
	return 1;
}

static int dissolveLua(lua_State *L) {
	float t;
	transitionParameter = (float)luaL_optnumber(L, 1, 1.0);
	t = (float)luaL_optnumber(L, 2, 0.5);
	if (t < 0) {
		t = 0.5;
		terminalPrintf("Invalid transition duration \"%s\" - use a nonnegative number", lua_tostring(L, 2));
	}
	transitionDuration = (Uint32)(t*1000);
	currentTransitionEffect = TRANSITION_DISSOLVE;
	captureScreenGL(0);
	transitionState = TRANSITION_PENDING;
	return 0;
}

static int rotateLua(lua_State *L) {
	float t;
	currentTransitionDirection = luaL_checknumber(L, 1);
	if (currentTransitionDirection < 0 || currentTransitionDirection > 3) {
		currentTransitionDirection = TRANSITION_LEFT;
		terminalPrintf("Unknown transition direction \"%s\" - use one of pipmak.left, pipmak.right, pipmak.up, pipmak.down", lua_tostring(L, 1));
	}
	transitionParameter = (float)luaL_optnumber(L, 2, 0);
	if (transitionParameter < -60 || transitionParameter >= 180) {
		transitionParameter = 0;
		terminalPrintf("Invalid rotation angle %s - use -60 <= angle < 180", lua_tostring(L, 2));
	}
	transitionParameter *= (float)M_PI/180;
	t = (float)luaL_optnumber(L, 3, 0.5);
	if (t < 0) {
		t = 0.5;
		terminalPrintf("Invalid transition duration %s - use a nonnegative number", lua_tostring(L, 3));
	}
	transitionDuration = (Uint32)(t*1000);
	currentTransitionEffect = TRANSITION_ROTATE;
	captureScreenGL(0);
	transitionState = TRANSITION_PENDING;
	return 0;
}

static int wipeLua(lua_State *L) {
	float t;
	currentTransitionDirection = luaL_checknumber(L, 1);
	if (currentTransitionDirection < 0 || currentTransitionDirection > 3) {
		currentTransitionDirection = TRANSITION_LEFT;
		terminalPrintf("Unknown transition direction \"%s\" - use one of pipmak.left, pipmak.right, pipmak.up, pipmak.down", lua_tostring(L, 1));
	}
	t = (float)luaL_optnumber(L, 2, 0.5);
	if (t < 0) {
		t = 0.5;
		terminalPrintf("Invalid transition duration %s - use a nonnegative number", lua_tostring(L, 2));
	}
	transitionDuration = (Uint32)(t*1000);
	currentTransitionEffect = TRANSITION_WIPE;
	captureScreenGL(0);
	transitionState = TRANSITION_PENDING;
	return 0;
}

static int setviewdirectionLua(lua_State *L) {
	azimuth = (GLfloat)luaL_checknumber(L, 1);
	elevation = (GLfloat)luaL_checknumber(L, 2);
	if (elevation > 80) elevation = 80;
	else if (elevation < -80) elevation = -80;
	while (azimuth >= 360) azimuth -= 360;
	while (azimuth < 0) azimuth += 360;
	updateListenerOrientation();
	return 0;
}

static int getviewdirectionLua(lua_State *L) {
	lua_pushnumber(L, azimuth);
	lua_pushnumber(L, elevation);
	return 2;
}

static int screensizeLua(lua_State *L) {
	SDL_Rect *screen = &screenSize;
	lua_pushnumber(L, screen->w);
	lua_pushnumber(L, screen->h);
	return 2;
}

static int setwindowedLua(lua_State *L) {
	terminalClear();
	cleanupGL();
	
	
	sdl2Window = SDL_CreateWindow("pipmak",
	SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	640, 480,
	SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GetWindowSize(sdl2Window, &screenSize.w, &screenSize.h);
	SDL_Rect *screen = &screenSize;
	
	setupGL();
	if (topMouseMode->mode == MOUSE_MODE_DIRECT) {
		mouseX = screen->w/2;
		mouseY = screen->h/2;
	}
	terminalPrintf("%d x %d", screen->w, screen->h);
	return 0;
}

static int getscreenmodesLua(lua_State *L) {
	SDL_Rect **modes;
	int i;

	// TODO(pabdulin): fix#8 SDL_ListModes
	modes = NULL; // 0 = 0; SDL_ListModes(NULL, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
	if (modes == NULL || modes == (SDL_Rect**)-1) {
		/*return an empty table*/
		lua_newtable(L);
		return 1;
	}

	lua_newtable(L);
	for (i=0; modes[i]; i++) {
		/* push table index */	
		lua_pushnumber(L, i+1);  
		/* push table value */
		lua_newtable(L);         /* every value of table is a subtable with component w and h */
		lua_pushnumber(L, 1);         /* push subtable index */
		lua_pushnumber(L, modes[i]->w); /* push subtable value */
		lua_settable(L, -3);
		lua_pushnumber(L, 2);         /* push subtable index */
		lua_pushnumber(L, modes[i]->h); /* push subtable value */
		lua_settable(L, -3);
		lua_settable(L, -3); 
	}
	return 1;
}

static int desktopsizeLua(lua_State *L) {
	/* return desktop size stored in main.c before calling SDL_CreateWindow() */
	lua_pushnumber(L, desktopsize.w);
	lua_pushnumber(L, desktopsize.h);
	return 2;
}

static int setfullscreenLua(lua_State *L) {
	SDL_Rect **modes;
	int i;

	// TODO(pabdulin): fix#8 SDL_ListModes
	modes = NULL; // 0 = 0; SDL_ListModes(NULL, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
	if (modes == NULL) {
		terminalPrint("Error switching to full screen: No video modes available", 0);
	}
	else {
		SDL_Rect defaultMode;
		SDL_Rect *defaultModes[2];
		if (modes == (SDL_Rect**)-1) { /*I don't think this should happen with fullscreen modes, but you never know...*/
			defaultMode.w = 640;
			defaultMode.h = 480;
			defaultModes[0] = &defaultMode;
			defaultModes[1] = NULL;
			modes = defaultModes;
		}
		i = 0;

		SDL_Rect *screen = &screenSize;
		if (lua_gettop(L) < 2 && !lua_istable(L, 1)) {  /* old syntax: parameter, if present, is boolean */ 
			if (lua_toboolean(L, 1)) {
				while (modes[i+1] != NULL && modes[i]->w * modes[i]->h >= screen->w * screen->h) i++;
			}
			else {
				while (modes[i+1] != NULL && modes[i+1]->w * modes[i+1]->h > screen->w * screen->h) i++;
			}
		}
		else {  /* new syntax: parameters are numbers, optionally enclosed in a table */
			int w, h;
			int modeFound = 0;
			
			if (lua_istable(L, 1)) {
				/* unpack the first two elements of the table into stack slots 1 and 2 as if they were passed as arguments directly */
				lua_settop(L, 1);
				lua_rawgeti(L, 1, 1);
				lua_rawgeti(L, 1, 2);
				lua_remove(L, 1);
			}
			w = luaL_checkint(L, 1);
			h = luaL_checkint(L, 2);
			i = 0;
			while (modes[i] != NULL) {
				if (modes[i]->w > w || modes[i]->h > h) {
					i++;
				}
				else {
					modeFound = 1;
					break;
				}
			}
			if (modeFound == 0) i--;
		}

		// TODO(pabdulin): check
		Uint32 screen_flags = SDL_GetWindowFlags(sdl2Window);
		if (modes[i]->w != screen->w || modes[i]->h != screen->h || (screen_flags & SDL_WINDOW_FULLSCREEN) == 0) {
			terminalClear();
			cleanupGL();
			sdl2Window = SDL_CreateWindow("pipmak",
				SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
				modes[i]->w, modes[i]->h, 
				SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
			SDL_GetWindowSize(sdl2Window, &screenSize.w, &screenSize.h);
			if (sdl2Window == NULL) {
				terminalPrintf("Error switching to full screen: %s", SDL_GetError());
				sdl2Window = SDL_CreateWindow("pipmak",
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					640, 480, 
					SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
				SDL_GetWindowSize(sdl2Window, &screenSize.w, &screenSize.h);
			}
			setupGL();
			SDL_SetRelativeMouseMode(SDL_TRUE);
			if (topMouseMode->mode == MOUSE_MODE_DIRECT) {
				mouseX = screen->w/2;
				mouseY = screen->h/2;
			}
		}
		terminalPrintf("%d x %d", screen->w, screen->h);
	}
	return 0;
}

static int getinterpolationLua(lua_State *L) {
	lua_pushboolean(L, (glTextureFilter == GL_LINEAR));
	return 1;
}

static int setinterpolationLua(lua_State *L) {
	CNode *node;
	if (lua_toboolean(L, 1)) {
		glTextureFilter = GL_LINEAR;
	}
	else {
		glTextureFilter = GL_NEAREST;
	}
	node = backgroundCNode;
	while (node != NULL) {
		node->updateInterpolation(node);
		node = node->next;
	}
	return 0;
}

static int getshowcontrolsLua(lua_State *L) {
	lua_pushboolean(L, showControls);
	return 1;
}

static int setshowcontrolsLua(lua_State *L) {
	int newShowControls = lua_toboolean(L, 1);
	if (newShowControls != showControls && backgroundCNode != NULL && backgroundCNode->hotspotmap != NULL) {
		if (newShowControls) backgroundCNode->hotspotmap->feedToGL(backgroundCNode->hotspotmap);
		else backgroundCNode->hotspotmap->removeFromGL(backgroundCNode->hotspotmap);
	}
	showControls = newShowControls;
	return 0;
}

static int getjoystickspeedLua(lua_State *L) {
	lua_pushnumber(L, joystickSpeed);
	return 1;
}

static int setjoystickspeedLua(lua_State *L) {
	joystickSpeed = (float)luaL_checknumber(L, 1);
	if (joystickSpeed <= 0) {
		joystickSpeed = 1.0;
		terminalPrintf("Invalid joystick speed %s - use a positive number", lua_tostring(L, 1));
	}
	return 0;
}

static int savegameLua(lua_State *L) {
	FILE *f;
	char *p;
	int i, w, h;
	Uint32 videoflags;
	CNode *node;
	MouseModeToken token = pushMouseMode(MOUSE_MODE_JOYSTICK);
	SDL_Rect *screen = &screenSize;
	w = screen->w;
	h = screen->h;

	// TODO(pabdulin): check
	Uint32 screen_flags = SDL_GetWindowFlags(sdl2Window);
	videoflags = screen_flags;
	if (videoflags & SDL_WINDOW_FULLSCREEN) {
		terminalClear();
		cleanupGL();
		sdl2Window = SDL_CreateWindow("pipmak",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			640, 480, 
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		SDL_GetWindowSize(sdl2Window, &screenSize.w, &screenSize.h);
	}
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_ShowCursor(SDL_ENABLE);
	p = saveGamePath();
	if (p != NULL) {
		f = fopen(p, "wb");
		if (f != NULL) {
			lua_pushstring(L, "serialize");
			lua_rawget(L, LUA_GLOBALSINDEX);
			lua_newtable(L);
			lua_pushstring(L, "state");
			lua_pushvalue(L, -1);
			lua_rawget(L, LUA_GLOBALSINDEX);
			lua_settable(L, -3);
			lua_pushliteral(L, "pipmak_internal");
			lua_rawget(L, LUA_GLOBALSINDEX);
			lua_pushliteral(L, "project");
			lua_rawget(L, -2);
			lua_pushliteral(L, "title");
			lua_pushvalue(L, -1);
			lua_rawget(L, -3);
			lua_settable(L, -5);
			lua_pop(L, 2); /*project, pipmak_internal*/
			lua_pushstring(L, "path");
			lua_pushstring(L, "pipmak-projectpath");
			lua_rawget(L, LUA_REGISTRYINDEX);
			lua_settable(L, -3);
			lua_pushstring(L, "node");
			if (backgroundCNode == NULL) {
				lua_pushliteral(L, "0");
			}
			else {
				lua_rawgeti(L, LUA_REGISTRYINDEX, backgroundCNode->noderef);
				lua_pushliteral(L, "path"); lua_rawget(L, -2);
				lua_remove(L, -2); /*node*/
			}
			lua_settable(L, -3);
			lua_pushliteral(L, "overlays");
			lua_newtable(L);
			node = (backgroundCNode == NULL) ? NULL : backgroundCNode->next;
			for (i = 1; node != NULL; node = node->next) {
				if (!node->dontsave) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
					lua_pushliteral(L, "path"); lua_rawget(L, -2);
					lua_remove(L, -2); /*node*/
					lua_rawseti(L, -2, i);
					i++;
				}
			}
			lua_rawset(L, -3);
			lua_pushstring(L, "az");
			lua_pushnumber(L, azimuth);
			lua_settable(L, -3);
			lua_pushstring(L, "el");
			lua_pushnumber(L, elevation);
			lua_settable(L, -3);
			lua_pushstring(L, "mouse");
			getmousemodeLua(L);
			lua_settable(L, -3);
			lua_pushstring(L, "w");
			lua_pushnumber(L, w);
			lua_settable(L, -3);
			lua_pushstring(L, "h");
			lua_pushnumber(L, h);
			lua_settable(L, -3);
			lua_pushstring(L, "fov");
			lua_pushnumber(L, verticalFOV);
			lua_settable(L, -3);
			lua_pushstring(L, "full");
			lua_pushboolean(L, videoflags & SDL_WINDOW_FULLSCREEN);
			lua_settable(L, -3);
			lua_pushstring(L, "texfilter");
			lua_pushnumber(L, glTextureFilter);
			lua_settable(L, -3);
			lua_pushstring(L, "joyspeed");
			lua_pushnumber(L, joystickSpeed);
			lua_settable(L, -3);
			if (lua_pcall(L, 1, 1, 0) != 0) {
				terminalPrintf("Error saving game: %s", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
			else {
				fwrite(lua_tostring(L, -1), 1, lua_strlen(L, -1), f);
				lua_pop(L, 1);
			}
			fclose(f);
		}
		else {
			terminalPrintf("Couldn't save game: %s", strerror(errno));
		}
		free(p);
	}
	SDL_ShowCursor(SDL_DISABLE);
	popMouseMode(token);
	if (videoflags & SDL_WINDOW_FULLSCREEN) {
		sdl2Window = SDL_CreateWindow("pipmak",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			w, h,
			videoflags);
		SDL_GetWindowSize(sdl2Window, &screenSize.w, &screenSize.h);
		SDL_SetRelativeMouseMode(SDL_TRUE);
		setupGL();
	}
	return 0;
}

static int opensavedgameLua(lua_State *L) {
	// TODO(pabdulin): check
	Uint32 screen_flags = SDL_GetWindowFlags(sdl2Window);
	if (screen_flags & SDL_WINDOW_FULLSCREEN) setwindowedLua(L);
	disruptiveInstruction = INSTR_OPENSAVEDGAME;
	return 0;
}

static int openprojectLua(lua_State *L) {
	// TODO(pabdulin): check
	Uint32 screen_flags = SDL_GetWindowFlags(sdl2Window);
	if (screen_flags & SDL_WINDOW_FULLSCREEN) setwindowedLua(L);
	disruptiveInstruction = INSTR_OPENPROJECT;
	return 0;
}

static int quitLua(lua_State *L) {
	quit(0);
	return 0;
}

static int pipmakversionLua(lua_State *L) {
	lua_pushnumber(L, PIPMAK_VERSION);
	return 1;
}

static int getverticalfovLua(lua_State *L) {
	lua_pushnumber(L, verticalFOV);
	return 1;
}

static int setverticalfovLua(lua_State *L) {
	verticalFOV = (GLfloat)luaL_checknumber(L, 1);
	if (verticalFOV < 5) {
		verticalFOV = 5;
		terminalPrintf("Invalid FOV %s - use a number between 5 and 95", lua_tostring(L, 1));
	}
	else if (verticalFOV > 95) {
		verticalFOV = 95;
		terminalPrintf("Invalid FOV %s - use a number between 5 and 95", lua_tostring(L, 1));
	}
	updateGLProjection();
	return 0;
}

static int nowLua(lua_State *L) {
	lua_pushnumber(L, (lua_Number)thisRedrawTime/1000);
	return 1;
}

static int saveequirectLua(lua_State *L) {
	SDL_RWops *rw;
	int i, j, f, w, h, k, l;
	const char *s;
	float az, el, x, y;
	SDL_Surface *image;
	SDL_Surface *faces[6];
	if (backgroundCNode == NULL || backgroundCNode->type != NODE_TYPE_CUBIC) {
		terminalPrint("pipmak.saveequirect() only works on cubic panoramas.", 0);
		return 0;
	}
	w = (int)luaL_optnumber(L, 1, 360);
	h = (int)luaL_optnumber(L, 2, 180);
	lua_rawgeti(L, LUA_REGISTRYINDEX, backgroundCNode->noderef);
	for (i = 0; i < 6; i++) {
		lua_rawgeti(L, -1, i+1);
		s = ((Image *)lua_touserdata(L, -1))->path;
		rw = PHYSFSRWOPS_openRead(s);
		if (rw == NULL) {
			terminalPrintf("Error loading face \"%s\": %s", s, SDL_GetError());
			rw = PHYSFSRWOPS_openRead("resources/missing.png");
		}
		image = IMG_Load_RW(rw, 1);
		if (image == NULL) {
			terminalPrintf("Error loading face \"%s\": %s", s, IMG_GetError());
			rw = PHYSFSRWOPS_openRead("resources/missing.png");
			image = IMG_Load_RW(rw, 1);
			if (image == NULL) printf("IMG_Load Error: %s\n", IMG_GetError());
		}
		faces[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, image->w, image->h, 24, BYTEORDER_DEPENDENT_RGB_MASKS);
		if (faces[i] == NULL) {
			terminalPrintf("Could not create RGB surface: %s", SDL_GetError());
		}
		SDL_BlitSurface(image, NULL, faces[i], NULL);
		SDL_FreeSurface(image);
		SDL_LockSurface(faces[i]);
		lua_pop(L, 1); /*image*/
	}
	lua_pop(L, 1); /*node*/
	image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 24, BYTEORDER_DEPENDENT_RGB_MASKS);
	if (image == NULL) {
		terminalPrintf("Could not create RGB surface: %s", SDL_GetError());
		return 0;
	}
	SDL_LockSurface(image);
	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			az = (i + 0.5f)/w*2*(float)M_PI;
			el = ((-0.5f - j)/h + 0.5f)*(float)M_PI;
			f = 0;
			while (az > M_PI/4) {
				az -= (float)M_PI/2;
				f++;
			}
			if (f == 4) f = 0;
			x = tanf(az);
			y = -sqrtf(1 + x*x)*tanf(el);
			if (y < -1) {
				f = 4;
				az = (i + 0.5f)/w*360*(float)M_PI/180;
				x = sinf(az)/tanf(el);
				y = cosf(az)/tanf(el);
			}
			else if (y > 1) {
				f = 5;
				az = (i + 0.5f)/w*360*(float)M_PI/180;
				x = -sinf(az)/tanf(el);
				y = cosf(az)/tanf(el);
			}
			k = (int)floorf((faces[f]->w - 1)*(x + 1)/2 + 0.5f);
			l = (int)floorf((faces[f]->h - 1)*(y + 1)/2 + 0.5f);
			((Uint8*)(image->pixels))[j*image->pitch + 3*i + 0] = ((Uint8*)(faces[f]->pixels))[l*faces[f]->pitch + 3*k + 0];
			((Uint8*)(image->pixels))[j*image->pitch + 3*i + 1] = ((Uint8*)(faces[f]->pixels))[l*faces[f]->pitch + 3*k + 1];
			((Uint8*)(image->pixels))[j*image->pitch + 3*i + 2] = ((Uint8*)(faces[f]->pixels))[l*faces[f]->pitch + 3*k + 2];
		}
	}
	SDL_UnlockSurface(image);
	if (SDL_SaveBMP(image, "equirect.bmp") != 0) {
		terminalPrintf("Could not save BMP file: %s", SDL_GetError());
	}
	SDL_FreeSurface(image);
	for (i = 0; i < 6; i++) {
		SDL_UnlockSurface(faces[i]);
		SDL_FreeSurface(faces[i]);
	}
	return 0;
}

static int setCpy2StdoutLua(lua_State *L) {
	/* Accept any value, just convert it to a boolean. */
	cpTermToStdout = lua_toboolean(L, 1);
	return 0;
}

static const luaL_reg pipmakFuncs[] = {
	{"print", printLua},
	{"printinplace", printinplaceLua},
	{"dofile", dofileLua},
	{"setcursor", setcursorLua},
	{"setstandardcursor", setstandardcursorLua},
	{"getmousemode", getmousemodeLua},
	{"setmousemode", setmousemodeLua},
	{"pushmousemode", pushmousemodeLua},
	{"popmousemode", popmousemodeLua},
	{"clickloc", clicklocLua},
	{"mouseloc", mouselocLua},
	{"mouseminusclickloc", mouseminusclicklocLua},
	{"mouseisdown", mouseisdownLua},
	{"getimage", getimageLua},
	{"loadcursor", getimageLua}, /*getimageLua serves as both pipmak.getimage and pipmak.loadcursor*/
	{"newimage", newimageLua},
	{"flushimagecache", flushimagecacheLua},
	{"getcurrentnode", getcurrentnodeLua},
	{"thisnode", thisnodeLua},
	{"backgroundnode", backgroundnodeLua},
	{"gotonode", gotonodeLua},
	{"overlaynode", overlaynodeLua},
	{"dissolve", dissolveLua},
	{"rotate", rotateLua},
	{"wipe", wipeLua},
	{"setviewdirection", setviewdirectionLua},
	{"getviewdirection", getviewdirectionLua},
	{"screensize", screensizeLua},
	{"setwindowed", setwindowedLua},
	{"setfullscreen", setfullscreenLua},
	{"getscreenmodes", getscreenmodesLua},
	{"desktopsize", desktopsizeLua},
	{"getinterpolation", getinterpolationLua},
	{"setinterpolation", setinterpolationLua},
	{"getshowcontrols", getshowcontrolsLua},
	{"setshowcontrols", setshowcontrolsLua},
	{"getjoystickspeed", getjoystickspeedLua},
	{"setjoystickspeed", setjoystickspeedLua},
	{"savegame", savegameLua},
	{"opensavedgame", opensavedgameLua},
	{"openproject", openprojectLua},
	{"quit", quitLua},
	{"version", pipmakversionLua},
	{"getverticalfov", getverticalfovLua},
	{"setverticalfov", setverticalfovLua},
	{"now", nowLua},
	{"saveequirect", saveequirectLua},
	{"copytermtostdout", setCpy2StdoutLua},
	{NULL, NULL}
};

/* Image methods ----------------------------------------*/

static int imageTostringLua(lua_State *L) {
	Image *m;
	m = (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 1, "image");
	lua_pushfstring(L, "image \"%s\"", m->path);
	return 1;
}

static int imageCollectLua(lua_State *L) {
	Image *m;
	m = (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 1, "image");
	IFDEBUG(printf("debug: image %s collected (texture ID %d)\n", m->path, (int)m->textureID);)
	glDeleteTextures(1, &(m->textureID));
	if (m->texrefcount > 0 && m->cursorHotX == -1 && m->cursorHotY == -1) { /*it's safe to collect cursors - by the time they're collected they can't be in use anymore, even though they still occupy GL textures*/
		terminalPrintf("Pipmak internal error: image %s collected while texture still in use (%d).\nThis is a bug, please report.", m->path, m->texrefcount);
	}
	if (m->data != NULL) {
		if (m->datarefcount > 0) {
			terminalPrintf("Pipmak internal error: image %s collected while data still in use (%d).\nThis is a bug, please report.", m->path, m->datarefcount);
		}
		else SDL_FreeSurface(m->data);
	}
	free(m->path);
	if (m->prev != NULL) m->prev->next = m->next;
	if (m->next != NULL) m->next->prev = m->prev;
	if (m == imageListStart) imageListStart = m->next;
	return 0;
}

static int imageSizeLua(lua_State *L) {
	Image *m;
	m = (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 1, "image");
	lua_pushnumber(L, m->w);
	lua_pushnumber(L, m->h);
	return 2;
}

static int imageColorLua(lua_State *L) {
	Image *m= (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 1, "image");
	if (lua_gettop(L) > 1) {
		m->drawingColor.r = (Uint8)floor(255*luaL_checknumber(L, 2) + 0.5);
		m->drawingColor.g = (Uint8)floor(255*luaL_checknumber(L, 3) + 0.5);
		m->drawingColor.b = (Uint8)floor(255*luaL_checknumber(L, 4) + 0.5);
		m->drawingColor.a = (Uint8)floor(255*luaL_optnumber(L, 5, 1.0) + 0.5);
		lua_settop(L, 1);
		return 1;
	}
	else {
		lua_pushnumber(L, m->drawingColor.r*(1.0/255));
		lua_pushnumber(L, m->drawingColor.g*(1.0/255));
		lua_pushnumber(L, m->drawingColor.b*(1.0/255));
		lua_pushnumber(L, m->drawingColor.a*(1.0/255));
		return 4;
	}
}

static int imageFillLua(lua_State *L) {
	Image *m;
	SDL_Rect rect;
	
	m = (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 1, "image");
	if (m->isFile) luaL_error(L, "can't modify image obtained from pipmak.getimage() - use pipmak.newimage() to create a modifiable image");
	if (m->bpp == 8) luaL_error(L, "fill is not supported on indexed images");
	if (lua_gettop(L) > 1) {
		rect.x = luaL_checkint(L, 2);
		rect.y = luaL_checkint(L, 3);
		rect.w = luaL_checkint(L, 4);
		rect.h = luaL_checkint(L, 5);
	}
	else {
		rect.x = 0;
		rect.y = 0;
		rect.w = m->w;
		rect.h = m->h;
	}
	getImageData(m, 1);
	SDL_FillRect(m->data, &rect, SDL_MapRGBA(m->data->format, m->drawingColor.r, m->drawingColor.g, m->drawingColor.b, m->drawingColor.a));
	
	if (m->textureID != 0) {
		glBindTexture(glTextureTarget, m->textureID);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, m->w);
		glTexSubImage2D(glTextureTarget, 0, rect.x, rect.y, rect.w, rect.h, (m->bpp == 32) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, (Uint8*)m->data->pixels + rect.y * m->data->pitch + rect.x * m->bpp/8);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
	
	lua_settop(L, 1);
	return 1;
}

static int imageDrawtextLua(lua_State *L) {
	Image *m;
	const char *t;
	SDL_Surface *textsurf;
	SDL_Color dummyColor = {0, 0, 0, 0};
	int x, y;
	int minx, miny, maxx, maxy;
	int srcrow = 0, srccol = 0, dstrow, dstcol;
	int ta, ba = 255, a = 255*255; /*the default values are used in the 24-bit case*/
	Uint8 *dst;
	char lineBuffer[300];
	char* bp;
	TTF_Font *font = NULL;
	enum { LEFT = 0, RIGHT = 1, CENTER = 4 } align; /*corresponds to pipmak.left, .right, .center*/
	
	m = (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 1, "image");
	if (m->isFile) luaL_error(L, "can't modify image obtained from pipmak.getimage() - use pipmak.newimage() to create a modifiable image");
	if (m->bpp == 8) luaL_error(L, "drawtext is not supported on indexed images");
	x = luaL_checkint(L, 2);
	y = luaL_checkint(L, 3);
	t = luaL_checkstring(L, 4);
	if (lua_isnumber(L, 5)) {
		align = lua_tonumber(L, 5);
	}
	else if (lua_gettop(L) > 4) {
		SDL_RWops *rw;
		int size = luaL_optint(L, 6, 11);
		align = luaL_optint(L, 7, LEFT);
		lua_settop(L, 5);
		absolutizePath(L, 0);
		rw = PHYSFSRWOPS_openRead(lua_tostring(L, -1));
		if (rw == NULL) {
			terminalPrintf("Error opening font file \"%s\": %s", lua_tostring(L, -1), SDL_GetError());
		}
		else {
			font = TTF_OpenFontRW(rw, 1, size);
			if (font == NULL) {
				terminalPrintf("Error opening font file \"%s\": %s", lua_tostring(L, -1), TTF_GetError());
			}
		}
	}
	else align = LEFT;
	if (font == NULL) font = verafont;
	if (align != LEFT && align != RIGHT && align != CENTER) {
		terminalPrintf("Invalid text alignment %d - use one of pipmak.left, pipmak.right, pipmak.center", align);
		align = LEFT;
	}
	minx = maxx = x;
	miny = maxy = y;
	
	while (*t != '\0') {
		bp = lineBuffer;
		while (*t != '\r' && *t != '\n' && *t != '\0' && bp < lineBuffer + sizeof(lineBuffer) - 1) *(bp++) = *(t++);
		*bp = '\0';
		if (*t == '\r') t++;
		if (*t == '\n') t++;
		if (*lineBuffer != '\0') {
			
			/*SDL can't do real RGBA->RGBA blits, so we use TTF_RenderUTF8_Shaded instead of ..._Blended and do the blitting ourselves*/
			textsurf = TTF_RenderUTF8_Shaded(font, lineBuffer, dummyColor, dummyColor);
			if (textsurf == NULL) luaL_error(L, "error rendering text: %s", TTF_GetError());
			assert(textsurf->format->palette->ncolors == 256); /*we're relying on an implementation detail of SDL_ttf here, I suppose...*/
			getImageData(m, 1);
			
			maxy = y + textsurf->h;
			switch (align) {
				case LEFT:
					if (maxx < x + textsurf->w) maxx = x + textsurf->w;
					break;
				case RIGHT:
					if (minx > x - textsurf->w) minx = x - textsurf->w;
					break;
				case CENTER:
					if (minx > x - textsurf->w/2) minx = x - textsurf->w/2;
					if (maxx < x + textsurf->w/2) maxx = x + textsurf->w/2;
					break;
			}
			
			if (y < 0) {
				srcrow = -y;
				dstrow = 0;
			}
			else {
				srcrow = 0;
				dstrow = y;
			}
			for (; srcrow < textsurf->h && dstrow < m->data->h; srcrow++, dstrow++) {
				dstcol = x;
				switch (align) {
					case LEFT: break;
					case RIGHT: dstcol -= textsurf->w; break;
					case CENTER: dstcol -= textsurf->w/2; break;
				}
				if (dstcol < 0) {
					srccol = -dstcol;
					dstcol = 0;
				}
				else {
					srccol = 0;
				}
				for (; srccol < textsurf->w && dstcol < m->data->w; srccol++, dstcol++) {
					dst = (Uint8*)m->data->pixels + dstrow*m->data->pitch + dstcol*m->bpp/8;
					ta = ((int)*((Uint8*)textsurf->pixels + srcrow*textsurf->pitch + srccol) * m->drawingColor.a + 127)/255;
					if (m->bpp == 32) {
						ba = *(dst + 3);
						a = 255*(ta + ba) - ta*ba;
						*(dst + 3) = (a + 127)/255;
					}
					if (a != 0) {
						*dst = (*dst*(255 - ta)*ba + m->drawingColor.r*ta*255 + a/2)/a;
						dst++;
						*dst = (*dst*(255 - ta)*ba + m->drawingColor.g*ta*255 + a/2)/a;
						dst++;
						*dst = (*dst*(255 - ta)*ba + m->drawingColor.b*ta*255 + a/2)/a;
					}
					else {
						/*Set these pixels' color even though they're fully transparent to get rid of the background-colored fringes around the text that occur when the image is magnified with bilinear interpolation (because OpenGL bilinearly interpolates color and alpha independently, which is not strictly correct). This introduces foreground-colored fringes where alpha changes to/from 0 in the background content instead, but that's hopefully a less common situation.*/
						*dst++ = m->drawingColor.r;
						*dst++ = m->drawingColor.g;
						*dst = m->drawingColor.b;
					}
				}
			}
			SDL_FreeSurface(textsurf);
			
			y += TTF_FontLineSkip(font);
		}
	}
	
	if (font != verafont) TTF_CloseFont(font);
	
	if (minx < 0) minx = 0; else if (minx > m->w) minx = m->w;
	if (maxx < 0) maxx = 0; else if (maxx > m->w) maxx = m->w;
	if (miny < 0) miny = 0; else if (miny > m->h) miny = m->h;
	if (maxy < 0) maxy = 0; else if (maxy > m->h) maxy = m->h;
	
	if (m->textureID != 0) {
		glBindTexture(glTextureTarget, m->textureID);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, m->w);
		glTexSubImage2D(glTextureTarget, 0, minx, miny, maxx-minx, maxy-miny, (m->bpp == 32) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, (Uint8*)m->data->pixels + miny * m->data->pitch + minx * m->bpp/8);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
	
	lua_settop(L, 1);
	return 1;
}

static int imageDrawimageLua(lua_State *L) {
	Image *m, *srcimg;
	SDL_Rect srcrect, dstrect;
	
	m = (Image*)luaL_checkudata(L, 1, "pipmak-image");
	if (m == NULL) luaL_typerror(L, 1, "image");
	if (m->isFile) luaL_error(L, "can't modify image obtained from pipmak.getimage() - use pipmak.newimage() to create a modifiable image");
	if (m->bpp == 8) luaL_error(L, "drawimage is not supported on indexed images");
	dstrect.x = luaL_checkint(L, 2);
	dstrect.y = luaL_checkint(L, 3);
	if (lua_type(L, 4) == LUA_TSTRING) {
		lua_pushcfunction(L, getimageLua);
		lua_pushvalue(L, 4);
		lua_call(L, 1, 1);
		lua_replace(L, 4);
	}
	srcimg = (Image*)luaL_checkudata(L, 4, "pipmak-image");
	if (srcimg == NULL) luaL_typerror(L, 4, "image");
	if (lua_gettop(L) > 4) {
		srcrect.x = luaL_checkint(L, 5);
		srcrect.y = luaL_checkint(L, 6);
		srcrect.w = luaL_checkint(L, 7);
		srcrect.h = luaL_checkint(L, 8);
	}
	else {
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = srcimg->w;
		srcrect.h = srcimg->h;
	}
	
	getImageData(m, 1);
	getImageData(srcimg, 1);
	SDL_SetAlpha(srcimg->data, 0, 255); /*copy instead of composite, because SDL can't do RGBA->RGBA blitting, and the RGBA->RGB blitting it does instead seems more confusing than useful*/
	SDL_BlitSurface(srcimg->data, &srcrect, m->data, &dstrect);
	
	if (m->textureID != 0) {
		glBindTexture(glTextureTarget, m->textureID);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, m->w);
		glTexSubImage2D(glTextureTarget, 0, dstrect.x, dstrect.y, dstrect.w, dstrect.h, (m->bpp == 32) ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, (Uint8*)m->data->pixels + dstrect.y * m->data->pitch + dstrect.x * m->bpp/8);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
	
	lua_settop(L, 1);
	return 1;
}

static const luaL_reg imageFuncs[] = {
	{"__tostring", imageTostringLua},
	{"__gc", imageCollectLua},
	{"size", imageSizeLua},
	{"color", imageColorLua},
	{"fill", imageFillLua},
	{"drawtext", imageDrawtextLua},
	{"drawimage", imageDrawimageLua},
	{NULL, NULL}
};

/* Node methods -----------------------------------------*/

static int nodeCloseoverlayLua(lua_State *L) {
	CNode *node;
	checkinstanceof(L, 1, "pipmak-node", "node");
	lua_pushliteral(L, "cnode");
	lua_rawget(L, -2);
	node = lua_touserdata(L, -1);
	if (node != NULL && node != backgroundCNode && node->entered) {
		leaveNode(node);
		if (touchedNode == NULL) updateTouchedNode();
	}
	return 0;
}

static int nodeMessageLua(lua_State *L) {
	CNode *oldCNode = thisCNode;
	checkinstanceof(L, 1, "pipmak-node", "node");
	lua_pushliteral(L, "cnode"); lua_rawget(L, 1); thisCNode = lua_touserdata(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "messages");
	lua_rawget(L, 1);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (lua_isfunction(L, -1)) {
		/*Lua stack: node, message, arguments, messages, function*/
		lua_insert(L, 3); lua_pop(L, 1);
		/*Lua stack: node, message, function, arguments*/
		lua_call(L, lua_gettop(L) - 3, LUA_MULTRET);
		lua_remove(L, 1); /*node*/
		lua_remove(L, 1); /*message*/
	}
	else {
		lua_settop(L, 0);
	}
	thisCNode = oldCNode;
	return lua_gettop(L);
}

static int nodeSetstandardcursorLua(lua_State * L) {
	CNode *node;
	checkinstanceof(L, 1, "pipmak-node", "node");
	lua_pushliteral(L, "cnode"); lua_rawget(L, 1); node = lua_touserdata(L, -1); lua_pop(L, 1);
	node->standardCursor = checkCursor(L, 2);
	return 0;
}

static const luaL_reg nodeFuncs[] = {
	{"closeoverlay", nodeCloseoverlayLua},
	{"message", nodeMessageLua},
	{"setstandardcursor", nodeSetstandardcursorLua},
	{NULL, NULL}
};


/*---------------------------------------------------------*/

/* does not leave any tables on the Lua stack, unlike Lua's standard libraries */
void luaopen_pipmak(lua_State *L) {
	luaL_openlib(L, "pipmak", pipmakFuncs, 0); lua_pop(L, 1);
	luaL_openlib(L, "pipmak", pipmakAudioFuncs, 0); lua_pop(L, 1);
	
	luaL_openlib(L, "pipmak_internal", pipmakInternalFuncs, 0); lua_pop(L, 1);
	luaL_openlib(L, "pipmak_internal", pipmakInternalTexteditorFuncs, 0);
	
	luaL_newmetatable(L, "pipmak-control");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_pushliteral(L, "meta_control");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1); /*control metatable*/
	
	luaL_newmetatable(L, "pipmak-patch");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_pushliteral(L, "meta_patch");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1); /*patch metatable*/
	
	luaL_newmetatable(L, "pipmak-image");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	luaL_openlib(L, NULL, imageFuncs, 0);
	lua_pushliteral(L, "meta_image");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1); /*image metatable*/
	
	luaL_newmetatable(L, "pipmak-sound");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	luaL_openlib(L, NULL, soundFuncs, 0);
	lua_pushliteral(L, "meta_sound");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1); /*sound metatable*/
	
	luaL_newmetatable(L, "pipmak-texteditor");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	luaL_openlib(L, NULL, texteditorFuncs, 0);
	lua_pushliteral(L, "meta_texteditor");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	lua_pop(L, 1); /*texteditor metatable*/
	
	luaL_newmetatable(L, "pipmak-node");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	luaL_openlib(L, NULL, nodeFuncs, 0);
	lua_pushliteral(L, "meta_node");
	lua_pushvalue(L, -2);
	lua_rawset(L, -4);
	/*Lua Stack: pipmak_internal, node metatable*/
	
	luaL_newmetatable(L, "pipmak-panel");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_pushvalue(L, -2);
	lua_setmetatable(L, -2);
	lua_pushliteral(L, "meta_panel");
	lua_pushvalue(L, -2);
	lua_rawset(L, -5);
	lua_pop(L, 1); /*panel metatable*/
	
	lua_pop(L, 1); /*node metatable*/
	
	lua_pop(L, 1); /*pipmak_internal*/
}
