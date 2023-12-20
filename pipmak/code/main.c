/*
 
 main.c, part of the Pipmak Game Engine
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

/* $Id: main.c 228 2011-04-30 19:40:44Z cwalther $ */

#ifndef _MSC_VER
#include <unistd.h> /* chdir */
#else
#include <direct.h> /* chdir */
#define chdir _chdir  /* prevent warning about deprecation of chdir */
#endif

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include "lua.h"
#include "physfs.h"

#include "images.h"
#include "nodes.h"
#include "misc.h"
#include "tools.h"
#include "platform.h"
#include "terminal.h"
#include "glstate.h"
#include "opensave.h"
#include "config.h"
#include "audio.h"
#include "textedit.h"

#include "controlColorPalette.h"
Uint32 lastMouseMoveTime, thisRedrawTime, transitionStartTime, mouseWarpEndTime = 0;
struct MouseModeStackEntry *topMouseMode;
Image *currentCursor, *standardCursor;
float joystickSpeed;
int mouseButton;
int mouseX, mouseY, clickX, clickY, mouseWarpStartX, mouseWarpStartY;
float mouseH, mouseV, clickH, clickV;
int touchedControl;
CNode *touchedNode;
GLfloat azimuth, elevation, minaz, maxaz, minel, maxel;
GLfloat verticalFOV;
SDL_Surface *screen;
CNode *backgroundCNode, *frontCNode, *thisCNode = NULL;
lua_State *L = NULL;
GLint glTextureFilter = GL_LINEAR;
GLenum glTextureTarget = GL_TEXTURE_RECTANGLE_NV; /*identical to the newer GL_T_R_EXT and GL_T_R_ARB, but SDL_opengl.h defines it as GL_T_R_NV*/
int showControls = 0;
GLuint screenshotTextureIDs[2];
enum TransitionState transitionState = TRANSITION_IDLE;
enum TransitionEffect currentTransitionEffect;
enum TransitionDirection currentTransitionDirection = TRANSITION_LEFT;
Uint32 transitionDuration = 0;
float transitionParameter = 2;
enum DisruptiveInstruction disruptiveInstruction = INSTR_NONE;
char *disruptiveInstructionArgumentCharP = NULL;
char *disruptiveInstructionPostCode;
Image *imageListStart = NULL;
int hotspotPaint = 1;
GLfloat screenViewMatrix[16] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	-1, 1, -1, 1
};
Image *cursors[NUMBER_OF_CURSORS];
SDL_Rect desktopsize;
int cpTermToStdout = 0;


