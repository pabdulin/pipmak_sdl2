/*
 
 tools.c, part of the Pipmak Game Engine
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

/* $Id: tools.c 157 2007-07-15 20:22:11Z cwalther $ */

#include "tools.h"

extern GLenum glTextureTarget;
extern int mouseX, mouseY;
extern CNode *backgroundCNode;


void toolFree(Tool *t) {
	free(t);
}

void toolPush(Tool *t) {
	t->next = backgroundCNode->tool;
	backgroundCNode->tool = t;
	t->selected(t);
}

void toolPop(ToolTag tag) {
	Tool *t, *tt;
	if (backgroundCNode->tool->tag == tag) {
		if (backgroundCNode->tool->next != NULL) { /* refuse to pop the last tool */
			t = backgroundCNode->tool->next;
			backgroundCNode->tool->deselected(backgroundCNode->tool);
			toolFree(backgroundCNode->tool);
			backgroundCNode->tool = t;
		}
	}
	else {
		t = backgroundCNode->tool;
		while (t->next != NULL && t->next->tag != tag) t = t->next;
		if (t->next != NULL) {
			t->next->deselected(t->next);
			tt = t->next->next;
			toolFree(t->next);
			t->next = tt;
		}
	}
}

void toolChoose(Tool *t) {
	if (backgroundCNode->tool->tag != t->tag) {
		backgroundCNode->tool->deselected(backgroundCNode->tool);
		t->next = backgroundCNode->tool->next;
		toolFree(backgroundCNode->tool);
		backgroundCNode->tool = t;
		t->selected(t);
	}
	else toolFree(t);
}

void drawStandardCursor(Image *cursor, float alpha) {
	if (cursor->textureID == 0) {
		feedImageToGL(cursor);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(glTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else glBindTexture(glTextureTarget, cursor->textureID);
	glColor4f(1.0, 1.0, 1.0, alpha);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f((GLfloat)(mouseX - cursor->cursorHotX), (GLfloat)(mouseY - cursor->cursorHotY));
	glTexCoord2f((float)cursor->w/cursor->textureWidth, 0);
	glVertex2f((GLfloat)(mouseX - cursor->cursorHotX + cursor->w), (GLfloat)(mouseY - cursor->cursorHotY));
	glTexCoord2f((float)cursor->w/cursor->textureWidth, (float)cursor->h/cursor->textureHeight);
	glVertex2f((GLfloat)(mouseX - cursor->cursorHotX + cursor->w), (GLfloat)(mouseY - cursor->cursorHotY + cursor->h));
	glTexCoord2f(0, (float)cursor->h/cursor->textureHeight);
	glVertex2f((GLfloat)(mouseX - cursor->cursorHotX), (GLfloat)(mouseY - cursor->cursorHotY + cursor->h));
	glEnd();
}

Tool *newTool(ToolTag tag, CNode *node) {
	switch (tag) {
		case TOOL_HAND: return newToolHand(node);
		case TOOL_PAN: return newToolPan(node);
		case TOOL_BRUSH: return newToolBrush(node);
		case TOOL_EYEDROPPER: return newToolEyedropper(node);
		default: return NULL;
	}
}
