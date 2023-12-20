/*
 
 hotspotmapSlide.c, part of the Pipmak Game Engine
 Copyright (c) 2004-2006 Christian Walther
 
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

/* $Id: hotspotmapSlide.c 65 2006-03-27 14:52:31Z cwalther $ */

#include "nodes.h"

#include "lua.h"
#include "lauxlib.h"

#include "terminal.h"


extern lua_State *L;
extern GLenum glTextureTarget;


/* pops a slide hotspotmap from the Lua stack */
static void init(CHotspotmap *map) {
	Image *image;
	lua_rawgeti(L, -1, 1);
	/*Lua stack: ..., hotspotmap, image*/
	image = lua_touserdata(L, -1);
	map->private.slide.w = image->w;
	map->private.slide.h = image->h;
	if (image->bpp != 8) {
		terminalPrintf("Warning: hotspot map \"%s\" will not work because it is not 8 bit per pixel (256 colors, indexed)", image->path);
	}
	map->private.slide.data = getImageData(image, 0);
	image->datarefcount++;
	map->private.slide.imageref = luaL_ref(L, LUA_REGISTRYINDEX); /*pops the image*/
	lua_pop(L, 1); /*hotspotmap*/
}

static void finalize(CHotspotmap *map) {
	Image *image;
	lua_rawgeti(L, LUA_REGISTRYINDEX, map->private.slide.imageref);
	image = (Image *)lua_touserdata(L, -1);
	if (image != NULL) image->datarefcount--;
	lua_pop(L, 1);
	luaL_unref(L, LUA_REGISTRYINDEX, map->private.slide.imageref);
}

static int getHotspot(CHotspotmap *map, float normH, float normV) {
	int t, u;
	t = (int)(normH*map->private.slide.w);
	if (t < 0) t = 0;
	else if (t >= map->private.slide.w) t = map->private.slide.w - 1;
	u = (int)(normV*map->private.slide.h);
	if (u < 0) u = 0;
	else if (u >= map->private.slide.h) u = map->private.slide.h - 1;
	return map->private.slide.data[u*4*((map->private.slide.w + 3)/4) + t];
}

static void feedToGL(CHotspotmap *map) {
	Image *img;
	lua_rawgeti(L, LUA_REGISTRYINDEX, map->private.slide.imageref);
	img = feedImageToGL(NULL);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	map->private.slide.displayList = glGenLists(1);
	glNewList(map->private.slide.displayList, GL_COMPILE);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(glTextureTarget, img->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(0, (float)(img->h)/img->textureHeight);
	glVertex2f(0, 1);
	glTexCoord2f((float)(img->w)/img->textureWidth, (float)(img->h)/img->textureHeight);
	glVertex2f(1, 1);
	glTexCoord2f((float)(img->w)/img->textureWidth, 0);
	glVertex2f(1, 0);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glEnd();
	glEndList();
	
	lua_pop(L, 1); /*image*/
}

static void removeFromGL(CHotspotmap *map) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, map->private.slide.imageref);
	releaseImageFromGL((Image *)lua_touserdata(L, -1));
	lua_pop(L, 1);
	glDeleteLists(map->private.slide.displayList, 1);
}

static void draw(CHotspotmap *map) {
	glCallList(map->private.slide.displayList);
}

static void setPixels(struct CHotspotmap *map, float normH, float normV, float radius, Uint8 value) {
	
}

void makeSlideCHotspotmap(CHotspotmap *map) {
	map->init = init;
	map->finalize = finalize;
	map->getHotspot = getHotspot;
	map->feedToGL = feedToGL;
	map->removeFromGL = removeFromGL;
	map->draw = draw;
	map->setPixels = setPixels;
}
