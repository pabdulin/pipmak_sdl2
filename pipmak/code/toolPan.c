/*
 
 toolPan.c, part of the Pipmak Game Engine
 Copyright (c) 2004-2007 Christian Walther
 
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

/* $Id: toolPan.c 157 2007-07-15 20:22:11Z cwalther $ */

#include "tools.h"

#include <math.h>
#include <stdlib.h>

#include "SDL_opengl.h"

#include "misc.h"
#include "audio.h"

extern int mouseX, mouseY, clickX, clickY;
extern float mouseH, mouseV;
extern int mouseButton;
extern GLenum glTextureTarget;
extern float joystickSpeed;
extern GLfloat azimuth, elevation, minaz, maxaz, minel, maxel;
extern SDL_Surface *screen;
extern struct MouseModeStackEntry *topMouseMode;
extern Image *cursors[NUMBER_OF_CURSORS];

#define MY(x) (self->private.pan.x)


static void selected(Tool *self) {
	MY(cursorRotCos) = MY(cursorRotSin) = 0;
	SDL_WarpMouseInWindow(sdl2Window, mouseX, mouseY);
}

static void deselected(Tool *self) {
	if (topMouseMode->mode == MOUSE_MODE_DIRECT) {
		mouseX = screen->w/2;
		mouseY = screen->h/2;
		SDL_WarpMouseInWindow(sdl2Window, mouseX, mouseY);
		SDL_GetRelativeMouseState(NULL, NULL);
	}
}

static void event(Tool *self, ButtonEventType type, int control, Uint32 frameDuration) {
	float f, g, h;
	switch (type) {
		case BUTTON_DOWN:
		case BUTTON_STILLDOWN:
			if (mouseX != clickX || mouseY != clickY) {
				f = (float)(-(mouseY - clickY));
				g = (float)(mouseX - clickX);
				h = sqrtf(f*f + g*g);
				MY(cursorRotCos) = f/h;
				MY(cursorRotSin) = g/h;
			}
			else {
				MY(cursorRotCos) = MY(cursorRotSin) = 0;
			}
			azimuth += 0.001f*joystickSpeed*frameDuration*(mouseX - clickX);
			while (azimuth >= (minaz + maxaz)/2 + 180) azimuth -= 360;
			while (azimuth < (minaz + maxaz)/2 - 180) azimuth += 360;
			if (azimuth < minaz) azimuth = minaz;
			else if (azimuth > maxaz) azimuth = maxaz;
			while (azimuth >= 360) azimuth -= 360;
			elevation -= 0.001f*cosf(elevation*(float)M_PI/180)*joystickSpeed*frameDuration*(mouseY - clickY);
			if (elevation < minel) elevation = minel;
			else if (elevation > maxel) elevation = maxel;
			updateListenerOrientation();
			xYtoAzEl(mouseX, mouseY, &mouseH, &mouseV);
			break;
		default:
			break;
	}
}

static void drawCursor(Tool *self, float alpha, int transitionRunning) {
	if (mouseButton) {
		Image *dot = cursors[CURSOR_DOT], *triangle = cursors[CURSOR_TRIANGLE];
		if (dot->textureID == 0) {
			feedImageToGL(dot);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		if (triangle->textureID == 0) {
			feedImageToGL(triangle);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		glColor4f(1.0, 1.0, 1.0, alpha);
		glBindTexture(glTextureTarget, dot->textureID);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f((GLfloat)(clickX - dot->cursorHotX), (GLfloat)(clickY - dot->cursorHotY));
		glTexCoord2f((float)dot->w/dot->textureWidth, 0);
		glVertex2f((GLfloat)(clickX - dot->cursorHotX + dot->w), (GLfloat)(clickY - dot->cursorHotY));
		glTexCoord2f((float)dot->w/dot->textureWidth, (float)dot->h/dot->textureHeight);
		glVertex2f((GLfloat)(clickX - dot->cursorHotX + dot->w), (GLfloat)(clickY - dot->cursorHotY + dot->h));
		glTexCoord2f(0, (float)dot->h/dot->textureHeight);
		glVertex2f((GLfloat)(clickX - dot->cursorHotX), (GLfloat)(clickY - dot->cursorHotY + dot->h));
		glEnd();

		if (MY(cursorRotCos) != 0 || MY(cursorRotSin) != 0) {
			glBindTexture(glTextureTarget, triangle->textureID);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(mouseX + 0.5f + MY(cursorRotCos)*(-triangle->cursorHotX - 0.5f) - MY(cursorRotSin)*(-triangle->cursorHotY - 0.5f), mouseY + 0.5f + MY(cursorRotSin)*(-triangle->cursorHotX - 0.5f) + MY(cursorRotCos)*(-triangle->cursorHotY - 0.5f));
			glTexCoord2f((float)triangle->w/triangle->textureWidth, 0);
			glVertex2f(mouseX + 0.5f + MY(cursorRotCos)*(triangle->w - triangle->cursorHotX - 0.5f) - MY(cursorRotSin)*(-triangle->cursorHotY - 0.5f), mouseY + 0.5f + MY(cursorRotSin)*(triangle->w - triangle->cursorHotX - 0.5f) + MY(cursorRotCos)*(-triangle->cursorHotY - 0.5f));
			glTexCoord2f((float)triangle->w/triangle->textureWidth, (float)triangle->h/triangle->textureHeight);
			glVertex2f(mouseX + 0.5f + MY(cursorRotCos)*(triangle->w - triangle->cursorHotX - 0.5f) - MY(cursorRotSin)*(triangle->h - triangle->cursorHotY - 0.5f), mouseY + 0.5f + MY(cursorRotSin)*(triangle->w - triangle->cursorHotX - 0.5f) + MY(cursorRotCos)*(triangle->h - triangle->cursorHotY - 0.5f));
			glTexCoord2f(0, (float)triangle->h/triangle->textureHeight);
			glVertex2f(mouseX + 0.5f + MY(cursorRotCos)*(-triangle->cursorHotX - 0.5f) - MY(cursorRotSin)*(triangle->h - triangle->cursorHotY - 0.5f), mouseY + 0.5f + MY(cursorRotSin)*(-triangle->cursorHotX - 0.5f) + MY(cursorRotCos)*(triangle->h - triangle->cursorHotY - 0.5f));
			glEnd();
		}
	}
	else {
		drawStandardCursor(cursors[CURSOR_PAN], alpha);
	}
}

Tool *newToolPan(CNode *node) {
	Tool *t = malloc(sizeof(Tool));
	if (t != NULL) {
		t->tag = TOOL_PAN;
		t->node = node;
		t->next = NULL;
		t->selected = selected;
		t->deselected = deselected;
		t->event = event;
		t->drawCursor = drawCursor;
		t->private.pan.cursorRotCos = 0;
		t->private.pan.cursorRotSin = 0;
	}
	return t;
}