void redrawGL(float fade) {
	CNode *node;
	Uint32 ticks = SDL_GetTicks();
	
	if (fade < 1) {
		float f, g, s;
		if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) {
			f = (float)screen->w;
			g = (float)screen->h;
		}
		else {
			f = 1;
			while (f < screen->w) f *= 2;
			f = screen->w/f;
			g = 1;
			while (g < screen->h) g *= 2;
			g = screen->h/g;
		}
		switch (currentTransitionEffect) {
			case TRANSITION_DISSOLVE:
				glLoadIdentity(); /*Slide View*/
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glBindTexture(glTextureTarget, screenshotTextureIDs[1]);
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-1, -1);
				glTexCoord2f(f, 0);
				glVertex2f(1, -1);
				glTexCoord2f(f, g);
				glVertex2f(1, 1);
				glTexCoord2f(0, g);
				glVertex2f(-1, 1);
				glEnd();
				s = (1-fade)*1 + fade*transitionParameter;
				glColor4f(1.0, 1.0, 1.0, 1-fade);
				glBindTexture(glTextureTarget, screenshotTextureIDs[0]);
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-s, -s);
				glTexCoord2f(f, 0);
				glVertex2f(s, -s);
				glTexCoord2f(f, g);
				glVertex2f(s, s);
				glTexCoord2f(0, g);
				glVertex2f(-s, s);
				glEnd();
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
				break;
			case TRANSITION_WIPE: {
				float f1, f2, f3, f4;
				switch (currentTransitionDirection) {
					case TRANSITION_LEFT:
						f1 = 0;
						f2 = 1-fade;
						f3 = 0;
						f4 = 1;
						break;
					case TRANSITION_RIGHT:
						f1 = fade;
						f2 = 1;
						f3 = 0;
						f4 = 1;
						break;
					case TRANSITION_UP:
						f1 = 0;
						f2 = 1;
						f3 = fade;
						f4 = 1;
						break;
					case TRANSITION_DOWN:
					default:
						f1 = 0;
						f2 = 1;
						f3 = 0;
						f4 = 1-fade;
						break;
				}
				glLoadIdentity(); /*Slide View*/
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glBindTexture(glTextureTarget, screenshotTextureIDs[1]);
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(-1, -1);
				glTexCoord2f(f, 0);
				glVertex2f(1, -1);
				glTexCoord2f(f, g);
				glVertex2f(1, 1);
				glTexCoord2f(0, g);
				glVertex2f(-1, 1);
				glEnd();
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glBindTexture(glTextureTarget, screenshotTextureIDs[0]);
				glBegin(GL_QUADS);
				glTexCoord2f(f1*f, f3*g);
				glVertex2f(-1+2*f1, -1+2*f3);
				glTexCoord2f(f2*f, f3*g);
				glVertex2f(-1+2*f2, -1+2*f3);
				glTexCoord2f(f2*f, f4*g);
				glVertex2f(-1+2*f2, -1+2*f4);
				glTexCoord2f(f1*f, f4*g);
				glVertex2f(-1+2*f1, -1+2*f4);
				glEnd();
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
			} break;
			case TRANSITION_ROTATE: {
				GLfloat transMatrix[16] = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 1
				};
				float x1, x2, x3, x4, y1, y2, y3, y4, z11, z12, z21, z22, z33, z34, z43, z44;
				transMatrix[11] = -tanf(fabsf(transitionParameter)/2);
				switch (currentTransitionDirection) {
					case TRANSITION_LEFT:
						x1 = fade*(-1 - 2*cosf(transitionParameter)) + (1-fade)*(-1);
						x2 = x3 = 1 - 2*fade;
						x4 = fade*1 + (1-fade)*(1+2*cosf(transitionParameter));
						y1 = y3 = -1;
						y2 = y4 = 1;
						z11 = z12 = fade*2*sinf(transitionParameter);
						z21 = z22 = z33 = z34 = 0;
						z43 = z44 = (1-fade)*2*sinf(transitionParameter);
						break;
					case TRANSITION_RIGHT:
						x1 = x4 = -1 + 2*fade;
						x2 = (1-fade)*1 + fade*(1+2*cosf(transitionParameter));
						x3 = (1-fade)*(-1 - 2*cosf(transitionParameter)) + fade*(-1);
						y1 = y3 = -1;
						y2 = y4 = 1;
						z21 = z22 = fade*2*sinf(transitionParameter);
						z33 = z34 = (1-fade)*2*sinf(transitionParameter);
						z11 = z12 = z43 = z44 = 0;
						break;
					case TRANSITION_UP:
						y1 = y4 = -1 + 2*fade;
						y2 = (1-fade)*1 + fade*(1+2*cosf(transitionParameter));
						y3 = (1-fade)*(-1 - 2*cosf(transitionParameter)) + fade*(-1);
						x1 = x3 = -1;
						x2 = x4 = 1;
						z22 = z12 = fade*2*sinf(transitionParameter);
						z21 = z11 = z44 = z34 = 0;
						z43 = z33 = (1-fade)*2*sinf(transitionParameter);
						break;
					case TRANSITION_DOWN:
					default:
						y1 = fade*(-1 - 2*cosf(transitionParameter)) + (1-fade)*(-1);
						y2 = y3 = 1 - 2*fade;
						y4 = fade*1 + (1-fade)*(1+2*cosf(transitionParameter));
						x1 = x3 = -1;
						x2 = x4 = 1;
						z11 = z21 = fade*2*sinf(transitionParameter);
						z12 = z22 = z33 = z43 = 0;
						z34 = z44 = (1-fade)*2*sinf(transitionParameter);
						break;
				}
				glLoadIdentity(); /*Rotation View*/
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadMatrixf(transMatrix);
				
				glClearColor(0.0, 0.0, 0.0, 1.0);
				glClear(GL_COLOR_BUFFER_BIT);
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glBindTexture(glTextureTarget, screenshotTextureIDs[0]);
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex3f(x1, y1, z11);
				glTexCoord2f(f, 0);
				glVertex3f(x2, y1, z21);
				glTexCoord2f(f, g);
				glVertex3f(x2, y2, z22);
				glTexCoord2f(0, g);
				glVertex3f(x1, y2, z12);
				glEnd();
				glBindTexture(glTextureTarget, screenshotTextureIDs[1]);
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex3f(x3, y3, z33);
				glTexCoord2f(f, 0);
				glVertex3f(x4, y3, z43);
				glTexCoord2f(f, g);
				glVertex3f(x4, y4, z44);
				glTexCoord2f(0, g);
				glVertex3f(x3, y4, z34);
				glEnd();
				
				glPopMatrix();
				glMatrixMode(GL_MODELVIEW);
			} break;
		}
		node = (backgroundCNode == NULL) ? NULL : backgroundCNode->next;
		while (node != NULL) {
			if (node->type == NODE_TYPE_PANEL) node->draw(node);
			node = node->next;
		}
	}
	else {
		/* GL matrices state for CNode->draw():
		- current matrix is GL_MODELVIEW (must be restored if changed)
		- projection is perspective (must be retained)
		- modelview is undefined (may be changed)
		*/
		node = backgroundCNode;
		while (node != NULL) {
			node->draw(node);
			node = node->next;
		}
	}
	
	glPushMatrix();
	glLoadMatrixf(screenViewMatrix);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	drawTerminal(ticks);
	
