/*
 
 hotspotmapEquirect.c, part of the Pipmak Game Engine
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

/* $Id: hotspotmapEquirect.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "nodes.h"

#include <math.h>

#include "lua.h"
#include "lauxlib.h"

#include "terminal.h"

extern lua_State *L;
extern GLenum glTextureTarget;


/* pops an equirectangular hotspotmap from the Lua stack */
static void init(CHotspotmap *map) {
	Image *image;
	lua_rawgeti(L, -1, 1);
	/*Lua stack: ..., hotspotmap, image*/
	image = lua_touserdata(L, -1);
	map->private.equirect.w = image->w;
	map->private.equirect.h = image->h;
	if (image->bpp != 8) {
		terminalPrintf("Warning: hotspot map \"%s\" will not work because it is not 8 bit per pixel (256 colors, indexed)", image->path);
	}
	map->private.equirect.data = getImageData(image, 0);
	image->datarefcount++;
	map->private.equirect.imageref = luaL_ref(L, LUA_REGISTRYINDEX); /*pops the image*/
	lua_pop(L, 1); /*hotspotmap*/
}

static void finalize(CHotspotmap *map) {
	Image *image;
	lua_rawgeti(L, LUA_REGISTRYINDEX, map->private.equirect.imageref);
	image = (Image *)lua_touserdata(L, -1);
	if (image != NULL) image->datarefcount--;
	lua_pop(L, 1);
	luaL_unref(L, LUA_REGISTRYINDEX, map->private.equirect.imageref);
}

static int getHotspot(CHotspotmap *map, float normH, float normV) {
	int t, u;
	t = (int)(normH*map->private.equirect.w);
	if (t < 0) t = 0;
	else if (t >= map->private.equirect.w) t = map->private.equirect.w - 1;
	u = (int)((0.5f - normV)*map->private.equirect.h);
	if (u < 0) u = 0;
	else if (u >= map->private.equirect.h) u = map->private.equirect.h - 1;
	return map->private.equirect.data[u*4*((map->private.equirect.w + 3)/4) + t];
}

static void feedToGL(CHotspotmap *map) {
	Image *img;
	GLfloat s, t1, t2;
	lua_rawgeti(L, LUA_REGISTRYINDEX, map->private.equirect.imageref);
	img = feedImageToGL(NULL);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	map->private.equirect.displayList = glGenLists(1);
	glNewList(map->private.equirect.displayList, GL_COMPILE);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(glTextureTarget, img->textureID);
	for (t1 = 0; t1 < 1 - 0.05f/2; t1 += 0.05f) {
		t2 = t1 + 0.05f;
		glBegin(GL_TRIANGLE_STRIP);
		for (s = 0; s < 1 - 1.0f/180/2; s += 1.0f/180) {
			glTexCoord2f(s*(float)(img->w)/img->textureWidth, t1*(float)(img->h)/img->textureHeight);
			glVertex3f(sinf(t1*(float)M_PI)*sinf(s*2*(float)M_PI), cosf(t1*(float)M_PI), -sinf(t1*(float)M_PI)*cosf(s*2*(float)M_PI));
			glTexCoord2f(s*(float)(img->w)/img->textureWidth, t2*(float)(img->h)/img->textureHeight);
			glVertex3f(sinf(t2*(float)M_PI)*sinf(s*2*(float)M_PI), cosf(t2*(float)M_PI), -sinf(t2*(float)M_PI)*cosf(s*2*(float)M_PI));
		}
		glTexCoord2f((float)(img->w)/img->textureWidth, t1*(float)(img->h)/img->textureHeight);
		glVertex3f(0, cosf(t1*(float)M_PI), -sinf(t1*(float)M_PI));
		glTexCoord2f((float)(img->w)/img->textureWidth, t2*(float)(img->h)/img->textureHeight);
		glVertex3f(0, cosf(t2*(float)M_PI), -sinf(t2*(float)M_PI));
		glEnd();
		/*debug: show wireframe*/
		/*glDisable(glTextureTarget);
		glDisable(GL_TEXTURE_2D);
		glColor4f(0, 0, 0, .3);
		glBegin(GL_LINE_LOOP);
		for (s = 0; s < 1 - 1.0f/180/2; s += 1.0f/180) {
			glVertex3f(sin(t1*M_PI)*sin(s*2*M_PI), cos(t1*M_PI), -sin(t1*M_PI)*cos(s*2*M_PI));
			glVertex3f(sin(t2*M_PI)*sin(s*2*M_PI), cos(t2*M_PI), -sin(t2*M_PI)*cos(s*2*M_PI));
		}
		glEnd();
		glBegin(GL_LINE_LOOP);
		for (s = 0; s < 1 - 1.0f/180/2; s += 1.0f/180) {
			glVertex3f(sin(t1*M_PI)*sin(s*2*M_PI), cos(t1*M_PI), -sin(t1*M_PI)*cos(s*2*M_PI));
		}
		glEnd();
		glEnable(glTextureTarget);
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0, 1.0, 1.0, 1.0);*/
	}
	glEndList();
	
	lua_pop(L, 1); /*image*/
}

