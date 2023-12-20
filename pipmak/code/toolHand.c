/*
 
 toolHand.c, part of the Pipmak Game Engine
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

/* $Id: toolHand.c 144 2007-01-03 20:00:28Z cwalther $ */

#include "tools.h"

#include <math.h>
#include <stdlib.h>

#include "lua.h"

#include "misc.h"
#include "config.h"


extern int mouseButton;
extern Image *currentCursor;
extern lua_State *L;
extern Uint32 thisRedrawTime, lastMouseMoveTime;
extern struct MouseModeStackEntry *topMouseMode;

#define MY(x) (self->private.hand.x)


static void effectiveEvent(Tool *self, ButtonEventType type, int control) {
	int clickedControl = MY(clickedControl); /*save this since a recursive call in a handler might pull it out from under us*/
	switch (type) {
		case BUTTON_DOWN:
			if (clickedControl != control) effectiveEvent(self, BUTTON_IDLE, control); /*ensure that onmouseenter etc. is called even when the current BUTTON_DOWN comes immediately after entering a new control, with no intervening BUTTON_IDLE*/
			if (control > 0) {
				callHandlerWithBoolean("onhilite", control, self->node, 1);
				callHandler("onmousedown", control, self->node);
			}
			else { /*fade out the cursor immediately when clicking in the void*/
				if (lastMouseMoveTime > thisRedrawTime - 1800) lastMouseMoveTime = thisRedrawTime - 1800;
			}
			break;
		case BUTTON_STILLDOWN:
			if (clickedControl > 0) {
				if (control != MY(lastControl) && (clickedControl == control || clickedControl == MY(lastControl))) {
					callHandlerWithBoolean("onhilite", clickedControl, self->node, (control == clickedControl));
				}
				callHandler("onmousestilldown", clickedControl, self->node);
			}
			break;
		case BUTTON_UP:
			if (clickedControl > 0) {
				if (control == clickedControl) {
					callHandler("onmouseup", control, self->node);
					callHandlerWithBoolean("onhilite", control, self->node, 0);
				}
				callHandler("onenddrag", clickedControl, self->node);
			}
			/*fall through so that the code below is executed at least once, even if another BUTTON_DOWN follows immediately*/
		case BUTTON_IDLE:
			if (control != clickedControl) {
				if (clickedControl > 0) callHandler("onmouseleave", clickedControl, self->node);
				if (control > 0) {
					lua_rawgeti(L, LUA_REGISTRYINDEX, self->node->noderef);
					lua_pushliteral(L, "controls"); lua_rawget(L, -2);
					lua_rawgeti(L, -1, control);
					lua_pushliteral(L, "cursor"); lua_rawget(L, -2);
					currentCursor = (lua_isuserdata(L, -1)) ? lua_touserdata(L, -1) : self->node->standardCursor;
					lua_pop(L, 4); /*cursor, control, controls, node*/
					callHandler("onmouseenter", control, self->node);
				}
				else currentCursor = self->node->standardCursor;
				MY(clickedControl) = control;
			}
			else if (control > 0) {
				callHandler("onmousewithin", control, self->node);
			}
			break;
	}
	MY(lastControl) = control;
}

static void event(Tool *self, ButtonEventType type, int control, Uint32 frameDuration) {
	if (topMouseMode->mode == MOUSE_MODE_DIRECT || (self->node->type & 1)) {
		effectiveEvent(self, type, control);
	}
	else { /*joystick and panoramic*/
		switch (type) {
			case BUTTON_DOWN:
				if (control == 0) {
					toolPush(newToolPan(self->node));
				}
				else {
					int dontpan;
					lua_rawgeti(L, LUA_REGISTRYINDEX, self->node->noderef);
					lua_pushliteral(L, "controls");
					lua_rawget(L, -2);
					lua_rawgeti(L, -1, control);
					lua_pushliteral(L, "dont_pan");
					lua_rawget(L, -2);
					dontpan = lua_toboolean(L, -1);
					lua_pop(L, 4); /*dont_pan, control, controls, node*/
					MY(clickWasHandled) = dontpan;
					if (dontpan) {
						effectiveEvent(self, BUTTON_DOWN, control);
					}
					else {
						MY(lastMouseDownTime) = thisRedrawTime;
					}
				}
				break;
			case BUTTON_STILLDOWN:
				if (MY(clickWasHandled)) {
					effectiveEvent(self, BUTTON_STILLDOWN, control);
				}
				else if (thisRedrawTime - MY(lastMouseDownTime) >= CLICK_DURATION) {
					toolPush(newToolPan(self->node));
				}
				else {
					effectiveEvent(self, BUTTON_IDLE, control);
				}
				break;
			case BUTTON_UP:
				if (!MY(clickWasHandled)) {
					effectiveEvent(self, BUTTON_DOWN, control);
				}
				effectiveEvent(self, BUTTON_UP, control);
				break;
			case BUTTON_IDLE:
				effectiveEvent(self, BUTTON_IDLE, control);
				break;
		}
	}
}

static void selected(Tool *self) {
}

static void deselected(Tool *self) {
	event(self, (mouseButton != 0), 0, 0); /*to call mouseleave handlers etc.*/
}

static void drawCursor(Tool *self, float alpha, int transitionRunning) {
	drawStandardCursor(currentCursor, alpha);
}

Tool *newToolHand(CNode *node) {
	Tool *t = malloc(sizeof(Tool));
	if (t != NULL) {
		t->tag = TOOL_HAND;
		t->node = node;
		t->next = NULL;
		t->selected = selected;
		t->deselected = deselected;
		t->event = event;
		t->drawCursor = drawCursor;
		t->private.hand.lastControl = -1;
		t->private.hand.clickedControl = -1;
		t->private.hand.clickWasHandled = 0;
		t->private.hand.lastMouseDownTime = 0;
	}
	return t;
}
