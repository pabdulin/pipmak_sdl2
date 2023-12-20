/*
 
 toolBrush.c, part of the Pipmak Game Engine
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

/* $Id: toolBrush.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "tools.h"

#include <math.h>
#include <stdlib.h>

#include "SDL_opengl.h"


extern float mouseH, mouseV;
extern GLfloat azimuth, elevation;
extern GLenum glTextureTarget;
extern int hotspotPaint;


static void event(Tool *self, ButtonEventType type, int control, Uint32 frameDuration) {
	if (type & 1) { /*BUTTON_DOWN or BUTTON_STILLDOWN*/
		if (self->node->hotspotmap != NULL) self->node->hotspotmap->setPixels(self->node->hotspotmap, mouseH/self->node->width, mouseV/self->node->height, 4, hotspotPaint);
	}
}

static void selected(Tool *self) {
}

static void deselected(Tool *self) {
}

static void drawCursor(Tool *self, float alpha, int transitionRunning) {
	if (!transitionRunning) {
		if (self->node->type == NODE_TYPE_SLIDE) {
			/*to be implemented*/
		}
		else {
			float r;
			int i, n;
			r = 4*(float)M_PI/180;
			n = (int)(400*r);
			if (n < 5) n = 5;
			glPopMatrix(); /*perspective*/
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(-elevation, 1, 0, 0);
			glRotatef(azimuth - mouseH, 0, 1, 0);
			glRotatef(mouseV, 1, 0, 0);
			glDisable(GL_TEXTURE_2D);
			glDisable(glTextureTarget);
			glBegin(GL_LINES);
			glColor4f(0, 0, 0, 0.5);
			for (i = 0; i < 2*n; i++) {
				glVertex3f(sinf(r)*sinf(i*(float)M_PI/n), sinf(r)*cosf(i*(float)M_PI/n), -cosf(r));
			}
			glColor4f(1, 1, 1, 0.5);
			for (i = 1; i <= 2*n; i++) {
				glVertex3f(sinf(r)*sinf(i*(float)M_PI/n), sinf(r)*cosf(i*(float)M_PI/n), -cosf(r));
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glEnable(glTextureTarget);
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
		}
	}
}

Tool *newToolBrush(CNode *node) {
	Tool *t = malloc(sizeof(Tool));
	if (t != NULL) {
		t->tag = TOOL_BRUSH;
		t->node = node;
		t->next = NULL;
		t->selected = selected;
		t->deselected = deselected;
		t->event = event;
		t->drawCursor = drawCursor;
	}
	return t;
}
