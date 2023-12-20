/*
 
 nodes.c, part of the Pipmak Game Engine
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

/* $Id: nodes.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "nodes.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "lua.h"
#include "lauxlib.h"

#include "tools.h"
#include "terminal.h"
#include "misc.h"
#include "config.h"


extern lua_State *L;
extern GLint glTextureFilter;
extern GLenum glTextureTarget;
extern CNode *backgroundCNode, *frontCNode, *thisCNode, *touchedNode;
extern int mouseX, mouseY, mouseWarpStartX, mouseWarpStartY;
extern int mouseButton;
extern Image *currentCursor, *standardCursor;
extern int touchedControl;
extern int showControls;
extern Uint32 thisRedrawTime, mouseWarpEndTime;
extern struct MouseModeStackEntry *topMouseMode;

static CNode *autofreedCNodes = NULL;


void freeCHotspotmap(CHotspotmap *map) {
	if (map->finalize != NULL) map->finalize(map);
	free(map);
}

void freeCNode(CNode *node) {
	Tool *t, *tt;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef); lua_pushnil(L); lua_setmetatable(L, -2); lua_pop(L, 1);
	if (node->finalize != NULL) node->finalize(node);
	luaL_unref(L, LUA_REGISTRYINDEX, node->noderef);
	if (node->hotspotmap != NULL) freeCHotspotmap(node->hotspotmap);
	if (node->texteditors != NULL) free(node->texteditors);
	t = node->tool;
	while (t != NULL) {
		tt = t->next;
		toolFree(t);
		t = tt;
	}
	free(node);
}

static void autofreeCNode(CNode *node) {
	node->next = autofreedCNodes;
	autofreedCNodes = node;
}

void freeAutofreedCNodes() {
	CNode *n, *nn;
	n = autofreedCNodes;
	while (n != NULL) {
		nn = n->next;
		freeCNode(n);
		n = nn;
	}
	autofreedCNodes = n;
}

void updateTouchedNode() {
	if (mouseButton == 0 || touchedNode == NULL) { /*as long as the mouse is down, we keep the old touchedNode, even if the mouse is currently outside of it*/
		CNode *n = frontCNode;
		while (n != NULL) {
			if (n->isInside(n, mouseX, mouseY)) break;
			n = n->prev;
		}
		if (n == NULL) n = backgroundCNode;
		if (n != touchedNode && n != NULL) {
			if (touchedNode != NULL) {
				touchedNode->tool->event(touchedNode->tool, BUTTON_IDLE, 0, 0); /*to call mouseleave handlers etc.*/
				callHandler("onmouseleave", 0, touchedNode);
			}
			callHandler("onmouseenter", 0, n);
			currentCursor = n->standardCursor;
			touchedNode = n;
			touchedControl = 0;
		}
	}
}

/*
 * Load the node whose path is on top of the Lua stack and return its C
 * representation. Warnings and errors are reported on the Pipmak terminal.
 * If there are errors, NULL is returned.
 */