static void removeFromGL(CHotspotmap *map) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, map->private.equirect.imageref);
	releaseImageFromGL((Image *)lua_touserdata(L, -1));
	lua_pop(L, 1);
	glDeleteLists(map->private.equirect.displayList, 1);
}

static void draw(CHotspotmap *map) {
	glCallList(map->private.equirect.displayList);
}

static void setPixels(struct CHotspotmap *map, float normH, float normV, float radius, Uint8 value) {
	int t, u, mint, minu, maxt, maxu, t1, t2;
	float f;
	Image *img;
	
	if (radius == 0) { /*fill one pixel*/
		t = (int)(normH*map->private.equirect.w);
		if (t < 0) t = 0;
		else if (t >= map->private.equirect.w) t = map->private.equirect.w - 1;
		u = (int)((0.5f - normV)*map->private.equirect.h);
		if (u < 0) u = 0;
		else if (u >= map->private.equirect.h) u = map->private.equirect.h - 1;
		map->private.equirect.data[u*4*((map->private.equirect.w + 3)/4) + t] = value;
		mint = maxt = t;
		minu = maxu = u;
	}
	else { /*let's scan convert a circle on the sphere in spherical coordinates*/
		radius /= 180;
		minu = (int)ceilf((0.5f - normV - radius)*map->private.equirect.h - 0.5f);
		if (minu < 0) minu = 0;
		maxu = (int)floorf((0.5f - normV + radius)*map->private.equirect.h - 0.5f);
		if (maxu >= map->private.equirect.h) maxu = map->private.equirect.h - 1;
		mint = maxt = (int)(normH*map->private.equirect.w);
		for (u = minu; u <= maxu; u++) {
			f = (0.5f - (u + 0.5f)/map->private.equirect.h)*(float)M_PI; /*elevation of the current scanline in radians*/
			f = (cosf(radius*(float)M_PI) - sinf(f)*sinf(normV*(float)M_PI))/(cosf(f)*cosf(normV*(float)M_PI)); /*cosine of the half width of the current span*/
			if (f < -1) { /*span covers the full range*/
				t1 = 0;
				t2 = map->private.equirect.w - 1;
			}
			else {
				f = acosf(f)*map->private.equirect.w/(2*(float)M_PI); /*half width of the current span in pixels*/
				t1 = (int)ceilf(normH*map->private.equirect.w - f - 0.5f);
				t2 = (int)floorf(normH*map->private.equirect.w + f - 0.5f);
			}
			if (t1 < mint) mint = t1;
			if (t2 > maxt) maxt = t2;
			for (t = t1; t <= t2; t++) {
				map->private.equirect.data[u*4*((map->private.equirect.w + 3)/4) + (t + map->private.equirect.w)%map->private.equirect.w] = value;
			}
		}
	}
	
	/*if the image is in a GL texture, update it within the bounding box of the touched pixels*/
	lua_rawgeti(L, LUA_REGISTRYINDEX, map->private.equirect.imageref);
	img = lua_touserdata(L, -1);
	if (img->textureID != 0) {
		if (mint < 0 || maxt >= map->private.equirect.w) {
			mint = 0;
			maxt = map->private.equirect.w - 1;
		}
		glBindTexture(glTextureTarget, img->textureID);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, (img->w + 3) & ~3);
		glTexSubImage2D(glTextureTarget, 0, mint, minu, maxt - mint + 1, maxu - minu + 1, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, &(map->private.equirect.data[minu*4*((map->private.equirect.w + 3)/4) + mint]));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
	lua_pop(L, 1); /*image*/
}

void makeEquirectCHotspotmap(CHotspotmap *map) {
	map->init = init;
	map->finalize = finalize;
	map->getHotspot = getHotspot;
	map->feedToGL = feedToGL;
	map->removeFromGL = removeFromGL;
	map->draw = draw;
	map->setPixels = setPixels;
}
