/*
 
 tools.h, part of the Pipmak Game Engine
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

/* $Id: tools.h 65 2006-03-27 14:52:31Z cwalther $ */

#ifndef TOOLS_H_SEEN
#define TOOLS_H_SEEN

#include "SDL.h"

#include "nodes.h"


enum Cursor { CURSOR_HAND, CURSOR_TRIANGLE, CURSOR_DOT, CURSOR_PAN, CURSOR_EYEDROPPER, NUMBER_OF_CURSORS };

typedef enum { TOOL_HAND, TOOL_PAN, TOOL_BRUSH, TOOL_EYEDROPPER, NUMBER_OF_TOOLS } ToolTag;
typedef enum { BUTTON_IDLE = 0, BUTTON_STILLDOWN = 1, BUTTON_UP = 2, BUTTON_DOWN = 3 } ButtonEventType;

typedef struct Tool {
	ToolTag tag;
	CNode *node;
	struct Tool *next;
	void (*selected)(struct Tool *);
	void (*deselected)(struct Tool *);
	void (*event)(struct Tool *, ButtonEventType type, int control, Uint32 frameDuration);
	void (*drawCursor)(struct Tool *, float alpha, int transitionRunning);
	union {
		struct {
			int lastControl, clickedControl;
			SDL_bool clickWasHandled;
			Uint32 lastMouseDownTime;
		} hand;
		struct {
			float cursorRotCos, cursorRotSin;
		} pan;
	} private;
} Tool;

void toolFree(Tool *t);
void toolPush(Tool *t);
void toolPop(ToolTag tag);
void toolChoose(Tool *t);
void drawStandardCursor(Image *cursor, float alpha);
Tool *newTool(ToolTag tag, CNode *node);
Tool *newToolBrush(CNode *node);
Tool *newToolEyedropper(CNode *node);
Tool *newToolHand(CNode *node);
Tool *newToolPan(CNode *node);

#endif /*TOOLS_H_SEEN*/