CNode *loadNode() {
	CNode *node;
	const char* path = lua_tostring(L, -1); /*convert the path to a string, in case it's a number*/
	assert(path != NULL);
	node = malloc(sizeof(CNode));
	if (node == NULL) {
		terminalPrintf("Error loading node %s: Out of memory", path);
		return NULL;
	}
	/*fill in enough to keep anything that could be called from node.lua happy*/
	node->hotspotmap = NULL;
	node->complete = 0;
	node->prev = node->next = NULL;
	node->tool = NULL;
	node->standardCursor = standardCursor;
	node->dontsave = 0;
	node->type = NODE_TYPE_PANEL; /*for setpanelbbox which will only be called when it's really a panel*/
	lua_newtable(L);
	lua_pushvalue(L, -1); node->noderef = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushliteral(L, "cnode"); lua_pushlightuserdata(L, node); lua_rawset(L, -3);
	lua_pushliteral(L, "path"); lua_pushvalue(L, -3); lua_rawset(L, -3);
	/*Lua stack: path, node*/
	
	lua_pushliteral(L, "pipmak_internal");
	lua_rawget(L, LUA_GLOBALSINDEX);
	lua_pushliteral(L, "loadnode");
	lua_rawget(L, -2);
	lua_pushvalue(L, -3); /*node*/
	if (pcallWithBacktrace(L, 1, 1, node) != 0) {
		terminalPrintf("Error running Lua file %s", lua_tostring(L, -1)); /*this only catches errors in defaults.lua, not those in node.lua*/
		lua_pop(L, 1);
		lua_pushboolean(L, 0);
	}
	if (!lua_toboolean(L, -1)) {
		/*it didn't work; error messages have been printed to the Pipmak terminal*/
		lua_pop(L, 2); /*false, pipmak_internal*/
		free(node);
		return NULL;
	}
	lua_pop(L, 2); /*true, pipmak_internal*/
	
	/*Lua stack: path, node*/
	
	lua_pushliteral(L, "type"); lua_rawget(L, -2); node->type = lua_tonumber(L, -1); lua_pop(L, 1);
	
	lua_pushliteral(L, "hotspotmap");
	lua_rawget(L, -2);
	if (lua_isnil(L, -1)) {
		node->hotspotmap = NULL;
		lua_pop(L, 1); /*nil*/
	}
	else {
		/*Lua stack: path, node, hotspotmap*/
		node->hotspotmap = malloc(sizeof(CHotspotmap));
		if (node->hotspotmap == NULL) {
			terminalPrintf("Error loading hotspot map of node %s: Out of memory", path);
		}
		else {
			lua_pushliteral(L, "type"); lua_rawget(L, -2); node->hotspotmap->type = lua_tonumber(L, -1); lua_pop(L, 1);
			switch (node->hotspotmap->type) {
				case NODE_TYPE_SLIDE:
					makeSlideCHotspotmap(node->hotspotmap);
					break;
				case NODE_TYPE_EQUIRECT:
					makeEquirectCHotspotmap(node->hotspotmap);
					break;
				default:
					terminalPrintf("Error loading hotspot map of node %s: Unknown hotspot map type %d", path, node->hotspotmap->type);
					node->hotspotmap->type = NODE_TYPE_EQUIRECT;
					makeEquirectCHotspotmap(node->hotspotmap);
					break;
			}
			node->hotspotmap->init(node->hotspotmap); /*pops the hotspotmap*/
		}
	}
	/*Lua stack: path, node*/
	
	lua_pushliteral(L, "lasthotspot"); lua_rawget(L, -2); node->nhotspots = (int)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "lastpatch"); lua_rawget(L, -2); node->npatches = (int)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "lasttexteditor"); lua_rawget(L, -2); node->ntexteditors = (int)lua_tonumber(L, -1); lua_pop(L, 1);
	
	if (node->ntexteditors > 0) {
		node->texteditors = malloc(node->ntexteditors*sizeof(TextEditor*));
		if (node->texteditors == NULL) {
			terminalPrintf("out of memory");
			node->ntexteditors = 0;
		}
		else {
			int i;
			lua_pushliteral(L, "texteditors");
			lua_rawget(L, -2);
			assert(lua_istable(L, -1));
			for (i = 0; i < node->ntexteditors; i++) {
				lua_rawgeti(L, -1, i+1);
				node->texteditors[i] = (TextEditor*)lua_touserdata(L, -1);
				lua_pop(L, 1);
			}
			lua_pop(L, 1); /*texteditors*/
		}
	}
	else node->texteditors = NULL;
	
	node->width = 360;
	node->height = 180;
	switch (node->type) {
		case NODE_TYPE_CUBIC:
			makeCubicCNode(node);
			break;
		case NODE_TYPE_SLIDE: {
			Image *image;
			lua_rawgeti(L, -1, 1);
			image = lua_touserdata(L, -1);
			node->width = image->w;
			node->height = image->h;
			lua_pop(L, 1); /*image*/
			makeSlideCNode(node);
		} break;
		case NODE_TYPE_PANEL:
			makePanelCNode(node);
			break;
		default:
			terminalPrintf("Error loading node %s: Unknown node type %d", path, node->type);
			node->type = NODE_TYPE_CUBIC;
			makeCubicCNode(node);
			break;
	}
	node->complete = 1;
	node->init(node);
	
	lua_pop(L, 1); /*node*/
	
	return node;
}

