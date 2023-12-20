/*
 
 panel.c, part of the Pipmak Game Engine
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

/* $Id: panel.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "nodes.h"

#include <math.h>
#include <stdlib.h>

#include "lua.h"

#include "terminal.h"
#include "misc.h"


extern lua_State *L;
extern GLenum glTextureTarget;
extern Uint8 controlColorPalette[256][3];
extern SDL_Surface *screen;
extern int showControls;
extern GLfloat screenViewMatrix[16];
extern Uint32 thisRedrawTime;


static void updatePosition(CNode *node, Uint32 duration) {
	lua_Number r, a;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_pushliteral(L, "relx"); lua_rawget(L, -2); r = lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "absx"); lua_rawget(L, -2); a = lua_tonumber(L, -1); lua_pop(L, 1);
	node->private.panel.endx = (int)floor(r*screen->w + a + 0.5);
	lua_pushliteral(L, "rely"); lua_rawget(L, -2); r = lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "absy"); lua_rawget(L, -2); a = lua_tonumber(L, -1); lua_pop(L, 1);
	node->private.panel.endy = (int)floor(r*screen->h + a + 0.5);
	lua_pop(L, 1); /*node*/
	if (duration != 0) {
		node->private.panel.startx = node->private.panel.x;
		node->private.panel.starty = node->private.panel.y;
		node->private.panel.duration = duration;
		node->private.panel.endtime = thisRedrawTime + duration;
	}
	else {
		node->private.panel.x = node->private.panel.endx;
		node->private.panel.y = node->private.panel.endy;
		node->private.panel.endtime = 0;
	}
}

static void init(CNode *node) {
	node->private.panel.patches = NULL;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_pushliteral(L, "w"); lua_rawget(L, -2); node->width = (int)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "h"); lua_rawget(L, -2); node->height = (int)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pop(L, 1); /*node*/
}

static void finalize(CNode *node) {
}

