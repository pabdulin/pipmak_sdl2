/*
 
 slide.c, part of the Pipmak Game Engine
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

/* $Id: slide.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "nodes.h"

#include <stdlib.h>
#include <math.h>

#include "lua.h"

#include "terminal.h"


extern lua_State *L;
extern GLint glTextureFilter;
extern GLenum glTextureTarget;
extern Uint8 controlColorPalette[256][3];
extern SDL_Window *sdl2Window;
extern SDL_Rect screenSize;
extern int showControls;
extern Uint32 thisRedrawTime;


static void init(CNode *node) {
	node->private.slide.patches = NULL;
}

static void finalize(CNode *node) {
}

/*update the internal representation of the patch on top of the Lua stack*/
static void updatePatch(CNode *node) {
	if (node->private.slide.patches != NULL) {
		CFlatPatch *p;
		lua_pushliteral(L, "id"); lua_rawget(L, -2); p = &(node->private.slide.patches[(int)lua_tonumber(L, -1) - 1]); lua_pop(L, 1);
		updateFlatPatch(p, 1, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.49f);
	}
}

static void feedToGL(CNode *node) {
	Image *img;
	int i, border;
	float r;
	SDL_Rect *screen = &screenSize;
	
	for (i = 0; i < 16; i++) node->private.slide.viewMatrix[i] = 0;
	node->private.slide.viewMatrix[10] = 0.001f;
	node->private.slide.viewMatrix[15] = 1;
	r = (float)(node->width*screen->h)/(node->height*screen->w);
	if (r <= 1.0f) {
		node->private.slide.viewMatrix[0] = 2*r/node->width;
		node->private.slide.viewMatrix[5] = -2.0f/node->height;
		node->private.slide.viewMatrix[12] = -r;
		node->private.slide.viewMatrix[13] = 1.0f;
	}
	else {
		node->private.slide.viewMatrix[0] = 2.0f/node->width;
		node->private.slide.viewMatrix[5] = -2.0f/(r*node->height);
		node->private.slide.viewMatrix[12] = -1.0f;
		node->private.slide.viewMatrix[13] = 1.0f/r;
	}
	
	/*get background image, feed it to gl, set glTextureFilter*/
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_pushliteral(L, "border"); lua_rawget(L, -2); border = lua_toboolean(L, -1); lua_pop(L, 1);
	lua_rawgeti(L, -1, 1);
	/*Lua stack: node, image*/
	img = feedImageToGL(NULL);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, glTextureFilter);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, glTextureFilter);
	
	if (img->bpp == 32) {
		node->private.slide.imagedata = getImageData(img, 0);
		img->datarefcount++;
		/*img will not be collected since it's referenced in the node's Lua representation*/
	}
	else {
		node->private.slide.imagedata = NULL;
	}
	
	/*make display list with background*/
	node->private.slide.displayList = glGenLists(1);
	glNewList(node->private.slide.displayList, GL_COMPILE);
	if (node->prev == NULL && (!border || img->bpp == 32)) {
		/*draw striped gray background if there's no other node behind the slide and it has transparent parts*/
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glDisable(GL_TEXTURE_2D);
		glDisable(glTextureTarget);
		glEnable(GL_POLYGON_STIPPLE);
		glColor3f(0.8f, 0.8f, 0.8f);
		glBegin(GL_QUADS);
		glVertex2f(-1, -1);
		glVertex2f(1, -1);
		glVertex2f(1, 1);
		glVertex2f(-1, 1);
		glEnd();
		glDisable(GL_POLYGON_STIPPLE);
		glEnable(GL_TEXTURE_2D);
		glEnable(glTextureTarget);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
	}
	if (border) {
		if (img->bpp == 32) { /*draw aspect ratio conversion bars the hard way*/
			glDisable(GL_TEXTURE_2D);
			glDisable(glTextureTarget);
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glBegin(GL_QUADS);
			glVertex2f(-10000, -10000); glVertex2f(-10000, 10000); glVertex2f(0, (GLfloat)node->height); glVertex2f(0, 0);
			glVertex2f(-10000, 10000); glVertex2f(10000, 10000); glVertex2f((GLfloat)node->width, (GLfloat)node->height); glVertex2f(0, (GLfloat)node->height);
			glVertex2f(10000, 10000); glVertex2f(10000, -10000); glVertex2f((GLfloat)node->width, 0); glVertex2f((GLfloat)node->width, (GLfloat)node->height);
			glVertex2f(10000, -10000); glVertex2f(-10000, -10000); glVertex2f(0, 0); glVertex2f((GLfloat)node->width, 0);
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glEnable(glTextureTarget);
		}
		else { /*draw aspect ratio conversion bars the easy way*/
			glClearColor(0.0, 0.0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
	}
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(glTextureTarget, img->textureID);
	glBegin(GL_QUADS);
	glTexCoord2f(0, (float)(img->h)/img->textureHeight);
	glVertex2f(0, (GLfloat)node->height);
	glTexCoord2f((float)(img->w)/img->textureWidth, (float)(img->h)/img->textureHeight);
	glVertex2f((GLfloat)node->width, (GLfloat)node->height);
	glTexCoord2f((float)(img->w)/img->textureWidth, 0);
	glVertex2f((GLfloat)node->width, 0);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glEnd();
	glEndList();
	
	lua_pop(L, 1); /*image*/
	
	/*feed patch images to gl and fill in aggregate arrays for patches*/
	node->private.slide.patches = malloc(node->npatches*sizeof(CFlatPatch));
	if (node->private.slide.patches == NULL) {
		terminalPrint("Out of memory.", 0);
	}
	else {
		lua_pushliteral(L, "patches");
		lua_rawget(L, -2);
		/*Lua stack: node, patches*/
		for (i = 0; i < node->npatches; i++) {
			node->private.slide.patches[i].image = NULL;
			lua_rawgeti(L, -1, i+1);
			updatePatch(node);
			lua_pop(L, 1); /*patch*/
		}
		lua_pop(L, 1); /*patches*/
	}
	lua_pop(L, 1); /*node*/
}

static void removeFromGL(CNode *node) {
	Image *img;
	int i;
	glDeleteLists(node->private.slide.displayList, 1);
	for (i = 0; i < node->npatches; i++) releaseImageFromGL(node->private.slide.patches[i].image);
	free(node->private.slide.patches);
	node->private.slide.patches = NULL;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_rawgeti(L, -1, 1);
	/*Lua stack: node, image*/
	img = lua_touserdata(L, -1);
	if (img != NULL) {
		if (node->private.slide.imagedata != NULL) img->datarefcount--;
		releaseImageFromGL(img);
	}
	lua_pop(L, 2); /*image, node*/
}

static void draw(CNode *node) {
	int i;
	glLoadMatrixf(node->private.slide.viewMatrix); /*Normalized Slide View*/
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glCallList(node->private.slide.displayList);
	for (i = 0; i < node->npatches; i++) {
		if (node->private.slide.patches[i].visible) {
			glColor4fv(node->private.slide.patches[i].color);
			glBindTexture(glTextureTarget, node->private.slide.patches[i].image->textureID);
			glInterleavedArrays(GL_T2F_V3F, 0, node->private.slide.patches[i].tvcoord);
			glDrawArrays(GL_QUAD_STRIP, 0, node->private.slide.patches[i].nvertices);
		}
	}
	for (i = 0; i < node->ntexteditors; i++) {
		textEditorDraw(node->texteditors[i], thisRedrawTime);
	}
	if (showControls && node->prev == NULL) {
		int k;
		float x, y, w, h;
		CFlatPatch *p;
		float s, rx, ry, rz, ux, uy, uz;
		glDisable(GL_TEXTURE_2D);
		glDisable(glTextureTarget);
		/*draw patch corners*/
		glColor4f(0.3f, 0.3f, 0.3f, 0.6f);
		for (p = node->private.slide.patches, i = 1; i <= node->npatches; p++, i++) {
			k = 5*p->nvertices - 8;
			rx = p->tvcoord[k] - p->tvcoord[2];
			ry = p->tvcoord[k+1] - p->tvcoord[3];
			rz = p->tvcoord[k+2] - p->tvcoord[4];
			s = sqrtf(rx*rx + ry*ry + rz*rz);
			if (s > 24) s = 8/s;
			else s = 1.0f/3;
			rx *= s; ry *= s; rz *= s;
			
			ux = p->tvcoord[2] - p->tvcoord[7];
			uy = p->tvcoord[3] - p->tvcoord[8];
			uz = p->tvcoord[4] - p->tvcoord[9];
			s = sqrtf(ux*ux + uy*uy + uz*uz);
			if (s > 24) s = 8/s;
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
			glBegin(GL_QUADS);
			glVertex2f(x, y + h);
			glVertex2f(x + w, y + h);
			glVertex2f(x + w, y);
			glVertex2f(x, y);
			glEnd();
		}
		lua_pop(L, 2); /*controls, node*/				
		glDisable(GL_POLYGON_STIPPLE);
		glEnable(GL_TEXTURE_2D);
		glEnable(glTextureTarget);
		/*draw hotspots*/
		if (node->hotspotmap != NULL) {
			glMatrixMode(GL_MODELVIEW);
			glScalef((GLfloat)node->width, (GLfloat)node->height, 1);
			glMatrixMode(GL_PROJECTION);
			node->hotspotmap->draw(node->hotspotmap);
		}
	}
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

static void updateInterpolation(CNode *node) {
	int i;
	GLint filter = glTextureFilter;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_rawgeti(L, -1, 1);
	glBindTexture(glTextureTarget, ((Image *)lua_touserdata(L, -1))->textureID);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, filter);
	lua_pop(L, 2); /*image, node*/
	if (node->private.slide.patches != NULL) {
		for (i = 0; i < node->npatches; i++) {
			glBindTexture(glTextureTarget, node->private.slide.patches[i].image->textureID);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, filter);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, filter);
		}
	}
}

static void mouseXYtoHV(CNode *node, int x, int y, float *h, float *v) {
	SDL_Rect *screen = &screenSize;
	if (screen->w*node->height > screen->h*node->width) {
		*h = (x - (float)screen->w/2)*node->height/screen->h + (float)node->width/2;
		*v = (float)(y*node->height/screen->h);
	}
	else {
		*h = (float)(x*node->width/screen->w);
		*v = (y - (float)screen->h/2)*node->width/screen->w + (float)node->height/2;
	}
}

static SDL_bool isInside(CNode *node, int x, int y) {
	if (node->private.slide.imagedata == NULL) {
		return SDL_TRUE;
	}
	else {
		float h, v;
		int t, u;
		mouseXYtoHV(node, x, y, &h, &v);
		t = (int)h;
		u = (int)v;
		if (t < 0) t = 0;
		else if (t >= node->width) t = node->width - 1;
		if (u < 0) u = 0;
		else if (u >= node->height) u = node->height - 1;
		if (node->private.slide.imagedata[4*(u*node->width + t) + 3] >= 128) {
			return SDL_TRUE;
		}
		else {
			return (getControl(node, h, v) != 0);
		}
	}
}

void makeSlideCNode(CNode *node) {
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
