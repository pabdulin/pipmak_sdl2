/*
 
 glstate.c, part of the Pipmak Game Engine
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

/* $Id: glstate.c 157 2007-07-15 20:22:11Z cwalther $ */

#include "glstate.h"

#include <string.h>

#include "SDL.h"
#include "SDL_opengl.h"

#include "nodes.h"
#include "images.h"
#include "config.h"
#include "misc.h"


extern GLfloat verticalFOV;
extern SDL_Surface *screen;
extern GLenum glTextureTarget;
extern GLuint screenshotTextureIDs[2];
extern Uint8 controlColorPalette[256][3];
extern GLfloat screenViewMatrix[16];
extern CNode *backgroundCNode;
extern int showControls;
extern Image *imageListStart;
extern enum TransitionState transitionState;


void updateGLProjection() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(verticalFOV, (GLdouble)screen->w/screen->h, 0.1, 1000);
	glMatrixMode(GL_MODELVIEW);
}

void setupGL() {
	int i;
	Uint32 handleStipple[32];
	GLfloat pixelMap[256];
	CNode *node;
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) {
 #ifdef WIN32
 		/* ATI's Windows XP driver (versions at least 2.0.5756 to 2.0.6120) has trouble with texture coordinates with rectangle textures on at least X800 and X1600 */
		if (strstr((const char*)glGetString(GL_RENDERER), "Radeon X") != NULL || strstr((const char*)glGetString(GL_RENDERER), "RADEON X") != NULL) {
			 glTextureTarget = GL_TEXTURE_2D;
		}
		else
 #endif
		if (strstr((const char*)glGetString(GL_EXTENSIONS), "_texture_rectangle") != NULL) {
			glEnable(GL_TEXTURE_RECTANGLE_NV);
		}
		else {
			glTextureTarget = GL_TEXTURE_2D;
			IFDEBUG(printf("GL_EXT_texture_rectangle unsupported, using GL_TEXTURE_2D.\n");)
		}
	}
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	glGenTextures(2, screenshotTextureIDs);
	glBindTexture(glTextureTarget, screenshotTextureIDs[0]);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(glTextureTarget, screenshotTextureIDs[1]);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	for (i = 0; i < 32; i++) {
		handleStipple[i] = (0x33333333 << (i%4)) | (3 >> (4 - i%4));
	}
	glPolygonStipple((GLubyte *)handleStipple);
	
	for (i = 0; i < 256; i++) pixelMap[i] = (float)controlColorPalette[i][0]/255;
	glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, pixelMap);
	for (i = 0; i < 256; i++) pixelMap[i] = (float)controlColorPalette[i][1]/255;
	glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, pixelMap);
	for (i = 0; i < 256; i++) pixelMap[i] = (float)controlColorPalette[i][2]/255;
	glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, pixelMap);
	pixelMap[0] = 0.0f;
	for (i = 1; i < 256; i++) pixelMap[i] = 0.5f;
	glPixelMapfv(GL_PIXEL_MAP_I_TO_A, 256, pixelMap);
	
	glViewport(0, 0, screen->w, screen->h);
	updateGLProjection();
	screenViewMatrix[0] = 2.0f/screen->w;
	screenViewMatrix[5] = -2.0f/screen->h;
	
	node = backgroundCNode;
	while (node != NULL) {
		nodeFeedToGL(node);
		node = node->next;
	}
}

void cleanupGL() {
	Image *img;
	CNode *node = backgroundCNode;
	while (node != NULL) {
		nodeRemoveFromGL(node);
		node = node->next;
	}
	img = imageListStart;
	while (img != NULL) {
		if (img->textureID != 0) {
			glDeleteTextures(1, &(img->textureID));
			img->textureID = 0;
			img->texrefcount = 0;
		}
		img = img->next;
	}
	glDeleteTextures(2, screenshotTextureIDs);
	transitionState = TRANSITION_IDLE;
	if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) glDisable(GL_TEXTURE_RECTANGLE_NV);
}

void captureScreenGL(int i) {
	int w, h;
	CNode *node;
	if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) {
		w = screen->w;
		h = screen->h;
	}
	else {
		w = 1;
		while (w < screen->w) w *= 2;
		h = 1;
		while (h < screen->h) h *= 2;
	}
	node = backgroundCNode;
	while (node != NULL) {
		if (node->type != NODE_TYPE_PANEL || node == backgroundCNode) node->draw(node);
		node = node->next;
	}
	glFlush();
	glBindTexture(glTextureTarget, screenshotTextureIDs[i]);
	glCopyTexImage2D(glTextureTarget, 0, GL_RGB, 0, 0, w, h, 0);
}
