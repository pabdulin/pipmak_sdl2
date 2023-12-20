/*
 
 nodes.h, part of the Pipmak Game Engine
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

/* $Id: nodes.h 211 2008-10-22 08:53:36Z cwalther $ */

#ifndef NODES_H_SEEN
#define NODES_H_SEEN

#include "SDL.h"
#include "SDL_opengl.h"

#include "images.h"
#include "textedit.h"


enum NodeType { NODE_TYPE_CUBIC = 0, NODE_TYPE_SLIDE = 1, NODE_TYPE_EQUIRECT = 2, NODE_TYPE_PANEL = 3 };
/*corresponds to pipmak.cubic, pipmak.slide, pipmak.equirect, pipmak.panel on the Lua side*/
/*even types are panoramic, odd ones flat*/

typedef struct {
	GLfloat tvcoord[40];
	GLsizei nvertices;
	GLfloat color[4];
	Image *image;
	int visible;
} CFlatPatch;

typedef struct CHotspotmap {
	void (*init)(struct CHotspotmap *);
	void (*finalize)(struct CHotspotmap *);
	int (*getHotspot)(struct CHotspotmap *, float, float);
	void (*feedToGL)(struct CHotspotmap *);
	void (*removeFromGL)(struct CHotspotmap *);
	void (*draw)(struct CHotspotmap *);
	void (*setPixels)(struct CHotspotmap *, float, float, float, Uint8);
	enum NodeType type;
	union {
		struct {
			int imageref;
			int w, h;
			Uint8 *data;
			GLuint displayList;
		} slide;
		struct {
			int imageref;
			int w, h;
			Uint8 *data;
			GLuint displayList;
		} equirect;
	} private;
} CHotspotmap;

struct Tool;

typedef struct CNode {
	void (*init)(struct CNode *);
	void (*finalize)(struct CNode *);
	void (*feedToGL)(struct CNode *);
	void (*removeFromGL)(struct CNode *);
	void (*draw)(struct CNode *);
	void (*updatePatch)(struct CNode *);
	void (*updateInterpolation)(struct CNode *);
	SDL_bool (*isInside)(struct CNode *, int x, int y);
	void (*mouseXYtoHV)(struct CNode *, int x, int y, float *h, float *v);
	int noderef;
	int width, height;
	enum NodeType type;
	int nhotspots, npatches, ntexteditors;
	unsigned int complete:1, dontsave:1, entered:1;
	CHotspotmap *hotspotmap;
	TextEditor **texteditors;
	struct Tool *tool;
	Image *standardCursor;
	struct CNode *prev, *next;
	union {
		struct {
			GLuint displayList;
			CFlatPatch *patches;
			GLfloat viewMatrix[16];
			Uint8 *imagedata;
		} slide;
		struct {
			GLuint displayList;
			CFlatPatch *patches;
		} cubic;
		struct {
			GLuint displayList;
			CFlatPatch *patches;
			int x, y, imagew, imageh;
			int leftmargin, rightmargin, topmargin, bottommargin; /*in 1/4 px - only used for isInside, where using fixed-point instead of floats is accurate enough*/
			int bbminx, bbmaxx, bbminy, bbmaxy;
			int startx, starty, endx, endy;
			Uint32 endtime, duration;
			Uint8 *imagedata;
			void (*updatePosition)(struct CNode *, Uint32 duration);
		} panel;
	} private;
} CNode;

void freeCHotspotmap(CHotspotmap *map);
void freeCNode(CNode *node);
void freeAutofreedCNodes();
void updateTouchedNode();
CNode *loadNode();
void leaveNode(CNode *node);
CNode *enterNode(CNode *prev, CNode *replaced);
CNode *overlayNode(int dontsave);
void nodeFeedToGL(CNode *node);
void nodeRemoveFromGL(CNode *node);
void callHandler(const char *handler, int control, CNode *node);
void callHandlerWithBoolean(const char *handler, int control, CNode *node, int arg);
int getControl(CNode *node, float mh, float mv);
void updateFlatPatch(CFlatPatch *p, int face, float bpw, float bph, float originx, float originy, float originz, float cutoff);

void makeSlideCNode(CNode *node);
void makeCubicCNode(CNode *node);
void makePanelCNode(CNode *node);

void makeSlideCHotspotmap(CHotspotmap *map);
void makeEquirectCHotspotmap(CHotspotmap *map);

#endif /*NODES_H_SEEN*/