#if (0) /* debug: show the contents of each texture (2D, not adapted to rectangle yet)*/
	int i, j;
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 7; j++) {
			if (glIsTexture(10*j+i)) {
				glColor4f(1.0, 1.0, 1.0, 1.0);
				glBindTexture(GL_TEXTURE_2D, 10*j+i);
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0);
				glVertex2f(64*i, 64*j);
				glTexCoord2f(1, 0);
				glVertex2f(64*i+60, 64*j);
				glTexCoord2f(1, 1);
				glVertex2f(64*i+60, 64*j+60);
				glTexCoord2f(0, 1);
				glVertex2f(64*i, 64*j+60);
				glEnd();
			}
		}
	}
#endif
	
	/* GL matrices state for drawCursor():
	- current matrix is GL_PROJECTION (must be restored if changed)
	- projection is identity (may be changed), perspective is below it in the stack (must be retained; stack depth must be restored)
	- modelview is screen view (may be changed), whatever node drawing used is below it (may be changed; stack depth must be restored)
	*/
	touchedNode->tool->drawCursor(touchedNode->tool, 1.0f - 0.7f*smoothCubic(((float)(((int)(ticks - lastMouseMoveTime)) - 1000))/2000), (fade < 1));
	
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	SDL_GL_SwapBuffers();
}

