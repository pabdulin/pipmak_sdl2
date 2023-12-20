/*
 
 toolEyedropper.c, part of the Pipmak Game Engine
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

/* $Id: toolEyedropper.c 102 2006-07-09 14:25:49Z cwalther $ */

#include "tools.h"

#include <math.h>
#include <stdlib.h>

#include "SDL_opengl.h"


extern float mouseH, mouseV;
extern int hotspotPaint;
extern Image *cursors[NUMBER_OF_CURSORS];


static void event(Tool *self, ButtonEventType type, int control, Uint32 frameDuration) {
	if (type & 1) { /*BUTTON_DOWN or BUTTON_STILLDOWN*/
		int n;
		if (self->node->hotspotmap != NULL) {
			n = self->node->hotspotmap->getHotspot(self->node->hotspotmap, mouseH/self->node->width, mouseV/self->node->height);
		}
		else n = 0;
		hotspotPaint = n;
	}
}

static void selected(Tool *self) {
}

static void deselected(Tool *self) {
}

static void drawCursor(Tool *self, float alpha, int transitionRunning) {
	drawStandardCursor(cursors[CURSOR_EYEDROPPER], alpha);
}

Tool *newToolEyedropper(CNode *node) {
	Tool *t = malloc(sizeof(Tool));
	if (t != NULL) {
		t->tag = TOOL_EYEDROPPER;
		t->node = node;
		t->next = NULL;
		t->selected = selected;
		t->deselected = deselected;
		t->event = event;
		t->drawCursor = drawCursor;
	}
	return t;
}
