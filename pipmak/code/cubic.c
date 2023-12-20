/*
 
 cubic.c, part of the Pipmak Game Engine
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

/* $Id: cubic.c 230 2011-08-17 19:07:41Z cwalther $ */

#include "nodes.h"

#include <math.h>
#include <stdlib.h>

#include "lua.h"

#include "terminal.h"
#include "misc.h"


extern lua_State *L;
extern GLint glTextureFilter;
extern GLenum glTextureTarget;
extern Uint8 controlColorPalette[256][3];
extern GLfloat azimuth, elevation, minaz, maxaz, minel, maxel;
extern int showControls;


static void init(CNode *node) {
	node->private.cubic.patches = NULL;
}

static void finalize(CNode *node) {
}

/*update the internal representation of the patch on top of the Lua stack*/
static void updatePatch(CNode *node) {
	if (node->private.cubic.patches != NULL) {
		int face;
		Image *bimg;
		CFlatPatch *p;
		
		lua_pushliteral(L, "id"); lua_rawget(L, -2); p = &(node->private.cubic.patches[(int)lua_tonumber(L, -1) - 1]); lua_pop(L, 1);
	
		lua_pushliteral(L, "face"); lua_rawget(L, -2); face = (int)lua_tonumber(L, -1); lua_pop(L, 1);
		if (face < 1 || face > 6) face = 1;
	
		lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
		lua_rawgeti(L, -1, face);
		bimg = lua_touserdata(L, -1);
		lua_pop(L, 2); /*image, node*/
		
		updateFlatPatch(p, face, 2.0f/(bimg->w - 1), 2.0f/(bimg->h - 1), -1 - 1.0f/(bimg->w - 1), 1 + 1.0f/(bimg->h - 1), -1.0f, 0.49f);
	}
}