int main(int argc, char *argv[]) {
	
	SDL_Event event;
	Uint32 lastRedrawTime = 0, frameDuration;
	int mouseDeltaX, mouseDeltaY;
	float smoothMouseDeltaX = 0, smoothMouseDeltaY = 0, smoothingCoeff;
	int t, u;
	int i;
	char **pfslist, **pfslisti;
	char *p, *q, *r;
	Uint8 utf8char[4];
	CNode *n;
	MouseModeToken shiftMouseModeToken = NULL;
	
	p = directoryFromPath(argv[0]);
	if (p != NULL) {
		chdir(p);
		free(p);
	}
	
	initGUI();
	
	setlocale(LC_NUMERIC, "C"); /*otherwise, Lua will fail to run defaults.lua if the current locale doesn't use a period as the decimal point*/ /*this must come after initGUI(), as GTK apparently resets it*/
	
	if((SDL_Init(SDL_INIT_VIDEO)==-1)) { 
		errorMessage("Could not initialize SDL: %s", SDL_GetError());
		quit(1);
	}
	
	// TODO(pabdulin): check fix#1 SDL_GetVideoInfo
	/* Store current desktop size - must be done before calling SDL_SetVideoMode */
	// const SDL_VideoInfo *videoinfo;   /* to get videoinfo and current desktop size */
	// videoinfo =  SDL_GetVideoInfo();
	// desktopsize.w = videoinfo->current_w;
	// desktopsize.h = videoinfo->current_h;
	// see: https://wiki.libsdl.org/SDL2/SDL_GetDesktopDisplayMode
	SDL_DisplayMode videoinfo;
	SDL_GetDesktopDisplayMode(0, &videoinfo);
	desktopsize.w = videoinfo.w;
	desktopsize.h = videoinfo.h;

	SDL_EnableUNICODE(SDL_ENABLE);
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	#if ! SDL_VERSION_ATLEAST(1, 2, 10)
		#define SDL_GL_SWAP_CONTROL 16
	#endif
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
	
	screen = SDL_SetVideoMode(640, 480, 0, SDL_OPENGL | SDL_RESIZABLE);
	if (screen == NULL) {
		errorMessage("Could not set video mode: %s", SDL_GetError());
		quit(1);
	}
	
	PHYSFS_init(argv[0]);
	if (!prependResourcesToPhysfsSearchpath()) {
		errorMessage("Could not find the Pipmak Resources file. Try reinstalling Pipmak.");
		quit(1);
	}
	
	verticalFOV = 45;
	setupGL();
		
	if (!initTerminal()) {
		errorMessage("Could not initialize the Pipmak terminal");
		quit(1);
	}
	
	if (!audioInit()) {
		errorMessage("Could not initialize audio");
	}
	
	topMouseMode = malloc(sizeof(struct MouseModeStackEntry));
	if (topMouseMode == NULL) {
		errorMessage("Out of memory");
		quit(1);
	}
	topMouseMode->next = NULL;
	topMouseMode->mode = MOUSE_MODE_JOYSTICK;
	joystickSpeed = 1;
	
	if (argc >= 2) {
		/*We're asked to open a specific file*/
		if (!openAndEnterProject(argv[1])) {
			/*It didn't work, so it's definitely not a pipmak project - let's see if it's a saved game*/
			if (!openSavedGame(argv[1])) {
				terminalPrintf("\"%s\" could neither be opened as a Pipmak project nor as a saved game.", argv[1]);
				openAndEnterProject(NULL);
			}
		}
	}
	else {
		/*We weren't asked to open a specific file, so let's go hunting for pipmak projects*/
		PHYSFS_addToSearchPath(PHYSFS_getBaseDir(), 1);
		pfslist = PHYSFS_getCdRomDirs();
		for (pfslisti = pfslist; *pfslisti != NULL; pfslisti++) {
			PHYSFS_addToSearchPath(*pfslisti, 1);
		}
		PHYSFS_freeList(pfslist);
		
		r = NULL;
		pfslist = PHYSFS_enumerateFiles("");
		for (pfslisti = pfslist; *pfslisti != NULL; pfslisti++) {
			if (strEndsWith(*pfslisti, ".pipmak")) {
				p = (char*)PHYSFS_getRealDir(*pfslisti);
				q = (char*)PHYSFS_getDirSeparator();
				r = (char*)malloc(strlen(p) + strlen(q) + strlen(*pfslisti) + 1);
				strcpy(r, p);
				if (!strEndsWith(p, q)) strcat(r, q);
				strcat(r, *pfslisti);
				break;
			}
		}
		PHYSFS_freeList(pfslist);
		
		pfslist = PHYSFS_getSearchPath();
		for (pfslisti = pfslist; *pfslisti != NULL; pfslisti++) {
			PHYSFS_removeFromSearchPath(*pfslisti);
		}
		PHYSFS_freeList(pfslist);
		
		prependResourcesToPhysfsSearchpath();
		if (r != NULL) {
			if (!openAndEnterProject(r)) {
				terminalPrintf("Couldn't open project \"%s\"", r);
				openAndEnterProject(NULL);
			}
			free(r);
		}
		else {
			openAndEnterProject(NULL);
		}
	}
	
	SDL_ShowCursor(SDL_DISABLE);
	
		
	for (;;) { /*Main Run Loop*/
		
		thisRedrawTime = SDL_GetTicks();
		frameDuration = thisRedrawTime - lastRedrawTime;
		
		/*Do disruptive actions that should not be done in the middle of a Lua function*/
		
		switch (disruptiveInstruction) {
			case INSTR_OPENSAVEDGAME: {
				/*Lua function has already switched to windowed*/
				SDL_WM_GrabInput(SDL_GRAB_OFF);
				SDL_ShowCursor(SDL_ENABLE);
				p = openSavedGamePath();
				SDL_ShowCursor(SDL_DISABLE);
				if (p != NULL) {
					if (!openSavedGame(p)) terminalPrintf("Couldn't open saved game \"%s\".", p);
					free(p);
				}
			} break;
			case INSTR_OPENPROJECT: {
				if (disruptiveInstructionArgumentCharP == NULL) {
					/*Lua function has already switched to windowed*/
					SDL_WM_GrabInput(SDL_GRAB_OFF);
					SDL_ShowCursor(SDL_ENABLE);
					p = openProjectPath();
					SDL_ShowCursor(SDL_DISABLE);
				}
				else {
					p = disruptiveInstructionArgumentCharP;
					disruptiveInstructionArgumentCharP = NULL;
				}
				if (p != NULL) {
					if (!openAndEnterProject(p)) terminalPrintf("Couldn't open project \"%s\".", p);
					free(p);
				}
				if (disruptiveInstructionPostCode != NULL) {
					if (luaL_loadbuffer(L, disruptiveInstructionPostCode, strlen(disruptiveInstructionPostCode), "postcode") != 0 || pcallWithBacktrace(L, 0, 0, NULL) != 0) {
						terminalPrint(lua_tostring(L, -1), 0);
						lua_pop(L, 1);
					}
					free(disruptiveInstructionPostCode);
					disruptiveInstructionPostCode = NULL;
				}
			} break;
			default:
				break;
		}
		disruptiveInstruction = INSTR_NONE;
		/*touchedNode may be NULL at this point*/
		
		/*Handle mouse motion*/
		
		if ( !(backgroundCNode->type & 1) /*panoramic*/ && topMouseMode->mode == MOUSE_MODE_DIRECT && backgroundCNode->tool->tag != TOOL_PAN) {
			SDL_GetRelativeMouseState(&mouseDeltaX, &mouseDeltaY);
			if (mouseDeltaX > screen->w/2 || mouseDeltaX < -screen->w/2 || mouseDeltaY > screen->h/2 || mouseDeltaY < -screen->h/2) mouseDeltaX = mouseDeltaY = 0; /*ignore large deltas that are caused by lifting a tablet pen*/
			if (mouseDeltaX != 0 || mouseDeltaY != 0) lastMouseMoveTime = thisRedrawTime;
			smoothingCoeff = 1.0f/(1.0f + 0.06f*frameDuration); /*the proper expression to get a constant decay time independent of frame rate would be 2^(-0.06*frameDuration), but I don't think this is worth calculating exponentials, and this crude approximation works well enough*/
			smoothMouseDeltaX = smoothingCoeff*smoothMouseDeltaX + (1.0f - smoothingCoeff)*mouseDeltaX;
			smoothMouseDeltaY = smoothingCoeff*smoothMouseDeltaY + (1.0f - smoothingCoeff)*mouseDeltaY;
			if (smoothMouseDeltaX != 0 || smoothMouseDeltaY != 0) {
				azimuth += verticalFOV/screen->h*smoothMouseDeltaX/cosf(elevation*(float)M_PI/180);
				while (azimuth >= (minaz + maxaz)/2 + 180) azimuth -= 360;
				while (azimuth < (minaz + maxaz)/2 - 180) azimuth += 360;
				if (azimuth < minaz) azimuth = minaz;
				else if (azimuth > maxaz) azimuth = maxaz;
				while (azimuth >= 360) azimuth -= 360;
				
				elevation -= verticalFOV/screen->h*smoothMouseDeltaY;
				if (elevation < minel) elevation = minel;
				else if (elevation > maxel) elevation = maxel;
				
				updateListenerOrientation();
			}
			if (mouseWarpEndTime != 0) {
				mouseX = screen->w/2 + (int)(smoothCubic((float)((int)mouseWarpEndTime - (int)thisRedrawTime)/MOUSE_WARP_DURATION)*(mouseWarpStartX - screen->w/2));
				mouseY = screen->h/2 + (int)(smoothCubic((float)((int)mouseWarpEndTime - (int)thisRedrawTime)/MOUSE_WARP_DURATION)*(mouseWarpStartY - screen->h/2));
				if (mouseWarpEndTime < thisRedrawTime) mouseWarpEndTime = 0;
			}
		}
		else {
			t = mouseX;
			u = mouseY;
			SDL_GetMouseState(&mouseX, &mouseY);
			if (mouseX != t || mouseY != u) lastMouseMoveTime = thisRedrawTime;
		}
		
		updateTouchedNode();
		touchedNode->mouseXYtoHV(touchedNode, mouseX, mouseY, &mouseH, &mouseV);
		touchedControl = getControl(touchedNode, mouseH, mouseV);
		touchedNode->tool->event(touchedNode->tool, (mouseButton != 0), touchedControl, frameDuration);
		
		/*Handle events*/
		
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_MOUSEMOTION:
					if (!(screen->flags & SDL_FULLSCREEN) && (SDL_GetAppState() & SDL_APPINPUTFOCUS)) {
						/*ungrab if the mouse runs against the window edge hard enough*/
						if ((mouseX == 0 && event.motion.xrel < 0) || (mouseX == screen->w-1 && event.motion.xrel > 0)
							|| (mouseY == 0 && event.motion.yrel < 0) || (mouseY == screen->h-1 && event.motion.yrel > 0)
						) {
							SDL_WM_GrabInput(SDL_GRAB_OFF);
							SDL_WarpMouse(mouseX, mouseY);
						}
						/*regrab, with a slightly inset boundary to allow resizing of the window (at least on Mac OS X, the resizing handle is inside the window and thus can't be used while grabbed)*/
						else if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_OFF
							&& mouseX > 5 && mouseX < screen->w-6
							&& mouseY > 5 && mouseY < screen->h-6
						) {
							SDL_WM_GrabInput(SDL_GRAB_ON);
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (mouseButton == 0 && transitionState == TRANSITION_IDLE ) {
						/*ignore event if another mouse button is already down
							and there isn't a transition running*/
						defocusTextEditors();
						mouseButton = event.button.button;
						clickX = mouseX;
						clickY = mouseY;
						clickH = mouseH;
						clickV = mouseV;
						if (mouseButton == SDL_BUTTON_RIGHT) {
							toolPush(newToolPan(backgroundCNode));
						}
						touchedNode->tool->event(touchedNode->tool, BUTTON_DOWN, touchedControl, frameDuration);
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if (SDL_GetMouseState(NULL, NULL) == 0 /*ignore event if other mouse buttons are still down*/
							&& mouseButton != 0 /*use this to determine the difference between a button	coming up from a click BEFORE a transition, and one	DURING a transition*/)
					{
						mouseButton = 0;
						touchedNode->tool->event(touchedNode->tool, BUTTON_UP, touchedControl, frameDuration);
						toolPop(TOOL_PAN);
					}
					break;
				case SDL_ACTIVEEVENT:
					if (event.active.gain == 0 && (event.active.state & (SDL_APPINPUTFOCUS | SDL_APPACTIVE)) && !(screen->flags & SDL_FULLSCREEN)) {
						SDL_WM_GrabInput(SDL_GRAB_OFF);
					}
					break;
				case SDL_VIDEORESIZE:
					terminalClear();
					cleanupGL();
					screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 0, SDL_OPENGL | SDL_RESIZABLE);
					setupGL();
					terminalPrintf("%d x %d\n", screen->w, screen->h);
					break;
				case SDL_QUIT:
					quit(0);
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						/*case SDLK_SPACE: //debug
							terminalPrintf("update: %d", updateFile("3/node.lua", 111, "myfunc%s*%b{}", "myfunc { a = %d, b = \"%s\" }", 15, "la"));
							break;*/
						case SDLK_LSHIFT:
						case SDLK_RSHIFT:
						case SDLK_CAPSLOCK:
							if (shiftMouseModeToken == NULL) shiftMouseModeToken = pushMouseMode((topMouseMode->mode + 1)%NUMBER_OF_MOUSE_MODES);
							break;
						case SDLK_LALT:
						case SDLK_RALT:
							if (backgroundCNode->tool->tag == TOOL_BRUSH) {
								toolPush(newToolEyedropper(backgroundCNode));
							}
							break;
						case SDLK_LCTRL:
						case SDLK_RCTRL:
						case SDLK_LMETA:
						case SDLK_RMETA:
							break;
						default:
							if (!textEditHandleKey(&event.key)) {
								if (event.key.keysym.unicode < 0x0080) {
									utf8char[0] = (event.key.keysym.unicode & 0x007F);
									utf8char[1] = 0;
								}
								else if (event.key.keysym.unicode < 0x0800) {
									utf8char[0] = 0xC0 | ((event.key.keysym.unicode & 0x07C0) >> 6);
									utf8char[1] = 0x80 | (event.key.keysym.unicode & 0x003F);
									utf8char[2] = 0;
								}
								else {
									utf8char[0] = 0xE0 | ((event.key.keysym.unicode & 0xF000) >> 12);
									utf8char[1] = 0x80 | ((event.key.keysym.unicode & 0x0FC0) >> 6);
									utf8char[2] = 0x80 | (event.key.keysym.unicode & 0x003F);
									utf8char[3] = 0;
								}
								n = frontCNode;
								i = 1;
								while (n != NULL && i) {
									lua_rawgeti(L, LUA_REGISTRYINDEX, n->noderef);
									lua_pushliteral(L, "onkeydown");
									lua_rawget(L, -2);
									if (lua_isfunction(L, -1)) {
										lua_pushnumber(L, event.key.keysym.sym);
										lua_pushstring(L, (char *)utf8char);
										if (pcallWithBacktrace(L, 2, 1, n) != 0) {
											terminalPrintf("Error running local keydown handler:\n%s", lua_tostring(L, -1));
											lua_pop(L, 1);
										}
										else {
											i = !lua_toboolean(L, -1);
											lua_pop(L, 1);
										}
									}
									else {
										lua_pop(L, 1); /*non-function*/
									}
									lua_pop(L, 1); /*node*/
									n = n->prev;
								}
								if (i) {
									lua_pushliteral(L, "pipmak_internal");
									lua_rawget(L, LUA_GLOBALSINDEX);
									lua_pushliteral(L, "project");
									lua_rawget(L, -2);
									lua_pushliteral(L, "onkeydown");
									lua_rawget(L, -2);
									if (lua_isfunction(L, -1)) {
										lua_pushnumber(L, event.key.keysym.sym);
										lua_pushstring(L, (char *)utf8char);
										if (pcallWithBacktrace(L, 2, 0, backgroundCNode) != 0) {
											terminalPrintf("Error running global keydown handler:\n%s", lua_tostring(L, -1));
											lua_pop(L, 1);
										}
									}
									else {
										lua_pop(L, 1); /*non-function*/
									}
									lua_pop(L, 2); /*project, pipmak_internal*/
								}
							}
							break;
					}
					break;
				case SDL_KEYUP:
					switch (event.key.keysym.sym) {
						case SDLK_LSHIFT:
						case SDLK_RSHIFT:
						case SDLK_CAPSLOCK:
							popMouseMode(shiftMouseModeToken);
							shiftMouseModeToken = NULL;
							break;
						case SDLK_LALT:
						case SDLK_RALT:
							toolPop(TOOL_EYEDROPPER);
							break;
						default: /*just to avoid a compiler warning*/
							break;
					}
					break;
			}
		}

		/*Handle timers*/
		
		lua_pushliteral(L, "pipmak_internal");
		lua_rawget(L, LUA_GLOBALSINDEX);
		lua_pushliteral(L, "runtimers");
		lua_rawget(L, -2);
		n = backgroundCNode;
		while (n != NULL) {
			lua_pushvalue(L, -1); /*runtimers*/
			lua_pushnumber(L, (lua_Number)thisRedrawTime/1000);
			if (pcallWithBacktrace(L, 1, 0, n) != 0) {
				/*should never be reached as errors are handled on the Lua side*/
				terminalPrintf("Error running scheduled function:\n%s", lua_tostring(L, -1));
				lua_pop(L, 1);
			}
			n = n->next;
		}
		lua_pop(L, 2); /*runtimers, pipmak_internal*/
		
		/*Free autofreed CNodes*/
		
		freeAutofreedCNodes();

		/*Redraw*/
		
		switch (transitionState) {
			case TRANSITION_PENDING:
				captureScreenGL(1);
				transitionStartTime = SDL_GetTicks();
				if (mouseWarpEndTime != 0) mouseWarpEndTime += transitionStartTime - thisRedrawTime;
				thisRedrawTime = transitionStartTime;
				transitionState = TRANSITION_RUNNING;
				/*fall through*/
			case TRANSITION_RUNNING:
				if (thisRedrawTime - transitionStartTime <= transitionDuration) {
					redrawGL(smoothCubic((float)(thisRedrawTime - transitionStartTime)/transitionDuration));
					break;
				}
				transitionState = TRANSITION_IDLE;
				/*fall through*/
			case TRANSITION_IDLE:
				redrawGL(1);
		}
		
		lastRedrawTime = thisRedrawTime;
	}
	
	/*never reached*/
	return 0;
}