void leaveNode(CNode *node) {
	int i;
	
	assert(node->entered); /*calling this function twice on the same node is a bug*/
	
	node->tool->deselected(node->tool);
	
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_pushliteral(L, "leavenodehandlers");
	lua_rawget(L, -2);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		if (pcallWithBacktrace(L, 0, 0, node) != 0) {
			terminalPrintf("Error running leavenode handler:\n%s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 2); /*leavenodehandlers, node*/
	
	nodeRemoveFromGL(node);
	
	for (i = 0; i < node->ntexteditors; i++) textEditorDestroy(node->texteditors[i]);
	
	if (node->prev == NULL) backgroundCNode = node->next;
	else node->prev->next = node->next;
	if (node->next == NULL) frontCNode = node->prev;
	else node->next->prev = node->prev;
	
	node->entered = 0;
	
	if (node == touchedNode) touchedNode = NULL;
	autofreeCNode(node);
}

/* Try to load the node whose path is on top of the Lua stack. If that works,
 * leave node <replaced> (if non-NULL), enter the new node and insert it into
 * the node stack after (in front of) node <prev>, or in the background if
 * <prev> is NULL, and return it. If loading the new node didn't work, return
 * NULL and leave <replaced> alone.
 * If the node stack would be empty after a failed load, enter node 0.
 */
CNode *enterNode(CNode *prev, CNode *replaced) {
	CNode *node;
	node = loadNode();
	/*Lua stack: path*/
	if (node != NULL) {
		ToolTag tool;

		if (replaced != NULL) leaveNode(replaced);
		
		tool = (prev != NULL || backgroundCNode == NULL) ? TOOL_HAND : backgroundCNode->tool->tag;
		
		if (prev == NULL) {
			node->prev = NULL;
			node->next = backgroundCNode;
			if (backgroundCNode != NULL) {
				backgroundCNode->prev = node;
			}
			else frontCNode = node;
			backgroundCNode = node;
		}
		else {
			node->prev = prev;
			node->next = prev->next;
			if (prev->next != NULL) prev->next->prev = node;
			else frontCNode = node;
			prev->next = node;
		}
		
		nodeFeedToGL(node);
		
		node->tool = newTool(tool, node);
		node->tool->selected(node->tool);
		
		lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
		lua_pushliteral(L, "enternodehandlers");
		lua_rawget(L, -2);
		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {
			if (pcallWithBacktrace(L, 0, 0, node) != 0) {
				terminalPrintf("Error running enternode handler:\n%s", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		lua_pop(L, 2); /*enternodehandlers, node*/
		
		node->entered = 1;
		
		thisRedrawTime = SDL_GetTicks();
		
		if (prev == NULL && topMouseMode->mode == MOUSE_MODE_DIRECT) {
			if (backgroundCNode->type & 1) {
				SDL_WarpMouse(mouseX, mouseY);
			}
			else { 
				mouseWarpStartX = mouseX;
				mouseWarpStartY = mouseY;
				mouseWarpEndTime = thisRedrawTime + MOUSE_WARP_DURATION;
				SDL_GetRelativeMouseState(NULL, NULL); /*reset accumulated relative motion*/
			}
		}
	}
	else if (backgroundCNode == NULL) {
		lua_pushliteral(L, "0");
		enterNode(NULL, NULL);
		lua_pop(L, 1);
	}
	return node;
}

/* Overlay the node whose path is on top of the Lua stack
 */
CNode *overlayNode(int dontsave) {
	CNode *node;
	assert(lua_type(L, -1) == LUA_TSTRING && lua_strlen(L, -1) > 0);
	/*check if a node with that path is already in the node stack*/
	node = (backgroundCNode == NULL) ? NULL : backgroundCNode->next;
	while (node != NULL) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
		lua_pushliteral(L, "path"); lua_rawget(L, -2);
		/*Lua stack: path, stacknode, stacknodepath*/
		if (lua_equal(L, -1, -3)) {
			lua_pop(L, 2);
			break;
		}
		lua_pop(L, 2);
		node = node->next;
	}
	if (node != NULL) { /*already being displayed as overlay: move to front*/
		if (node != frontCNode) {
			/*remove from old position*/
			node->prev->next = node->next;
			node->next->prev = node->prev;
			/*insert at the end*/
			node->prev = frontCNode;
			node->next = NULL;
			frontCNode->next = node;
			frontCNode = node;
		}
	}
	else { /*not being displayed as overlay: load & enter*/
		node = enterNode(frontCNode, NULL);
	}
	if (node != NULL) node->dontsave = dontsave;
	return node;
}

void nodeFeedToGL(CNode *node) {
	int i;
	node->feedToGL(node);
	if (node->prev == NULL && showControls && node->hotspotmap != NULL) node->hotspotmap->feedToGL(node->hotspotmap);
	for (i = 0; i < node->ntexteditors; i++) textEditorFeedToGL(node->texteditors[i]);
}

void nodeRemoveFromGL(CNode *node) {
	int i;
	for (i = 0; i < node->ntexteditors; i++) textEditorRemoveFromGL(node->texteditors[i]);
	if (showControls && node->hotspotmap != NULL) node->hotspotmap->removeFromGL(node->hotspotmap);
	node->removeFromGL(node);
}

void callHandler(const char *handler, int control, CNode *node) {
	if (node == NULL) return;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	if (control != 0) {
		lua_pushliteral(L, "controls");
		lua_rawget(L, -2);
		lua_rawgeti(L, -1, control);
	}
	/*Lua stack: node, controls, control *or* node*/
	if (!lua_isnil(L, -1)) {
		lua_pushstring(L, handler);
		lua_rawget(L, -2);
		if (!lua_isnil(L, -1)) {
			lua_pushvalue(L, -2); /*control or node*/
			if (pcallWithBacktrace(L, 1, 0, node) != 0) {
				if (control == 0) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
					lua_pushliteral(L, "path"); lua_rawget(L, -2);
					terminalPrintf("Error running %s handler of node %s:\n%s", handler, lua_tostring(L, -1), lua_tostring(L, -3));
					lua_pop(L, 2); /*node, path*/
				}
				else {
					terminalPrintf("Error running %s handler of %s %d:\n%s", handler, (control < 256) ? "hotspot" : "handle", (control < 256) ? control : (control-255), lua_tostring(L, -1));
				}
				lua_pop(L, 1);
			}
		}
		else lua_pop(L, 1); /*nil*/
	}
	if (control != 0) lua_pop(L, 3); /*control, controls, node*/
	else lua_pop(L, 1); /*node*/
}

void callHandlerWithBoolean(const char *handler, int control, CNode *node, int arg) {
	if (node == NULL) return;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_pushliteral(L, "controls");
	lua_rawget(L, -2);
	lua_rawgeti(L, -1, control);
	/*Lua stack: node, controls, control*/
	if (!lua_isnil(L, -1)) {
		lua_pushstring(L, handler);
		lua_rawget(L, -2);
		if (!lua_isnil(L, -1)) {
			lua_pushvalue(L, -2); /*control*/
			lua_pushboolean(L, arg);
			if (pcallWithBacktrace(L, 2, 0, node) != 0) {
				terminalPrintf("Error running %s handler of %s %d:\n%s", handler, (control < 256)?"hotspot":"handle", (control < 256)?control:(control-255), lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		else lua_pop(L, 1); /*nil*/
	}
	lua_pop(L, 3); /*control, controls, node*/
}

int getControl(CNode *node, float mh, float mv) {
	int control;
	if (node->hotspotmap != NULL) {
		control = node->hotspotmap->getHotspot(node->hotspotmap, mh/node->width, mv/node->height);
		if (control > node->nhotspots) control = 0;
	}
	else control = 0;
	lua_rawgeti(L, LUA_REGISTRYINDEX, node->noderef);
	lua_pushliteral(L, "getcontrol");
	lua_rawget(L, -2);
	lua_pushvalue(L, -2);
	lua_pushnumber(L, mh);
	lua_pushnumber(L, mv);
	lua_pushnumber(L, control);
	lua_call(L, 4, 1);
	control = (int)lua_tonumber(L, -1);
	lua_pop(L, 2); /*number, node*/
	return control;
}

static void vrotate(float vector[], int axis, float angle) {
	if (angle != 0.0f) {
		int ai = (axis+1)%3;
		int bi = (axis+2)%3;
		float s = sinf(angle);
		float c = cosf(angle);
		float a = vector[ai];
		float b = vector[bi];
		vector[ai] = c*a - s*b;
		vector[bi] = s*a + c*b;
	}
}

/* Common parts of node->updatePatch() for node classes with flat patches (currently, all of them).
 * Arguments:
 * p - the patch, must also be at the top of the Lua stack
 * face - face number (1..6) for cubic nodes, 1 for flat nodes
 * bpw, bph - width and height of a background pixel in world units ((1, -1) for flat nodes)
 * origin{xyz} - top left corner of the background image (face 1) in world coordinates
 * cutoff - width of the margin cut off for interpolation in patch pixels (0 for panels where there is no interpolation, 0.49 for others)
 * (0.49 is close enough to 0.5 that the interpolation error is unnoticeable and far enough from it that the sampling position of the outermost pixel still hits the patch when patch pixels are aligned to screen pixels)
 */
void updateFlatPatch(CFlatPatch *p, int face, float bpw, float bph, float originx, float originy, float originz, float cutoff) {
	Image *img;
	int i;
	float lm, rm, w, h;
	float t1, t2;
	float s, t;
	float right[3] = {0.0f, 0.0f, 0.0f}, up[3] = {0.0f, 0.0f, 0.0f}, loc[3];
	float *tv;
	const char *rotationorder, *raxis;
	const char *anglename;
	
	lua_pushliteral(L, "image");
	lua_rawget(L, -2);
	img = lua_touserdata(L, -1);
	/*Lua stack: patch, image*/
	if (img != p->image) {
		if (p->image != NULL) releaseImageFromGL(p->image);
		p->image = feedImageToGL(img);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, (cutoff == 0) ? GL_NEAREST : glTextureFilter);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, (cutoff == 0) ? GL_NEAREST : glTextureFilter);
	}
	
	/* right and up vectors are the dimensions of one background pixel in world coordinates */
	right[0] = bpw;
	up[1] = bph;
	
	/* angles */
	lua_pushliteral(L, "rotationorder");
	lua_rawget(L, -3);
	rotationorder = lua_tostring(L, -1);
	if (rotationorder == NULL) rotationorder = "xyz";
	for (raxis = strchr(rotationorder, '\0') - 1; raxis >= rotationorder; raxis--) { /* backwards since we stay in unrotated coordinates */
		switch (*raxis) {
			case 'x': anglename = "anglex"; i = 0; break;
			case 'y': anglename = "angley"; i = 1; break;
			case 'z': anglename = "angle";  i = 2; break;
			default:
				terminalPrintf("Expected 'x', 'y', or 'z' in rotationorder, got '%c' - ignoring", *raxis);
				i = -1;
		}
		if (i != -1) {
			lua_pushstring(L, anglename);
			lua_rawget(L, -4);
			if (!lua_isnil(L, -1)) {
				s = (float)lua_tonumber(L, -1)*(float)M_PI/180;
				if (bph < 0 && *raxis != 'y') s = -s; /* reverse the angle if the world coordinate system is flipped, unless it's the y rotation, which happens in the x-z plane that is unaffected by the flip */
				vrotate(right, i, s);
				vrotate(up, i, s);
			}
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1); /*rotationorder*/
	
	/* location */
	lua_pushliteral(L, "nx");
	lua_rawget(L, -3);
	if (!lua_isnil(L, -1)) {
		loc[0] = (float)lua_tonumber(L, -1);
	}
	else {
		lua_pop(L, 1);
		lua_pushliteral(L, "x"); lua_rawget(L, -3);
		loc[0] = originx + (float)lua_tonumber(L, -1)*bpw;
	}
	lua_pop(L, 1);
	
	lua_pushliteral(L, "ny");
	lua_rawget(L, -3);
	if (!lua_isnil(L, -1)) {
		loc[1] = (float)lua_tonumber(L, -1);
	}
	else {
		lua_pop(L, 1);
		lua_pushliteral(L, "y"); lua_rawget(L, -3);
		loc[1] = originy - (float)lua_tonumber(L, -1)*bph;
	}
	lua_pop(L, 1);
	
	lua_pushliteral(L, "nz");
	lua_rawget(L, -3);
	if (!lua_isnil(L, -1)) {
		loc[2] = (float)lua_tonumber(L, -1);
	}
	else {
		loc[2] = originz;
	}
	lua_pop(L, 1);
	
	switch (face) {
		case 1:
			break;
		case 2:
			/* rotate -90 degrees around y axis */
			s = right[0]; right[0] = -right[2]; right[2] = s;
			s = up[0]; up[0] = -up[2]; up[2] = s;
			s = loc[0]; loc[0] = -loc[2]; loc[2] = s;
			break;
		case 3:
			/* rotate 180 degrees around y axis */
			right[0] = -right[0]; right[2] = -right[2];
			up[0] = -up[0]; up[2] = -up[2];
			loc[0] = -loc[0]; loc[2] = -loc[2];
			break;
		case 4:
			/* rotate 90 degrees around y axis */
			s = right[0]; right[0] = right[2]; right[2] = -s;
			s = up[0]; up[0] = up[2]; up[2] = -s;
			s = loc[0]; loc[0] = loc[2]; loc[2] = -s;
			break;
		case 5:
			/* rotate 90 degrees around x axis */
			s = right[1]; right[1] = -right[2]; right[2] = s;
			s = up[1]; up[1] = -up[2]; up[2] = s;
			s = loc[1]; loc[1] = -loc[2]; loc[2] = s;
			break;
		case 6:
			/* rotate -90 degrees around x axis */
			s = right[1]; right[1] = right[2]; right[2] = -s;
			s = up[1]; up[1] = up[2]; up[2] = -s;
			s = loc[1]; loc[1] = loc[2]; loc[2] = -s;
			break;
	}
	
	/* margins in patch pixels */
	lua_pushliteral(L, "leftmargin"); lua_rawget(L, -3); lm = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "rightmargin"); lua_rawget(L, -3); rm = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	
	/* width & height in background pixels */
	lua_pushliteral(L, "nw");
	lua_rawget(L, -3);
	if (!lua_isnil(L, -1)) {
		/*convert nw = visible width in world coordinates*/
		w = (float)lua_tonumber(L, -1)/bpw; /*visible width in background pixels*/
		if (lm >= cutoff) {
			if (rm >= cutoff) {
				w += 2*cutoff;
			}
			else {
				w = (w + cutoff - lm)*(img->w - lm - rm)/(img->w - cutoff - lm) + lm + rm;
			}
		}
		else {
			if (rm >= cutoff) {
				w = (w + cutoff - rm)*(img->w - lm - rm)/(img->w - cutoff - rm) + lm + rm;
			}
			else {
				w = w*(img->w - lm - rm)/(img->w - 2*cutoff) + lm + rm;
			}
		}
	}
	else {
		lua_pop(L, 1);
		lua_pushliteral(L, "w"); lua_rawget(L, -3);
		w = (float)lua_tonumber(L, -1);
	}
	lua_pop(L, 1);
	
	lua_pushliteral(L, "nh");
	lua_rawget(L, -3);
	if (!lua_isnil(L, -1)) {
		/*convert nh = visible height in world coordinates (simpler because there are no margins)*/
		h = (float)lua_tonumber(L, -1)/bph;
		if (cutoff != 0) h *= img->h/(img->h - 2*cutoff);
	}
	else {
		lua_pop(L, 1);
		lua_pushliteral(L, "h"); lua_rawget(L, -3);
		h = (float)lua_tonumber(L, -1);
	}
	lua_pop(L, 1);
	
	/* horizontal anchor offset in patch pixels, convert to background pixels and negate */
	lua_pushliteral(L, "anchorh"); lua_rawget(L, -3); s = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	if (rm != 0 && s >= img->w - rm) s = s - img->w + w;
	else if (lm != 0 && s <= lm) {} /*nothing to do*/
	else s = lm + (s - lm)*(w - lm - rm)/(img->w - lm - rm);
	s = -s;
	
	t1 = cutoff/img->textureHeight;
	t2 = (img->h - cutoff)/img->textureHeight;
	
	tv = p->tvcoord;
	
	/* left edge, texture coordinates */
	tv[0] = tv[5] = cutoff/img->textureWidth;
	tv[1] = t1;
	tv[6] = t2;
	
	if (lm <= cutoff) {
		/* left edge, 'right' component */
		t = s + lm + (cutoff - lm)*(w - lm - rm)/(img->w - lm - rm);
		tv[2] = tv[7] = loc[0] + t*right[0];
		tv[3] = tv[8] = loc[1] + t*right[1];
		tv[4] = tv[9] = loc[2] + t*right[2];
		tv += 10;
	}
	else {
		/* left edge, 'right' component */
		t = s + cutoff;
		tv[2] = tv[7] = loc[0] + t*right[0];
		tv[3] = tv[8] = loc[1] + t*right[1];
		tv[4] = tv[9] = loc[2] + t*right[2];
		tv += 10;
	
		/* left margin edge, texture coordinates */
		tv[0] = tv[5] = lm/img->textureWidth;
		tv[1] = t1;
		tv[6] = t2;
		
		/* left margin edge, 'right' component */
		t = s + lm;
		tv[2] = tv[7] = loc[0] + t*right[0];
		tv[3] = tv[8] = loc[1] + t*right[1];
		tv[4] = tv[9] = loc[2] + t*right[2];
		tv += 10;
	}
	
	s += w;
	
	if (rm <= cutoff) {
		/* right edge, 'right' component */
		t = s - rm - (cutoff - rm)*(w - lm - rm)/(img->w - lm - rm);
		tv[2] = tv[7] = loc[0] + t*right[0];
		tv[3] = tv[8] = loc[1] + t*right[1];
		tv[4] = tv[9] = loc[2] + t*right[2];
	}
	else {
		/* right margin edge, texture coordinates */
		tv[0] = tv[5] = (img->w - rm)/img->textureWidth;
		tv[1] = t1;
		tv[6] = t2;
		
		/* right margin edge, 'right' component */
		t = s - rm;
		tv[2] = tv[7] = loc[0] + t*right[0];
		tv[3] = tv[8] = loc[1] + t*right[1];
		tv[4] = tv[9] = loc[2] + t*right[2];
		tv += 10;
		
		/* right edge, 'right' component */
		t = s - cutoff;
		tv[2] = tv[7] = loc[0] + t*right[0];
		tv[3] = tv[8] = loc[1] + t*right[1];
		tv[4] = tv[9] = loc[2] + t*right[2];
	}
	
	/* right edge, texture coordinates */
	tv[0] = tv[5] = (img->w - cutoff)/img->textureWidth;
	tv[1] = t1;
	tv[6] = t2;
	
	p->nvertices = (GLsizei)((tv - p->tvcoord)/5 + 2);
	
	/* vertical anchor offset in patch pixels, convert to background pixels */
	lua_pushliteral(L, "anchorv"); lua_rawget(L, -3); s = (float)lua_tonumber(L, -1)*h/img->h; lua_pop(L, 1);
	
	s -= cutoff*h/img->h;
	
	/* top edge, 'up' component */
	for (tv = p->tvcoord, i = p->nvertices/2; i != 0; tv += 10, i--) {
		 tv[2] += s*up[0];
		 tv[3] += s*up[1];
		 tv[4] += s*up[2];
	}
	
	s -= (img->h - 2*cutoff)*h/img->h;
	
	/* bottom edge, 'up' component */
	for (tv = p->tvcoord, i = p->nvertices/2; i != 0; tv += 10, i--) {
		 tv[7] += s*up[0];
		 tv[8] += s*up[1];
		 tv[9] += s*up[2];
	}
	
	lua_pop(L, 1); /*image*/
	
	lua_pushliteral(L, "visible"); lua_rawget(L, -2); p->visible = lua_toboolean(L, -1); lua_pop(L, 1);
	
	lua_pushliteral(L, "r"); lua_rawget(L, -2); p->color[0] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "g"); lua_rawget(L, -2); p->color[1] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "b"); lua_rawget(L, -2); p->color[2] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
	lua_pushliteral(L, "a"); lua_rawget(L, -2); p->color[3] = (float)lua_tonumber(L, -1); lua_pop(L, 1);
}