static void feedToGL(CNode *node) {
	Image *faces[6];
	int i;
	int flip;
	float s1, s2, t1, t2, t;
	
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	
	lua_pushliteral(L, "minaz"); lua_rawget(L, -2); s1 = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "maxaz"); lua_rawget(L, -2); s2 = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "minel"); lua_rawget(L, -2); t1 = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "maxel"); lua_rawget(L, -2); t2 = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	while (s1 < 360) { s1 += 360; s2 += 360; }
	while (s1 >= 720) { s1 -= 360; s2 -= 360; }
	while (s2 < s1) s2 += 360;
	while (s2 > s1 + 360) s2 -= 360;
	minaz = s1;
	maxaz = s2;
	if (t1 < -90) t1 = -90;
	else if (t1 > 90) t1 = 90;
	if (t2 < t1) t2 = t1;
	else if (t2 > 90) t2 = 90;
	minel = t1;
	maxel = t2;
	
	lua_pushliteral(L, "flip"); lua_rawget(L, -2); flip = (int)lua_tonumber(L, -1); lua_pop(L, 1);
	
	/*get background images, feed them to gl, set glTextureFilter*/
	for (i = 0; i < 6; i++) {
		lua_rawgeti(L, -1, i+1);
		/*Lua stack: node, image*/
		faces[i] = feedImageToGL(NULL);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, glTextureFilter);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, glTextureFilter);
		lua_pop(L, 1); /*image*/ /*this is safe, the image won't be collected as long as it's referenced in node->noderef*/
	}
	
	/*make display list with background*/
	node->private.cubic.displayList = glGenLists(1);
	glNewList(node->private.cubic.displayList, GL_COMPILE);
	glColor4f(1.0, 1.0, 1.0, 1.0);
			
	/*front*/
	s1 = 0.5f/faces[0]->textureWidth;
	s2 = (faces[0]->w - 0.5f)/faces[0]->textureWidth;
	if ((flip >> 0) & 1) {
		t = s1; s1 = s2; s2 = t;
	}
	t1 = 0.5f/faces[0]->textureHeight;
	t2 = (faces[0]->h - 0.5f)/faces[0]->textureHeight;
	if ((flip >> 1) & 1) {
		t = t1; t1 = t2; t2 = t;
	}
	glBindTexture(glTextureTarget, faces[0]->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(s1, t2);
	glVertex3f(-1, -1, -1);
	glTexCoord2f(s2, t2);
	glVertex3f(1, -1, -1);
	glTexCoord2f(s2, t1);
	glVertex3f(1, 1, -1);
	glTexCoord2f(s1, t1);
	glVertex3f(-1, 1, -1);
	glEnd();
	
	/*right*/
	s1 = 0.5f/faces[1]->textureWidth;
	s2 = (faces[1]->w - 0.5f)/faces[1]->textureWidth;
	if ((flip >> 2) & 1) {
		t = s1; s1 = s2; s2 = t;
	}
	t1 = 0.5f/faces[1]->textureHeight;
	t2 = (faces[1]->h - 0.5f)/faces[1]->textureHeight;
	if ((flip >> 3) & 1) {
		t = t1; t1 = t2; t2 = t;
	}
	glBindTexture(glTextureTarget, faces[1]->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(s1, t2);
	glVertex3f(1, -1, -1);
	glTexCoord2f(s2, t2);
	glVertex3f(1, -1, 1);
	glTexCoord2f(s2, t1);
	glVertex3f(1, 1, 1);
	glTexCoord2f(s1, t1);
	glVertex3f(1, 1, -1);
	glEnd();
	
	/*back*/
	s1 = 0.5f/faces[2]->textureWidth;
	s2 = (faces[2]->w - 0.5f)/faces[2]->textureWidth;
	if ((flip >> 4) & 1) {
		t = s1; s1 = s2; s2 = t;
	}
	t1 = 0.5f/faces[2]->textureHeight;
	t2 = (faces[2]->h - 0.5f)/faces[2]->textureHeight;
	if ((flip >> 5) & 1) {
		t = t1; t1 = t2; t2 = t;
	}
	glBindTexture(glTextureTarget, faces[2]->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(s1, t2);
	glVertex3f(1, -1, 1);
	glTexCoord2f(s2, t2);
	glVertex3f(-1, -1, 1);
	glTexCoord2f(s2, t1);
	glVertex3f(-1, 1, 1);
	glTexCoord2f(s1, t1);
	glVertex3f(1, 1, 1);
	glEnd();
	
	/*left*/
	s1 = 0.5f/faces[3]->textureWidth;
	s2 = (faces[3]->w - 0.5f)/faces[3]->textureWidth;
	if ((flip >> 6) & 1) {
		t = s1; s1 = s2; s2 = t;
	}
	t1 = 0.5f/faces[3]->textureHeight;
	t2 = (faces[3]->h - 0.5f)/faces[3]->textureHeight;
	if ((flip >> 7) & 1) {
		t = t1; t1 = t2; t2 = t;
	}
	glBindTexture(glTextureTarget, faces[3]->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(s1, t2);
	glVertex3f(-1, -1, 1);
	glTexCoord2f(s2, t2);
	glVertex3f(-1, -1, -1);
	glTexCoord2f(s2, t1);
	glVertex3f(-1, 1, -1);
	glTexCoord2f(s1, t1);
	glVertex3f(-1, 1, 1);
	glEnd();
	
	/*top*/
	s1 = 0.5f/faces[4]->textureWidth;
	s2 = (faces[4]->w - 0.5f)/faces[4]->textureWidth;
	if ((flip >> 8) & 1) {
		t = s1; s1 = s2; s2 = t;
	}
	t1 = 0.5f/faces[4]->textureHeight;
	t2 = (faces[4]->h - 0.5f)/faces[4]->textureHeight;
	if ((flip >> 9) & 1) {
		t = t1; t1 = t2; t2 = t;
	}
	glBindTexture(glTextureTarget, faces[4]->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(s1, t2);
	glVertex3f(-1, 1, -1);
	glTexCoord2f(s2, t2);
	glVertex3f(1, 1, -1);
	glTexCoord2f(s2, t1);
	glVertex3f(1, 1, 1);
	glTexCoord2f(s1, t1);
	glVertex3f(-1, 1, 1);
	glEnd();
	
	/*bottom*/
	s1 = 0.5f/faces[5]->textureWidth;
	s2 = (faces[5]->w - 0.5f)/faces[5]->textureWidth;
	if ((flip >> 10) & 1) {
		t = s1; s1 = s2; s2 = t;
	}
	t1 = 0.5f/faces[5]->textureHeight;
	t2 = (faces[5]->h - 0.5f)/faces[5]->textureHeight;
	if ((flip >> 11) & 1) {
		t = t1; t1 = t2; t2 = t;
	}
	glBindTexture(glTextureTarget, faces[5]->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(s1, t2);
	glVertex3f(-1, -1, 1);
	glTexCoord2f(s2, t2);
	glVertex3f(1, -1, 1);
	glTexCoord2f(s2, t1);
	glVertex3f(1, -1, -1);
	glTexCoord2f(s1, t1);
	glVertex3f(-1, -1, -1);
	glEnd();
	
	glEndList();
	
	
	/*feed patch images to gl and fill in aggregate arrays for patches*/
	node->private.cubic.patches = malloc(node->npatches*sizeof(CFlatPatch));
	if (node->private.cubic.patches == NULL) {
		terminalPrint("Out of memory.", 0);
	}
	else {
		lua_pushliteral(L, "patches");
		lua_rawget(L, -2);
		/*Lua stack: node, patches*/
		for (i = 0; i < node->npatches; i++) {
			node->private.cubic.patches[i].image = NULL;
			lua_rawgeti(L, -1, i+1);
			updatePatch(node);
			lua_pop(L, 1); /*patch*/
		}
		lua_pop(L, 1); /*patches*/
	}
	lua_pop(L, 1); /*node*/
}

static void removeFromGL(CNode *node) {
	int i;
	Image *img;
	glDeleteLists(node->private.cubic.displayList, 1);
	for (i = 0; i < node->npatches; i++) releaseImageFromGL(node->private.cubic.patches[i].image);
	free(node->private.cubic.patches);
	node->private.cubic.patches = NULL;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	for (i = 0; i < 6; i++) {
		lua_rawgeti(L, -1, i+1);
		/*Lua stack: node, image*/
		img = lua_touserdata(L, -1);
		if (img != NULL) releaseImageFromGL(img);
		lua_pop(L, 1); /*image*/
	}
	lua_pop(L, 1); /*node*/
}

static void draw(CNode *node) {
	int i;
	glLoadIdentity(); /*Camera View*/
	glRotatef(-elevation, 1, 0, 0);
	glRotatef(azimuth, 0, 1, 0);
	glCallList(node->private.cubic.displayList);
	for (i = 0; i < node->npatches; i++) {
		if (node->private.cubic.patches[i].visible) {
			glColor4fv(node->private.cubic.patches[i].color);
			glBindTexture(glTextureTarget, node->private.cubic.patches[i].image->textureID);
			glInterleavedArrays(GL_T2F_V3F, 0, node->private.cubic.patches[i].tvcoord);
			glDrawArrays(GL_QUAD_STRIP, 0, node->private.cubic.patches[i].nvertices);
		}
	}
	if (showControls && node->prev == NULL) {
		int k;
		float x, y, w, h, az;
		CFlatPatch *p;
		float s, rx, ry, rz, ux, uy, uz;
		glDisable(GL_TEXTURE_2D);
		glDisable(glTextureTarget);
		/*draw patch corners*/
		glColor4f(0.3f, 0.3f, 0.3f, 0.6f);
		for (p = node->private.cubic.patches, i = 1; i <= node->npatches; p++, i++) {
			k = 5*p->nvertices - 8;
			rx = p->tvcoord[k] - p->tvcoord[2];
			ry = p->tvcoord[k+1] - p->tvcoord[3];
			rz = p->tvcoord[k+2] - p->tvcoord[4];
			s = sqrtf(rx*rx + ry*ry + rz*rz);
			if (s > 0.075f) s = 0.025f/s;
			else s = 1.0f/3;
			rx *= s; ry *= s; rz *= s;
			
			ux = p->tvcoord[2] - p->tvcoord[7];
			uy = p->tvcoord[3] - p->tvcoord[8];
			uz = p->tvcoord[4] - p->tvcoord[9];
			s = sqrtf(ux*ux + uy*uy + uz*uz);
			if (s > 0.075f) s = 0.025f/s;
			else s = 1.0f/3;
			ux *= s; uy *= s; uz *= s;
			
			glBegin(GL_TRIANGLES);
			glVertex3fv(&(p->tvcoord[7]));
			glVertex3f(p->tvcoord[7] + rx, p->tvcoord[8] + ry, p->tvcoord[9] + rz);
			glVertex3f(p->tvcoord[7] + ux, p->tvcoord[8] + uy, p->tvcoord[9] + uz);
			glVertex3fv(&(p->tvcoord[k+5]));
			glVertex3f(p->tvcoord[k+5] + ux, p->tvcoord[k+6] + uy, p->tvcoord[k+7] + uz);
			glVertex3f(p->tvcoord[k+5] - rx, p->tvcoord[k+6] - ry, p->tvcoord[k+7] - rz);
			glVertex3fv(&(p->tvcoord[k]));
			glVertex3f(p->tvcoord[k] - rx, p->tvcoord[k+1] - ry, p->tvcoord[k+2] - rz);
			glVertex3f(p->tvcoord[k] - ux, p->tvcoord[k+1] - uy, p->tvcoord[k+2] - uz);
			glVertex3fv(&(p->tvcoord[2]));
			glVertex3f(p->tvcoord[2] - ux, p->tvcoord[3] - uy, p->tvcoord[4] - uz);
			glVertex3f(p->tvcoord[2] + rx, p->tvcoord[3] + ry, p->tvcoord[4] + rz);
			glEnd();
		}
		/*draw handles*/
		glEnable(GL_POLYGON_STIPPLE);
		lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
		lua_pushliteral(L, "lasthandle"); lua_rawget(L, -2); k = (int)lua_tonumber(L, -1); lua_pop(L, 1);
		lua_pushliteral(L, "controls");
		lua_rawget(L, -2);
		/*Lua stack: node, controls*/
		for (i = k - 255; i >= 1; i--) {
			lua_rawgeti(L, -1, i + 255);
			lua_pushliteral(L, "x"); lua_rawget(L, -2); x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
			lua_pushliteral(L, "y"); lua_rawget(L, -2); y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
			lua_pushliteral(L, "w"); lua_rawget(L, -2); w = (float)lua_tonumber(L, -1); lua_pop(L, 1);
			lua_pushliteral(L, "h"); lua_rawget(L, -2); h = (float)lua_tonumber(L, -1); lua_pop(L, 1);
			lua_pop(L, 1); /*handle*/
			glColor4ub(controlColorPalette[i][0], controlColorPalette[i][1], controlColorPalette[i][2], 191);
			glBegin(GL_QUAD_STRIP);
			y *= (float)M_PI/180;
			x *= (float)M_PI/180;
			h = y - h*(float)M_PI/180;
			w = x + w*(float)M_PI/180;
			for (az = x; az < w; az += 0.1f) {
				glVertex3f(cosf(y)*sinf(az), sinf(y), -cosf(y)*cosf(az));
				glVertex3f(cosf(h)*sinf(az), sinf(h), -cosf(h)*cosf(az));
			}
			glVertex3f(cosf(y)*sinf(w), sinf(y), -cosf(y)*cosf(w));
			glVertex3f(cosf(h)*sinf(w), sinf(h), -cosf(h)*cosf(w));
			glEnd();
		}
		lua_pop(L, 2); /*controls, node*/				
		glDisable(GL_POLYGON_STIPPLE);
		glEnable(GL_TEXTURE_2D);
		glEnable(glTextureTarget);
		/*draw hotspots*/
		if (node->hotspotmap != NULL) node->hotspotmap->draw(node->hotspotmap);
	}
}

static void updateInterpolation(CNode *node) {
	int i;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	for (i = 1; i <= 6; i++) {
		lua_rawgeti(L, -1, i);
		glBindTexture(glTextureTarget, ((Image *)lua_touserdata(L, -1))->textureID);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, glTextureFilter);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, glTextureFilter);
		lua_pop(L, 1); /*image*/
	}
	lua_pop(L, 1); /*node*/
	if (node->private.cubic.patches != NULL) {
		for (i = 0; i < node->npatches; i++) {
			glBindTexture(glTextureTarget, node->private.cubic.patches[i].image->textureID);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, glTextureFilter);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, glTextureFilter);
		}
	}
}

static SDL_bool isInside(CNode *node, int x, int y) {
	return SDL_TRUE;
}

static void mouseXYtoHV(CNode *node, int x, int y, float *h, float *v) {
	xYtoAzEl(x, y, h, v);
}

void makeCubicCNode(CNode *node) {
	node->init = init;
	node->finalize = finalize;
	node->feedToGL = feedToGL;
	node->removeFromGL = removeFromGL;
	node->draw = draw;
	node->updatePatch = updatePatch;
	node->updateInterpolation = updateInterpolation;
	node->isInside = isInside;
	node->mouseXYtoHV = mouseXYtoHV;
}