/*update the internal representation of the patch on top of the Lua stack*/
static void updatePatch(CNode *node) {
	if (node->private.panel.patches != NULL) {
		CFlatPatch *p;
		lua_pushliteral(L, "id"); lua_rawget(L, -2); p = &(node->private.panel.patches[(int)lua_tonumber(L, -1) - 1]); lua_pop(L, 1);
		updateFlatPatch(p, 1, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	}
}

static void feedToGL(CNode *node) {
	Image *img;
	int i, j;
	float vx[4], vy[4], tx[4], ty[4];
	float lm, rm, tm, bm;
	
	updatePosition(node, 0);
	
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_rawgeti(L, -1, 1);
	/*Lua stack: node, image*/
	img = feedImageToGL(NULL);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	if (img->bpp == 32) {
		node->private.panel.imagedata = getImageData(img, 0);
		img->datarefcount++;
		/*img will not be collected since it's referenced in the node's Lua representation*/
	}
	else {
		node->private.panel.imagedata = NULL;
	}
	node->private.panel.imagew = img->w;
	node->private.panel.imageh = img->h;
	
	lua_pushliteral(L, "leftmargin"); lua_rawget(L, -3); lm = (lua_isnil(L, -1)) ? (float)img->w/2 : (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "rightmargin"); lua_rawget(L, -3); rm = (lua_isnil(L, -1)) ? (float)img->w/2 : (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "topmargin"); lua_rawget(L, -3); tm = (lua_isnil(L, -1)) ? (float)img->h/2 : (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "bottommargin"); lua_rawget(L, -3); bm = (lua_isnil(L, -1)) ? (float)img->h/2 : (float)lua_tonumber(L, -1); lua_pop(L, 1);
	node->private.panel.leftmargin = (int)(4*lm);
	node->private.panel.rightmargin = (int)(4*rm);
	node->private.panel.topmargin = (int)(4*tm);
	node->private.panel.bottommargin = (int)(4*bm);
	
	node->private.panel.displayList = glGenLists(1);
	glNewList(node->private.panel.displayList, GL_COMPILE);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(glTextureTarget, img->textureID);
	vx[0] = 0; vx[1] = lm; vx[2] = node->width - rm; vx[3] = (float)node->width;
	vy[0] = 0; vy[1] = tm; vy[2] = node->height - bm; vy[3] = (float)node->height;
	tx[0] = 0; tx[1] = lm/img->textureWidth; tx[2] = (img->w - rm)/img->textureWidth; tx[3] = (float)(img->w)/img->textureWidth;
	ty[0] = 0; ty[1] = tm/img->textureHeight; ty[2] = (img->h - bm)/img->textureHeight; ty[3] = (float)(img->h)/img->textureHeight;
	for (i = 0; i < 3; i++) {
		glBegin(GL_QUAD_STRIP);
		for (j = 0; j < 4; j++) {
			glTexCoord2f(tx[j], ty[i]);
			glVertex2f(vx[j], vy[i]);
			glTexCoord2f(tx[j], ty[i+1]);
			glVertex2f(vx[j], vy[i+1]);
		}
		glEnd();
	}
	glEndList();
	
	lua_pop(L, 1); /*image*/
	
	/*feed patch images to gl and fill in aggregate arrays for patches*/
	node->private.panel.patches = malloc(node->npatches*sizeof(CFlatPatch));
	if (node->private.panel.patches == NULL) {
		terminalPrint("Out of memory.", 0);
	}
	else {
		lua_pushliteral(L, "patches");
		lua_rawget(L, -2);
		/*Lua stack: node, patches*/
		for (i = 0; i < node->npatches; i++) {
			node->private.panel.patches[i].image = NULL;
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
	glDeleteLists(node->private.panel.displayList, 1);
	for (i = 0; i < node->npatches; i++) releaseImageFromGL(node->private.panel.patches[i].image);
	free(node->private.panel.patches);
	node->private.panel.patches = NULL;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_rawgeti(L, -1, 1);
	img = lua_touserdata(L, -1);
	if (img != NULL) {
		if (node->private.panel.imagedata != NULL) img->datarefcount--;
		releaseImageFromGL(img);
	}
	lua_pop(L, 2); /*image, node*/
}

static void draw(CNode *node) {
	int i;
	
	glLoadMatrixf(screenViewMatrix);
	glTranslatef((GLfloat)node->private.panel.x, (GLfloat)node->private.panel.y, 0);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	/*draw striped gray background if there's no other node behind the panel*/
	if (node->prev == NULL) {
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
	
	/*draw panel*/
	glCallList(node->private.panel.displayList);
	
	/*draw patches*/
	for (i = 0; i < node->npatches; i++) {
		if (node->private.panel.patches[i].visible) {
			glColor4fv(node->private.panel.patches[i].color);
			glBindTexture(glTextureTarget, node->private.panel.patches[i].image->textureID);
			glInterleavedArrays(GL_T2F_V3F, 0, node->private.panel.patches[i].tvcoord);
			glDrawArrays(GL_QUAD_STRIP, 0, node->private.panel.patches[i].nvertices);
		}
	}
	
	/*draw text editors*/
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
		for (p = node->private.panel.patches, i = 1; i <= node->npatches; p++, i++) {
			k = 5*p->nvertices - 8;
			rx = p->tvcoord[k] - p->tvcoord[2];
			ry = p->tvcoord[k+1] - p->tvcoord[3];
			rz = p->tvcoord[k+2] - p->tvcoord[4];
			s = sqrtf(rx*rx + ry*ry + rz*rz);
			if (s > 25.5f) s = 8.5f/s;
			else s = 1.0f/3;
			rx *= s; ry *= s; rz *= s;
			
			ux = p->tvcoord[2] - p->tvcoord[7];
			uy = p->tvcoord[3] - p->tvcoord[8];
			uz = p->tvcoord[4] - p->tvcoord[9];
			s = sqrtf(ux*ux + uy*uy + uz*uz);
			if (s > 25.5f) s = 8.5f/s;
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
		glMatrixMode(GL_MODELVIEW);
		glScalef((GLfloat)node->width, (GLfloat)node->height, 1);
		glMatrixMode(GL_PROJECTION);
		if (node->hotspotmap != NULL) node->hotspotmap->draw(node->hotspotmap);
	}
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	
	if (node->private.panel.endtime != 0) {
		float l = smoothCubic((float)((int)node->private.panel.endtime - (int)thisRedrawTime)/node->private.panel.duration);
		node->private.panel.x = (int)floorf(l*node->private.panel.startx + (1-l)*node->private.panel.endx + 0.5f);
		node->private.panel.y = (int)floorf(l*node->private.panel.starty + (1-l)*node->private.panel.endy + 0.5f);
		if (l == 0) node->private.panel.endtime = 0;
	}
}

static void updateInterpolation(CNode *node) {
}

static void mouseXYtoHV(CNode *node, int x, int y, float *h, float *v) {
	*h = (float)(x - node->private.panel.x);
	*v = (float)(y - node->private.panel.y);
}

static SDL_bool isInside(CNode *node, int x, int y) {
	int ix, iy;
	ix = x - node->private.panel.x;
	iy = y - node->private.panel.y;
	if (!(ix >= node->private.panel.bbminx && ix < node->private.panel.bbmaxx && iy >= node->private.panel.bbminy && iy < node->private.panel.bbmaxy)) {
		return SDL_FALSE;
	}
	else {
		float h, v;
		if (ix >= 0 && ix < node->width && iy >= 0 && iy < node->height) {
			if (node->private.panel.imagedata == NULL) {
				return SDL_TRUE;
			}
			else {
				if (4*ix >= 4*node->width - node->private.panel.rightmargin) ix += node->private.panel.imagew - node->width;
				else if (4*ix > node->private.panel.leftmargin) ix = ((4*node->private.panel.imagew - node->private.panel.leftmargin - node->private.panel.rightmargin)*(4*ix - node->private.panel.leftmargin)/(4*node->width - node->private.panel.leftmargin - node->private.panel.rightmargin) + node->private.panel.leftmargin)/4;
				if (4*iy >= 4*node->height - node->private.panel.bottommargin) iy += node->private.panel.imageh - node->height;
				else if (4*iy > node->private.panel.topmargin) iy = ((4*node->private.panel.imageh - node->private.panel.topmargin - node->private.panel.bottommargin)*(4*iy - node->private.panel.topmargin)/(4*node->height - node->private.panel.topmargin - node->private.panel.bottommargin) + node->private.panel.topmargin)/4;
				if (node->private.panel.imagedata[4*(iy*node->private.panel.imagew + ix) + 3] >= 128) {
					return SDL_TRUE;
				}
			}
		}
		mouseXYtoHV(node, x, y, &h, &v);
		return (getControl(node, h, v) != 0);
	}
}

void makePanelCNode(CNode *node) {
	node->init = init;
	node->finalize = finalize;
	node->feedToGL = feedToGL;
	node->removeFromGL = removeFromGL;
	node->draw = draw;
	node->updatePatch = updatePatch;
	node->updateInterpolation = updateInterpolation;
	node->isInside = isInside;
	node->mouseXYtoHV = mouseXYtoHV;
	node->private.panel.updatePosition = updatePosition;
}
